#pragma once
#include "VectorMath.h"

namespace wabc {

template<class T>
struct releaser { void operator()(T* v) { v->release(); } };


class IEntity
{
public:
    virtual ~IEntity() {};
};

class ICamera : public IEntity
{
public:
    virtual float3 getPosition() const = 0;
    virtual float3 getDirection() const = 0;
    virtual float3 getUp() const = 0;
    virtual float getFOV() const = 0;
};

class IMesh : public IEntity
{
public:
    virtual std::span<float3> getPoints() const = 0;
    virtual std::span<float3> getNormals() const = 0;
    virtual GLuint getPointBuffer() const = 0;
    virtual GLuint getNormalBuffer() const = 0;
};

class IPoints : public IEntity
{
public:
    virtual std::span<float3> getPoints() const = 0;
    virtual GLuint getPointBuffer() const = 0;
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
};
IScene* CreateScene_();
using IScenePtr = std::shared_ptr<IScene>;
inline IScenePtr CreateScene() { return IScenePtr(CreateScene_(), releaser<IScene>()); }


class IRenderer
{
public:
    enum class DrawType
    {
        Points,
        Wireframe,
        Faces,
    };

    virtual ~IRenderer() {};
    virtual void release() = 0;

    virtual bool initialize(GLFWwindow* v) = 0;

    virtual void beginScene() = 0;
    virtual void endScene() = 0;
    virtual void setCamera(float3 pos, float3 target, float fov, float near_, float far_) = 0;
    virtual void draw(IMesh* mesh) = 0;
    virtual void draw(IPoints* points) = 0;
};
IRenderer* CreateRenderer_();
using IRendererPtr = std::shared_ptr<IRenderer>;
inline IRendererPtr CreateRenderer() { return IRendererPtr(CreateRenderer_(), releaser<IRenderer>()); }

} // namespace wabc
