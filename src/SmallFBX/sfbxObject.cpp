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
        sfbxPrint("GetFbxObjectType(): unknown type \"%s\"\n", n.c_str());
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
        sfbxPrint("GetFbxObjectSubType(): unknown subtype \"%s\"\n", n.c_str());
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
    std::string name = m_name;
    name += (char)0;
    name += (char)1;
    name += GetFbxObjectName(getType());

    auto objects = m_document->findNode(sfbxS_Objects);
    m_node = objects->createChild(GetFbxObjectName(getType()));
    m_node->addProperty(m_id);
    m_node->addProperty(name);
    m_node->addProperty(GetFbxObjectSubName(getSubType()));

    if (auto connections = m_document->findNode(sfbxS_Connections)) {
        auto add_link_oo = [connections, this](int64 id) {
            connections->addChild(sfbxS_C, sfbxS_OO, getID(), id);
        };

        auto parents = getParents();
        if (parents.empty()) {
            // this seems required
            add_link_oo(0);
        }
        else {
            for (auto parent : parents)
                add_link_oo(parent->getID());
        }
    }
}

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
        if (getType() != ObjectType::Model)
            setName(v->getName());
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
            else if (pname == sfbxS_LclTranslation) {
                m_position = float3{
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                    (float)GetPropertyValue<float64>(p, 6),
                };
            }
            else if (pname == sfbxS_RotationOrder) {
                m_rotation_order = (RotationOrder)GetPropertyValue<int32>(p, 4);
            }
            else if (pname == sfbxS_PreRotation) {
                m_pre_rotation = float3{
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                    (float)GetPropertyValue<float64>(p, 6),
                };
            }
            else if (pname == sfbxS_PostRotation) {
                m_post_rotation = float3{
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

#define sfbxVector3d(V) (float64)V.x, (float64)V.y, (float64)V.z

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
        properties->addChild(sfbxS_P,
            sfbxS_LclTranslation, sfbxS_LclTranslation, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_position));
    }

    // rotation active
    if (m_pre_rotation != float3::zero() || m_post_rotation != float3::zero() || m_rotation != float3::zero()) {
        properties->addChild(sfbxS_P,
            sfbxS_RotationActive, sfbxS_bool, sfbxS_Empty, sfbxS_Empty, (int32)1);

        // rotation order
        if (m_rotation_order != RotationOrder::XYZ) {
            properties->addChild(sfbxS_P,
                sfbxS_RotationOrder, sfbxS_RotationOrder, sfbxS_Empty, sfbxS_A, (int32)m_rotation_order);
        }

        // pre-rotation
        if (m_pre_rotation != float3::zero()) {
            properties->addChild(sfbxS_P,
                sfbxS_PreRotation, sfbxS_Vector3D, sfbxS_Vector, sfbxS_Empty, sfbxVector3d(m_pre_rotation));
        }

        // post-rotation
        if (m_post_rotation != float3::zero()) {
            properties->addChild(sfbxS_P,
                sfbxS_PostRotation, sfbxS_Vector3D, sfbxS_Vector, sfbxS_Empty, sfbxVector3d(m_post_rotation));
        }

        // rotation
        if (m_rotation != float3::zero()) {
            properties->addChild(sfbxS_P,
                sfbxS_LclRotation, sfbxS_LclRotation, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_rotation));
        }
    }

    // scale
    if (m_scale!= float3::one()) {
        properties->addChild(sfbxS_P,
            sfbxS_LclScale, sfbxS_LclScale, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_scale));
    }
}

void Model::addParent(Object* v)
{
    super::addParent(v);
    if (auto model = as<Model>(v))
        m_parent_model = model;
}

void Model::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<NodeAttribute>(v))
        m_node_attributes.push_back(attr);
    else if (auto material = as<Material>(v))
        m_materials.push_back(material);
    else if (auto mesh = as<Mesh>(v))
        m_mesh = mesh;
}

Model* Model::getParentModel() const { return m_parent_model; }
span<NodeAttribute*> Model::getNodeAttributes() const { return make_span(m_node_attributes); }
span<Material*> Model::getMaterials() const { return make_span(m_materials); }
Camera* Model::getCamera() const { return as<Camera>(const_cast<Model*>(this)); }
Light* Model::getLight() const { return as<Light>(const_cast<Model*>(this)); }
Mesh* Model::getMesh() const { return m_mesh; }

