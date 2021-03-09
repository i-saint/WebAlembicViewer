#pragma once
#include "sfbxObject.h"

namespace sfbx {

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
    void eraseChild(Object* v) override;

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
    void eraseParent(Object* v) override;
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


} // namespace sfbx
