#include "pch.h"
#include "sfbxObject.h"
#include "sfbxDocument.h"

namespace sfbx {

ObjectType GetFbxObjectType(const std::string& n)
{
    if (n.empty())
        return ObjectType::Unknown;
    else if (n == "NodeAttribute")
        return ObjectType::Attribute;
    else if (n == "Model")
        return ObjectType::Model;
    else if (n == "Geometry")
        return ObjectType::Geometry;
    else if (n == "Deformer")
        return ObjectType::Deformer;
    else if (n == "Pose")
        return ObjectType::Pose;
    else if (n == "Material")
        return ObjectType::Material;
    else {
        printf("GetFbxObjectType(): unknown type \"%s\"\n", n.c_str());
        return ObjectType::Unknown;
    }
}

const char* GetFbxObjectName(ObjectType t)
{
    switch (t) {
    case ObjectType::Attribute: return "NodeAtrribute";
    case ObjectType::Model: return "Model";
    case ObjectType::Geometry: return "Geometry";
    case ObjectType::Deformer: return "Deformer";
    case ObjectType::Pose: return "Pose";
    case ObjectType::Material: return "Material";
    default: return "";
    }
}


ObjectSubType GetFbxObjectSubType(const std::string& n)
{
    if (n.empty())
        return ObjectSubType::Unknown;
    else if (n == "Mesh")
        return ObjectSubType::Mesh;
    else if (n == "Shape")
        return ObjectSubType::Shape;
    else if (n == "Root")
        return ObjectSubType::Root;
    else if (n == "LimbNode")
        return ObjectSubType::LimbNode;
    else if (n == "Skin")
        return ObjectSubType::Skin;
    else if (n == "Cluster")
        return ObjectSubType::Cluster;
    else if (n == "BindPose")
        return ObjectSubType::BindPose;
    else if (n == "BlendShape")
        return ObjectSubType::BlendShape;
    else if (n == "BlendShapeChannel")
        return ObjectSubType::BlendShapeChannel;
    else {
        printf("GetFbxObjectSubType(): unknown subtype \"%s\"\n", n.c_str());
        return ObjectSubType::Unknown;
    }
}

const char* GetFbxObjectSubName(ObjectSubType t)
{
    switch (t) {
    case ObjectSubType::Mesh: return "Mesh";
    case ObjectSubType::Shape: return "Shape";
    case ObjectSubType::Root: return "Root";
    case ObjectSubType::LimbNode: return "LimbNode";
    case ObjectSubType::Skin: return "Skin";
    case ObjectSubType::Cluster: return "Cluster";
    case ObjectSubType::BindPose: return "BindPose";
    case ObjectSubType::BlendShape: return "BlendShape";
    case ObjectSubType::BlendShapeChannel: return "BlendShapeChannel";
    default: return "";
    }
}


Object::Object()
{
}

Object::~Object()
{
}

ObjectType Object::getType() const
{
    return ObjectType::Unknown;
}

void Object::readDataFronNode()
{
    if (auto n = getNode()) {
        m_id = n->getProperty(0)->getValue<int64>();
        m_name = n->getProperty(1)->getString();
        m_subtype = GetFbxObjectSubType(n->getProperty(2)->getString());
    }
}

void Object::createNode()
{
    //m_node = MakeNode();
    //m_node->setName(GetFbxObjectName(getType()));
    //m_node->addProperty(m_id);
    //m_node->addProperty(m_name);
    //m_node->addProperty(GetFbxObjectSubName(m_subtype));
}

ObjectSubType Object::getSubType() const { return m_subtype; }
int64 Object::getID() const { return m_id; }
Node* Object::getNode() const { return m_node; }
Object* Object::getParent() const { return m_parent; }
span<Object*> Object::getChildren() const { return make_span(m_children); }

void Object::setSubType(ObjectSubType v) { m_subtype = v; }
void Object::setID(int64 id) { m_id = id; }
void Object::setNode(Node* v) { m_node = v; }

void Object::addChild(Object* v)
{
    if (v) {
        m_children.push_back(v);
        v->m_parent = this;
    }
}



Attribute::Attribute()
{
}

ObjectType Attribute::getType() const
{
    return ObjectType::Attribute;
}

void Attribute::readDataFronNode()
{
    super::readDataFronNode();
    // todo
}

void Attribute::createNode()
{
    super::createNode();
    // todo
}



Model::Model()
{
}

ObjectType Model::getType() const
{
    return ObjectType::Model;
}


void Model::readDataFronNode()
{
    super::readDataFronNode();
    // todo
}

void Model::createNode()
{
    super::createNode();
    // todo
}

float3 Model::getPosition() const { return m_position; }
float3 Model::getRotation() const { return m_rotation; }
float3 Model::getScale() const { return m_scale; }

void Model::setPosition(float3 v) { m_position = v; }
void Model::setRotation(float3 v) { m_rotation = v; }
void Model::setScale(float3 v) { m_scale = v; }

Geometry::Geometry()
{
}

ObjectType Geometry::getType() const
{
    return ObjectType::Geometry;
}

void Geometry::readDataFronNode()
{
    super::readDataFronNode();
    auto n = getNode();
    if (!n)
        return;

    // vertices
    m_points = n->findChildProperty("Vertices")->getArray<double3>();

    if (m_subtype == ObjectSubType::Mesh) {
        // indices
        if (auto pindices = n->findChildProperty("PolygonVertexIndex")) {
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

        // normals
        if (auto nnormals = n->findChild("LayerElementNormal")) {
            auto mapping = nnormals->findChildProperty("MappingInformationType");
            auto ref = nnormals->findChildProperty("ReferenceInformationType");

            auto src_normals = nnormals->findChildProperty("Normals")->getArray<double3>();
            m_normals = src_normals;
        }

        // uv
        if (auto nuv = n->findChild("LayerElementUV")) {
            auto mapping = nuv->findChildProperty("MappingInformationType");
            auto ref = nuv->findChildProperty("ReferenceInformationType");

            auto src_uv = nuv->findChildProperty("UV")->getArray<double2>();
            m_uv = src_uv;
        }
    }
    else if (m_subtype == ObjectSubType::Shape) {
        // indices
        m_indices = n->findChildProperty("Indexes")->getArray<int>();

        // normals
        m_points = n->findChildProperty("Normals")->getArray<double3>();
    }

}

void Geometry::createNode()
{
    super::createNode();
    // todo
}

span<int> Geometry::getCounts() const { return make_span(m_counts); }
span<int> Geometry::getIndices() const { return make_span(m_indices); }
span<float3> Geometry::getPoints() const { return make_span(m_points); }
span<float3> Geometry::getNormals() const { return make_span(m_normals); }
span<float2> Geometry::getUV() const { return make_span(m_uv); }

void Geometry::setCounts(span<int> v) { m_counts = v; }
void Geometry::setIndices(span<int> v) { m_indices = v; }
void Geometry::setPoints(span<float3> v) { m_points = v; }
void Geometry::setNormals(span<float3> v) { m_normals = v; }
void Geometry::setUV(span<float2> v) { m_uv = v; }


Deformer::Deformer()
{
}

ObjectType Deformer::getType() const
{
    return ObjectType::Deformer;
}

void Deformer::readDataFronNode()
{
    super::readDataFronNode();
    auto n = getNode();
    if (!n)
        return;

    if (m_subtype == ObjectSubType::Skin) {
        // nothing to do
    }
    else if (m_subtype == ObjectSubType::Cluster) {
        m_indices = n->findChildProperty("Indexes")->getArray<int>();
        m_weights = n->findChildProperty("Weights")->getArray<float64>();
        m_transform = n->findChildProperty("Transform")->getValue<double4x4>();
        m_transform_link = n->findChildProperty("TransformLink")->getValue<double4x4>();
    }
    else if (m_subtype == ObjectSubType::BlendShape) {
        // nothing to do
    }
    else if (m_subtype == ObjectSubType::BlendShapeChannel) {

    }
}

void Deformer::createNode()
{
    super::createNode();
    // todo
}

span<int> Deformer::getIndices() const { return make_span(m_indices); }
span<float> Deformer::getWeights() const { return make_span(m_weights); }
const float4x4& Deformer::getTransform() const { return m_transform; }
const float4x4& Deformer::getTransformLink() const { return m_transform_link; }

void Deformer::setIndices(span<int> v) { m_indices = v; }
void Deformer::setWeights(span<float> v) { m_weights = v; }
void Deformer::getTransform(const float4x4& v) { m_transform = v; }
void Deformer::getTransformLink(const float4x4& v) { m_transform_link = v; }


Pose::Pose()
{
}

ObjectType Pose::getType() const
{
    return ObjectType::Pose;
}



void Pose::readDataFronNode()
{
    super::readDataFronNode();
    auto n = getNode();
    if (!n)
        return;

    if (m_subtype == ObjectSubType::BindPose) {
        for (auto c : n->getChildren()) {
            if (c->getName() == "PoseNode") {
                auto nid = c->findChildProperty("Node")->getValue<int64>();
                auto mat = c->findChildProperty("Matrix")->getValue<double4x4>();
                m_bindpose.push_back({ m_document->findObject(nid), float4x4(mat) });
            }
        }
    }
}

void Pose::createNode()
{
    super::createNode();
    // todo
}

std::span<sfbx::Pose::BindPose> Pose::getBindPose() const
{
    return make_span(m_bindpose);
}


Material::Material()
{
}

ObjectType Material::getType() const
{
    return ObjectType::Material;
}

void Material::createNode()
{
    super::createNode();
    // todo
}

} // namespace sfbx
