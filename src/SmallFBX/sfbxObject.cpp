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
        m_id = GetPropertyValue<int64>(n, 0);
        m_name = GetPropertyString(n, 1);
        m_subtype = GetFbxObjectSubType(GetPropertyString(n, 2));
    }
}

void Object::constructNodes()
{
    auto objects = m_document->findNode(sfbxS_Objects);
    m_node = objects->createChild(GetFbxObjectName(getType()));
    m_node->addProperty(m_id);
    m_node->addProperty(m_name);
    m_node->addProperty(GetFbxObjectSubName(m_subtype));

    if (!m_parents.empty()) {
        auto connections = m_document->findNode(sfbxS_Connections);
        for (auto parent : getParents()) {
            auto c = connections->createChild(sfbxS_C);
            c->addProperty(sfbxS_OO);
            c->addProperty(getID());
            c->addProperty(parent->getID());
        }
    }
}

ObjectSubType Object::getSubType() const { return m_subtype; }
int64 Object::getID() const { return m_id; }
const std::string& Object::getName() const { return m_name; }
Node* Object::getNode() const { return m_node; }

span<Object*> Object::getParents() const  { return make_span(m_parents); }
span<Object*> Object::getChildren() const { return make_span(m_children); }
Object* Object::getParent(size_t i) const { return i < m_parents.size() ? m_parents[i] : nullptr; }
Object* Object::getChild(size_t i) const  { return i < m_children.size() ? m_children[i] : nullptr; }

void Object::setSubType(ObjectSubType v) { m_subtype = v; }
void Object::setID(int64 id) { m_id = id; }
void Object::setName(const std::string& v) { m_name = v; }
void Object::setNode(Node* v) { m_node = v; }

Object* Object::createChild(ObjectType type)
{
    auto ret = m_document->createObject(type);
    addChild(ret);
    return ret;
}

void Object::addChild(Object* v)
{
    if (v) {
        m_children.push_back(v);
        v->addParent(this);
    }
}

void Object::addParent(Object* v)
{
    if (v) {
        m_parents.push_back(v);
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
            auto pname = GetPropertyString(p);
            if (pname == sfbxS_Visibility) {
                m_visibility = GetPropertyValue<bool>(p, 4);
            }
            else if (pname == sfbxS_RotationOrder) {
                m_rotation_order = (RotationOrder)GetPropertyValue<int32>(p, 4);
            }
            else if (pname == sfbxS_LclTranslation) {
                m_position = float3{
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                    (float)GetPropertyValue<float64>(p, 6),
                };
            }
            else if (pname == sfbxS_LclRotation) {
                m_rotation = float3{
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                    (float)GetPropertyValue<float64>(p, 6),
                };
            }
            else if (pname == sfbxS_LclScale) {
                m_scale = float3{
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                    (float)GetPropertyValue<float64>(p, 6),
                };
            }
        }
    }
}

