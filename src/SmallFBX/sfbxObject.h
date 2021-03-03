#pragma once
#include "sfbxNode.h"

namespace sfbx {

enum class ObjectType : int
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

enum class ObjectSubType : int
{
    Unknown,
    Light,
    Camera,
    Root,
    LimbNode,
    Mesh,
    Shape,
    Skin,
    Cluster,
    BindPose,
    BlendShape,
    BlendShapeChannel,
};

ObjectType    GetFbxObjectType(const std::string& n);
ObjectType    GetFbxObjectType(Node* n);
const char*   GetFbxObjectName(ObjectType t);
ObjectSubType GetFbxObjectSubType(const std::string& n);
ObjectSubType GetFbxObjectSubType(Node* n);
const char*   GetFbxObjectSubName(ObjectSubType t);


class Object
{
friend class Document;
public:
    virtual ~Object();
    virtual ObjectType getType() const;
    virtual void constructObject();
    virtual void constructNodes();

    template<class T> T* createChild(const std::string& name = "");
    virtual void addChild(Object* v);

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

protected:
    Object(const Object&) = delete;
    Object& operator=(const Object) = delete;
    Object();
    virtual void addParent(Object* v);

    Document* m_document{};
    Node* m_node{};
    int64 m_id{};
    std::string m_name;
    ObjectSubType m_subtype{};

    std::vector<Object*> m_parents;
    std::vector<Object*> m_children;
};


class NodeAttribute : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
    void constructObject() override;
    void constructNodes() override;

protected:
};


class Model : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
    void constructObject() override;
    void constructNodes() override;
    void addChild(Object* v) override;

    Model* getParentModel() const;
    span<NodeAttribute*> getNodeAttributes() const;
    span<Material*> getMaterials() const;
    Camera* getCamera() const;
    Light* getLight() const;
    Mesh* getMesh() const;

    bool getVisibility() const;
    RotationOrder getRotationOrder() const;
    float3 getPosition() const;
    float3 getPreRotation() const;
    float3 getRotation() const;
    float3 getPostRotation() const;
    float3 getScale() const;
    float4x4 getLocalMatrix() const;
    float4x4 getGlobalMatrix() const;

    void setVisibility(bool v);
    void setRotationOrder(RotationOrder v);
    void setPosition(float3 v);
    void setPreRotation(float3 v);
    void setRotation(float3 v);
    void setPostRotation(float3 v);
    void setScale(float3 v);

protected:
    void addParent(Object* v) override;

    Model* m_parent_model{};
    std::vector<NodeAttribute*> m_node_attributes;
    std::vector<Material*> m_materials;
    Mesh* m_mesh{};

    bool m_visibility = true;
    RotationOrder m_rotation_order = RotationOrder::XYZ;
    float3 m_position{};
    float3 m_pre_rotation{};
    float3 m_rotation{};
    float3 m_post_rotation{};
    float3 m_scale{1.0f, 1.0f, 1.0f};
};

class Light : public Model
{
friend class Document;
using super = Model;
public:
    void constructObject() override;
    void constructNodes() override;

protected:
};

class Camera : public Model
{
friend class Document;
using super = Model;
public:
    void constructObject() override;
    void constructNodes() override;

protected:
};

class Root : public Model
{
friend class Document;
using super = Model;
public:
};

class LimbNode : public Model
{
friend class Document;
using super = Model;
public:
};


class Geometry : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
    void addChild(Object* v) override;

    span<Deformer*> getDeformers() const;

protected:
    std::vector<Deformer*> m_deformers;
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

class Mesh : public Geometry
{
friend class Document;
using super = Geometry;
public:
    void constructObject() override;
    void constructNodes() override;

    span<int> getCounts() const;
    span<int> getIndices() const;
    span<float3> getPoints() const;
    span<LayerElementF3> getNormalLayers() const;
    span<LayerElementF2> getUVLayers() const;
    span<LayerElementF4> getColorLayers() const;

    void setCounts(span<int> v);
    void setIndices(span<int> v);
    void setPoints(span<float3> v);
    void addNormalLayer(LayerElementF3&& v);
    void addUVLayer(LayerElementF2&& v);
    void addColorLayer(LayerElementF4&& v);

protected:
    RawVector<int> m_counts;
    RawVector<int> m_indices;
    RawVector<float3> m_points;
    std::vector<LayerElementF3> m_normal_layers;
    std::vector<LayerElementF2> m_uv_layers;
    std::vector<LayerElementF4> m_color_layers;
};

