#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxDocument.h"


namespace sfbx {

ObjectClass GetFbxObjectClass(const std::string& n)
{
    if (n.empty())
        return ObjectClass::Unknown;
    else if (n == sfbxS_NodeAttribute)      return ObjectClass::NodeAttribute;
    else if (n == sfbxS_Model)              return ObjectClass::Model;
    else if (n == sfbxS_Geometry)           return ObjectClass::Geometry;
    else if (n == sfbxS_Deformer)           return ObjectClass::Deformer;
    else if (n == sfbxS_Pose)               return ObjectClass::Pose;
    else if (n == sfbxS_Material)           return ObjectClass::Material;
    else if (n == sfbxS_AnimationStack)     return ObjectClass::AnimationStack;
    else if (n == sfbxS_AnimationLayer)     return ObjectClass::AnimationLayer;
    else if (n == sfbxS_AnimationCurveNode) return ObjectClass::AnimationCurveNode;
    else if (n == sfbxS_AnimationCurve)     return ObjectClass::AnimationCurve;
    else {
        sfbxPrint("GetFbxObjectClass(): unknown type \"%s\"\n", n.c_str());
        return ObjectClass::Unknown;
    }
}
ObjectClass GetFbxObjectClass(Node* n)
{
    return GetFbxObjectClass(n->getName());
}

const char* GetFbxClassName(ObjectClass t)
{
    switch (t) {
    case ObjectClass::NodeAttribute:     return sfbxS_NodeAttribute;
    case ObjectClass::Model:             return sfbxS_Model;
    case ObjectClass::Geometry:          return sfbxS_Geometry;
    case ObjectClass::Deformer:          return sfbxS_Deformer;
    case ObjectClass::Pose:              return sfbxS_Pose;
    case ObjectClass::Material:          return sfbxS_Material;
    case ObjectClass::AnimationStack:    return sfbxS_AnimationStack;
    case ObjectClass::AnimationLayer:    return sfbxS_AnimationLayer;
    case ObjectClass::AnimationCurveNode:return sfbxS_AnimationCurveNode;
    case ObjectClass::AnimationCurve:    return sfbxS_AnimationCurve;
    default: return "";
    }
}


ObjectSubClass GetFbxObjectSubClass(const std::string& n)
{
    if (n.empty()) return ObjectSubClass::Unknown;
    else if (n == sfbxS_Null)       return ObjectSubClass::Null;
    else if (n == sfbxS_Light)      return ObjectSubClass::Light;
    else if (n == sfbxS_Camera)     return ObjectSubClass::Camera;
    else if (n == sfbxS_Mesh)       return ObjectSubClass::Mesh;
    else if (n == sfbxS_Shape)      return ObjectSubClass::Shape;
    else if (n == sfbxS_Root)       return ObjectSubClass::Root;
    else if (n == sfbxS_LimbNode)   return ObjectSubClass::LimbNode;
    else if (n == sfbxS_Skin)       return ObjectSubClass::Skin;
    else if (n == sfbxS_Cluster)    return ObjectSubClass::Cluster;
    else if (n == sfbxS_BindPose)   return ObjectSubClass::BindPose;
    else if (n == sfbxS_BlendShape) return ObjectSubClass::BlendShape;
    else if (n == sfbxS_BlendShapeChannel) return ObjectSubClass::BlendShapeChannel;
    else {
        sfbxPrint("GetFbxObjectSubClass(): unknown subtype \"%s\"\n", n.c_str());
        return ObjectSubClass::Unknown;
    }
}

ObjectSubClass GetFbxObjectSubClass(Node* n)
{
    return GetFbxObjectSubClass(GetPropertyString(n, 2));
}

const char* GetFbxSubClassName(ObjectSubClass t)
{
    switch (t) {
    case ObjectSubClass::Null:       return sfbxS_Null;
    case ObjectSubClass::Light:      return sfbxS_Light;
    case ObjectSubClass::Camera:     return sfbxS_Camera;
    case ObjectSubClass::Mesh:       return sfbxS_Mesh;
    case ObjectSubClass::Shape:      return sfbxS_Shape;
    case ObjectSubClass::Root:       return sfbxS_Root;
    case ObjectSubClass::LimbNode:   return sfbxS_LimbNode;
    case ObjectSubClass::Skin:       return sfbxS_Skin;
    case ObjectSubClass::Cluster:    return sfbxS_Cluster;
    case ObjectSubClass::BindPose:   return sfbxS_BindPose;
    case ObjectSubClass::BlendShape: return sfbxS_BlendShape;
    case ObjectSubClass::BlendShapeChannel: return sfbxS_BlendShapeChannel;
    default: return "";
    }
}

std::string MakeObjectName(const std::string& name, const std::string& type)
{
    std::string ret = name;
    ret += (char)0;
    ret += (char)1;
    ret += type;
    return ret;
}


Object::Object()
{
    m_id = (int64)this;
}

Object::~Object()
{
}

ObjectClass Object::getClass() const { return ObjectClass::Unknown; }
ObjectSubClass Object::getSubClass() const { return ObjectSubClass::Unknown; }

std::string Object::getObjectName() const
{
    return MakeObjectName(m_name, GetFbxClassName(getClass()));
}

void Object::setNode(Node* n)
{
    m_node = n;
    if (n) {
        // do these in constructObject() is too late because of referencing other objects...
        m_id = GetPropertyValue<int64>(n, 0);
        m_name = GetPropertyString(n, 1);
    }
}

void Object::constructObject()
{
}

void Object::constructNodes()
{
    auto objects = m_document->findNode(sfbxS_Objects);
    m_node = objects->createChild(GetFbxClassName(getClass()));
    m_node->addProperties(m_id, getObjectName(), GetFbxSubClassName(getSubClass()));

    auto parents = getParents();
    if (parents.empty()) {
        // this cannot be omitted
        addLinkOO(0);
    }
    else {
        for (auto parent : parents)
            addLinkOO(parent->getID());
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
        if (getClass() != ObjectClass::Model && v->getClass() == ObjectClass::Model)
            setName(v->getName());
    }
}

void Object::addLinkOO(int64 id)
{
    if (auto connections = m_document->findNode(sfbxS_Connections)) {
        connections->createChild(sfbxS_C, sfbxS_OO, getID(), id);
    }
}

int64 Object::getID() const { return m_id; }
const std::string& Object::getName() const { return m_name; }
Node* Object::getNode() const { return m_node; }

span<Object*> Object::getParents() const  { return make_span(m_parents); }
span<Object*> Object::getChildren() const { return make_span(m_children); }
Object* Object::getParent(size_t i) const { return i < m_parents.size() ? m_parents[i] : nullptr; }
Object* Object::getChild(size_t i) const  { return i < m_children.size() ? m_children[i] : nullptr; }

void Object::setID(int64 id) { m_id = id; }
void Object::setName(const std::string& v) { m_name = v; }



ObjectClass NodeAttribute::getClass() const
{
    return ObjectClass::NodeAttribute;
}

ObjectSubClass NullAttribute::getSubClass() const { return ObjectSubClass::Null; }

void NullAttribute::constructNodes()
{
    super::constructNodes();
    getNode()->createChild(sfbxS_TypeFlags, sfbxS_Null);
}

ObjectSubClass RootAttribute::getSubClass() const { return ObjectSubClass::Root; }

void RootAttribute::constructNodes()
{
    super::constructNodes();
    getNode()->createChild(sfbxS_TypeFlags, sfbxS_Null, sfbxS_Skeleton, sfbxS_Root);
}

ObjectSubClass LimbNodeAttribute::getSubClass() const { return ObjectSubClass::LimbNode; }

void LimbNodeAttribute::constructNodes()
{
    super::constructNodes();
    getNode()->createChild(sfbxS_TypeFlags, sfbxS_Skeleton);
}

ObjectSubClass LightAttribute::getSubClass() const { return ObjectSubClass::Light; }

void LightAttribute::constructNodes()
{
    super::constructNodes();
    // todo
}

ObjectSubClass CameraAttribute::getSubClass() const { return ObjectSubClass::Camera; }

void CameraAttribute::constructNodes()
{
    super::constructNodes();
    // todo
}



ObjectClass Model::getClass() const { return ObjectClass::Model; }

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
    n->createChild(sfbxS_Version, sfbxI_ModelVersion);

