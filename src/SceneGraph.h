#pragma once
#include "WebAlembicViewer.h"

using namespace Alembic;

namespace wabc {

using sfbx::make_span;
using sfbx::RawVector;

class Camera : public ICamera
{
public:
    const std::string& getPath() const override { return m_path; }
    float3 getPosition() const override { return m_position; }
    float3 getDirection() const override { return m_direction; }
    float3 getUp() const override { return m_up; }
    float getFocalLength() const override { return m_focal_length; }
    float2 getAperture() const override { return m_aperture; }
    float2 getLensShift() const override { return m_lens_shift; }
    float getNearPlane() const override { return m_near; }
    float getFarPlane() const override { return m_far; }

public:
    std::string m_path;
    float3 m_position{};
    float3 m_direction{ 0.0f, 0.0f, 1.0f };
    float3 m_up{ 0.0f, 1.0f, 0.0f };
    float m_focal_length = 30.0f;
    float2 m_aperture{ 36.0f, 24.0f };
    float2 m_lens_shift{};
    float m_near = 0.01f;
    float m_far = 100.0f;
};
using CameraPtr = std::shared_ptr<Camera>;


class Skin : public ISkin
{
public:
    span<int> getJointCounts() const override { return make_span(m_counts); }
    span<JointWeight> getJointWeights() const override { return make_span(m_weights); }
    span<float4x4> getJointMatrices() const override { return make_span(m_matrices); }

    template<class Vec, class Mul>
    bool deformImpl(span<Vec> dst, span<Vec> src, const Mul& mul) const;

    bool deformPoints(span<float3> dst, span<float3> src) const override;
    bool deformNormals(span<float3> dst, span<float3> src) const override;

public:
    RawVector<int> m_counts;
    RawVector<JointWeight> m_weights;
    RawVector<float4x4> m_matrices;
};
using SkinPtr = std::shared_ptr<Skin>;


class BlendShape : public IBlendShape
{
public:
    span<int> getIndices() const override { return make_span(m_indices); }
    span<float3> getDeltaPoints() const override { return make_span(m_delta_points); }
    span<float3> getDeltaNormals() const override { return make_span(m_delta_normals); }

    bool deformPoints(span<float3> dst, span<float3> src, float w) const override;
    bool deformNormals(span<float3> dst, span<float3> src, float w) const override;

public:
    RawVector<int> m_indices;
    RawVector<float3> m_delta_points;
    RawVector<float3> m_delta_normals;
};
using BlendShapePtr = std::shared_ptr<BlendShape>;


class Mesh : public IMesh
{
public:
    Mesh();
    ~Mesh() override;
    span<float3> getPoints() const override { return make_span(m_points); }
    span<float3> getNormals() const override { return make_span(m_normals); }
    span<float3> getPointsEx() const override { return make_span(m_points_ex); }
    span<float3> getNormalsEx() const override { return make_span(m_normals_ex); }
    span<int> getCounts() const override { return make_span(m_counts); }
    span<int> getFaceIndices() const override { return make_span(m_face_indices); }
    span<int> getWireframeIndices() const override { return make_span(m_wireframe_indices); }

#ifdef wabcWithGL
    GLuint getPointsBuffer() const override { return m_buf_points; }
    GLuint getPointsExBuffer() const override { return m_buf_points_ex; }
    GLuint getNormalsExBuffer() const override { return m_buf_normals_ex; }
    GLuint getWireframeIndicesBuffer() const override { return m_buf_wireframe_indices; }
#endif

    void clear();
    void upload();

public:
    RawVector<float3> m_points;
    RawVector<float3> m_normals;
    RawVector<float3> m_points_ex;
    RawVector<float3> m_normals_ex;

    RawVector<int> m_counts;
    RawVector<int> m_face_indices;
    RawVector<int> m_wireframe_indices;

#ifdef wabcWithGL
    GLuint m_buf_points{};
    GLuint m_buf_points_ex{};
    GLuint m_buf_normals_ex{};
    GLuint m_buf_wireframe_indices{};
#endif
};
using MeshPtr = std::shared_ptr<Mesh>;


class Points : public IPoints
{
public:
    Points();
    ~Points() override;
    span<float3> getPoints() const override { return make_span(m_points); }
#ifdef wabcWithGL
    GLuint getPointBuffer() const override { return m_vb_points; }
#endif

    void clear();
    void upload();

public:
    RawVector<float3> m_points;
#ifdef wabcWithGL
    GLuint m_vb_points{};
#endif
};
using PointsPtr = std::shared_ptr<Points>;

} // namespace wabc
