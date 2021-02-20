#include "pch.h"
#include "WebAlembicViewer.h"

using namespace Alembic;

namespace wabc {

template<class T> inline std::span<T> MakeSpan(const T& v) { return { (T*)&v, 1 }; }
template<class T> inline std::span<T> MakeSpan(const std::vector<T>& v) { return { (T*)v.data(), v.size() }; }
template<class T> inline std::span<T> MakeSpan(const T* v, size_t n) { return { (T*)v, n }; }
template<class T>
inline auto MakeSpan(Alembic::Util::shared_ptr<Abc::TypedArraySample<T>> s)
{
    using value_type = typename Abc::TypedArraySample<T>::value_type;
    if (s)
        return std::span<value_type>{ (value_type*)s->get(), s->size() };
    else
        return std::span<value_type>{ (value_type*)nullptr, (size_t)0 };
}

template<class T> inline T* Expand(std::vector<T>& v, size_t n)
{
    size_t pos = v.size();
    v.resize(pos + n);
    return v.data() + pos;
}


class Camera : public ICamera
{
public:
    float3 getPosition() const { return m_position; }
    float3 getDirection() const { return m_direction; }
    float3 getUp() const { return m_up; }
    float getFOV() const { return m_fov; }

private:
    float3 m_position{};
    float3 m_direction{ 0.0f, 0.0f, 1.0f };
    float3 m_up{ 0.0f, 1.0f, 0.0f };
    float m_fov = 30.0f;
};

class Mesh : public IMesh
{
public:
    float4x4 getTransform() const override { return m_transform; }
    std::span<int> getIndices() const override { return MakeSpan(m_indices); }
    std::span<float3> getPoints() const override { return MakeSpan(m_points); }
    std::span<float3> getNormals() const override { return MakeSpan(m_normals); }
    void clear();

public:
    float4x4 m_transform = float4x4::identity();
    std::vector<int> m_indices;
    std::vector<float3> m_points;
    std::vector<float3> m_normals;
};
using MeshPtr = std::shared_ptr<Mesh>;


class Scene : public IScene
{
public:
    struct ImportContext
    {
        Abc::IObject obj;
        double time = 0.0;
        float4x4 local_matrix = float4x4::identity();
        float4x4 global_matrix = float4x4::identity();
    };

    void release() override;

    bool load(const char* path) override;
    void unload() override;

    std::tuple<double, double> getTimeRange() const override;
    void seek(double time) override;

    void scanNodes(ImportContext ctx);
    void seekImpl(ImportContext ctx);

    double getTime() const override { return m_time; }
    IMesh* getMesh() override { return m_mono_mesh.get(); }

private:
    std::shared_ptr<std::fstream> m_stream;
    Abc::IArchive m_archive;

    std::map<void*, size_t> m_sample_counts;
    std::tuple<double, double> m_time_range;

