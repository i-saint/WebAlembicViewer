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
    AnimationStack,
    AnimationLayer,
    AnimationCurveNode,
    AnimationCurve,
};

enum class ObjectSubType : int
{
    Unknown,
    Light,
    Camera,
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
    virtual void constructObject();
    virtual void constructNodes();

    ObjectSubType getSubType() const;
    int64 getID() const;
    const std::string& getName() const;
    Node* getNode() const;

    span<Object*> getParents() const;
    span<Object*> getChildren() const;
    Object* getParent(size_t i = 0) const;
    Object* getChild(size_t i = 0) const;

    void setSubType(ObjectSubType v);
    void setID(int64 v);
    void setName(const std::string& v);
    void setNode(Node* v);

    Object* createChild(ObjectType type);
    template<class T> T* createChild();
    void addChild(Object* v);

protected:
    Object(const Object&) = delete;
    Object& operator=(const Object) = delete;
    Object();
    void addParent(Object* v);

    Document* m_document{};
    Node* m_node{};
    int64 m_id{};
    std::string m_name;
    ObjectSubType m_subtype{};

    std::vector<Object*> m_parents;
    std::vector<Object*> m_children;
};


class Attribute : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
    void constructObject() override;
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
    void constructObject() override;
    void constructNodes() override;

    bool getVisibility() const;
    RotationOrder getRotationOrder() const;
    float3 getPosition() const;
    float3 getRotation() const;
    float3 getScale() const;
    float4x4 getLocalMatrix() const;
    float4x4 getGlobalMatrix() const;

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


template<class T>
struct LayerElement
{
    std::string name;
    RawVector<T> data;
    RawVector<int> indices; // can be empty. in that case, size of data must equal with vertex count or index count.
};
using LayerElementF2 = LayerElement<float2>;
using LayerElementF3 = LayerElement<float3>;
using LayerElementF4 = LayerElement<float4>;

struct JointWeight
{
    int index; // index of joint
    float weight;
};


// subtype: Mesh, Shape
// if Mesh, it is usual poly mesh data.
// if Shape, it is blend shape target. in this case, points & normals are delta (deference from original).
class Geometry : public Object
{
friend class Document;
using super = Object;
public:
    struct MeshData : noncopyable
    {
        RawVector<int> counts;
        RawVector<int> indices;
        RawVector<float3> points;
        std::vector<LayerElementF3> normal_layers;
        std::vector<LayerElementF2> uv_layers;
        std::vector<LayerElementF4> color_layers;
    };

    struct ShapeData : noncopyable
    {
        RawVector<int> indices;
        RawVector<float3> delta_points;
        RawVector<float3> delta_normals;
    };

    ObjectType getType() const override;
    void constructObject() override;
    void constructNodes() override;

    MeshData* getMeshData();
    ShapeData* getShapeData();

protected:
    Geometry();

    std::unique_ptr<MeshData> m_mesh_data;
    std::unique_ptr<ShapeData> m_shape_data;
};


// subtype: Skin, Cluster, BlendShape, BlendShapeChannel
class Deformer : public Object
{
friend class Document;
using super = Object;
public:
    struct BlendShapeData : noncopyable
    {
        std::vector<Deformer*> channels;
    };

    struct BlendShapeChannelData : noncopyable
    {
        std::vector<Geometry*> shapes;
    };

    struct SkinData : noncopyable
    {
        std::vector<Deformer*> clusters;
    };

    struct ClusterData : noncopyable
    {
        RawVector<int> indices;
        RawVector<float> weights;
        float4x4 transform = float4x4::identity();
        float4x4 transform_link = float4x4::identity();
    };

    struct JointWeights // copyable
    {
        int max_joints_per_vertex = 0;
        RawVector<int> counts; // per-vertex. counts of affected joints.
        RawVector<int> offsets; // per-vertex. offset to weights.
        RawVector<JointWeight> weights; // vertex * affected joints (total of counts). weights of affected joints.
    };

    struct JointMatrices
    {
        RawVector<float4x4> bindpose;
        RawVector<float4x4> transform;
    };

    ObjectType getType() const override;
    void constructObject() override;
    void constructNodes() override;

    BlendShapeData* getBlendShapeData();
    BlendShapeChannelData* getBlendShapeChannelData();
    SkinData* getSkinData();
    ClusterData* getClusterData();


    // for Skin subtype
    JointWeights skinGetJointWeightsVariable();
    JointWeights skinGetJointWeightsFixed(int joints_per_vertex);
    JointMatrices skinGetJointMatrices();

protected:
    Deformer();

    std::unique_ptr<BlendShapeData> m_blendshape_data;
    std::unique_ptr<BlendShapeChannelData> m_blendshape_channel_data;
    std::unique_ptr<SkinData> m_skin_data;
    std::unique_ptr<ClusterData> m_cluster_data;
};


class Pose : public Object
{
friend class Document;
using super = Object;
public:
    struct BindPoseData : noncopyable
    {
        struct JointData
        {
            Model* joint{};
            float4x4 matrix = float4x4::identity();
        };

        std::vector<JointData> joints;
    };

    ObjectType getType() const override;
    void constructObject() override;
    void constructNodes() override;

    BindPoseData* getBindPoseData();

protected:
    Pose();

    std::unique_ptr<BindPoseData> m_bindpose_data;
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



class AnimationStack : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
};

class AnimationLayer : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
};

class AnimationCurveNode : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
};

class AnimationCurve : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
};

} // sfbx
