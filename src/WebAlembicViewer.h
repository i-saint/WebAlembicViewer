#pragma once
#include "VectorMath.h"

namespace wabc {

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
    virtual float4x4 getTransform() const = 0;
    virtual std::span<float3> getPoints() const = 0;
    virtual std::span<int> getIndices() const = 0;
};

class IScene
{
public:
    virtual bool load(const char* path) = 0;
    virtual void close() = 0;

    virtual std::tuple<double, double> getTimeRange() const = 0;
    virtual void seek(double time) = 0;

    virtual std::span<IEntity*> getEntities() = 0;
};


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
    virtual void beginScene() = 0;
    virtual void endScene() = 0;
    virtual void setCamera(ICamera* cam) = 0;
    virtual void draw(IMesh* mesh) = 0;
};

} // namespace wabc
