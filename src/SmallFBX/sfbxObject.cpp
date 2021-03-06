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

std::string MakeNodeName(const std::string& name, const std::string& type)
{
    std::string ret = name;
    ret += (char)0x00;
    ret += (char)0x01;
    ret += type;
    return ret;
}

bool IsNodeName(const std::string& name)
{
    size_t n = name.size();
    if (n > 2) {
        for (size_t i = 0; i < n - 1; ++i)
            if (name[i] == 0x00 && name[i + 1] == 0x01)
                return true;
    }
    return false;
}

bool SplitNodeName(span<char> node_name, span<char>& name, span<char>& classname)
{
    const char* str = node_name.data();
    size_t n = node_name.size();
    if (n > 2) {
        for (size_t i = 0; i < n - 1; ++i) {
            if (str[i] == 0x00 && str[i + 1] == 0x01) {
                name = make_span(str, i);
                i += 2;
                classname = make_span(str + i, n - i);
                return true;
            }
        }
    }
    name = make_span(node_name);
    return false;
}



Object::Object()
{
    m_id = (int64)this;
}

Object::~Object() {}
ObjectClass Object::getClass() const { return ObjectClass::Unknown; }
ObjectSubClass Object::getSubClass() const { return ObjectSubClass::Unknown; }
const char* Object::getClassName() const { return GetFbxClassName(getClass()); }

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
    if (m_id == 0)
        return;

    auto objects = m_document->findNode(sfbxS_Objects);
    m_node = objects->createChild(GetFbxClassName(getClass()));
    m_node->addProperties(m_id, getName(), GetFbxSubClassName(getSubClass()));

    for (auto parent : getParents())
        addLinkOO(parent->getID());
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
    if (v)
        m_parents.push_back(v);
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
void Object::setName(const std::string& v) { m_name = IsNodeName(v) ? v : MakeNodeName(v, getClassName()); }



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

} // namespace sfbx