    auto properties = n->createChild(sfbxS_Properties70);

    // attribute
    properties->createChild(sfbxS_P, "DefaultAttributeIndex", "int", "Integer", "", 0);

    // position
    if (m_position != float3::zero()) {
        properties->createChild(sfbxS_P,
            sfbxS_LclTranslation, sfbxS_LclTranslation, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_position));
    }

    // rotation active
    if (m_pre_rotation != float3::zero() || m_post_rotation != float3::zero() || m_rotation != float3::zero()) {
        properties->createChild(sfbxS_P,
            sfbxS_RotationActive, sfbxS_bool, sfbxS_Empty, sfbxS_Empty, (int32)1);

        // rotation order
        if (m_rotation_order != RotationOrder::XYZ) {
            properties->createChild(sfbxS_P,
                sfbxS_RotationOrder, sfbxS_RotationOrder, sfbxS_Empty, sfbxS_A, (int32)m_rotation_order);
        }

        // pre-rotation
        if (m_pre_rotation != float3::zero()) {
            properties->createChild(sfbxS_P,
                sfbxS_PreRotation, sfbxS_Vector3D, sfbxS_Vector, sfbxS_Empty, sfbxVector3d(m_pre_rotation));
        }

        // post-rotation
        if (m_post_rotation != float3::zero()) {
            properties->createChild(sfbxS_P,
                sfbxS_PostRotation, sfbxS_Vector3D, sfbxS_Vector, sfbxS_Empty, sfbxVector3d(m_post_rotation));
        }

        // rotation
        if (m_rotation != float3::zero()) {
            properties->createChild(sfbxS_P,
                sfbxS_LclRotation, sfbxS_LclRotation, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_rotation));
        }
    }

    // scale
    if (m_scale!= float3::one()) {
        properties->createChild(sfbxS_P,
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
}

Model* Model::getParentModel() const { return m_parent_model; }

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
    if (m_post_rotation != float3::zero())
        ret *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_post_rotation * DegToRad)));
    if (m_rotation != float3::zero())
        ret *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_rotation * DegToRad)));
    if (m_pre_rotation != float3::zero())
        ret *= transpose(to_mat4x4(rotate_euler(m_rotation_order, m_pre_rotation * DegToRad)));

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

