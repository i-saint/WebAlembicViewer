#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxDocument.h"

namespace sfbx {

ObjectClass Model::getClass() const { return ObjectClass::Model; }

void Model::constructObject()
{
    super::constructObject();
    auto n = getNode();
    if (!n)
        return;

    EnumerateProperties(n, [this](Node* p) {
        auto get_int = [p]() -> int {
            if (GetPropertyCount(p) == 5)
                return GetPropertyValue<int32>(p, 4);
#ifdef sfbxEnableLegacyFormatSupport
            else if (GetPropertyCount(p) == 4) {
                return GetPropertyValue<int32>(p, 3);
            }
#endif
            return 0;
        };

        auto get_float3 = [p]() -> float3 {
            if (GetPropertyCount(p) == 7) {
                return float3{
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                    (float)GetPropertyValue<float64>(p, 6),
                };
            }
#ifdef sfbxEnableLegacyFormatSupport
            else if (GetPropertyCount(p) == 6) {
                return float3{
                    (float)GetPropertyValue<float64>(p, 3),
                    (float)GetPropertyValue<float64>(p, 4),
                    (float)GetPropertyValue<float64>(p, 5),
                };
            }
#endif
            return {};
        };

        auto pname = GetPropertyString(p);
        if (pname == sfbxS_Visibility) {
            m_visibility = GetPropertyValue<bool>(p, 4);
        }
        else if (pname == sfbxS_LclTranslation)
            m_position = get_float3();
        else if (pname == sfbxS_RotationOrder)
            m_rotation_order = (RotationOrder)get_int();
        else if (pname == sfbxS_PreRotation)
            m_pre_rotation = get_float3();
        else if (pname == sfbxS_PostRotation)
            m_post_rotation = get_float3();
        else if (pname == sfbxS_LclRotation)
            m_rotation = get_float3();
        else if (pname == sfbxS_LclScale)
            m_scale = get_float3();
        });
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
    if (m_position != float3::zero())
        properties->createChild(sfbxS_P,
            sfbxS_LclTranslation, sfbxS_LclTranslation, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_position));

    // rotation
    if (m_pre_rotation != float3::zero() || m_post_rotation != float3::zero() || m_rotation != float3::zero()) {
        // rotation active
        properties->createChild(sfbxS_P,
            sfbxS_RotationActive, sfbxS_bool, sfbxS_Empty, sfbxS_Empty, (int32)1);
        // rotation order
        if (m_rotation_order != RotationOrder::XYZ)
            properties->createChild(sfbxS_P,
                sfbxS_RotationOrder, sfbxS_RotationOrder, sfbxS_Empty, sfbxS_A, (int32)m_rotation_order);
        // pre-rotation
        if (m_pre_rotation != float3::zero())
            properties->createChild(sfbxS_P,
                sfbxS_PreRotation, sfbxS_Vector3D, sfbxS_Vector, sfbxS_Empty, sfbxVector3d(m_pre_rotation));
        // post-rotation
        if (m_post_rotation != float3::zero())
            properties->createChild(sfbxS_P,
                sfbxS_PostRotation, sfbxS_Vector3D, sfbxS_Vector, sfbxS_Empty, sfbxVector3d(m_post_rotation));
        // rotation
        if (m_rotation != float3::zero())
            properties->createChild(sfbxS_P,
                sfbxS_LclRotation, sfbxS_LclRotation, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_rotation));
    }

    // scale
    if (m_scale!= float3::one())
        properties->createChild(sfbxS_P,
            sfbxS_LclScale, sfbxS_LclScale, sfbxS_Empty, sfbxS_A, sfbxVector3d(m_scale));
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

void Null::constructNodes()
{
    if (!m_attr)
        m_attr = createChild<NullAttribute>();
    super::constructNodes();
}

void Null::addChild(Object* v)
{
    super::addChild(v);
    if (auto attr = as<NullAttribute>(v))
        m_attr = attr;
}


ObjectSubClass Root::getSubClass() const { return ObjectSubClass::Root; }

void Root::constructNodes()
{
    if (!m_attr)
        m_attr = createChild<RootAttribute>();
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

void Mesh::constructObject()
{
    super::constructObject();

#ifdef sfbxEnableLegacyFormatSupport
    // in old fbx, Model::Mesh has geometry data
    auto n = getNode();
    if (n->findChild(sfbxS_Vertices)) {
        auto geom = getGeometry();
        geom->setNode(n);
        geom->constructObject();
    }
#endif
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
    if (!m_geom)
        m_geom = createChild<GeomMesh>(getName());
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

} // namespace sfbx
