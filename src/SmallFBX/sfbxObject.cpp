#include "pch.h"
#include "sfbxInternal.h"
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
    m_id = (int64)this;
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

void Object::constructNodes()
{
    auto objects = m_document->findNode(sfbxS_Objects);
    m_node = objects->createNode(GetFbxObjectName(getType()));
    m_node->addProperty(m_id);
    m_node->addProperty(m_name);
    m_node->addProperty(GetFbxObjectSubName(m_subtype));

    auto connections = m_document->findNode(sfbxS_Connections);
    auto c = connections->createNode(sfbxS_C);
    c->addProperty(sfbxS_OO);
    c->addProperty(m_id);
    c->addProperty(m_parent ? m_parent->getID() : 0);
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

void Attribute::constructNodes()
{
    super::constructNodes();
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
    auto n = getNode();
    if (!n)
        return;

    if (auto prop = n->findChild(sfbxS_Properties70)) {
        for (auto p : prop->getChildren()) {
            auto pname = p->getProperty(0)->getString();
            if (pname == sfbxS_Visibility) {
                m_visibility = p->getProperty(4)->getValue<bool>();
            }
            else if (pname == sfbxS_RotationOrder) {
                m_rotation_order = (RotationOrder)p->getProperty(4)->getValue<int32>();
            }
            else if (pname == sfbxS_LclTranslation) {
                m_position = float3{
                    (float)p->getProperty(4)->getValue<float64>(),
                    (float)p->getProperty(5)->getValue<float64>(),
                    (float)p->getProperty(6)->getValue<float64>(),
                };
            }
            else if (pname == sfbxS_LclRotation) {
                m_rotation = float3{
                    (float)p->getProperty(4)->getValue<float64>(),
                    (float)p->getProperty(5)->getValue<float64>(),
                    (float)p->getProperty(6)->getValue<float64>(),
                };
            }
            else if (pname == sfbxS_LclScale) {
                m_scale = float3{
                    (float)p->getProperty(4)->getValue<float64>(),
                    (float)p->getProperty(5)->getValue<float64>(),
                    (float)p->getProperty(6)->getValue<float64>(),
                };
            }
        }
    }
    // todo
}

void Model::constructNodes()
{
    super::constructNodes();
    auto n = getNode();
    if (!n)
        return;

    // version
    n->createNode(sfbxS_Version)->addProperty(sfbxI_ModelVersion);

    auto properties = n->createNode(sfbxS_Properties70);

    // position
    if (m_position != float3::zero()) {
        auto p = properties->createNode(sfbxS_P);
        p->addProperty(sfbxS_LclTranslation);
        p->addProperty(sfbxS_LclTranslation);
        p->addProperty(sfbxS_Empty);
        p->addProperty(sfbxS_A);
        p->addProperty((float64)m_position.x);
        p->addProperty((float64)m_position.y);
        p->addProperty((float64)m_position.z);
    }

    // rotation
    if (m_rotation != float3::zero()) {
        {
            auto p = properties->createNode(sfbxS_P);
            p->addProperty(sfbxS_RotationOrder);
            p->addProperty(sfbxS_RotationOrder);
            p->addProperty(sfbxS_Empty);
            p->addProperty(sfbxS_A);
            p->addProperty((int32)m_rotation_order);
        }
        {
            auto p = properties->createNode(sfbxS_P);
            p->addProperty(sfbxS_LclRotation);
            p->addProperty(sfbxS_LclRotation);
            p->addProperty(sfbxS_Empty);
            p->addProperty(sfbxS_A);
            p->addProperty((float64)m_rotation.x);
            p->addProperty((float64)m_rotation.y);
            p->addProperty((float64)m_rotation.z);
        }
    }

    // scale
    if (m_scale!= float3::one()) {
        auto p = properties->createNode(sfbxS_P);
        p->addProperty(sfbxS_LclScale);
        p->addProperty(sfbxS_LclScale);
        p->addProperty(sfbxS_Empty);
        p->addProperty(sfbxS_A);
        p->addProperty((float64)m_scale.x);
        p->addProperty((float64)m_scale.y);
        p->addProperty((float64)m_scale.z);
    }
}

bool Model::getVisibility() const { return m_visibility; }
RotationOrder Model::getRotationOrder() const { return m_rotation_order; }
float3 Model::getPosition() const { return m_position; }
float3 Model::getRotation() const { return m_rotation; }
float3 Model::getScale() const { return m_scale; }

void Model::setVisibility(bool v) { m_visibility = v; }
void Model::setRotationOrder(RotationOrder v) { m_rotation_order = v; }
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
    m_points = n->findChildProperty(sfbxS_Vertices)->getArray<double3>();

    if (m_subtype == ObjectSubType::Mesh) {
        // indices
        if (auto pindices = n->findChildProperty(sfbxS_PolygonVertexIndex)) {
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
        if (auto nnormals = n->findChild(sfbxS_LayerElementNormal)) {
            auto mapping = nnormals->findChildProperty(sfbxS_MappingInformationType);
            auto ref = nnormals->findChildProperty(sfbxS_ReferenceInformationType);

            auto src_normals = nnormals->findChildProperty(sfbxS_Normals)->getArray<double3>();
            m_normals = src_normals;
        }

        // uv
        if (auto nuv = n->findChild(sfbxS_LayerElementUV)) {
            auto mapping = nuv->findChildProperty(sfbxS_MappingInformationType);
            auto ref = nuv->findChildProperty(sfbxS_ReferenceInformationType);

            auto src_uv = nuv->findChildProperty(sfbxS_UV)->getArray<double2>();
            m_uv = src_uv;
        }
    }
    else if (m_subtype == ObjectSubType::Shape) {
        // indices
        m_indices = n->findChildProperty(sfbxS_Indexes)->getArray<int>();

        // normals
        m_points = n->findChildProperty(sfbxS_Normals)->getArray<double3>();
    }

}

void Geometry::constructNodes()
{
    super::constructNodes();
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
        m_indices = n->findChildProperty(sfbxS_Indexes)->getArray<int>();
        m_weights = n->findChildProperty(sfbxS_Weights)->getArray<float64>();
        m_transform = n->findChildProperty(sfbxS_Transform)->getValue<double4x4>();
        m_transform_link = n->findChildProperty(sfbxS_TransformLink)->getValue<double4x4>();
    }
    else if (m_subtype == ObjectSubType::BlendShape) {
        // nothing to do
    }
    else if (m_subtype == ObjectSubType::BlendShapeChannel) {

    }
}

void Deformer::constructNodes()
{
    super::constructNodes();
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
            if (c->getName() == sfbxS_PoseNode) {
                auto nid = c->findChildProperty(sfbxS_Node)->getValue<int64>();
                auto mat = c->findChildProperty(sfbxS_Marix)->getValue<double4x4>();
                m_bindpose.push_back({ m_document->findObject(nid), float4x4(mat) });
            }
        }
    }
}

void Pose::constructNodes()
{
    super::constructNodes();
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

void Material::constructNodes()
{
    super::constructNodes();
    // todo
}

} // namespace sfbx
