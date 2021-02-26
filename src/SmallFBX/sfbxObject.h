#pragma once
#include "sfbxNode.h"

namespace sfbx {

enum class Objectype
{
    Unknown,
    Model,
    Geometry,
    Deformer,
    Pose,
    Material,
};

class Object
{
public:
    Object(NodePtr n = nullptr);
    virtual ~Object();

    virtual Objectype getType() const;
    uint64_t getID() const { return m_id; }

protected:
    NodePtr m_node;
    int64 m_id = 0;
    std::string m_name;
    std::string m_type;
};


class Model : public Object
{
using super = Object;
public:
    Model(NodePtr n = nullptr);
    Objectype getType() const override;

    void addAttribute(ObjectPtr v);

protected:
    std::vector<ObjectPtr> m_attributes;
};


class Geometry : public Object
{
using super = Object;
public:
    Geometry(NodePtr n = nullptr);
    Objectype getType() const override;

    span<int> getCounts() const;
    span<int> getIndices() const;
    span<float3> getPoints() const;
    span<float3> getNormals() const;
    span<float2> getUV() const;

protected:
    RawVector<int> m_counts;
    RawVector<int> m_indices;
    RawVector<float3> m_points;
    RawVector<float3> m_normals;
    RawVector<float2> m_uv;
};


class Deformer : public Object
{
using super = Object;
public:
    Deformer(NodePtr n = nullptr);
    Objectype getType() const override;

protected:
    RawVector<int> m_indices;
    RawVector<float> m_weights;
};


class Pose : public Object
{
using super = Object;
public:
    Pose(NodePtr n = nullptr);
    Objectype getType() const override;

protected:
};


class Material : public Object
{
using super = Object;
public:
    Material(NodePtr n = nullptr);
    Objectype getType() const override;

protected:
};



template<class... T> inline ObjectPtr   MakeObject(T&&... v)   { return std::make_shared<Object>(std::forward<T>(v)...); }
template<class... T> inline ModelPtr    MakeModel(T&&... v)    { return std::make_shared<Model>(std::forward<T>(v)...); }
template<class... T> inline GeometryPtr MakeGeometry(T&&... v) { return std::make_shared<Geometry>(std::forward<T>(v)...); }
template<class... T> inline DeformerPtr MakeDeformer(T&&... v) { return std::make_shared<Deformer>(std::forward<T>(v)...); }
template<class... T> inline PosePtr     MakePose(T&&... v)     { return std::make_shared<Pose>(std::forward<T>(v)...); }
template<class... T> inline MaterialPtr MakeMaterial(T&&... v) { return std::make_shared<Material>(std::forward<T>(v)...); }

} // sfbx
