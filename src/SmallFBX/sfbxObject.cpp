#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxDocument.h"


namespace sfbx {

ObjectType GetFbxObjectType(const std::string& n)
{
    if (n.empty())
        return ObjectType::Unknown;
    else if (n == sfbxS_NodeAttribute)      return ObjectType::NodeAttribute;
    else if (n == sfbxS_Model)              return ObjectType::Model;
    else if (n == sfbxS_Geometry)           return ObjectType::Geometry;
    else if (n == sfbxS_Deformer)           return ObjectType::Deformer;
    else if (n == sfbxS_Pose)               return ObjectType::Pose;
    else if (n == sfbxS_Material)           return ObjectType::Material;
    else if (n == sfbxS_AnimationStack)     return ObjectType::AnimationStack;
    else if (n == sfbxS_AnimationLayer)     return ObjectType::AnimationLayer;
    else if (n == sfbxS_AnimationCurveNode) return ObjectType::AnimationCurveNode;
    else if (n == sfbxS_AnimationCurve)     return ObjectType::AnimationCurve;
    else {
        printf("GetFbxObjectType(): unknown type \"%s\"\n", n.c_str());
        return ObjectType::Unknown;
    }
}
ObjectType GetFbxObjectType(Node* n)
{
    return GetFbxObjectType(n->getName());
}

const char* GetFbxObjectName(ObjectType t)
{
    switch (t) {
    case ObjectType::NodeAttribute:     return sfbxS_NodeAttribute;
    case ObjectType::Model:             return sfbxS_Model;
    case ObjectType::Geometry:          return sfbxS_Geometry;
    case ObjectType::Deformer:          return sfbxS_Deformer;
    case ObjectType::Pose:              return sfbxS_Pose;
    case ObjectType::Material:          return sfbxS_Material;
    case ObjectType::AnimationStack:    return sfbxS_AnimationStack;
    case ObjectType::AnimationLayer:    return sfbxS_AnimationLayer;
    case ObjectType::AnimationCurveNode:return sfbxS_AnimationCurveNode;
    case ObjectType::AnimationCurve:    return sfbxS_AnimationCurve;
    default: return "";
    }
}


ObjectSubType GetFbxObjectSubType(const std::string& n)
{
    if (n.empty()) return ObjectSubType::Unknown;
    else if (n == sfbxS_Light)      return ObjectSubType::Light;
    else if (n == sfbxS_Camera)     return ObjectSubType::Camera;
    else if (n == sfbxS_Mesh)       return ObjectSubType::Mesh;
    else if (n == sfbxS_Shape)      return ObjectSubType::Shape;
    else if (n == sfbxS_Root)       return ObjectSubType::Root;
    else if (n == sfbxS_LimbNode)   return ObjectSubType::LimbNode;
    else if (n == sfbxS_Skin)       return ObjectSubType::Skin;
    else if (n == sfbxS_Cluster)    return ObjectSubType::Cluster;
    else if (n == sfbxS_BindPose)   return ObjectSubType::BindPose;
    else if (n == sfbxS_BlendShape) return ObjectSubType::BlendShape;
    else if (n == sfbxS_BlendShapeChannel) return ObjectSubType::BlendShapeChannel;
    else {
        printf("GetFbxObjectSubType(): unknown subtype \"%s\"\n", n.c_str());
        return ObjectSubType::Unknown;
    }
}

ObjectSubType GetFbxObjectSubType(Node* n)
{
    return GetFbxObjectSubType(GetPropertyString(n, 2));
}

const char* GetFbxObjectSubName(ObjectSubType t)
{
    switch (t) {
    case ObjectSubType::Light:      return sfbxS_Light;
    case ObjectSubType::Camera:     return sfbxS_Camera;
    case ObjectSubType::Mesh:       return sfbxS_Mesh;
    case ObjectSubType::Shape:      return sfbxS_Shape;
    case ObjectSubType::Root:       return sfbxS_Root;
    case ObjectSubType::LimbNode:   return sfbxS_LimbNode;
    case ObjectSubType::Skin:       return sfbxS_Skin;
    case ObjectSubType::Cluster:    return sfbxS_Cluster;
    case ObjectSubType::BindPose:   return sfbxS_BindPose;
    case ObjectSubType::BlendShape: return sfbxS_BlendShape;
    case ObjectSubType::BlendShapeChannel: return sfbxS_BlendShapeChannel;
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

template<class T> T* Object::createChild(const std::string& name)
{
    auto ret = m_document->createObject<T>();
    ret->setName(name);
    addChild(ret);
    return ret;
}
#define Body(T) template T* Object::createChild(const std::string& name);
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



ObjectType NodeAttribute::getType() const
{
    return ObjectType::NodeAttribute;
}

void NodeAttribute::constructObject()
{
    super::constructObject();
    // todo
}

void NodeAttribute::constructNodes()
{
    super::constructNodes();
    // todo
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

float4x4 Model::getLocalMatrix() const
{
    return transform(m_position, rotate_euler(m_rotation_order, m_rotation), m_scale);
}

float4x4 Model::getGlobalMatrix() const
{
    float4x4 pmat = float4x4::identity();
    for (Object* p = getParent(); p; p = p->getParent()) {
        if (Model* pm = as<Model>(p)) {
            pmat = pm->getGlobalMatrix();
            break;
        }
    }
    return pmat * getLocalMatrix();
}

void Model::setVisibility(bool v) { m_visibility = v; }
void Model::setRotationOrder(RotationOrder v) { m_rotation_order = v; }
void Model::setPosition(float3 v) { m_position = v; }
void Model::setRotation(float3 v) { m_rotation = v; }
void Model::setScale(float3 v) { m_scale = v; }


void Light::constructObject()
{
    super::constructObject();
    // todo
}

void Light::constructNodes()
{
    super::constructNodes();
    // todo
}


void Camera::constructObject()
{
    super::constructObject();
    // todo
}

void Camera::constructNodes()
{
    super::constructNodes();
    // todo
}



ObjectType Geometry::getType() const { return ObjectType::Geometry; }

template<class D, class S>
static inline void CreatePropertyAndCopy(Node* dst_node, RawVector<S> src)
{
    auto dst_prop = dst_node->createProperty();
    auto dst = dst_prop->allocateArray<D>(src.size());
    copy(dst, make_span(src));
}

void Mesh::constructObject()
{
    super::constructObject();

    for (auto n : getNode()->getChildren()) {
        if (n->getName() == sfbxS_Vertices) {
            // points
            m_points = GetPropertyArray<double3>(n);
        }
        else if (n->getName() == sfbxS_PolygonVertexIndex) {
            // counts & indices
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
        }
        else if (n->getName() == sfbxS_LayerElementNormal) {
            // normals
            //auto mapping = n->findChildProperty(sfbxS_MappingInformationType);
            //auto ref = n->findChildProperty(sfbxS_ReferenceInformationType);
            LayerElementF3 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            tmp.data = GetChildPropertyArray<double3>(n, sfbxS_Normals);
            tmp.indices = GetChildPropertyArray<int>(n, sfbxS_NormalsIndex);
            m_normal_layers.push_back(tmp);
        }
        else if (n->getName() == sfbxS_LayerElementUV) {
            // uv
            LayerElementF2 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            tmp.data = GetChildPropertyArray<double2>(n, sfbxS_UV);
            tmp.indices = GetChildPropertyArray<int>(n, sfbxS_UVIndex);
            m_uv_layers.push_back(tmp);
        }
        else if (n->getName() == sfbxS_LayerElementColor) {
            // colors
            LayerElementF4 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            tmp.data = GetChildPropertyArray<double4>(n, sfbxS_Colors);
            tmp.indices = GetChildPropertyArray<int>(n, sfbxS_ColorIndex);
            m_color_layers.push_back(tmp);
        }
    }
}

void Mesh::constructNodes()
{
    super::constructNodes();

    Node* n = getNode();

    // points
    CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Vertices), m_points);

    // indices
    {
        auto* src_counts = m_counts.data();
        auto dst_node = n->createChild(sfbxS_PolygonVertexIndex);
        auto dst_prop = dst_node->createProperty();
        auto dst_indices = dst_prop->allocateArray<int>(m_indices.size()).data();

        size_t cpoints = 0;
        for (int i : m_indices) {
            if (++cpoints == *src_counts) {
                i = ~i; // negative value indicates the last index in the face
                cpoints = 0;
                ++src_counts;
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

span<int> Mesh::getCounts() const { return make_span(m_counts); }
span<int> Mesh::getIndices() const { return make_span(m_indices); }
span<float3> Mesh::getPoints() const { return make_span(m_points); }
span<LayerElementF3> Mesh::getNormalLayers() const { return make_span(m_normal_layers); }
span<LayerElementF2> Mesh::getUVLayers() const { return make_span(m_uv_layers); }
span<LayerElementF4> Mesh::getColorLayers() const { return make_span(m_color_layers); }

void Mesh::setCounts(span<int> v) { m_counts = v; }
void Mesh::setIndices(span<int> v) { m_indices = v; }
void Mesh::setPoints(span<float3> v) { m_points = v; }
void Mesh::addNormalLayer(LayerElementF3&& v) { m_normal_layers.push_back(v); }
void Mesh::addUVLayer(LayerElementF2&& v) { m_uv_layers.push_back(v); }
void Mesh::addColorLayer(LayerElementF4&& v) { m_color_layers.push_back(v); }


void Shape::constructObject()
{
    super::constructObject();

    for (auto n : getNode()->getChildren()) {
        if (n->getName() == sfbxS_Indexes) {
            m_indices = GetPropertyArray<int>(n);
        }
        else if (n->getName() == sfbxS_Vertices) {
            m_delta_points = GetPropertyArray<double3>(n);
        }
        else if (n->getName() == sfbxS_Normals) {
            m_delta_normals = GetPropertyArray<double3>(n);
        }
    }
}

void Shape::constructNodes()
{
    super::constructNodes();

    Node* n = getNode();
    CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Vertices), m_delta_points);
    CreatePropertyAndCopy<int>(n->createChild(sfbxS_Indexes), m_indices);
    CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Normals), m_delta_normals);
}

Mesh* Shape::getBaseMesh()
{
    for (auto* p = getParent(); p; p=p->getParent()) {
        if (auto mesh = as<Mesh>(p))
            return mesh;
    }
    return nullptr;
}

span<int> Shape::getIndices() const { return make_span(m_indices); }
span<float3> Shape::getDeltaPoints() const { return make_span(m_delta_points); }
span<float3> Shape::getDeltaNormals() const { return make_span(m_delta_normals); }

void Shape::setIndices(span<int> v) { m_indices = v; }
void Shape::setDeltaPoints(span<float3> v) { m_delta_points = v; }
void Shape::setDeltaNormals(span<float3> v) { m_delta_normals = v; }



ObjectType Deformer::getType() const
{
    return ObjectType::Deformer;
}


void Skin::constructObject()
{
    super::constructObject();

    for (auto child : getChildren()) {
        if (auto cluster = as<Cluster>(child)) {
            m_clusters.push_back(cluster);
        }
        else {
            printf("sfbx::Deformer::constructObject(): non-Deformer cluster object\n");
        }
    }
}

void Skin::constructNodes()
{
    super::constructNodes();
}

void Skin::addChild(Object* v)
{
    super::addChild(v);
    if (auto cluster = as<Cluster>(v))
        m_clusters.push_back(cluster);
}

span<Cluster*> Skin::getClusters() const { return make_span(m_clusters); }

JointWeights Skin::getJointWeightsVariable()
{
    JointWeights ret;
    auto mesh = as<Mesh>(getParent());
    if (!mesh)
        return ret;

    size_t cclusters = m_clusters.size();
    size_t cpoints = mesh->getPoints().size();
    size_t total_weights = 0;

    // setup counts
    ret.counts.resize(cpoints, 0);
    for (auto cluster : m_clusters) {
        auto indices = cluster->getIndices();
        for (int vi : indices)
            ret.counts[vi]++;
        total_weights += indices.size();
    }

    // setup offsets
    ret.offsets.resize(cpoints);
    size_t offset = 0;
    int max_joints_per_vertex = 0;
    for (size_t pi = 0; pi < cpoints; ++pi) {
        ret.offsets[pi] = offset;
        int c = ret.counts[pi];
        offset += c;
        max_joints_per_vertex = std::max(max_joints_per_vertex, c);
    }
    ret.max_joints_per_vertex = max_joints_per_vertex;

    // setup weights
    ret.counts.zeroclear(); // clear to recount
    ret.weights.resize(total_weights);
    for (size_t ci = 0; ci < cclusters; ++ci) {
        auto cluster = m_clusters[ci];
        auto indices = cluster->getIndices();
        auto weights = cluster->getWeights();
        size_t cweights = weights.size();
        for (size_t wi = 0; wi < cweights; ++wi) {
            int vi = indices[wi];
            float weight = weights[wi];
            int pos = ret.offsets[vi] + ret.counts[vi]++;
            ret.weights[pos] = { (int)ci, weight };
        }
    }
    return ret;
}

JointWeights Skin::getJointWeightsFixed(int joints_per_vertex)
{
    JointWeights tmp = getJointWeightsVariable();
    if (tmp.weights.empty())
        return tmp;

    JointWeights ret;
    size_t cpoints = tmp.counts.size();
    ret.max_joints_per_vertex = std::min(joints_per_vertex, tmp.max_joints_per_vertex);
    ret.counts.resize(cpoints);
    ret.offsets.resize(cpoints);
    ret.weights.resize(cpoints * joints_per_vertex);
    ret.weights.zeroclear();

    auto normalize_weights = [](span<JointWeight> weights) {
        float total = 0.0f;
        for (auto& w : weights)
            total += w.weight;

        if (total != 0.0f) {
            float rcp_total = 1.0f / total;
            for (auto& w : weights)
                w.weight *= rcp_total;
        }
    };

    for (size_t pi = 0; pi < cpoints; ++pi) {
        int count = tmp.counts[pi];
        ret.counts[pi] = std::min(count, joints_per_vertex);
        ret.offsets[pi] = joints_per_vertex * pi;

        auto* src = &tmp.weights[tmp.offsets[pi]];
        auto* dst = &ret.weights[ret.offsets[pi]];
        if (count < joints_per_vertex) {
            copy(dst, src, size_t(count));
        }
        else {
            std::nth_element(src, src + joints_per_vertex, src + count,
                [](auto& a, auto& b) { return a.weight < b.weight; });
            normalize_weights(make_span(src, joints_per_vertex));
            copy(dst, src, size_t(joints_per_vertex));
        }
    }
    return ret;
}

JointMatrices Skin::getJointMatrices()
{
    JointMatrices ret;

    size_t cclusters = m_clusters.size();

    ret.bindpose.resize(cclusters);
    ret.global_transform.resize(cclusters);
    ret.joint_transform.resize(cclusters);
    for (size_t ci = 0; ci < cclusters; ++ci) {
        auto bindpose = m_clusters[ci]->getTransform();
        ret.bindpose[ci] = bindpose;
        if (auto trans = as<Model>(m_clusters[ci]->getChild())) {
            auto global_matrix = trans->getGlobalMatrix();
            ret.global_transform[ci] = global_matrix;
            ret.joint_transform[ci] = bindpose * global_matrix;
        }
        else {
            // should not be here
            printf("sfbx::Deformer::skinMakeJointMatrices(): Cluster has non-Model child\n");
            ret.global_transform[ci] = ret.joint_transform[ci] = float4x4::identity();
        }
    }
    return ret;
}


void Cluster::constructObject()
{
    super::constructObject();

    auto n = getNode();
    m_indices = GetChildPropertyArray<int>(n, sfbxS_Indexes);
    m_weights = GetChildPropertyArray<float64>(n, sfbxS_Weights);
    m_transform = GetChildPropertyValue<double4x4>(n, sfbxS_Transform);
    m_transform_link = GetChildPropertyValue<double4x4>(n, sfbxS_TransformLink);
}

void Cluster::constructNodes()
{
    super::constructNodes();
}

span<int> Cluster::getIndices() const { return make_span(m_indices); }
span<float> Cluster::getWeights() const { return make_span(m_weights); }
float4x4 Cluster::getTransform() const { return m_transform; }
float4x4 Cluster::getTransformLink() const { return m_transform_link; }

void Cluster::setIndices(span<int> v) { m_indices = v; }
void Cluster::setWeights(span<float> v) { m_weights = v; }
void Cluster::setTransform(float4x4 v) { m_transform = v; }
void Cluster::setTransformLink(float4x4 v) { m_transform_link = v; }


void BlendShape::constructObject()
{
    super::constructObject();
}

void BlendShape::constructNodes()
{
    super::constructNodes();
}


void BlendShapeChannel::constructObject()
{
    super::constructObject();
}

void BlendShapeChannel::constructNodes()
{
    super::constructNodes();
}



ObjectType Pose::getType() const { return ObjectType::Pose; }

void BindPose::constructObject()
{
    super::constructObject();

    for (auto n : getNode()->getChildren()) {
        if (n->getName() == sfbxS_PoseNode) {
            auto nid = GetChildPropertyValue<int64>(n, sfbxS_Node);
            auto mat = GetChildPropertyValue<double4x4>(n, sfbxS_Marix);
            auto model = as<Model>(m_document->findObject(nid));
            if (model) {
                m_pose_data.push_back({ model, float4x4(mat) });
            }
            else {
                printf("sfbx::Pose::constructObject(): non-Model joint object\n");
            }
        }
    }
}

void BindPose::constructNodes()
{
    super::constructNodes();
    // todo
}

span<BindPose::PoseData> BindPose::getPoseData() const { return make_span(m_pose_data); }
void BindPose::addPoseData(const PoseData& v) { m_pose_data.push_back(v); }




Material::Material() {}
ObjectType Material::getType() const { return ObjectType::Material; }

void Material::constructObject()
{
    super::constructNodes();
    // todo
}

void Material::constructNodes()
{
    super::constructNodes();
    // todo
}



ObjectType AnimationStack::getType() const { return ObjectType::AnimationStack; }


ObjectType AnimationLayer::getType() const { return ObjectType::AnimationLayer; }

void AnimationLayer::constructObject()
{
    super::constructObject();
    for (auto* n : getChildren()) {
        if (auto* cnode = as<AnimationCurveNode>(n)) {
            // operator== doesn't return the expected result because node names may contain '\0' (e.g. "T\0AnimCurveNode")
            if (std::strcmp(cnode->getName().c_str(), sfbxS_T) == 0)
                m_position = cnode;
            else if (std::strcmp(cnode->getName().c_str(), sfbxS_R) == 0)
                m_rotation = cnode;
            else if (std::strcmp(cnode->getName().c_str(), sfbxS_S) == 0)
                m_scale = cnode;
            else if (std::strcmp(cnode->getName().c_str(), sfbxS_FocalLength) == 0)
                m_focal_length = cnode;
        }
    }
}

void AnimationLayer::constructNodes()
{
    super::constructNodes();
    // todo
}

AnimationCurveNode* AnimationLayer::getPosition() const     { return m_position; }
AnimationCurveNode* AnimationLayer::getRotation() const     { return m_rotation; }
AnimationCurveNode* AnimationLayer::getScale() const        { return m_scale; }
AnimationCurveNode* AnimationLayer::getFocalLength() const  { return m_focal_length; }


ObjectType AnimationCurveNode::getType() const { return ObjectType::AnimationCurveNode; }

void AnimationCurveNode::constructObject()
{
    super::constructObject();
    for (auto* n : getChildren()) {
        if (auto* curve = as<AnimationCurve>(n))
            m_curves.push_back(curve);
    }
}

void AnimationCurveNode::constructNodes()
{
    super::constructNodes();
    // todo
}

void AnimationCurveNode::addChild(Object* v)
{
    super::addChild(v);
    if (auto curve = as<AnimationCurve>(v))
        m_curves.push_back(curve);
}

float AnimationCurveNode::getStartTime() const
{
    return m_curves.empty() ? 0.0f : m_curves[0]->getStartTime();
}

float AnimationCurveNode::getEndTime() const
{
    return m_curves.empty() ? 0.0f : m_curves[0]->getEndTime();
}

float AnimationCurveNode::evaluate(float time) const
{
    if (m_curves.empty())
        return 0.0f;
    return m_curves[0]->evaluate(time);
}

float3 AnimationCurveNode::evaluate3(float time) const
{
    if (m_curves.size() != 3)
        return float3::zero();

    return float3{
        m_curves[0]->evaluate(time),
        m_curves[1]->evaluate(time),
        m_curves[2]->evaluate(time),
    };
}

void AnimationCurveNode::addValue(float time, float value)
{
    if (m_curves.empty()) {
        createChild<AnimationCurve>();
    }
    if (m_curves.size() != 1) {
        printf("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
    }
    m_curves[0]->addValue(time, value);
}

void AnimationCurveNode::addValue(float time, float3 value)
{
    if (m_curves.empty()) {
        createChild<AnimationCurve>();
        createChild<AnimationCurve>();
        createChild<AnimationCurve>();
    }
    if (m_curves.size() != 3) {
        printf("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
    }
    m_curves[0]->addValue(time, value.x);
    m_curves[1]->addValue(time, value.y);
    m_curves[2]->addValue(time, value.z);
}

ObjectType AnimationCurve::getType() const { return ObjectType::AnimationCurve; }

void AnimationCurve::constructObject()
{
    super::constructObject();

    auto n = getNode();
    m_default = (float32)GetChildPropertyValue<float64>(n, sfbxS_Default);
    transform(m_times, GetChildPropertyArray<int64>(n, sfbxS_KeyTime),
        [](int64 v) { return float((double)v / sfbxI_TicksPerSecond); });
    m_values = GetChildPropertyArray<float32>(n, sfbxS_KeyValueFloat);
}

void AnimationCurve::constructNodes()
{
    super::constructNodes();
    // todo
}

span<float> AnimationCurve::getTimes() const { return make_span(m_times); }
span<float> AnimationCurve::getValues() const { return make_span(m_values); }

float AnimationCurve::getStartTime() const { return m_times.empty() ? 0.0f : m_times.front(); }
float AnimationCurve::getEndTime() const { return m_times.empty() ? 0.0f : m_times.back(); }

float AnimationCurve::evaluate(float time) const
{
    if (m_times.empty())
        return m_default;
    else if (time <= m_times.front())
        return m_values.front();
    else if (time >= m_times.back())
        return m_values.back();
    else {
        // lerp
        auto it = std::lower_bound(m_times.begin(), m_times.end(), time);
        size_t i = std::distance(m_times.begin(), it);

        float t2 = m_times[i];
        float v2 = m_values[i];
        if (time == t2)
            return v2;

        float t1 = m_times[i - 1];
        float v1 = m_values[i - 1];
        float w = (time - t1) / (t2 - t1);
        return v1 + (v2 - v1) * w;
    }
}

void AnimationCurve::setTimes(span<float> v) { m_times = v; }
void AnimationCurve::setValues(span<float> v) { m_values = v; }

void AnimationCurve::addValue(float time, float value)
{
    m_times.push_back(time);
    m_values.push_back(value);
}

} // namespace sfbx