void Model::constructNodes()
{
    super::constructNodes();
    auto n = getNode();
    if (!n)
        return;

    // version
    n->createChild(sfbxS_Version)->addProperty(sfbxI_ModelVersion);

    auto properties = n->createChild(sfbxS_Properties70);

    // position
    if (m_position != float3::zero()) {
        auto p = properties->createChild(sfbxS_P);
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
            auto p = properties->createChild(sfbxS_P);
            p->addProperty(sfbxS_RotationOrder);
            p->addProperty(sfbxS_RotationOrder);
            p->addProperty(sfbxS_Empty);
            p->addProperty(sfbxS_A);
            p->addProperty((int32)m_rotation_order);
        }
        {
            auto p = properties->createChild(sfbxS_P);
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
        auto p = properties->createChild(sfbxS_P);
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

    auto handle_vertices = [this](Node* n) {
        if (n->getName() != sfbxS_Vertices)
            return false;
        m_points = GetPropertyArray<double3>(n);
        return true;
    };

    auto handle_polygon_indices = [this](Node* n) {
        if (n->getName() != sfbxS_PolygonVertexIndex)
            return false;
        auto src_indices = GetPropertyArray<int>(n);
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
        return true;
    };

    auto handle_normal_layer = [this](Node* n) {
        if (n->getName() != sfbxS_LayerElementNormal)
            return false;
        //auto mapping = n->findChildProperty(sfbxS_MappingInformationType);
        //auto ref = n->findChildProperty(sfbxS_ReferenceInformationType);
        LayerElementF3 tmp;
        tmp.name = GetChildPropertyString(n, sfbxS_Name);
        tmp.data = GetChildPropertyArray<double3>(n, sfbxS_Normals);
        tmp.indices = GetChildPropertyArray<int>(n, sfbxS_NormalsIndex);
        addNormalLayer(std::move(tmp));
        return true;
    };

    auto handle_uv_layer = [this](Node* n) {
        if (n->getName() != sfbxS_LayerElementUV)
            return false;
        LayerElementF2 tmp;
        tmp.name = GetChildPropertyString(n, sfbxS_Name);
        tmp.data = GetChildPropertyArray<double2>(n, sfbxS_UV);
        tmp.indices = GetChildPropertyArray<int>(n, sfbxS_UVIndex);
        addUVLayer(std::move(tmp));
        return true;
    };

    auto handle_color_layer = [this](Node* n) {
        if (n->getName() != sfbxS_LayerElementColor)
            return false;
        LayerElementF4 tmp;
        tmp.name = GetChildPropertyString(n, sfbxS_Name);
        tmp.data = GetChildPropertyArray<double4>(n, sfbxS_Colors);
        tmp.indices = GetChildPropertyArray<int>(n, sfbxS_ColorIndex);
        addColorLayer(std::move(tmp));
        return true;
    };

    auto handle_shape_indices = [this](Node* n) {
        if (n->getName() != sfbxS_Indexes)
            return false;
        m_indices = GetPropertyArray<int>(n);
        return true;
    };

    auto handle_shape_normals = [this](Node* n) {
        if (n->getName() != sfbxS_Normals)
            return false;
        LayerElementF3 tmp;
        tmp.data = GetPropertyArray<double3>(n);
        addNormalLayer(std::move(tmp));
        return true;
    };


    if (m_subtype == ObjectSubType::Mesh) {
        for (auto c : n->getChildren()) {
            handle_vertices(c) ||
                handle_polygon_indices(c) ||
                handle_normal_layer(c) ||
                handle_uv_layer(c) ||
                handle_color_layer(c);
        }
    }
    else if (m_subtype == ObjectSubType::Shape) {
        for (auto c : n->getChildren()) {
            handle_vertices(c) ||
                handle_shape_indices(c) ||
                handle_shape_normals(c);
        }
    }
}


template<class D, class S>
static inline void CreatePropertyAndCopy(Node* dst_node, span<S> src)
{
    auto dst_prop = dst_node->createProperty();
    auto dst = dst_prop->allocateArray<D>(src.size());
    copy(dst, src);
}

void Geometry::constructNodes()
{
    super::constructNodes();

    auto n = getNode();

    // points
    CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Vertices), getPoints());

    if (m_subtype == ObjectSubType::Mesh) {
        // indices
        {
            auto indices = getIndices();
            auto counts = getCounts().data();

            auto dst_node = n->createChild(sfbxS_PolygonVertexIndex);
            auto dst_prop = dst_node->createProperty();
            auto dst_indices = dst_prop->allocateArray<int>(indices.size()).data();

            size_t cpoints = 0;
            for (int i : indices) {
                if (++cpoints == *counts) {
                    i = ~i; // negative value indicates the last index in the face
                    cpoints = 0;
                    ++counts;
                }
                *dst_indices++ = i;
            }
        }

        // normal layers
        for (auto& layer : m_normal_layers) {
            auto normals_layer = n->createChild(sfbxS_LayerElementNormal);
            // todo
        }

        // uv layers
        for (auto& layer : m_uv_layers) {
            auto uv_layer = n->createChild(sfbxS_LayerElementUV);
            // todo
        }

        // color layers
        for (auto& layer : m_color_layers) {
            auto color_layer = n->createChild(sfbxS_LayerElementColor);
            // todo
        }
    }
    else if (m_subtype == ObjectSubType::Shape) {
        //CreatePropertyAndCopy<int>(n->createChild(sfbxS_Indexes), getIndices());
        //CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Normals), getNormals());
    }
}

span<int>    Geometry::getCounts() const { return make_span(m_counts); }
span<int>    Geometry::getIndices() const { return make_span(m_indices); }
span<float3> Geometry::getPoints() const { return make_span(m_points); }
span<LayerElementF3> Geometry::getNormalLayers() const { return make_span(m_normal_layers); }
span<LayerElementF2> Geometry::getUVLayers() const { return make_span(m_uv_layers); }
span<LayerElementF4> Geometry::getColorLayers() const { return make_span(m_color_layers); }

void Geometry::setCounts(span<int> v) { m_counts = v; }
void Geometry::setIndices(span<int> v) { m_indices = v; }
void Geometry::setPoints(span<float3> v) { m_points = v; }
void Geometry::addNormalLayer(const LayerElementF3& v)  { m_normal_layers.push_back(v); }
void Geometry::addNormalLayer(LayerElementF3&& v)       { m_normal_layers.push_back(std::move(v)); }
void Geometry::addUVLayer(const LayerElementF2& v)      { m_uv_layers.push_back(v); }
void Geometry::addUVLayer(LayerElementF2&& v)           { m_uv_layers.push_back(std::move(v)); }
void Geometry::addColorLayer(const LayerElementF4& v)   { m_color_layers.push_back(v); }
void Geometry::addColorLayer(LayerElementF4&& v)        { m_color_layers.push_back(std::move(v)); }


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
        m_indices = GetChildPropertyArray<int>(n, sfbxS_Indexes);
        m_weights = GetChildPropertyArray<float64>(n, sfbxS_Weights);
        m_transform = GetChildPropertyValue<double4x4>(n, sfbxS_Transform);
        m_transform_link = GetChildPropertyValue<double4x4>(n, sfbxS_TransformLink);
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
                auto nid = GetChildPropertyValue<int64>(c, sfbxS_Node);
                auto mat = GetChildPropertyValue<double4x4>(c, sfbxS_Marix);
                m_joints.push_back({ m_document->findObject(nid), float4x4(mat) });
            }
        }
    }
}

void Pose::constructNodes()
{
    super::constructNodes();
    // todo
}

std::span<sfbx::Pose::JointData> Pose::getJoints() const
{
    return make_span(m_joints);
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
