#pragma once
#include "sfbxNode.h"

namespace sfbx {

enum class ObjectClass : int
{
    Unknown,
    NodeAttribute,
    Model,
    Geometry,
    Deformer,
    Pose,
    Material,
    AnimationStack,
    AnimationLayer,
    AnimationCurveNode,
    AnimationCurve,
};

enum class ObjectSubClass : int
{
    Unknown,
    Null,
    Root,
    LimbNode,
    Light,
    Camera,
    Mesh,
    Shape,
    Skin,
    Cluster,
    BindPose,
    BlendShape,
    BlendShapeChannel,
};

ObjectClass     GetObjectClass(string_view n);
ObjectClass     GetObjectClass(Node* n);
const char*     GetObjectClassName(ObjectClass t);
ObjectSubClass  GetObjectSubClass(string_view n);
ObjectSubClass  GetObjectSubClass(Node* n);
const char*     GetObjectSubClassName(ObjectSubClass t);
std::string     MakeObjectName(string_view name, string_view type);
// true if name is in object name format (display name + \x00 \x01 + class name)
bool IsObjectName(string_view name);
// split node name into display name & class name (e.g. "hoge\x00\x01Mesh" -> "hoge" & "Mesh")
bool SplitObjectName(string_view node_name, string_view& display_name, string_view& class_name);


// base object class

class Object : public std::enable_shared_from_this<Object>
{
friend class Document;
public:
    virtual ~Object();
    virtual ObjectClass getClass() const;
    virtual ObjectSubClass getSubClass() const;

    template<class T> T* createChild(string_view name = {});
    virtual void addChild(Object* v);
    virtual void eraseChild(Object* v);

    int64 getID() const;
    string_view getName() const; // in object name format (e.g. "hoge\x00\x01Mesh")
    string_view getDisplayName() const; // return display part (e.g. "hoge" if object name is "hoge\x00\x01Mesh")
    Node* getNode() const;

    span<Object*> getParents() const;
    span<Object*> getChildren() const;
    Object* getParent(size_t i = 0) const;
    Object* getChild(size_t i = 0) const;
    Object* findChild(string_view name) const;

    virtual void setID(int64 v);
    virtual void setName(string_view v);
    virtual void setNode(Node* v);

protected:
    Object();
    Object(const Object&) = delete;
    Object& operator=(const Object) = delete;

    virtual void constructObject(); // import data from fbx nodes
    virtual void constructNodes(); // export data to fbx nodes
    virtual void constructLinks(); // export connections to fbx nodes
    virtual string_view getClassName() const;
    virtual void addParent(Object* v);
    virtual void eraseParent(Object* v);

    Document* m_document{};
    Node* m_node{};
    int64 m_id{};
    std::string m_name;

    std::vector<Object*> m_parents;
    std::vector<Object*> m_children;
};


} // sfbx
