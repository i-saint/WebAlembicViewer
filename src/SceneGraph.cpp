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
    const std::string& getPath() const override { return m_path; }
    float3 getPosition() const override { return m_position; }
    float3 getDirection() const override { return m_direction; }
    float3 getUp() const override { return m_up; }
    float getFocalLength() const override { return m_focal_length; }
    float2 getAperture() const override { return m_aperture; }
    float2 getLensShift() const override { return m_lens_shift; }
    float getNearPlane() const override { return m_near; }
    float getFarPlane() const override { return m_far; }

public:
    std::string m_path;
    float3 m_position{};
    float3 m_direction{ 0.0f, 0.0f, 1.0f };
    float3 m_up{ 0.0f, 1.0f, 0.0f };
    float m_focal_length = 30.0f;
    float2 m_aperture{ 36.0f, 24.0f };
    float2 m_lens_shift{};
    float m_near = 0.01f;
    float m_far = 100.0f;
};
using CameraPtr = std::shared_ptr<Camera>;

class Mesh : public IMesh
{
public:
    Mesh();
    ~Mesh() override;
    std::span<float3> getPoints() const override { return MakeSpan(m_points); }
    std::span<float3> getPointsEx() const override { return MakeSpan(m_points_ex); }
    std::span<float3> getNormalsEx() const override { return MakeSpan(m_normals_ex); }
    std::span<int> getWireframeIndices() const override { return MakeSpan(m_wireframe_indices); }
    GLuint getPointsBuffer() const override { return m_buf_points; }
    GLuint getPointsExBuffer() const override { return m_buf_points_ex; }
    GLuint getNormalsExBuffer() const override { return m_buf_normals_ex; }
    GLuint getWireframeIndicesBuffer() const override { return m_buf_wireframe_indices; }

    void clear();
    void upload();

public:
    std::vector<float3> m_points;
    std::vector<float3> m_points_ex;
    std::vector<float3> m_normals_ex;
    std::vector<int> m_wireframe_indices;
    GLuint m_buf_points{};
    GLuint m_buf_points_ex{};
    GLuint m_buf_normals_ex{};
    GLuint m_buf_wireframe_indices{};
};
using MeshPtr = std::shared_ptr<Mesh>;


class Points : public IPoints
{
public:
    Points();
    ~Points() override;
    std::span<float3> getPoints() const override { return MakeSpan(m_points); }
    GLuint getPointBuffer() const override { return m_vb_points; }

    void clear();
    void upload();

public:
    std::vector<float3> m_points;
    GLuint m_vb_points{};
};
using PointsPtr = std::shared_ptr<Points>;


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
    IPoints* getPoints() override { return m_mono_points.get(); }
    std::span<ICamera*> getCameras() override { return MakeSpan(m_cameras); }

private:
    std::shared_ptr<std::fstream> m_stream;
    Abc::IArchive m_archive;

    std::map<void*, size_t> m_sample_counts;
    std::tuple<double, double> m_time_range;

    double m_time = -1.0;
    MeshPtr m_mono_mesh;
    PointsPtr m_mono_points;

    std::map<std::string, CameraPtr> m_camera_table;
    std::vector<ICamera*> m_cameras;
};



Mesh::Mesh()
{
    glGenBuffers(1, &m_buf_points);
    glGenBuffers(1, &m_buf_points_ex);
    glGenBuffers(1, &m_buf_normals_ex);
    glGenBuffers(1, &m_buf_wireframe_indices);
}

Mesh::~Mesh()
{
    glDeleteBuffers(1, &m_buf_points);
    glDeleteBuffers(1, &m_buf_points_ex);
    glDeleteBuffers(1, &m_buf_normals_ex);
    glDeleteBuffers(1, &m_buf_wireframe_indices);
}

void Mesh::clear()
{
    m_points.clear();
    m_points_ex.clear();
    m_normals_ex.clear();
    m_wireframe_indices.clear();
}

void Mesh::upload()
{
    if (!m_points.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_buf_points);
        glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(float3), m_points.data(), GL_STREAM_DRAW);
    }
    if (!m_points_ex.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_buf_points_ex);
        glBufferData(GL_ARRAY_BUFFER, m_points_ex.size() * sizeof(float3), m_points_ex.data(), GL_STREAM_DRAW);
    }
    if (!m_normals_ex.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_buf_normals_ex);
        glBufferData(GL_ARRAY_BUFFER, m_normals_ex.size() * sizeof(float3), m_normals_ex.data(), GL_STREAM_DRAW);
    }
    if (!m_wireframe_indices.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_buf_wireframe_indices);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_wireframe_indices.size() * sizeof(int), m_wireframe_indices.data(), GL_STREAM_DRAW);
    }
}


Points::Points()
{
    glGenBuffers(1, &m_vb_points);
}

Points::~Points()
{
    glDeleteBuffers(1, &m_vb_points);
}

void Points::clear()
{
    m_points.clear();
}

void Points::upload()
{
    if (!m_points.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vb_points);
        glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(float3), m_points.data(), GL_DYNAMIC_DRAW);
    }
}


void Scene::release()
{
    delete this;
}