bool Model::getVisibility() const { return m_visibility; }
RotationOrder Model::getRotationOrder() const { return m_rotation_order; }
float3 Model::getPosition() const { return m_position; }

float3 Model::getPreRotation() const { return m_pre_rotation; }
float3 Model::getRotation() const { return m_rotation; }
float3 Model::getPostRotation() const { return m_post_rotation; }
float3 Model::getScale() const { return m_scale; }

float4x4 Model::getLocalMatrix() const
{
    // scale
    float4x4 ret = scale44(m_scale);

    // position
    if (m_pre_rotation != float3::zero())
        ret *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_pre_rotation * DegToRad)));
    if (m_rotation != float3::zero())
        ret *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_rotation * DegToRad)));
    if (m_post_rotation != float3::zero())
        ret *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_post_rotation * DegToRad)));

    // translation
    (float3&)ret[3] = m_position;
    return ret;
}

float4x4 Model::getGlobalMatrix() const
{
    float4x4 pmat = m_parent_model ? m_parent_model->getGlobalMatrix() : float4x4::identity();
    return getLocalMatrix() * pmat;
}

void Model::setVisibility(bool v) { m_visibility = v; }
void Model::setRotationOrder(RotationOrder v) { m_rotation_order = v; }
void Model::setPosition(float3 v) { m_position = v; }
void Model::setPreRotation(float3 v) { m_pre_rotation = v; }
void Model::setRotation(float3 v) { m_rotation = v; }
void Model::setPostRotation(float3 v) { m_post_rotation = v; }
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

void Geometry::addChild(Object* v)
{
    super::addChild(v);
    if (auto deformer = as<Deformer>(v))
        m_deformers.push_back(deformer);
}

span<Deformer*> Geometry::getDeformers() const { return make_span(m_deformers); }


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
            m_normal_layers.push_back(std::move(tmp));
        }
        else if (n->getName() == sfbxS_LayerElementUV) {
            // uv
            LayerElementF2 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            tmp.data = GetChildPropertyArray<double2>(n, sfbxS_UV);
            tmp.indices = GetChildPropertyArray<int>(n, sfbxS_UVIndex);
            m_uv_layers.push_back(std::move(tmp));
        }
        else if (n->getName() == sfbxS_LayerElementColor) {
            // colors
            LayerElementF4 tmp;
            tmp.name = GetChildPropertyString(n, sfbxS_Name);
            tmp.data = GetChildPropertyArray<double4>(n, sfbxS_Colors);
            tmp.indices = GetChildPropertyArray<int>(n, sfbxS_ColorIndex);
            m_color_layers.push_back(std::move(tmp));
        }
    }
}

