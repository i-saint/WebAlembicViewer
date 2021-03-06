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

ObjectClass     GetFbxObjectClass(const std::string& n);
ObjectClass     GetFbxObjectClass(Node* n);
const char*     GetFbxClassName(ObjectClass t);
ObjectSubClass  GetFbxObjectSubClass(const std::string& n);
ObjectSubClass  GetFbxObjectSubClass(Node* n);
const char*     GetFbxSubClassName(ObjectSubClass t);
std::string     MakeNodeName(const std::string& name, const std::string& type);
// true if name is in node name format (object name + \x00 \x01 + class name)
bool IsNodeName(const std::string& name);
// split node name into object name & class name (e.g. "hoge\x00\x01Mesh" -> "hoge" & "Mesh")
bool SplitNodeName(span<char> node_name, span<char>& name, span<char>& classname);


// base object class

class Object
{
friend class Document;
public:
    virtual ~Object();
    virtual ObjectClass getClass() const;
    virtual ObjectSubClass getSubClass() const;
    virtual void constructObject();
    virtual void constructNodes();

    template<class T> T* createChild(const std::string& name = "");
    virtual void addChild(Object* v);

    int64 getID() const;
    const std::string& getName() const; // in node name format (e.g. "hoge\x00\x01Mesh")
    Node* getNode() const;

    span<Object*> getParents() const;
    span<Object*> getChildren() const;
    Object* getParent(size_t i = 0) const;
    Object* getChild(size_t i = 0) const;

    virtual void setID(int64 v);
    virtual void setName(const std::string& v);
    virtual void setNode(Node* v);

protected:
    Object(const Object&) = delete;
    Object& operator=(const Object) = delete;
    Object();
    virtual const char* getClassName() const;
    virtual void addParent(Object* v);
    void addLinkOO(int64 id);

    Document* m_document{};
    Node* m_node{};
    int64 m_id{};
    std::string m_name;

    std::vector<Object*> m_parents;
    std::vector<Object*> m_children;
};


// NodeAttribute and its subclasses:
//  (NullAttribute, RootAttribute, LimbNodeAttribute, LightAttribute, CameraAttribute)

class NodeAttribute : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
};


class NullAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void constructNodes() override;
};


class RootAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void constructNodes() override;
};


class LimbNodeAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void constructNodes() override;
};


class LightAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void constructNodes() override;
};


class CameraAttribute : public NodeAttribute
{
using super = NodeAttribute;
public:
    ObjectSubClass getSubClass() const override;
    void constructNodes() override;
};



// Model and its subclasses:
//  (Null, Root, LimbNode, Light, Camera)

class Model : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void constructObject() override;
    void constructNodes() override;
    void addChild(Object* v) override;

    Model* getParentModel() const;

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
    bool m_visibility = true;
    RotationOrder m_rotation_order = RotationOrder::XYZ;
    float3 m_position{};
    float3 m_pre_rotation{};
    float3 m_rotation{};
    float3 m_post_rotation{};
    float3 m_scale = float3::one();
};


class Null : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void constructNodes() override;
    void addChild(Object* v) override;

protected:
    NullAttribute* m_attr{};
};


class Root : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void constructNodes() override;
    void addChild(Object* v) override;

protected:
    RootAttribute* m_attr{};
};


class LimbNode : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void constructNodes() override;
    void addChild(Object* v) override;

protected:
    LimbNodeAttribute* m_attr{};
};


class Mesh : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void constructNodes() override;
    void addChild(Object* v) override;

    GeomMesh* getGeometry();
    span<Material*> getMaterials() const;

protected:
    GeomMesh* m_geom{};
    std::vector<Material*> m_materials;
};


class Light : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void constructObject() override;
    void constructNodes() override;
    void addChild(Object* v) override;

protected:
    LightAttribute* m_attr{};
};


class Camera : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void constructObject() override;
    void constructNodes() override;
    void addChild(Object* v) override;

