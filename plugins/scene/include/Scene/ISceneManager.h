#pragma once
#include <vector>
#include <string>

class Scene;

class ISceneManager {
public:
    virtual ~ISceneManager() = default;
    virtual Scene* GetActiveScene() = 0;
    virtual void SetActiveScene(Scene* scene) = 0;
};
