#pragma once
#include "VectorMath.h"

namespace wabc {

template<class T>
struct releaser { void operator()(T* v) { if (v) v->release(); } };


class IEntity
{
public:
    virtual ~IEntity() {};
};

class ICamera : public IEntity
{
public:
    virtual const std::string& getPath() const = 0;
    virtual float3 getPosition() const = 0;
    virtual float3 getDirection() const = 0;
    virtual float3 getUp() const = 0;
    virtual float getFocalLength() const = 0;
    virtual float2 getAperture() const = 0;
    virtual float2 getLensShift() const = 0;
    virtual float getNearPlane() const = 0;
    virtual float getFarPlane() const = 0;
};

class IMesh : public IEntity
{
public:
    virtual std::span<float3> getPoints() const = 0;
    virtual std::span<float3> getPointsEx() const = 0; // expanded (not indexed)
    virtual std::span<float3> getNormalsEx() const = 0; // expanded (not indexed)
    virtual std::span<int> getWireframeIndices() const = 0;
#ifdef wabcWithGL
    virtual GLuint getPointsBuffer() const = 0;
    virtual GLuint getPointsExBuffer() const = 0;
    virtual GLuint getNormalsExBuffer() const = 0;
    virtual GLuint getWireframeIndicesBuffer() const = 0;
#endif
};

class IPoints : public IEntity
{
public:
    virtual std::span<float3> getPoints() const = 0;
#ifdef wabcWithGL
    virtual GLuint getPointBuffer() const = 0;
#endif
};

class IScene
{
public:
    virtual ~IScene() {};
    virtual void release() = 0;

    virtual bool load(const char* path) = 0;
    virtual void unload() = 0;

    virtual std::tuple<double, double> getTimeRange() const = 0;
    virtual void seek(double time) = 0;

    virtual double getTime() const = 0;
    virtual IMesh* getMesh() = 0;     // monolithic mesh
    virtual IPoints* getPoints() = 0; // monolithic points
    virtual std::span<ICamera*> getCameras() = 0;
};
IScene* CreateScene_();
using IScenePtr = std::shared_ptr<IScene>;
inline IScenePtr CreateScene() { return IScenePtr(CreateScene_(), releaser<IScene>()); }


enum class SensorFitMode
{
    Auto = 0,
    Horizontal = 1,
    Vertical = 2,
};

class IRenderer
{
public:
    virtual ~IRenderer() {};
    virtual void release() = 0;

    virtual bool initialize(GLFWwindow* v) = 0;
    virtual void setCamera(float3 pos, float3 dir, float3 up, float fov, float znear, float zfar, float2 shift = float2::zero()) = 0;
    virtual void setCamera(ICamera* cam, SensorFitMode ft = SensorFitMode::Auto) = 0;
    virtual void setDrawPoints(bool v) = 0;
    virtual void setDrawWireframe(bool v) = 0;
    virtual void setDrawFaces(bool v) = 0;

    virtual void beginDraw() = 0;
    virtual void endDraw() = 0;
    virtual void draw(IMesh* mesh) = 0;
    virtual void draw(IPoints* points) = 0;
};
IRenderer* CreateRenderer_();
using IRendererPtr = std::shared_ptr<IRenderer>;
inline IRendererPtr CreateRenderer() { return IRendererPtr(CreateRenderer_(), releaser<IRenderer>()); }

} // namespace wabc
