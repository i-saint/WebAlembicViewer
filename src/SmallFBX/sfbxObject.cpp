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

void Object::setNode(Node* n)
{
    m_node = n;
    if (n) {
        m_id = GetPropertyValue<int64>(n, 0);
        m_name = GetPropertyString(n, 1);
        m_subtype = GetFbxObjectSubType(GetPropertyString(n, 2));
    }
}

void Object::constructObject()
{
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

Object* Object::createChild(ObjectType type)
{
    auto ret = m_document->createObject(type);
    addChild(ret);
    return ret;
}

template<class T> T* Object::createChild()
{
    auto ret = m_document->createObject<T>();
    addChild(ret);
    return ret;
}
#define Body(T) template T* Object::createChild();
sfbxEachObjectType(Body)
#undef Body


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

void Attribute::constructObject()
{
    super::constructObject();
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

void Model::constructObject()
{
    super::constructObject();
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

void Geometry::constructObject()
{
    super::constructObject();
    auto n = getNode();
    if (!n)
        return;

    auto handle_mesh_vertices = [this](Node* n) {
        if (n->getName() != sfbxS_Vertices)
            return false;
        getMeshData()->points = GetPropertyArray<double3>(n);
        return true;
    };

    auto handle_mesh_indices = [this](Node* n) {
        if (n->getName() != sfbxS_PolygonVertexIndex)
            return false;

        auto& counts = getMeshData()->counts;
        auto& indices = getMeshData()->indices;

        auto src_indices = GetPropertyArray<int>(n);
        size_t cindices = src_indices.size();
        counts.resize(cindices); // reserve
        indices.resize(cindices);

        const int* src = src_indices.data();
        int* dst_counts = counts.data();
        int* dst_indices = indices.data();
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
        counts.resize(cfaces); // fit to actual size
        return true;
    };

    auto handle_mesh_normal_layer = [this](Node* n) {
        if (n->getName() != sfbxS_LayerElementNormal)
            return false;
        //auto mapping = n->findChildProperty(sfbxS_MappingInformationType);
        //auto ref = n->findChildProperty(sfbxS_ReferenceInformationType);
        LayerElementF3 tmp;
        tmp.name = GetChildPropertyString(n, sfbxS_Name);
        tmp.data = GetChildPropertyArray<double3>(n, sfbxS_Normals);
        tmp.indices = GetChildPropertyArray<int>(n, sfbxS_NormalsIndex);
        getMeshData()->normal_layers.push_back(std::move(tmp));
        return true;
    };

    auto handle_mesh_uv_layer = [this](Node* n) {
        if (n->getName() != sfbxS_LayerElementUV)
            return false;
        LayerElementF2 tmp;
        tmp.name = GetChildPropertyString(n, sfbxS_Name);
        tmp.data = GetChildPropertyArray<double2>(n, sfbxS_UV);
        tmp.indices = GetChildPropertyArray<int>(n, sfbxS_UVIndex);
        getMeshData()->uv_layers.push_back(std::move(tmp));
        return true;
    };

    auto handle_mesh_color_layer = [this](Node* n) {
        if (n->getName() != sfbxS_LayerElementColor)
            return false;
        LayerElementF4 tmp;
        tmp.name = GetChildPropertyString(n, sfbxS_Name);
        tmp.data = GetChildPropertyArray<double4>(n, sfbxS_Colors);
        tmp.indices = GetChildPropertyArray<int>(n, sfbxS_ColorIndex);
        getMeshData()->color_layers.push_back(std::move(tmp));
        return true;
    };


    auto handle_shape_indices = [this](Node* n) {
        if (n->getName() != sfbxS_Indexes)
            return false;
        getShapeData()->indices = GetPropertyArray<int>(n);
        return true;
    };

    auto handle_shape_points = [this](Node* n) {
        if (n->getName() != sfbxS_Vertices)
            return false;
        getShapeData()->points = GetPropertyArray<double3>(n);
        return true;
    };

    auto handle_shape_normals = [this](Node* n) {
        if (n->getName() != sfbxS_Normals)
            return false;
        getShapeData()->normals = GetPropertyArray<double3>(n);
        return true;
    };


    if (m_subtype == ObjectSubType::Mesh) {
        for (auto c : n->getChildren()) {
            handle_mesh_vertices(c) ||
                handle_mesh_indices(c) ||
                handle_mesh_normal_layer(c) ||
                handle_mesh_uv_layer(c) ||
                handle_mesh_color_layer(c);
        }
    }
    else if (m_subtype == ObjectSubType::Shape) {
        for (auto c : n->getChildren()) {
            handle_shape_indices(c) ||
                handle_shape_points(c) ||
                handle_shape_normals(c);
        }
    }
}


template<class D, class S>
static inline void CreatePropertyAndCopy(Node* dst_node, RawVector<S> src)
{
    auto dst_prop = dst_node->createProperty();
    auto dst = dst_prop->allocateArray<D>(src.size());
    copy(dst, make_span(src));
}

void Geometry::constructNodes()
{
    super::constructNodes();

    auto n = getNode();
    if (m_subtype == ObjectSubType::Mesh) {
        auto& data = *getMeshData();

        // points
        CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Vertices), data.points);

        // indices
        {
            auto& indices = data.indices;
            auto counts = data.counts.data();

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
        for (auto& layer : data.normal_layers) {
            auto normals_layer = n->createChild(sfbxS_LayerElementNormal);
            // todo
        }

        // uv layers
        for (auto& layer : data.uv_layers) {
            auto uv_layer = n->createChild(sfbxS_LayerElementUV);
            // todo
        }

        // color layers
        for (auto& layer : data.color_layers) {
            auto color_layer = n->createChild(sfbxS_LayerElementColor);
            // todo
        }
    }
    else if (m_subtype == ObjectSubType::Shape) {
        auto& data = *getShapeData();
        CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Vertices), data.points);
        CreatePropertyAndCopy<int>(n->createChild(sfbxS_Indexes), data.indices);
        CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Normals), data.normals);
    }
}

