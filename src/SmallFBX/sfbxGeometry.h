#pragma once
#include "sfbxNode.h"

namespace sfbx {

class Geometry
{
public:
    Geometry(NodePtr n);

    span<int> getCounts() const;
    span<int> getIndices() const;
    span<float3> getPoints() const;
    span<float3> getNormals() const;
    span<float2> getUV() const;

private:
    NodePtr m_node;

    RawVector<int> m_counts;
    RawVector<int> m_indices;
    RawVector<float3> m_points;
    RawVector<float3> m_normals;
    RawVector<float2> m_uv;
};

template<class... T>
inline GeometryPtr MakeGeometry(T&&... v)
{
    return std::make_shared<Geometry>(std::forward<T>(v)...);
}

} // sfbx
