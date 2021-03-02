#include "pch.h"
#include "WebAlembicViewer.h"
#include "SceneGraph.h"

namespace wabc {

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

IScene* LoadScene_(const char* path)
{
    if (!path)
        return nullptr;
    size_t len = std::strlen(path);
    if (len > 4) {
        char ext[3]{};
        for (int i = 0; i < 3; ++i)
            ext[i] = std::tolower(path[len - 3 + i]);

        if (std::memcmp(ext, "abc", 3) == 0) {
            auto scene = CreateSceneABC_();
            if (scene->load(path))
                return scene;
            else
                scene->release();
        }
        else if (std::memcmp(ext, "fbx", 3) == 0) {
            auto scene = CreateSceneFBX_();
            if (scene->load(path))
                return scene;
            else
                scene->release();
        }
    }
    return nullptr;
}

} // namespace wabc
