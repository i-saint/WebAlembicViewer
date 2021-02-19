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
        return std::span<value_type>{ (value_type*)nullptr, 0 };
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

    bool load(const char* path) override;
    void close() override;

    std::tuple<double, double> getTimeRange() const override;
    void seek(double time) override;
    void seekImpl(ImportContext& ctx);

    IMesh* getMesh() override { return m_mesh.get(); }

private:
    std::shared_ptr<std::fstream> m_stream;
    Abc::IArchive m_archive;

    MeshPtr m_mesh;
};


void Mesh::clear()
{
    m_transform = float4x4::identity();
    m_indices.clear();
    m_points.clear();
    m_normals.clear();
}


bool Scene::load(const char* path)
{
    close();

    try
    {
        // Abc::IArchive doesn't accept wide string path. so create file stream with wide string path and pass it.
        // (VisualC++'s std::ifstream accepts wide string)
        m_stream.reset(new std::fstream());
        m_stream->open(path, std::ios::in | std::ios::binary);
        if (!m_stream->is_open()) {
            close();
            return false;
        }

        std::vector< std::istream*> streams{ m_stream.get() };
        Alembic::AbcCoreOgawa::ReadArchive archive_reader(streams);
        m_archive = Abc::IArchive(archive_reader(path), Abc::kWrapExisting, Abc::ErrorHandler::kThrowPolicy);
        return true;
    }
    catch (Alembic::Util::Exception e)
    {
        close();

#ifdef wabcEnableHDF5
        try
        {
            m_archive = Abc::IArchive(AbcCoreHDF5::ReadArchive(), path);
        }
        catch (Alembic::Util::Exception e2)
        {
            close();
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
        return false;
#endif
    }
    return false;
}

void Scene::close()
{
    m_archive = {};
    m_stream = {};
}

std::tuple<double, double> Scene::getTimeRange() const
{
    return { 0.0, 0.0 };
}

void Scene::seek(double time)
{
    if (!m_archive)
        return;

    if (!m_mesh)
        m_mesh = std::make_shared<Mesh>();
    m_mesh->clear();

    ImportContext ctx;
    ctx.obj = m_archive.getTop();
    ctx.time = time;
    seekImpl(ctx);
}

void Scene::seekImpl(ImportContext& ctx)
{
    auto process_children = [&](ImportContext cctx) {
        auto obj = ctx.obj;
        size_t n = obj.getNumChildren();
        for (size_t ci = 0; ci < n; ++ci) {
            cctx.obj = obj.getChild(ci);
            seekImpl(cctx);
        }
    };

    auto obj = ctx.obj;
    auto time = ctx.time;
    const auto& metadata = obj.getMetaData();
    if (AbcGeom::IXformSchema::matches(metadata)) {
        auto schema = AbcGeom::IXform(obj).getSchema();
        AbcGeom::XformSample sample;
        schema.get(sample, time);
        auto m = sample.getMatrix();

        ImportContext cctx = ctx;
        cctx.local_matrix.assign((double4x4&)m);
        cctx.global_matrix = cctx.local_matrix * ctx.global_matrix;

        process_children(cctx);
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

        float3* dst_points = Expand(m_mesh->m_points, num_triangles * 3);
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

        process_children(ctx);
    }
    else if (AbcGeom::IPointsSchema::matches(metadata)) {
        // todo
        process_children(ctx);
    }
    else {
        process_children(ctx);
    }
}

} // namespace wabc