Geometry::MeshData* Geometry::getMeshData()
{
    if (!m_mesh_data) {
        if (m_subtype == ObjectSubType::Unknown)
            m_subtype = ObjectSubType::Mesh;
        m_mesh_data.reset(new MeshData());
    }
    return m_mesh_data.get();
}

Geometry::ShapeData* Geometry::getShapeData()
{
    if (!m_shape_data) {
        if (m_subtype == ObjectSubType::Unknown)
            m_subtype = ObjectSubType::Shape;
        m_shape_data.reset(new ShapeData());
    }
    return m_shape_data.get();
}



Deformer::Deformer()
{
}

ObjectType Deformer::getType() const
{
    return ObjectType::Deformer;
}

void Deformer::constructObject()
{
    super::constructObject();
    auto n = getNode();
    if (!n)
        return;

    if (m_subtype == ObjectSubType::Skin) {
        auto& data = *getSkinData();
        for (auto child : getChildren()) {
            if (child->getType() == ObjectType::Model && child->getSubType() == ObjectSubType::Cluster) {
                if (auto deformer = dynamic_cast<Deformer*>(child)) {
                    data.clusters.push_back(deformer);
                }
                else {
                    printf("sfbx::Deformer::constructObject(): non-Deformer cluster object\n");
                }
            }
        }
    }
    else if (m_subtype == ObjectSubType::Cluster) {
        auto& data = *getClusterData();
        data.indices = GetChildPropertyArray<int>(n, sfbxS_Indexes);
        data.weights = GetChildPropertyArray<float64>(n, sfbxS_Weights);
        data.transform = GetChildPropertyValue<double4x4>(n, sfbxS_Transform);
        data.transform_link = GetChildPropertyValue<double4x4>(n, sfbxS_TransformLink);
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

Deformer::BlendShapeData* Deformer::getBlendShapeData()
{
    if (!m_blendshape_data) {
        if (m_subtype == ObjectSubType::Unknown)
            m_subtype = ObjectSubType::BlendShape;
        m_blendshape_data.reset(new BlendShapeData());
    }
    return m_blendshape_data.get();
}

Deformer::BlendShapeChannelData* Deformer::getBlendShapeChannelData()
{
    if (!m_blendshape_channel_data) {
        if (m_subtype == ObjectSubType::Unknown)
            m_subtype = ObjectSubType::BlendShapeChannel;
        m_blendshape_channel_data.reset(new BlendShapeChannelData());
    }
    return m_blendshape_channel_data.get();
}

Deformer::SkinData* Deformer::getSkinData()
{
    if (!m_skin_data) {
        if (m_subtype == ObjectSubType::Unknown)
            m_subtype = ObjectSubType::Skin;
        m_skin_data.reset(new SkinData());
    }
    return m_skin_data.get();
}

Deformer::ClusterData* Deformer::getClusterData()
{
    if (!m_cluster_data) {
        if (m_subtype == ObjectSubType::Unknown)
            m_subtype = ObjectSubType::Cluster;
        m_cluster_data.reset(new ClusterData());
    }
    return m_cluster_data.get();
}


Pose::Pose()
{
}

ObjectType Pose::getType() const
{
    return ObjectType::Pose;
}



void Pose::constructObject()
{
    super::constructObject();
    auto n = getNode();
    if (!n)
        return;

    if (m_subtype == ObjectSubType::BindPose) {
        auto& data = *getBindPoseData();
        for (auto c : n->getChildren()) {
            if (c->getName() == sfbxS_PoseNode) {
                auto nid = GetChildPropertyValue<int64>(c, sfbxS_Node);
                auto mat = GetChildPropertyValue<double4x4>(c, sfbxS_Marix);
                auto joint = dynamic_cast<Model*>(m_document->findObject(nid));
                if (joint) {
                    data.joints.push_back({ joint, float4x4(mat) });
                }
                else {
                    printf("sfbx::Pose::constructObject(): non-Model joint object\n");
                }
            }
        }
    }
}

void Pose::constructNodes()
{
    super::constructNodes();
    // todo
}

Pose::BindPoseData* Pose::getBindPoseData()
{
    if (!m_bindpose_data) {
        if (m_subtype == ObjectSubType::Unknown)
            m_subtype = ObjectSubType::BindPose;
        m_bindpose_data.reset(new BindPoseData());
    }
    return m_bindpose_data.get();
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