void Mesh::constructNodes()
{
    setSubType(ObjectSubType::Mesh);
    super::constructNodes();

    Node* n = getNode();

    n->addChild(sfbxS_GeometryVersion, sfbxI_GeometryVersion);

    // points
    n->addChild(sfbxS_Vertices, MakeAdaptor<double3>(m_points));

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
    }

    auto add_mapping_and_reference_info = [this](Node* node, const auto& layer) {
        if (layer.data.size() == m_indices.size() || layer.indices.size() == m_indices.size())
            node->addChild(sfbxS_MappingInformationType, "ByPolygonVertex");
        else if (layer.data.size() == m_points.size() && layer.indices.empty())
            node->addChild(sfbxS_MappingInformationType, "ByControllPoint");

        if (!layer.indices.empty())
            node->addChild(sfbxS_ReferenceInformationType, "IndexToDirect");
        else
            node->addChild(sfbxS_ReferenceInformationType, "Direct");
    };

    int clayers = 0;

    // normal layers
    for (auto& layer : m_normal_layers) {
        ++clayers;
        auto l = n->createChild(sfbxS_LayerElementNormal);
        l->addChild(sfbxS_Version, sfbxI_LayerElementNormalVersion);
        l->addChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
        l->addChild(sfbxS_Normals, MakeAdaptor<double3>(layer.data));
        if (!layer.indices.empty())
            l->addChild(sfbxS_NormalsIndex, layer.indices);
    }

    // uv layers
    for (auto& layer : m_uv_layers) {
        ++clayers;
        auto l = n->createChild(sfbxS_LayerElementUV);
        l->addChild(sfbxS_Version, sfbxI_LayerElementUVVersion);
        l->addChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
        l->addChild(sfbxS_UV, MakeAdaptor<double2>(layer.data));
        if (!layer.indices.empty())
            l->addChild(sfbxS_UVIndex, layer.indices);
    }

    // color layers
    for (auto& layer : m_color_layers) {
        ++clayers;
        auto l = n->createChild(sfbxS_LayerElementColor);
        l->addChild(sfbxS_Version, sfbxI_LayerElementColorVersion);
        l->addChild(sfbxS_Name, layer.name);

        add_mapping_and_reference_info(l, layer);
        l->addChild(sfbxS_Colors, MakeAdaptor<double4>(layer.data));
        if (!layer.indices.empty())
            l->addChild(sfbxS_ColorIndex, layer.indices);
    }

    if (clayers) {
        auto l = n->createChild(sfbxS_Layer);
        l->addProperties(0);
        l->addChild(sfbxS_Version, sfbxI_LayerVersion);
        if (!m_normal_layers.empty()) {
            auto le = l->createChild(sfbxS_LayerElement);
            le->addChild(sfbxS_Type, sfbxS_LayerElementNormal);
            le->addChild(sfbxS_TypeIndex, 0);
        }
        if (!m_uv_layers.empty()) {
            auto le = l->createChild(sfbxS_LayerElement);
            le->addChild(sfbxS_Type, sfbxS_LayerElementUV);
            le->addChild(sfbxS_TypeIndex, 0);
        }
        if (!m_color_layers.empty()) {
            auto le = l->createChild(sfbxS_LayerElement);
            le->addChild(sfbxS_Type, sfbxS_LayerElementColor);
            le->addChild(sfbxS_TypeIndex, 0);
        }
    }
}

void Mesh::addParent(Object* v)
{
    super::addParent(v);
    if (auto model = as<Model>(v))
        model->setSubType(ObjectSubType::Mesh);
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
    setSubType(ObjectSubType::Shape);
    super::constructNodes();

    Node* n = getNode();
    n->addChild(sfbxS_Vertices, MakeAdaptor<double3>(m_delta_points));
    n->addChild(sfbxS_Indexes, m_indices);
    n->addChild(sfbxS_Normals, MakeAdaptor<double3>(m_delta_normals));
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
}

void Skin::constructNodes()
{
    setSubType(ObjectSubType::Skin);
    super::constructNodes();
}

void Skin::addParent(Object* v)
{
    super::addParent(v);
    if (auto mesh = as<Mesh>(v))
        m_mesh = mesh;
}

void Skin::addChild(Object* v)
{
    super::addChild(v);
    if (auto cluster = as<Cluster>(v))
        m_clusters.push_back(cluster);
}

Mesh* Skin::getMesh() const { return m_mesh; }
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
            sfbxPrint("sfbx::Deformer::skinMakeJointMatrices(): Cluster has non-Model child\n");
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
    setSubType(ObjectSubType::Cluster);
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
    setSubType(ObjectSubType::BlendShape);
    super::constructNodes();
}


void BlendShapeChannel::constructObject()
{
    super::constructObject();
}

void BlendShapeChannel::constructNodes()
{
    setSubType(ObjectSubType::BlendShapeChannel);
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
                sfbxPrint("sfbx::Pose::constructObject(): non-Model joint object\n");
            }
        }
    }
}

void BindPose::constructNodes()
{
    setSubType(ObjectSubType::BindPose);
    super::constructNodes();
    // todo
}

span<BindPose::PoseData> BindPose::getPoseData() const { return make_span(m_pose_data); }
void BindPose::addPoseData(const PoseData& v) { m_pose_data.push_back(v); }




Material::Material() {}
ObjectType Material::getType() const { return ObjectType::Material; }

void Material::constructObject()
{
    super::constructObject();
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
        sfbxPrint("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
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
        sfbxPrint("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
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
