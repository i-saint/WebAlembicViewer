#include "pch.h"
#include "sfbxGeometry.h"

namespace sfbx {

Geometry::Geometry(NodePtr n)
    : m_node(n)
{
    // indices
    if (auto pindices = n->findChildProperty("PolygonVertexIndex", 0)) {
        auto src_indices = pindices->getArray<int>();
        size_t cindices = src_indices.size();
        m_counts.resize(cindices); // reserve
        m_indices.resize(cindices);

        const int* src = src_indices.data();
        int* dst_counts = m_counts.data();
        int* dst_indices = m_indices.data();
        size_t cfaces = 0;
        size_t cpoints = 0;
        for (int i : src_indices) {
            ++cpoints;
            if (i < 0) { // negative value indicates the last index in the face
                i = ~i;
                dst_counts[cfaces++] = cpoints;
                cpoints = 0;
            }
            *dst_indices++ = i;
        }
        m_counts.resize(cfaces); // fit to actual size
    }

    // vertices
    if (auto pvertices = n->findChildProperty("Vertices", 0)) {
        auto src_points = pvertices->getArray<double3>();
        size_t cpoints = src_points.size();
        m_points.resize(cpoints);

        const double3* src = src_points.data();
        float3* dst = m_points.data();
        for (size_t i = 0; i < cpoints; ++i)
            *dst++ = float3(*src++);
    }

    // normals
    if (auto nnormals = n->findChild("LayerElementNormal")) {
        auto mapping = nnormals->findChildProperty("MappingInformationType", 0);
        auto ref = nnormals->findChildProperty("ReferenceInformationType", 0);

        auto pnormals = nnormals->findChildProperty("Normals", 0);
        auto src_normals = pnormals->getArray<double3>();
        size_t cnormals = src_normals.size();
        m_normals.resize(cnormals);

        const double3* src = src_normals.data();
        float3* dst = m_normals.data();
        for (size_t i = 0; i < cnormals; ++i)
            *dst++ = float3(*src++);

    }

    // uv
    if (auto nuv = n->findChild("LayerElementUV")) {
        auto mapping = nuv->findChildProperty("MappingInformationType", 0);
        auto ref = nuv->findChildProperty("ReferenceInformationType", 0);

        auto puv = nuv->findChildProperty("UV", 0);
        auto src_uv = puv->getArray<double2>();
        size_t cuv = src_uv.size();
        m_uv.resize(cuv);

        const double2* src = src_uv.data();
        float2* dst = m_uv.data();
        for (size_t i = 0; i < cuv; ++i)
            *dst++ = float2(*src++);
    }
}

span<int> Geometry::getCounts() const { return make_span(m_counts); }
span<int> Geometry::getIndices() const { return make_span(m_indices); }
span<float3> Geometry::getPoints() const { return make_span(m_points); }
span<float3> Geometry::getNormals() const { return make_span(m_normals); }
span<float2> Geometry::getUV() const { return make_span(m_uv); }

} // namespace sfbx
