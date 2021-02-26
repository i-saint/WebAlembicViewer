#include "pch.h"
#include "sfbxObject.h"

namespace sfbx {

ObjecType GetFbxObjectType(const std::string& n)
{
    if (n == "NodeAttribute")
        return ObjecType::Attribute;
    else if (n == "Model")
        return ObjecType::Model;
    else if (n == "Geometry")
        return ObjecType::Geometry;
    else if (n == "Deformer")
        return ObjecType::Deformer;
    else if (n == "Pose")
        return ObjecType::Pose;
    else if (n == "Material")
        return ObjecType::Material;
    return ObjecType::Unknown;
}

const char* GetFbxObjectName(ObjecType t)
{
    switch (t) {
    case ObjecType::Attribute: return "NodeAtrribute";
    case ObjecType::Model: return "Model";
    case ObjecType::Geometry: return "Geometry";
    case ObjecType::Deformer: return "Deformer";
    case ObjecType::Pose: return "Pose";
    case ObjecType::Material: return "Material";
    default: return "";
    }
}


ObjectSubType GetFbxObjectSubType(const std::string& n)
{
    if (n == "Mesh")
        return ObjectSubType::Mesh;
    else if (n == "Root")
        return ObjectSubType::Root;
    else if (n == "LimbNode")
        return ObjectSubType::LimbNode;
    else if (n == "Skin")
        return ObjectSubType::Skin;
    else if (n == "Cluster")
        return ObjectSubType::Cluster;
    return ObjectSubType::Unknown;
}

const char* GetFbxObjectSubName(ObjectSubType t)
{
    switch (t) {
    case ObjectSubType::Mesh: return "Mesh";
    case ObjectSubType::Root: return "Root";
    case ObjectSubType::LimbNode: return "LimbNode";
    case ObjectSubType::Skin: return "Skin";
    case ObjectSubType::Cluster: return "Cluster";
    default: return "";
    }
}


Object::Object(NodePtr n)
    : m_node(n)
{
}

Object::~Object()
{
}

ObjecType Object::getType() const { return ObjecType::Unknown; }

sfbx::ObjectSubType Object::getSubType() const
{
    return m_subtype;
}

int64     Object::getID() const   { return m_id; }
NodePtr   Object::getNode() const { return m_node; }

void Object::readDataFronNode()
{
    auto n = getNode();
    if (n) {
        m_id = n->getProperty(0)->getValue<int64>();
        m_name = n->getProperty(1)->getString();
        m_subtype = GetFbxObjectSubType(n->getProperty(2)->getString());
    }
}

void Object::createNodes()
{
    m_node = MakeNode();
    m_node->setName(GetFbxObjectName(getType()));
    m_node->addProperty(m_id);
    m_node->addProperty(m_name);
    m_node->addProperty(GetFbxObjectSubName(m_subtype));
}


Attribute::Attribute(NodePtr n)
    : super(n)
{
}

ObjecType Attribute::getType() const
{
    return ObjecType::Attribute;
}

void Attribute::createNodes()
{
    super::createNodes();
    // todo
}


Model::Model(NodePtr n)
    : super(n)
{
}

ObjecType Model::getType() const
{
    return ObjecType::Model;
}

void Model::addAttribute(ObjectPtr v)
{
    m_attributes.push_back(v);
}


void Model::createNodes()
{
    super::createNodes();
    // todo
}

Geometry::Geometry(NodePtr n)
    : super(n)
{
}

ObjecType Geometry::getType() const
{
    return ObjecType::Geometry;
}

void Geometry::readDataFronNode()
{
    super::readDataFronNode();
    auto n = getNode();
    if (!n)
        return;

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

void Geometry::createNodes()
{
    super::createNodes();
    // todo
}

span<int> Geometry::getCounts() const { return make_span(m_counts); }
span<int> Geometry::getIndices() const { return make_span(m_indices); }
span<float3> Geometry::getPoints() const { return make_span(m_points); }
span<float3> Geometry::getNormals() const { return make_span(m_normals); }
span<float2> Geometry::getUV() const { return make_span(m_uv); }

void Geometry::setCounts(span<int> v) { m_counts.assign(v); }
void Geometry::setIndices(span<int> v) { m_indices.assign(v); }
void Geometry::setPoints(span<float3> v) { m_points.assign(v); }
void Geometry::setNormals(span<float3> v) { m_normals.assign(v); }
void Geometry::setUV(span<float2> v) { m_uv.assign(v); }


Deformer::Deformer(NodePtr n)
    : super(n)
{
}

ObjecType Deformer::getType() const
{
    return ObjecType::Deformer;
}

void Deformer::readDataFronNode()
{
    super::readDataFronNode();
    auto n = getNode();
    if (!n)
        return;

    if (m_subtype == ObjectSubType::Cluster) {
        auto indices = n->findChildProperty("Indexes", 0)->getArray<int>();
        auto weights = n->findChildProperty("Weights", 0)->getArray<float64>();
        size_t c = indices.size();
        if (c != 0 && weights.size() == c) {
            m_indices.assign(indices);
            m_weights.assign(weights);
        }

        m_transform = n->findChildProperty("Transform", 0)->getValue<double4x4>();
        m_transform_link = n->findChildProperty("TransformLink", 0)->getValue<double4x4>();
    }
}

void Deformer::createNodes()
{
    super::createNodes();
    // todo
}

span<int> Deformer::getIndices() const { return make_span(m_indices); }
span<float> Deformer::getWeights() const { return make_span(m_weights); }
const float4x4& Deformer::getTransform() const { return m_transform; }
const float4x4& Deformer::getTransformLink() const { return m_transform_link; }

void Deformer::setIndices(span<int> v) { m_indices.assign(v); }
void Deformer::setWeights(span<float> v) { m_weights.assign(v); }
void Deformer::getTransform(const float4x4& v) { m_transform = v; }
void Deformer::getTransformLink(const float4x4& v) { m_transform_link = v; }


Pose::Pose(NodePtr n)
    : super(n)
{
}

ObjecType Pose::getType() const
{
    return ObjecType::Pose;
}



void Pose::createNodes()
{
    super::createNodes();
    // todo
}


Material::Material(NodePtr n)
    : super(n)
{
}

ObjecType Material::getType() const
{
    return ObjecType::Material;
}

void Material::createNodes()
{
    super::createNodes();
    // todo
}

} // namespace sfbx
