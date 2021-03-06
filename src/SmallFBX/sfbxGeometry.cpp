#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxDocument.h"

namespace sfbx {

ObjectClass Geometry::getClass() const { return ObjectClass::Geometry; }

void Geometry::addChild(Object* v)
{
    super::addChild(v);
    if (auto deformer = as<Deformer>(v))
        m_deformers.push_back(deformer);
}

span<Deformer*> Geometry::getDeformers() const { return make_span(m_deformers); }


ObjectSubClass GeomMesh::getSubClass() const { return ObjectSubClass::Mesh; }

void GeomMesh::constructObject()
{
    super::constructObject();

    for (auto n : getNode()->getChildren()) {
        if (n->getName() == sfbxS_Vertices) {
            // points
            GetPropertyArray<double3>(m_points, n);
        }
        else if (n->getName() == sfbxS_PolygonVertexIndex) {
            // counts & indices
            GetPropertyArray<int>(m_indices, n);
            m_counts.resize(m_indices.size()); // reserve
            int* dst_counts = m_counts.data();
            size_t cfaces = 0;
            size_t cpoints = 0;
            for (int& i : m_indices) {
                ++cpoints;
                if (i < 0) { // negative value indicates the last index in the face
                    i = ~i;
                    dst_counts[cfaces++] = cpoints;
                    cpoints = 0;
                }
            }
            m_counts.resize(cfaces); // fit to actual size
        }
        else if (n->getName() == sfbxS_LayerElementNormal) {
            // normals
            //auto mapping = n->findChildProperty(sfbxS_MappingInformationType);
            //auto ref = n->findChildProperty(sfbxS_ReferenceInformationType);
            LayerElementF3 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            GetChildPropertyArray<double3>(tmp.data, n, sfbxS_Normals);
            GetChildPropertyArray<int>(tmp.indices, n, sfbxS_NormalsIndex);
            m_normal_layers.push_back(std::move(tmp));
        }
        else if (n->getName() == sfbxS_LayerElementUV) {
            // uv
            LayerElementF2 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            GetChildPropertyArray<double2>(tmp.data, n, sfbxS_UV);
            GetChildPropertyArray<int>(tmp.indices, n, sfbxS_UVIndex);
            m_uv_layers.push_back(std::move(tmp));
        }
        else if (n->getName() == sfbxS_LayerElementColor) {
            // colors
            LayerElementF4 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            GetChildPropertyArray<double4>(tmp.data, n, sfbxS_Colors);
            GetChildPropertyArray<int>(tmp.indices, n, sfbxS_ColorIndex);
            m_color_layers.push_back(std::move(tmp));
        }
    }
}

void GeomMesh::constructNodes()
{
    super::constructNodes();

    Node* n = getNode();

    n->createChild(sfbxS_GeometryVersion, sfbxI_GeometryVersion);

    // points
    n->createChild(sfbxS_Vertices, MakeAdaptor<double3>(m_points));

    // indices
    {
        // check if counts and indices are valid
        size_t total_counts = 0;
        for (int c : m_counts)
            total_counts += c;

        if (total_counts != m_indices.size()) {
            sfbxPrint("sfbx::Mesh: *** indices mismatch with counts ***\n");
        }
        else {
            auto* src_counts = m_counts.data();
            auto dst_node = n->createChild(sfbxS_PolygonVertexIndex);
            auto dst_prop = dst_node->createProperty();
            auto dst = dst_prop->allocateArray<int>(m_indices.size()).data();

            size_t cpoints = 0;
            for (int i : m_indices) {
                if (++cpoints == *src_counts) {
                    i = ~i; // negative value indicates the last index in the face
                    cpoints = 0;
                    ++src_counts;
                }
                *dst++ = i;
            }
        }
    }

    auto add_mapping_and_reference_info = [this](Node* node, const auto& layer) {
        if (layer.data.size() == m_indices.size() || layer.indices.size() == m_indices.size())
            node->createChild(sfbxS_MappingInformationType, "ByPolygonVertex");
        else if (layer.data.size() == m_points.size() && layer.indices.empty())
            node->createChild(sfbxS_MappingInformationType, "ByControllPoint");

        if (!layer.indices.empty())
            node->createChild(sfbxS_ReferenceInformationType, "IndexToDirect");
        else
            node->createChild(sfbxS_ReferenceInformationType, "Direct");
    };

    int clayers = 0;

    // normal layers
    for (auto& layer : m_normal_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;
        auto l = n->createChild(sfbxS_LayerElementNormal);
        l->createChild(sfbxS_Version, sfbxI_LayerElementNormalVersion);
        l->createChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
        l->createChild(sfbxS_Normals, MakeAdaptor<double3>(layer.data));
        if (!layer.indices.empty())
            l->createChild(sfbxS_NormalsIndex, layer.indices);
    }

    // uv layers
    for (auto& layer : m_uv_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;
        auto l = n->createChild(sfbxS_LayerElementUV);
        l->createChild(sfbxS_Version, sfbxI_LayerElementUVVersion);
        l->createChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
        l->createChild(sfbxS_UV, MakeAdaptor<double2>(layer.data));
        if (!layer.indices.empty())
            l->createChild(sfbxS_UVIndex, layer.indices);
    }

    // color layers
    for (auto& layer : m_color_layers) {
        if (layer.data.empty())
            continue;

        ++clayers;
        auto l = n->createChild(sfbxS_LayerElementColor);
        l->createChild(sfbxS_Version, sfbxI_LayerElementColorVersion);
        l->createChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
        l->createChild(sfbxS_Colors, MakeAdaptor<double4>(layer.data));
        if (!layer.indices.empty())
            l->createChild(sfbxS_ColorIndex, layer.indices);
    }

    if (clayers) {
        auto l = n->createChild(sfbxS_Layer, 0);
        l->createChild(sfbxS_Version, sfbxI_LayerVersion);
        if (!m_normal_layers.empty()) {
            auto le = l->createChild(sfbxS_LayerElement);
            le->createChild(sfbxS_Type, sfbxS_LayerElementNormal);
            le->createChild(sfbxS_TypeIndex, 0);
        }
        if (!m_uv_layers.empty()) {
            auto le = l->createChild(sfbxS_LayerElement);
            le->createChild(sfbxS_Type, sfbxS_LayerElementUV);
            le->createChild(sfbxS_TypeIndex, 0);
        }
        if (!m_color_layers.empty()) {
            auto le = l->createChild(sfbxS_LayerElement);
            le->createChild(sfbxS_Type, sfbxS_LayerElementColor);
            le->createChild(sfbxS_TypeIndex, 0);
        }
    }
}

span<int> GeomMesh::getCounts() const { return make_span(m_counts); }
span<int> GeomMesh::getIndices() const { return make_span(m_indices); }
span<float3> GeomMesh::getPoints() const { return make_span(m_points); }
span<LayerElementF3> GeomMesh::getNormalLayers() const { return make_span(m_normal_layers); }
span<LayerElementF2> GeomMesh::getUVLayers() const { return make_span(m_uv_layers); }
span<LayerElementF4> GeomMesh::getColorLayers() const { return make_span(m_color_layers); }

void GeomMesh::setCounts(span<int> v) { m_counts = v; }
void GeomMesh::setIndices(span<int> v) { m_indices = v; }
void GeomMesh::setPoints(span<float3> v) { m_points = v; }
void GeomMesh::addNormalLayer(LayerElementF3&& v) { m_normal_layers.push_back(v); }
void GeomMesh::addUVLayer(LayerElementF2&& v) { m_uv_layers.push_back(v); }
void GeomMesh::addColorLayer(LayerElementF4&& v) { m_color_layers.push_back(v); }

Skin* GeomMesh::createSkin()
{
    return createChild<Skin>();
}

BlendShape* GeomMesh::createBlendshape()
{
    return createChild<BlendShape>();
}


ObjectSubClass Shape::getSubClass() const { return ObjectSubClass::Shape; }

void Shape::constructObject()
{
    super::constructObject();

    for (auto n : getNode()->getChildren()) {
        if (n->getName() == sfbxS_Indexes)
            GetPropertyArray<int>(m_indices, n);
        else if (n->getName() == sfbxS_Vertices)
            GetPropertyArray<double3>(m_delta_points, n);
        else if (n->getName() == sfbxS_Normals)
            GetPropertyArray<double3>(m_delta_normals, n);
    }
}

void Shape::constructNodes()
{
    super::constructNodes();

    Node* n = getNode();
    n->createChild(sfbxS_Version, sfbxI_ShapeVersion);
    if (!m_indices.empty())
        n->createChild(sfbxS_Indexes, m_indices);
    if (!m_delta_points.empty())
        n->createChild(sfbxS_Vertices, MakeAdaptor<double3>(m_delta_points));
    if (!m_delta_normals.empty())
        n->createChild(sfbxS_Normals, MakeAdaptor<double3>(m_delta_normals));
}

span<int> Shape::getIndices() const { return make_span(m_indices); }
span<float3> Shape::getDeltaPoints() const { return make_span(m_delta_points); }
span<float3> Shape::getDeltaNormals() const { return make_span(m_delta_normals); }

void Shape::setIndices(span<int> v) { m_indices = v; }
void Shape::setDeltaPoints(span<float3> v) { m_delta_points = v; }
void Shape::setDeltaNormals(span<float3> v) { m_delta_normals = v; }

} // namespace sfbx
