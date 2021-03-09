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

    int64 getID() const;
    string_view getName() const; // in object name format (e.g. "hoge\x00\x01Mesh")
    string_view getDisplayName() const; // return display part (e.g. "hoge" if object name is "hoge\x00\x01Mesh")
    Node* getNode() const;

    span<Object*> getParents() const;
    span<Object*> getChildren() const;
    Object* getParent(size_t i = 0) const;
    Object* getChild(size_t i = 0) const;

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
    void constructObject() override;
    void constructNodes() override;
    void addParent(Object* v) override;
    void propagateDirty();
    void updateMatrices() const;

    Model* m_parent_model{};
    std::vector<Model*> m_child_models;

    bool m_visibility = true;
    RotationOrder m_rotation_order = RotationOrder::XYZ;
    float3 m_position{};
    float3 m_pre_rotation{};
    float3 m_rotation{};
    float3 m_post_rotation{};
    float3 m_scale = float3::one();

    mutable bool m_matrix_dirty = true;
    mutable float4x4 m_matrix_local = float4x4::identity();
    mutable float4x4 m_matrix_global = float4x4::identity();
};


class Null : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(Object* v) override;

protected:
    void constructNodes() override;

    NullAttribute* m_attr{};
};

class Root : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(Object* v) override;

protected:
    void constructNodes() override;

    RootAttribute* m_attr{};
};


class LimbNode : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(Object* v) override;

protected:
    void constructNodes() override;

    LimbNodeAttribute* m_attr{};
};


class Mesh : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(Object* v) override;

    GeomMesh* getGeometry();
    span<Material*> getMaterials() const;

protected:
    void constructObject() override;

    GeomMesh* m_geom{};
    std::vector<Material*> m_materials;
};


class Light : public Model
{
using super = Model;
public:
    ObjectSubClass getSubClass() const override;
    void addChild(Object* v) override;

protected:
    void constructObject() override;
    void constructNodes() override;

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

template<class T>
inline constexpr bool is_deformer = std::is_base_of_v<Deformer, T>;

class Geometry : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(Object* v) override;

    bool hasDeformer() const;
    span<Deformer*> getDeformers() const;

    // T: Skin, BlendShape
    template<class T, sfbxRestrict(is_deformer<T>)>
    T* createDeformer();

protected:
    std::vector<Deformer*> m_deformers;
};


template<class T>
struct LayerElement
{
    std::string name;
    RawVector<int> indices; // can be empty. in that case, size of data must equal with vertex count or index count.
    RawVector<T> data;
    RawVector<T> data_deformed;
};
using LayerElementF2 = LayerElement<float2>;
using LayerElementF3 = LayerElement<float3>;
using LayerElementF4 = LayerElement<float4>;

class GeomMesh : public Geometry
{
using super = Geometry;
public:
    ObjectSubClass getSubClass() const override;

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

    span<float3> getPointsDeformed();
    span<float3> getNormalsDeformed(size_t layer_index = 0);

protected:
    void constructObject() override;
    void constructNodes() override;

    RawVector<int> m_counts;
    RawVector<int> m_indices;
    RawVector<float3> m_points;
    RawVector<float3> m_points_deformed;
    std::vector<LayerElementF3> m_normal_layers;
    std::vector<LayerElementF2> m_uv_layers;
    std::vector<LayerElementF4> m_color_layers;
};

class Shape : public Geometry
{
using super = Geometry;
public:
    ObjectSubClass getSubClass() const override;

    span<int> getIndices() const;
    span<float3> getDeltaPoints() const;
    span<float3> getDeltaNormals() const;

    void setIndices(span<int> v);
    void setDeltaPoints(span<float3> v);
    void setDeltaNormals(span<float3> v);

public:
    void constructObject() override;
    void constructNodes() override;

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

    GeomMesh* getBaseMesh() const;

    // apply deform to dst. size of dst must be equal with base mesh.
    virtual void deformPoints(span<float3> dst) const;
    virtual void deformNormals(span<float3> dst) const;
};

class SubDeformer : public Object
{
using super = Object;
public:
protected:
    ObjectClass getClass() const override;
    string_view getClassName() const override;
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
    void addChild(Object* v) override;

    GeomMesh* getMesh() const;
    span<Cluster*> getClusters() const;
    const JointWeights& getJointWeights() const;
    JointWeights createFixedJointWeights(int joints_per_vertex) const;
    const JointMatrices& getJointMatrices() const;

    // joint should be Null, Root or LimbNode
    Cluster* createCluster(Model* joint);

    // apply deform to dst. size of dst must be equal with base mesh.
    void deformPoints(span<float3> dst) const override;
    void deformNormals(span<float3> dst) const override;

protected:
    void constructObject() override;
    void constructNodes() override;
    void addParent(Object* v) override;