    double m_time = 0.0;
    MeshPtr m_mono_mesh;
};


void Mesh::clear()
{
    m_transform = float4x4::identity();
    m_indices.clear();
    m_points.clear();
    m_normals.clear();
}


void Scene::release()
{
    delete this;
}

bool Scene::load(const char* path)
{
    unload();

    try
    {
        // Abc::IArchive doesn't accept wide string path. so create file stream with wide string path and pass it.
        // (VisualC++'s std::ifstream accepts wide string)
        m_stream.reset(new std::fstream());
        m_stream->open(path, std::ios::in | std::ios::binary);
        if (!m_stream->is_open()) {
            unload();
            return false;
        }

        std::vector< std::istream*> streams{ m_stream.get() };
        Alembic::AbcCoreOgawa::ReadArchive archive_reader(streams);
        m_archive = Abc::IArchive(archive_reader(path), Abc::kWrapExisting, Abc::ErrorHandler::kThrowPolicy);
    }
    catch (Alembic::Util::Exception e)
    {
        unload();

#ifdef wabcEnableHDF5
        try
        {
            m_archive = Abc::IArchive(AbcCoreHDF5::ReadArchive(), path);
        }
        catch (Alembic::Util::Exception e2)
        {
            unload();
            //sgDbgPrint(
            //    "failed to open %s\n"
            //    "it may not an alembic file"
            //    , path);
            return false;
        }
#else
        //sgDbgPrint(
        //    "failed to open %s\n"
        //    "it may not an alembic file or not in Ogawa format (HDF5 is not supported)"
        //    , path);
#endif
    }

    if (m_archive) {
        ImportContext ctx;
        ctx.obj = m_archive.getTop();
        scanNodes(ctx);

        // setup time range
        m_time_range = { 0.0, 0.0 };
        uint32_t nt = m_archive.getNumTimeSamplings();
        for (uint32_t ti = 1; ti < nt; ++ti) {
            double time_start = 0.0, time_end = 0.0;

            auto ts = m_archive.getTimeSampling(ti);
            auto tst = ts->getTimeSamplingType();
            if (tst.isUniform() || tst.isCyclic()) {
                auto start = ts->getStoredTimes()[0];
                uint32_t num_samples = (uint32_t)m_sample_counts[ts.get()];
                uint32_t samples_per_cycle = tst.getNumSamplesPerCycle();
                double time_per_cycle = tst.getTimePerCycle();
                uint32_t num_cycles = num_samples / samples_per_cycle;

                if (tst.isUniform()) {
                    time_start = start;
                    time_end = num_cycles > 0 ? start + (time_per_cycle * (num_cycles - 1)) : start;
                }
                else if (tst.isCyclic()) {
                    auto& times = ts->getStoredTimes();
                    if (!times.empty()) {
                        size_t ntimes = times.size();
                        time_start = start + (times.front() - time_per_cycle);
                        time_end = start + (times.back() - time_per_cycle) + (time_per_cycle * num_cycles);
                    }
                }
            }
            else if (tst.isAcyclic()) {
                auto& s = ts->getStoredTimes();
                if (!s.empty()) {
                    time_start = s.front();
                    time_end = s.back();
                }
            }

            if (ti == 1) {
                m_time_range = { time_start, time_end };
            }
            else {
                std::get<0>(m_time_range) = std::min(std::get<0>(m_time_range), time_start);
                std::get<1>(m_time_range) = std::max(std::get<1>(m_time_range), time_end);
            }
        }
    }

    return m_archive.valid();
}

void Scene::unload()
{
    m_archive = {};
    m_stream = {};

    m_sample_counts = {};
    m_time_range = {};

    m_time = 0.0;
}

std::tuple<double, double> Scene::getTimeRange() const
{
    return m_time_range;
}

void Scene::scanNodes(ImportContext ctx)
{
    auto update_sample_count = [this](auto& schema) {
        auto ts = schema.getTimeSampling();
        auto& n = m_sample_counts[ts.get()];
        n = std::max(n, schema.getNumSamples());
    };

    auto obj = ctx.obj;
    const auto& metadata = obj.getMetaData();
    if (AbcGeom::IXformSchema::matches(metadata)) {
        auto schema = AbcGeom::IXform(obj).getSchema();
        update_sample_count(schema);
    }
    else if (AbcGeom::ICameraSchema::matches(metadata)) {
        auto schema = AbcGeom::ICamera(obj).getSchema();
        update_sample_count(schema);
    }
    else if (AbcGeom::IPolyMeshSchema::matches(metadata)) {
        auto schema = AbcGeom::IPolyMesh(obj).getSchema();
        update_sample_count(schema);
    }
    else if (AbcGeom::IPointsSchema::matches(metadata)) {
        auto schema = AbcGeom::IPoints(obj).getSchema();
        update_sample_count(schema);
    }
    else {
    }

    size_t n = obj.getNumChildren();
    for (size_t ci = 0; ci < n; ++ci) {
        ctx.obj = obj.getChild(ci);
        scanNodes(ctx);
    }
}

void Scene::seek(double time)
{
    if (!m_archive)
        return;

    m_time = time;
    if (!m_mono_mesh)
        m_mono_mesh = std::make_shared<Mesh>();
    m_mono_mesh->clear();

    ImportContext ctx;
    ctx.obj = m_archive.getTop();
    ctx.time = time;
    seekImpl(ctx);
}

void Scene::seekImpl(ImportContext ctx)
{
    auto obj = ctx.obj;
    auto time = ctx.time;
    const auto& metadata = obj.getMetaData();
    if (AbcGeom::IXformSchema::matches(metadata)) {
        auto schema = AbcGeom::IXform(obj).getSchema();

        AbcGeom::XformSample sample;
        schema.get(sample, time);
        auto m = sample.getMatrix();
        ctx.local_matrix.assign((double4x4&)m);
        ctx.global_matrix = ctx.local_matrix * ctx.global_matrix;
    }
    else if (AbcGeom::ICameraSchema::matches(metadata)) {
        auto schema = AbcGeom::ICamera(obj).getSchema();
    }
    else if (AbcGeom::IPolyMeshSchema::matches(metadata)) {
        auto schema = AbcGeom::IPolyMesh(obj).getSchema();

        AbcGeom::IPolyMeshSchema::Sample sample;
        schema.get(sample, time);
        auto counts = MakeSpan(sample.getFaceCounts());
        auto indices = MakeSpan(sample.getFaceIndices());
        auto points_orig = MakeSpan(sample.getPositions());

        size_t num_points = points_orig.size();
        std::vector<float3> points(num_points);
        for (size_t i = 0; i < num_points; ++i)
            points[i] = mul_p(ctx.global_matrix, (float3&)points_orig[i]);

        int num_triangles = 0;
        for (int c : counts) {
            if (c > 2)
                num_triangles += c - 2;
        }

        float3* dst_points = Expand(m_mono_mesh->m_points, num_triangles * 3);
        const float3* src_points = points.data();
        const int* src_indices = indices.data();
        for (int c : counts) {
            if (c > 2) {
                for (int fi = 0; fi < c - 2; ++fi) {
                    int i0 = src_indices[0];
                    int i1 = src_indices[1 + fi];
                    int i2 = src_indices[2 + fi];
                    *dst_points++ = src_points[i0];
                    *dst_points++ = src_points[i1];
                    *dst_points++ = src_points[i2];
                }
            }
            src_indices += c;
        }
    }
    else if (AbcGeom::IPointsSchema::matches(metadata)) {
        auto schema = AbcGeom::IPoints(obj).getSchema();
        // todo
    }

    size_t n = obj.getNumChildren();
    for (size_t ci = 0; ci < n; ++ci) {
        ctx.obj = obj.getChild(ci);
        seekImpl(ctx);
    }
}

IScene* CreateScene_()
{
    return new Scene();
}

} // namespace wabc