class Shape : public Geometry
{
friend class Document;
using super = Geometry;
public:
    void constructObject() override;
    void constructNodes() override;

    Mesh* getBaseMesh();
    span<int> getIndices() const;
    span<float3> getDeltaPoints() const;
    span<float3> getDeltaNormals() const;

    void setIndices(span<int> v);
    void setDeltaPoints(span<float3> v);
    void setDeltaNormals(span<float3> v);

public:
    RawVector<int> m_indices;
    RawVector<float3> m_delta_points;
    RawVector<float3> m_delta_normals;
};


// subtype: Skin, Cluster, BlendShape, BlendShapeChannel
class Deformer : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
};


struct JointWeight
{
    int index; // index of joint/cluster
    float weight;
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
    RawVector<float4x4> global_transform;
    RawVector<float4x4> joint_transform;
};

class Skin : public Deformer
{
friend class Document;
using super = Deformer;
public:
    void constructObject() override;
    void constructNodes() override;
    void addChild(Object* v) override;

    Mesh* getMesh() const;
    span<Cluster*> getClusters() const;
    JointWeights getJointWeightsVariable();
    JointWeights getJointWeightsFixed(int joints_per_vertex);
    JointMatrices getJointMatrices();

protected:
    void addParent(Object* v) override;

    Mesh* m_mesh{};
    std::vector<Cluster*> m_clusters;
};


class Cluster : public Deformer
{
friend class Document;
using super = Deformer;
public:
    void constructObject() override;
    void constructNodes() override;

    span<int> getIndices() const;
    span<float> getWeights() const;
    float4x4 getTransform() const;
    float4x4 getTransformLink() const;

    void setIndices(span<int> v);
    void setWeights(span<float> v);
    void setTransform(float4x4 v);
    void setTransformLink(float4x4 v);

protected:
    RawVector<int> m_indices;
    RawVector<float> m_weights;
    float4x4 m_transform = float4x4::identity();
    float4x4 m_transform_link = float4x4::identity();
};


class BlendShape : public Deformer
{
friend class Document;
using super = Deformer;
public:
    void constructObject() override;
    void constructNodes() override;

protected:
};


class BlendShapeChannel : public Deformer
{
friend class Document;
using super = Deformer;
public:
    void constructObject() override;
    void constructNodes() override;

protected:
    std::vector<Shape*> m_shapes;
};



class Pose : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
};

class BindPose : public Pose
{
friend class Document;
using super = Pose;
public:
    struct PoseData
    {
        Model* object{};
        float4x4 matrix = float4x4::identity();
    };

    void constructObject() override;
    void constructNodes() override;

    span<PoseData> getPoseData() const;
    void addPoseData(const PoseData& v);

protected:
    std::vector<PoseData> m_pose_data;
};


class Material : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
    void constructObject() override;
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
    void constructObject() override;
    void constructNodes() override;

    AnimationCurveNode* getPosition() const;
    AnimationCurveNode* getRotation() const;
    AnimationCurveNode* getScale() const;
    AnimationCurveNode* getFocalLength() const;

protected:
    AnimationCurveNode* m_position{};
    AnimationCurveNode* m_rotation{};
    AnimationCurveNode* m_scale{};
    AnimationCurveNode* m_focal_length{};
};

class AnimationCurveNode : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
    void constructObject() override;
    void constructNodes() override;
    void addChild(Object* v) override;

    float getStartTime() const;
    float getEndTime() const;
    float evaluate(float time) const;
    float3 evaluate3(float time) const;

    void addValue(float time, float value);
    void addValue(float time, float3 value);

protected:
    std::vector<AnimationCurve*> m_curves;
};

class AnimationCurve : public Object
{
friend class Document;
using super = Object;
public:
    ObjectType getType() const override;
    void constructObject() override;
    void constructNodes() override;

    span<float> getTimes() const;
    span<float> getValues() const;
    float getStartTime() const;
    float getEndTime() const;
    float evaluate(float time) const;

    void setTimes(span<float> v);
    void setValues(span<float> v);
    void addValue(float time, float value);

protected:
    float m_default{};
    RawVector<float> m_times;
    RawVector<float> m_values;
};

} // sfbx