    GeomMesh* m_mesh{};
    std::vector<Cluster*> m_clusters;
    mutable JointWeights m_weights;
    mutable JointMatrices m_joint_matrices;
};


class Cluster : public SubDeformer
{
using super = SubDeformer;
public:
    ObjectSubClass getSubClass() const override;

    span<int> getIndices() const;
    span<float> getWeights() const;
    float4x4 getTransform() const;
    float4x4 getTransformLink() const;

    void setIndices(span<int> v);
    void setWeights(span<float> v);
    void setBindMatrix(float4x4 v); // v: global matrix of the joint (not inverted)

protected:
    void constructObject() override;
    void constructNodes() override;

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
    void addChild(Object* v) override;

    span<BlendShapeChannel*> getChannels() const;
    BlendShapeChannel* createChannel(string_view name);
    BlendShapeChannel* createChannel(Shape* shape);

    void deformPoints(span<float3> dst) const override;
    void deformNormals(span<float3> dst) const override;

protected:
    void constructObject() override;
    void constructNodes() override;

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

    float getWeight() const;
    span<ShapeData> getShapeData() const;
    // weight: 0.0f - 100.0f
    void addShape(Shape* shape, float weight = 100.0f);

    void setWeight(float v);
    void deformPoints(span<float3> dst) const;
    void deformNormals(span<float3> dst) const;

protected:
    void constructObject() override;
    void constructNodes() override;

    std::vector<ShapeData> m_shape_data;
    float m_weight = 0.0f;
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

    span<PoseData> getPoseData() const;
    void addPoseData(Model* joint, float4x4 bind_matrix);

protected:
    void constructObject() override;
    void constructNodes() override;

    std::vector<PoseData> m_pose_data;
};


// material

class Material : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;

protected:
    void constructObject() override;
    void constructNodes() override;
};


// animation-related classes
// (AnimationStack, AnimationLayer, AnimationCurveNode, AnimationCurve)

enum class AnimationKind
{
    Unknown,
    Position,       // float3
    Rotation,       // float3
    Scale,          // float3
    DeformWeight,   // float
    FocalLength,    // float
    filmboxTypeID,  // int16
    lockInfluenceWeights, // int32
};

class AnimationStack : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(Object* v) override;

    float getLocalStart() const;
    float getLocalStop() const;
    float getReferenceStart() const;
    float getReferenceStop() const;
    span<AnimationLayer*> getAnimationLayers() const;

    AnimationLayer* createLayer(string_view name = {});

    void applyAnimation(float time);

    bool remap(Document* doc);

protected:
    string_view getClassName() const override;
    void constructObject() override;
    void constructNodes() override;

    float m_local_start{};
    float m_local_stop{};
    float m_reference_start{};
    float m_reference_stop{};
    std::vector<AnimationLayer*> m_anim_layers;
};

class AnimationLayer : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(Object* v) override;

    span<AnimationCurveNode*> getAnimationCurveNodes() const;

    AnimationCurveNode* createCurveNode(AnimationKind kind, Object* target);

    void applyAnimation(float time);

    bool remap(Document* doc);

protected:
    string_view getClassName() const override;
    void constructObject() override;
    void constructNodes() override;

    std::vector<AnimationCurveNode*> m_anim_nodes;
};

class AnimationCurveNode : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;
    void addChild(Object* v) override;

    AnimationKind getAnimationKind() const;
    Object* getAnimationTarget() const;
    span<AnimationCurve*> getAnimationCurves() const;
    float getStartTime() const;
    float getStopTime() const;

    // evaluate curve(s)
    float evaluate(float time) const;
    float3 evaluate3(float time) const;

    // apply evaluated value to target
    void applyAnimation(float time) const;

    void initialize(AnimationKind kind, Object* target);
    void addValue(float time, float value);
    void addValue(float time, float3 value);

    bool remap(Document* doc);

protected:
    string_view getClassName() const override;
    void constructObject() override;
    void constructNodes() override;
    void constructLinks() override;

    AnimationKind m_kind = AnimationKind::Unknown;
    std::vector<AnimationCurve*> m_curves;
};

class AnimationCurve : public Object
{
using super = Object;
public:
    ObjectClass getClass() const override;

    span<float> getTimes() const;
    span<float> getValues() const;
    float getStartTime() const;
    float getStopTime() const;
    float evaluate(float time) const;

    void setTimes(span<float> v);
    void setValues(span<float> v);
    void addValue(float time, float value);

protected:
    string_view getClassName() const override;
    void constructObject() override;
    void constructNodes() override;
    void constructLinks() override;

    float m_default{};
    RawVector<float> m_times;
    RawVector<float> m_values;
};

} // sfbx