protected:
    CameraAttribute* m_attr{};
};



// Geometry and its subclasses:
//  (Mesh, Shape)

class Geometry : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
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

class GeomMesh : public Geometry
{
using super = Geometry;
public:
    ObjectSubClass getSubClass() const override;
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

    Skin* createSkin();
    BlendShape* createBlendshape();

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
using super = Geometry;
public:
    ObjectSubClass getSubClass() const override;
    void constructObject() override;
    void constructNodes() override;

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


// Deformer and its subclasses:
//  (Skin, Cluster, BlendShape, BlendShapeChannel)

class Deformer : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
};

class SubDeformer : public Deformer
{
using super = Deformer;
public:
protected:
    const char* getClassName() const override;
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
using super = Deformer;
public:
    ObjectSubClass getSubClass() const override;
    void constructObject() override;
    void constructNodes() override;
    void addChild(Object* v) override;

    GeomMesh* getMesh() const;
    span<Cluster*> getClusters() const;
    JointWeights getJointWeightsVariable();
    JointWeights getJointWeightsFixed(int joints_per_vertex);
    JointMatrices getJointMatrices();

    // joint should be Null, Root or LimbNode
    Cluster* createCluster(Model* joint);

protected:
    void addParent(Object* v) override;

    GeomMesh* m_mesh{};
    std::vector<Cluster*> m_clusters;
};


class Cluster : public SubDeformer
{
using super = SubDeformer;
public:
    ObjectSubClass getSubClass() const override;
    void constructObject() override;
    void constructNodes() override;
    void addChild(Object* v) override;

    span<int> getIndices() const;
    span<float> getWeights() const;
    float4x4 getTransform() const;
    float4x4 getTransformLink() const;

    void setIndices(span<int> v);
    void setWeights(span<float> v);
    void setBindMatrix(float4x4 v); // v: global matrix of the joint (not inverted)

protected:
    RawVector<int> m_indices;
    RawVector<float> m_weights;
    float4x4 m_transform = float4x4::identity();
    float4x4 m_transform_link = float4x4::identity();
};


class BlendShape : public Deformer
{
using super = Deformer;
public:
    ObjectSubClass getSubClass() const override;
    void constructObject() override;
    void constructNodes() override;
    void addChild(Object* v) override;

    span<BlendShapeChannel*> getChannels() const;
    BlendShapeChannel* createChannel(const char* name);

protected:
    std::vector<BlendShapeChannel*> m_channels;
};


class BlendShapeChannel : public SubDeformer
{
using super = SubDeformer;
public:
    struct ShapeData
    {
        Shape* shape;
        float weight;
    };

    ObjectSubClass getSubClass() const override;
    void constructObject() override;
    void constructNodes() override;

    span<ShapeData> getShapeData() const;

    // weight: 0.0f - 100.0f
    void addShape(Shape* shape, float weight = 100.0f);
    Shape* createShape(const char* name, float weight = 100.0f);

protected:
    std::vector<ShapeData> m_shape_data;
};



// Pose and its subclasses:
//  (BindPose only for now. probably RestPose will be added)

class Pose : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
};

class BindPose : public Pose
{
using super = Pose;
public:
    struct PoseData
    {
        Model* object;
        float4x4 matrix;
    };

    ObjectSubClass getSubClass() const override;
    void constructObject() override;
    void constructNodes() override;

    span<PoseData> getPoseData() const;
    void addPoseData(Model* joint, float4x4 bind_matrix);

protected:
    std::vector<PoseData> m_pose_data;
};


// material

class Material : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void constructObject() override;
    void constructNodes() override;
};


// animation-related classes
// (AnimationStack, AnimationLayer, AnimationCurveNode, AnimationCurve)

class AnimationStack : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
};

class AnimationLayer : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
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
using super = Object;
public:
    ObjectClass getClass() const override;
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
using super = Object;
public:
    ObjectClass getClass() const override;
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
