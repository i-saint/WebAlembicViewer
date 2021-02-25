#pragma once
#include "sfbxNode.h"

namespace sfbx {

class Geometry
{
public:
    Geometry(NodePtr n);

    span<int> getPolygonIndices() const;
    span<double3> getVertices() const;
    span<double3> getNormals() const;
    span<double2> getUV() const;

private:
    NodePtr m_node;
    PropertyPtr m_polygon_indices;
    PropertyPtr m_vertices;
    PropertyPtr m_normals;
    PropertyPtr m_uv;
};

} // sfbx
