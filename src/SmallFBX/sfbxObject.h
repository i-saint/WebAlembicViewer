#pragma once
#include "sfbxNode.h"

namespace sfbx {

enum class ObjectType : int
{
    Unknown,
    Attribute,
    Model,
    Geometry,
    Deformer,
    Pose,
    Material,
};

enum class ObjectSubType : int
{
    Unknown,
    Mesh,
    Shape,
    Root,
    LimbNode,
    Skin,
    Cluster,
    BindPose,
    BlendShape,
    BlendShapeChannel,
};

enum class RotationOrder : int
{
    XYZ,
    XZY,
    YZX,
    YXZ,
    ZXY,
    ZYX,
    SphericXYZ
};

ObjectType    GetFbxObjectType(const std::string& n);
const char*   GetFbxObjectName(ObjectType t);
ObjectSubType GetFbxObjectSubType(const std::string& n);
const char*   GetFbxObjectSubName(ObjectSubType t);


class Object
{
friend class Document;
public:
    virtual ~Object();
    virtual ObjectType getType() const;
    virtual void readDataFronNode();
    virtual void constructNodes();

    ObjectSubType getSubType() const;
    int64 getID() const;
    Node* getNode() const;
    Object* getParent() const;
    span<Object*> getChildren() const;

    void setSubType(ObjectSubType v);
    void setID(int64 v);
    void setNode(Node* v);
    void addChild(Object* v);

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
    void readDataFronNode() override;
    void constructNodes() override;

protected:
    Attribute();
};


class Model : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
    void readDataFronNode() override;
    void constructNodes() override;

    bool getVisibility() const;
    RotationOrder getRotationOrder() const;
    float3 getPosition() const;
    float3 getRotation() const;
    float3 getScale() const;

    void setVisibility(bool v);
    void setRotationOrder(RotationOrder v);
    void setPosition(float3 v);
    void setRotation(float3 v);
    void setScale(float3 v);

protected:
    Model();

    bool m_visibility = true;
    RotationOrder m_rotation_order = RotationOrder::XYZ;
    float3 m_position{};
    float3 m_rotation{};
    float3 m_scale{1.0f, 1.0f, 1.0f};
};


class Geometry : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
    void readDataFronNode() override;
    void constructNodes() override;

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
    void constructNodes() override;

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
    struct BindPose
    {
        Object* joint{};
        float4x4 matrix = float4x4::identity();
    };

    ObjectType getType() const override;
    void readDataFronNode() override;
    void constructNodes() override;

    span<BindPose> getBindPose() const;

protected:
    Pose();

    std::vector<BindPose> m_bindpose;
};


class Material : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;

    void constructNodes() override;

protected:
    Material();
};

} // sfbx
