#include "pch.h"
#include "WebAlembicViewer.h"
#include "SceneGraph.h"

using namespace Alembic;

namespace wabc {

template<class T>
inline auto make_span(Alembic::Util::shared_ptr<Abc::TypedArraySample<T>> s)
{
    using value_type = typename Abc::TypedArraySample<T>::value_type;
    if (s)
        return span<value_type>{ (value_type*)s->get(), s->size() };
    else
        return span<value_type>{ (value_type*)nullptr, (size_t)0 };
}

template<class T> inline T* Expand(std::vector<T>& v, size_t n)
{
    size_t pos = v.size();
    v.resize(pos + n);
    return v.data() + pos;
}




// Mul: e.g. [](float4x4, float3) -> float3
template<class Vec, class Mul>
bool Skin::deformImpl(span<Vec> dst, span<Vec> src, const Mul& mul) const
{
    if (m_counts.size() != src.size() || m_counts.size() != dst.size()) {
        printf("Skin::deformImpl(): vertex count mismatch\n");
        return false;
    }

    const JointWeight* weights = m_weights.data();
    size_t nvertices = src.size();
    for (size_t vi = 0; vi < nvertices; ++vi) {
        Vec p = src[vi];
        Vec r{};
        int cjoints = m_counts[vi];
        for (int bi = 0; bi < cjoints; ++bi) {
            JointWeight w = weights[bi];
            r += mul(m_matrices[w.index], p) * w.weight;
        }
        dst[vi] = r;
        weights += cjoints;
    }
    return true;
}

bool Skin::deformPoints(span<float3> dst, span<float3> src) const
{
    return deformImpl(dst, src,
        [](float4x4 m, float3 p) { return mul_p(m, p); });
}

bool Skin::deformNormals(span<float3> dst, span<float3> src) const
{
    return deformImpl(dst, src,
        [](float4x4 m, float3 p) { return mul_v(m, p); });
}


bool BlendShape::deformPoints(span<float3> dst, span<float3> src, float w) const
{
    if (dst.data() != src.data())
        memcpy(dst.data(), src.data(), src.size_bytes());

    size_t c = m_indices.size();
    for (size_t i = 0; i < c; ++i)
        dst[m_indices[i]] += m_delta_points[i];
    return true;
}

bool BlendShape::deformNormals(span<float3> dst, span<float3> src, float w) const
{
    if (dst.data() != src.data())
        memcpy(dst.data(), src.data(), src.size_bytes());

    size_t c = m_indices.size();
    for (size_t i = 0; i < c; ++i)
        dst[m_indices[i]] += m_delta_normals[i];
    return true;
}




Mesh::Mesh()
{
#ifdef wabcWithGL
    glGenBuffers(1, &m_buf_points);
    glGenBuffers(1, &m_buf_points_ex);
    glGenBuffers(1, &m_buf_normals_ex);
    glGenBuffers(1, &m_buf_wireframe_indices);
#endif
}

Mesh::~Mesh()
{
#ifdef wabcWithGL
    glDeleteBuffers(1, &m_buf_points);
    glDeleteBuffers(1, &m_buf_points_ex);
    glDeleteBuffers(1, &m_buf_normals_ex);
    glDeleteBuffers(1, &m_buf_wireframe_indices);
#endif
}

void Mesh::clear()
{
    m_points.clear();
    m_points_ex.clear();
    m_normals.clear();
    m_normals_ex.clear();

    m_counts.clear();
    m_face_indices.clear();
    m_wireframe_indices.clear();
}

void Mesh::upload()
{
#ifdef wabcWithGL
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
#endif
}



Points::Points()
{
#ifdef wabcWithGL
    glGenBuffers(1, &m_vb_points);
#endif
}

Points::~Points()
{
#ifdef wabcWithGL
    glDeleteBuffers(1, &m_vb_points);
#endif
}

void Points::clear()
{
    m_points.clear();
}

void Points::upload()
{
#ifdef wabcWithGL
    if (!m_points.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, m_vb_points);
        glBufferData(GL_ARRAY_BUFFER, m_points.size() * sizeof(float3), m_points.data(), GL_DYNAMIC_DRAW);
    }
#endif
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
        auto counts = make_span(sample.getFaceCounts());
        auto indices = make_span(sample.getFaceIndices());
        auto points = make_span(sample.getPositions());

        // make points in global space
        int num_faces = (int)counts.size();
        int num_indices = (int)indices.size();
        int num_points = (int)points.size();
        int index_offset = (int)m_mono_mesh->m_points.size();
        float3* dst_points = Expand(m_mono_mesh->m_points, num_points);
        for (int i = 0; i < num_points; ++i)
            dst_points[i] = mul_p(ctx.global_matrix, (float3&)points[i]);

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

 
        const float3* src_points = dst_points;
        const int* src_indices = indices.data();
        int* dst_counts = Expand(m_mono_mesh->m_counts, num_faces);
        int* dst_findices = Expand(m_mono_mesh->m_face_indices, num_indices);
        int* dst_windices = Expand(m_mono_mesh->m_wireframe_indices, num_lines * 2);
        float3* dst_points_ex = Expand(m_mono_mesh->m_points_ex, num_triangles * 3);

        // setup indices & vertices

        for (int i = 0; i < num_faces; ++i)
            dst_counts[i] = counts[i];

        for (int i = 0; i < num_indices; ++i)
            dst_findices[i] = src_indices[i] + index_offset;

        for (int c : counts) {
            if (c == 2) {
                // add wire frame indices
                *dst_windices++ = src_indices[0] + index_offset;
                *dst_windices++ = src_indices[1] + index_offset;
            }
            else if (c > 2) {
                // add wire frame indices
                for (int fi = 0; fi < c; ++fi) {
                    *dst_windices++ = src_indices[fi] + index_offset;
                    *dst_windices++ = (fi == c - 1 ? src_indices[0] : src_indices[fi + 1]) + index_offset;
                }

                // add triangle vertices
                // todo: handle flip faces option
                for (int fi = 0; fi < c - 2; ++fi) {
                    int i0 = src_indices[0];
                    int i1 = src_indices[1 + fi];
                    int i2 = src_indices[2 + fi];
                    *dst_points_ex++ = src_points[i0];
                    *dst_points_ex++ = src_points[i1];
                    *dst_points_ex++ = src_points[i2];
                }
            }
            src_indices += c;
        }
    }
    else if (AbcGeom::IPointsSchema::matches(metadata)) {
        auto schema = AbcGeom::IPoints(obj).getSchema();

        AbcGeom::IPointsSchema::Sample sample;
        schema.get(sample, ss);

        auto points_orig = make_span(sample.getPositions());
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
