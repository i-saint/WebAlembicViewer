#pragma once
#include "sfbxNode.h"

namespace sfbx {

enum class ObjectType
{
    Unknown,
    Attribute,
    Model,
    Geometry,
    Deformer,
    Pose,
    Material,
};

enum class ObjectSubType
{
    Unknown,
    Mesh,
    Shape,
    Root,
    LimbNode,
    Skin,
    Cluster,
    BlendShape,
    BlendShapeChannel,
};

ObjectType     GetFbxObjectType(const std::string& n);
const char*   GetFbxObjectName(ObjectType t);
ObjectSubType GetFbxObjectSubType(const std::string& n);
const char*   GetFbxObjectSubName(ObjectSubType t);


class Object
{
friend class Document;
public:
    virtual ~Object();
    virtual ObjectType getType() const;
    ObjectSubType getSubType() const;
    int64 getID() const;
    Node* getNode() const;
    Object* getParent() const;
    span<Object*> getChildren() const;

    void setSubType(ObjectSubType v);
    void setID(int64 v);
    void setNode(Node* v);
    void addChild(Object* v);

    virtual void readDataFronNode();
    virtual void createNode();

protected:
    Object();

    Document* m_document{};
    Node* m_node{};
    int64 m_id{};
    std::string m_name;
    ObjectSubType m_subtype{};

    Object* m_parent{};
    std::vector<Object*> m_children;
};


class Attribute : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;

    virtual void createNodes();

protected:
    Attribute();
};


class Model : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;

    void createNode() override;

protected:
    Model();
};


class Geometry : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;

    void readDataFronNode() override;
    void createNode() override;

    span<int> getCounts() const;
    span<int> getIndices() const;
    span<float3> getPoints() const;
    span<float3> getNormals() const;
    span<float2> getUV() const;

    void setCounts(span<int> v);
    void setIndices(span<int> v);
    void setPoints(span<float3> v);
    void setNormals(span<float3> v);
    void setUV(span<float2> v);

protected:
    Geometry();

    RawVector<int> m_counts;
    RawVector<int> m_indices;
    RawVector<float3> m_points;
    RawVector<float3> m_normals;
    RawVector<float2> m_uv;
};


class Deformer : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;

    void readDataFronNode() override;
    void createNode() override;

    span<int> getIndices() const;
    span<float> getWeights() const;
    const float4x4& getTransform() const;
    const float4x4& getTransformLink() const;

    void setIndices(span<int> v);
    void setWeights(span<float> v);
    void getTransform(const float4x4& v);
    void getTransformLink(const float4x4& v);

protected:
    Deformer();

    RawVector<int> m_indices;
    RawVector<float> m_weights;
    float4x4 m_transform = float4x4::identity();
    float4x4 m_transform_link = float4x4::identity();
};


class Pose : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;

    void createNode() override;


protected:
    Pose();
};


class Material : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;

    void createNode() override;

protected:
    Material();
};



template<class... T> inline ObjectPtr    MakeObject(T&&... v)    { return std::make_shared<Object>(std::forward<T>(v)...); }
template<class... T> inline AttributePtr MakeAttribute(T&&... v) { return std::make_shared<Attribute>(std::forward<T>(v)...); }
template<class... T> inline ModelPtr     MakeModel(T&&... v)     { return std::make_shared<Model>(std::forward<T>(v)...); }
template<class... T> inline GeometryPtr  MakeGeometry(T&&... v)  { return std::make_shared<Geometry>(std::forward<T>(v)...); }
template<class... T> inline DeformerPtr  MakeDeformer(T&&... v)  { return std::make_shared<Deformer>(std::forward<T>(v)...); }
template<class... T> inline PosePtr      MakePose(T&&... v)      { return std::make_shared<Pose>(std::forward<T>(v)...); }
template<class... T> inline MaterialPtr  MakeMaterial(T&&... v)  { return std::make_shared<Material>(std::forward<T>(v)...); }

} // sfbx