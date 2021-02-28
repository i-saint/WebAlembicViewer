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
    else if (n == "AnimationStack")
        return ObjectType::AnimationStack;
    else if (n == "AnimationLayer")
        return ObjectType::AnimationLayer;
    else if (n == "AnimationCurveNode")
        return ObjectType::AnimationCurveNode;
    else if (n == "AnimationCurve")
        return ObjectType::AnimationCurve;
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
    case ObjectType::AnimationStack: return "AnimationStack";
    case ObjectType::AnimationLayer: return "AnimationLayer";
    case ObjectType::AnimationCurveNode: return "AnimationCurveNode";
    case ObjectType::AnimationCurve: return "AnimationCurve";
    default: return "";
    }
}


ObjectSubType GetFbxObjectSubType(const std::string& n)
{
    if (n.empty())
        return ObjectSubType::Unknown;
    else if (n == "Light")
        return ObjectSubType::Light;
    else if (n == "Camera")
        return ObjectSubType::Camera;
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
    case ObjectSubType::Light: return "Light";
    case ObjectSubType::Camera: return "Camera";
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
        getShapeData()->delta_points = GetPropertyArray<double3>(n);
        return true;
    };

    auto handle_shape_normals = [this](Node* n) {
        if (n->getName() != sfbxS_Normals)
            return false;
        getShapeData()->delta_normals = GetPropertyArray<double3>(n);
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
        CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Vertices), data.delta_points);
        CreatePropertyAndCopy<int>(n->createChild(sfbxS_Indexes), data.indices);
        CreatePropertyAndCopy<double3>(n->createChild(sfbxS_Normals), data.delta_normals);
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
            if (child->getType() == ObjectType::Deformer && child->getSubType() == ObjectSubType::Cluster) {
                if (auto deformer = as<Deformer>(child)) {
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


Deformer::JointWeights Deformer::skinGetJointWeightsVariable()
{
    JointWeights ret;
    auto geom = as<Geometry>(getParent(0));
    if (getSubType() != ObjectSubType::Skin || !geom || geom->getSubType() != ObjectSubType::Mesh)
        return ret;

    auto& mesh = *geom->getMeshData();
    auto& skin = *getSkinData();

    size_t cclusters = skin.clusters.size();
    size_t cpoints = mesh.points.size();
    size_t total_weights = 0;

    // setup counts
    ret.counts.resize(cpoints, 0);
    for (auto ncluster : skin.clusters) {
        auto& cluster = *ncluster->getClusterData();
        for (int vi : cluster.indices)
            ret.counts[vi]++;
        total_weights += cluster.indices.size();
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
        auto& cluster = *skin.clusters[ci]->getClusterData();
        size_t cweights = cluster.indices.size();
        for (size_t wi = 0; wi < cweights; ++wi) {
            int vi = cluster.indices[wi];
            float weight = cluster.weights[wi];
            int pos = ret.offsets[vi] + ret.counts[vi]++;
            ret.weights[pos] = { (int)ci, weight };
        }
    }
    return ret;
}

Deformer::JointWeights Deformer::skinGetJointWeightsFixed(int joints_per_vertex)
{
    JointWeights ret;
    auto geom = as<Geometry>(getParent(0));
    if (getSubType() != ObjectSubType::Skin || !geom || geom->getSubType() != ObjectSubType::Mesh)
        return ret;

    JointWeights tmp = skinGetJointWeightsVariable();
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

Deformer::JointMatrices Deformer::skinGetJointMatrices()
{
    JointMatrices ret;
    if (getSubType() != ObjectSubType::Skin)
        return ret;

    auto& skin = *getSkinData();
    size_t cclusters = skin.clusters.size();

    ret.bindpose.resize(cclusters);
    ret.transform.resize(cclusters);
    for (size_t ci = 0; ci < cclusters; ++ci) {
        ret.bindpose[ci] = skin.clusters[ci]->getClusterData()->transform;
        if (auto trans = as<Model>(skin.clusters[ci]->getChild())) {
            ret.transform[ci] = trans->getGlobalMatrix();
        }
        else {
            // should not be here
            printf("sfbx::Deformer::skinMakeJointMatrices(): Cluster has non-Model child\n");
            ret.transform[ci] = float4x4::identity();
        }
    }
    return ret;
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
                auto joint = as<Model>(m_document->findObject(nid));
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

ObjectType AnimationStack::getType() const { return ObjectType::AnimationStack; }
ObjectType AnimationLayer::getType() const { return ObjectType::AnimationLayer; }
ObjectType AnimationCurveNode::getType() const { return ObjectType::AnimationCurveNode; }
ObjectType AnimationCurve::getType() const { return ObjectType::AnimationCurve; }


} // namespace sfbx