void Scene::unload()
{
    m_archive = {};
    m_stream = {};

    m_sample_counts = {};
    m_time_range = {};

    m_time = -1.0;
    m_mono_mesh = {};
    m_mono_points = {};

    m_cameras = {};
    m_camera_table = {};
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
        m_mono_mesh = std::make_shared<Mesh>();
        m_mono_points = std::make_shared<Points>();

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

        auto cam = std::make_shared<Camera>();
        cam->m_path = obj.getFullName();
        m_camera_table[cam->m_path] = cam;
        m_cameras.push_back(cam.get());
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
    if (!m_archive || time == m_time)
        return;

    m_time = time;
    m_mono_mesh->clear();
    m_mono_points->clear();

    ImportContext ctx;
    ctx.obj = m_archive.getTop();
    ctx.time = time;
    seekImpl(ctx);

    m_mono_mesh->upload();
    m_mono_points->upload();
}

void Scene::seekImpl(ImportContext ctx)
{
    auto obj = ctx.obj;
    auto ss = Abc::ISampleSelector(ctx.time);

    const auto& metadata = obj.getMetaData();
    if (AbcGeom::IXformSchema::matches(metadata)) {
        auto schema = AbcGeom::IXform(obj).getSchema();

        AbcGeom::XformSample sample;
        schema.get(sample, ss);
        auto m = sample.getMatrix();
        ctx.local_matrix.assign((double4x4&)m);
        ctx.global_matrix = ctx.local_matrix * ctx.global_matrix;
    }
    else if (AbcGeom::ICameraSchema::matches(metadata)) {
        auto schema = AbcGeom::ICamera(obj).getSchema();

        AbcGeom::CameraSample sample;
        schema.get(sample, ss);

        auto& dst = m_camera_table[obj.getFullName()];
        if (dst) {
            float3 pos = extract_position(ctx.global_matrix);
            float3 dir = normalize(mul_v(ctx.global_matrix, float3{ 0.0f, 0.0f, -1.0f }));

            dst->m_position = pos;
            dst->m_direction = dir;
            dst->m_up = float3::up();

            dst->m_focal_length = (float)sample.getFocalLength();
            dst->m_aperture = float2{
                (float)sample.getHorizontalAperture(),
                (float)sample.getVerticalAperture()
            } * 10.0f; // cm to mm
            dst->m_lens_shift = float2{
                (float)(sample.getHorizontalFilmOffset() / sample.getHorizontalAperture()),
                (float)(sample.getVerticalFilmOffset() / sample.getVerticalAperture())
            };

            dst->m_near = std::max((float)sample.getNearClippingPlane(), 0.01f);
            dst->m_far = std::max((float)sample.getFarClippingPlane(), dst->m_near);
        }
        else {
            // should not be here
        }
    }
    else if (AbcGeom::IPolyMeshSchema::matches(metadata)) {
        auto schema = AbcGeom::IPolyMesh(obj).getSchema();

        AbcGeom::IPolyMeshSchema::Sample sample;
        schema.get(sample, ss);
        auto counts = MakeSpan(sample.getFaceCounts());
        auto indices = MakeSpan(sample.getFaceIndices());
        auto points_orig = MakeSpan(sample.getPositions());

        // make points in global space
        size_t num_points = points_orig.size();
        int index_offset = (int)m_mono_mesh->m_points.size();
        float3* points = Expand(m_mono_mesh->m_points, num_points);
        for (size_t i = 0; i < num_points; ++i)
            points[i] = mul_p(ctx.global_matrix, (float3&)points_orig[i]);

        // count primitives and allocate space
        int num_lines = 0;
        int num_triangles = 0;
        for (int c : counts) {
            if (c == 2) {
                num_lines += 1;
            }
            else if (c >= 3) {
                num_triangles += c - 2;
                num_lines += c;
            }
        }

        float3* dst_points = Expand(m_mono_mesh->m_points_ex, num_triangles * 3);
        int* dst_indices = Expand(m_mono_mesh->m_wireframe_indices, num_lines * 2);
        const float3* src_points = points;
        const int* src_indices = indices.data();

        // setup indices & vertices
        for (int c : counts) {
            if (c == 2) {
                // add wire frame indices
                *dst_indices++ = src_indices[0] + index_offset;
                *dst_indices++ = src_indices[1] + index_offset;
            }
            else if (c > 2) {
                // add wire frame indices
                for (int fi = 0; fi < c; ++fi) {
                    *dst_indices++ = src_indices[fi] + index_offset;
                    *dst_indices++ = (fi == c - 1 ? src_indices[0] : src_indices[fi + 1]) + index_offset;
                }

                // add triangle vertices
                // todo: handle flip faces option
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

        AbcGeom::IPointsSchema::Sample sample;
        schema.get(sample, ss);

        auto points_orig = MakeSpan(sample.getPositions());
        size_t num_points = points_orig.size();

        float3* points = Expand(m_mono_points->m_points, num_points);
        for (size_t i = 0; i < num_points; ++i)
            points[i] = mul_p(ctx.global_matrix, (float3&)points_orig[i]);
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
