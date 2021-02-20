#include "pch.h"
#include "WebAlembicViewer.h"

namespace wabc {

class Renderer : public IRenderer
{
public:
    void release() override;

    void beginScene() override;
    void endScene() override;
    void setCamera(ICamera* cam) override;
    void draw(IMesh* mesh) override;

private:
};

void Renderer::release()
{
    delete this;
}

void Renderer::beginScene()
{
}

void Renderer::endScene()
{
}

void Renderer::setCamera(ICamera* cam)
{
}

void Renderer::draw(IMesh* mesh)
{
}

IRenderer* CreateRenderer_()
{
    return new Renderer();
}

} // namespace wabc