ObjectSubClass Null::getSubClass() const { return ObjectSubClass::Null; }


ObjectSubClass Root::getSubClass() const { return ObjectSubClass::Root; }

void Root::constructNodes()
{
    if (!m_attr)
        m_attr = createChild<RootAttribute>();
    addLinkOO(0);
    super::constructNodes();
}

void Root::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<RootAttribute>(v))
        m_attr = attr;
}

ObjectSubClass LimbNode::getSubClass() const { return ObjectSubClass::LimbNode; }

void LimbNode::constructNodes()
{
    if (!m_attr)
        m_attr = createChild<LimbNodeAttribute>();
    super::constructNodes();
}


void LimbNode::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<LimbNodeAttribute>(v))
        m_attr = attr;
}

ObjectSubClass Mesh::getSubClass() const { return ObjectSubClass::Mesh; }

void Mesh::constructNodes()
{
    super::constructNodes();
}

void Mesh::addChild(Object* v)
{
    super::addChild(v);
    if (auto geom = as<GeomMesh>(v))
        m_geom = geom;
    else if (auto material = as<Material>(v))
        m_materials.push_back(material);
}

GeomMesh* Mesh::getGeometry()
{
    if (!m_geom) {
        // m_geom will be set in addChild()
        createChild<GeomMesh>(getName());
    }
    return m_geom;
}
span<Material*> Mesh::getMaterials() const
{
    return make_span(m_materials);
}



ObjectSubClass Light::getSubClass() const { return ObjectSubClass::Light; }

void Light::constructObject()
{
    super::constructObject();
    auto n = getNode();
    // todo
}

void Light::constructNodes()
{
    if (!m_attr)
        m_attr = createChild<LightAttribute>();
    super::constructNodes();

    // todo
}

void Light::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<LightAttribute>(v))
        m_attr = attr;
}


ObjectSubClass Camera::getSubClass() const { return ObjectSubClass::Camera; }

void Camera::constructObject()
{
    super::constructObject();
    auto n = getNode();
    // todo
}

void Camera::constructNodes()
{
    if (!m_attr)
        m_attr = createChild<CameraAttribute>();
    super::constructNodes();
    // todo
}

void Camera::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<CameraAttribute>(v))
        m_attr = attr;
}



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


ObjectSubClass Shape::getSubClass() const { return ObjectSubClass::Shape; }

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
    n->createChild(sfbxS_Vertices, MakeAdaptor<double3>(m_delta_points));
    n->createChild(sfbxS_Indexes, m_indices);
    n->createChild(sfbxS_Normals, MakeAdaptor<double3>(m_delta_normals));
}

