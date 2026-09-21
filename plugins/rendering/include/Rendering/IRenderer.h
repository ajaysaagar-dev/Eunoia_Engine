#pragma once

class Scene;
class OrbitCamera;

class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual void RenderScene(Scene* scene, const OrbitCamera* camera) = 0;
};