GeomMesh* Shape::getBaseMesh()
{
    for (auto* p = getParent(); p; p = p->getParent()) {
        if (auto mesh = as<GeomMesh>(p))
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



ObjectClass Deformer::getClass() const
{
    return ObjectClass::Deformer;
}

std::string SubDeformer::getObjectName() const
{
    return MakeObjectName(getName(), sfbxS_SubDeformer);
}


ObjectSubClass Skin::getSubClass() const { return ObjectSubClass::Skin; }

void Skin::constructObject()
{
    super::constructObject();
}

void Skin::constructNodes()
{
    super::constructNodes();

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_SkinVersion);
    n->createChild(sfbxS_Link_DeformAcuracy, (float64)50.0);

}

void Skin::addParent(Object* v)
{
    super::addParent(v);
    if (auto mesh = as<GeomMesh>(v))
        m_mesh = mesh;
}

void Skin::addChild(Object* v)
{
    super::addChild(v);
    if (auto cluster = as<Cluster>(v))
        m_clusters.push_back(cluster);
}

GeomMesh* Skin::getMesh() const { return m_mesh; }
span<Cluster*> Skin::getClusters() const { return make_span(m_clusters); }

JointWeights Skin::getJointWeightsVariable()
{
    JointWeights ret;
    auto mesh = as<GeomMesh>(getParent());
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


ObjectSubClass Cluster::getSubClass() const { return ObjectSubClass::Cluster; }

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

    auto n = getNode();
    n->createChild(sfbxS_Version, sfbxI_ClusterVersion);
    n->createChild(sfbxS_Mode, sfbxS_Total1);
    n->createChild(sfbxS_UserData, "", "");
    if (!m_indices.empty())
        n->createChild(sfbxS_Indexes, m_indices);
    if (!m_weights.empty())
        n->createChild(sfbxS_Weights, MakeAdaptor<float64>(m_weights));
    if (m_transform != float4x4::identity())
        n->createChild(sfbxS_Transform, (double4x4)m_transform);
    if (m_transform_link != float4x4::identity())
        n->createChild(sfbxS_TransformLink, (double4x4)m_transform_link);
}

void Cluster::addChild(Object* v)
{
    super::addChild(v);
    if (auto model = as<Model>(v))
        setName(v->getName());
}

span<int> Cluster::getIndices() const { return make_span(m_indices); }
span<float> Cluster::getWeights() const { return make_span(m_weights); }
float4x4 Cluster::getTransform() const { return m_transform; }
float4x4 Cluster::getTransformLink() const { return m_transform_link; }

void Cluster::setIndices(span<int> v) { m_indices = v; }
void Cluster::setWeights(span<float> v) { m_weights = v; }
void Cluster::setTransform(float4x4 v) { m_transform = v; }
void Cluster::setTransformLink(float4x4 v) { m_transform_link = v; }


ObjectSubClass BlendShape::getSubClass() const { return ObjectSubClass::BlendShape; }

void BlendShape::constructObject()
{
    super::constructObject();
}

void BlendShape::constructNodes()
{
    super::constructNodes();
}


ObjectSubClass BlendShapeChannel::getSubClass() const { return ObjectSubClass::BlendShapeChannel; }

void BlendShapeChannel::constructObject()
{
    super::constructObject();
}

void BlendShapeChannel::constructNodes()
{
    super::constructNodes();
}



ObjectClass Pose::getClass() const { return ObjectClass::Pose; }

ObjectSubClass BindPose::getSubClass() const { return ObjectSubClass::BindPose; }

void BindPose::constructObject()
{
    super::constructObject();

    for (auto n : getNode()->getChildren()) {
        if (n->getName() == sfbxS_PoseNode) {
            auto nid = GetChildPropertyValue<int64>(n, sfbxS_Node);
            auto mat = GetChildPropertyValue<double4x4>(n, sfbxS_Matrix);
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
    super::constructNodes();

    auto n = getNode();
    n->createChild(sfbxS_Type, sfbxS_BindPose);
    n->createChild(sfbxS_Version, sfbxI_BindPoseVersion);
    n->createChild(sfbxS_NbPoseNodes, (int32)m_pose_data.size());
    for (auto& d : m_pose_data) {
        auto pn = n->createChild(sfbxS_PoseNode);
        pn->createChild(sfbxS_Node, (int64)d.object);
        pn->createChild(sfbxS_Matrix, (double4x4)d.matrix);
    }
}

span<BindPose::PoseData> BindPose::getPoseData() const { return make_span(m_pose_data); }
void BindPose::addPoseData(Model* joint, float4x4 bind_matrix) { m_pose_data.push_back({ joint, bind_matrix }); }




ObjectClass Material::getClass() const { return ObjectClass::Material; }

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



ObjectClass AnimationStack::getClass() const { return ObjectClass::AnimationStack; }


ObjectClass AnimationLayer::getClass() const { return ObjectClass::AnimationLayer; }

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


ObjectClass AnimationCurveNode::getClass() const { return ObjectClass::AnimationCurveNode; }

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
    else if (m_curves.size() != 1) {
        sfbxPrint("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
        return;
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
    else if (m_curves.size() != 3) {
        sfbxPrint("afbx::AnimationCurveNode::addValue() curve count mismatch\n");
        return;
    }
    m_curves[0]->addValue(time, value.x);
    m_curves[1]->addValue(time, value.y);
    m_curves[2]->addValue(time, value.z);
}

ObjectClass AnimationCurve::getClass() const { return ObjectClass::AnimationCurve; }

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
