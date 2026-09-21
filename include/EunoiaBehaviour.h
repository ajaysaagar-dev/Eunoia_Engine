#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>
#include "EngineLogger.h"

// Forward declarations
struct GameObject;
class Scene;
struct LightComponent;
struct PrimitiveMesh;
class OrbitCamera;

// Aliases matching dev.md conventions
using Light = LightComponent;
using MeshRenderer = PrimitiveMesh;
using Camera = OrbitCamera;

enum class BehaviourPropertyType {
    Bool,
    Int,
    Int64,
    Float,
    Double,
    String,
    Vec2,
    Vec3,
    Vec4,
    Color3,
    Color4,
    ObjectRef
};

enum class ObjectRefType {
    Actor,
    Light,
    Mesh,
    Camera
};

inline const char* ObjectRefTypeToString(ObjectRefType type) {
    switch (type) {
        case ObjectRefType::Actor:  return "Actor";
        case ObjectRefType::Light:  return "Light";
        case ObjectRefType::Mesh:   return "Mesh";
        case ObjectRefType::Camera: return "Camera";
        default:                    return "Object";
    }
}

class EunoiaBehaviour;

struct BehaviourProperty {
    std::string name;
    std::string displayName;
    std::string category = "General";
    BehaviourPropertyType type = BehaviourPropertyType::Float;
    ObjectRefType refType = ObjectRefType::Actor;

    // Direct pointer to property memory in behaviour instance
    void* dataPtr = nullptr;

    // Persistent target ID in Scene for ObjectRef properties (-1 = none)
    int targetId = -1;

    // Range metadata
    bool hasRange = false;
    float minVal = 0.0f;
    float maxVal = 0.0f;

    // Status
    bool isMissing = false;
};

// ============================================================================
// RespectiveObjectAccessor
// Supports both syntax forms specified in dev.md Section 15:
//   GetRespectiveObject.Light(WarningLight);
//   GetRespectiveObject(WarningLight);
// ============================================================================
struct RespectiveObjectAccessor {
    EunoiaBehaviour* behaviour = nullptr;

    template<typename T>
    T* operator()(T* assignedObject) const {
        return assignedObject;
    }

    LightComponent* Light(void* objRef) const;
    PrimitiveMesh*  Mesh(void* objRef) const;
    GameObject*     Entity(void* objRef) const;
    GameObject*     Actor(void* objRef) const;
    GameObject*     Shape(void* objRef) const;
    GameObject*     Transform(void* objRef) const;
    OrbitCamera*    Camera(void* objRef) const;
};

#ifdef GetClassName
#undef GetClassName
#endif

// ============================================================================
// EunoiaBehaviour — Canonical Base Behaviour Class (dev.md Section 1)
// ============================================================================
class EunoiaBehaviour {
public:
    virtual ~EunoiaBehaviour() = default;

    // Lifecycle methods (dev.md Section 1, 3)
    virtual void OnCreate() {}
    virtual void OnEnable() {}
    virtual void Start() {}
    virtual void Update(float deltaTime) {}
    virtual void FixedUpdate(float fixedDeltaTime) {}
    virtual void LateUpdate(float deltaTime) {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}

    // Owner / Entity access (dev.md Section 1)
    GameObject* GetOwner() const { return m_owner; }
    GameObject* GetEntity() const { return m_owner; }
    void SetOwner(GameObject* owner) { m_owner = owner; }

    Scene* GetScene() const { return m_scene; }
    void SetScene(Scene* scene) { m_scene = scene; }

    // Enabled state (dev.md Section 17)
    bool IsEnabled() const { return m_enabled; }
    void SetEnabled(bool enabled) {
        if (m_enabled == enabled) return;
        m_enabled = enabled;
        if (m_enabled) {
            OnEnable();
        } else {
            OnDisable();
        }
    }

    bool HasStarted() const { return m_hasStarted; }
    void SetStarted(bool started) { m_hasStarted = started; }

    // Metadata & Typing
    virtual std::string GetClassName() const { return m_className; }
    virtual std::string GetDisplayName() const { return m_displayName.empty() ? m_className : m_displayName; }
    void SetClassName(const std::string& name) { m_className = name; }
    void SetDisplayName(const std::string& name) { m_displayName = name; }

    // Cloning support for deep copying scene snapshots (Undo/Redo & PlayMode)
    virtual std::unique_ptr<EunoiaBehaviour> Clone() const = 0;

    // Reflection & Properties (dev.md Section 7, 8, 23, 24)
    virtual void RegisterProperties() {}
    std::vector<BehaviourProperty>& GetProperties() { return m_properties; }
    const std::vector<BehaviourProperty>& GetProperties() const { return m_properties; }

    // Scene object reference resolution (dev.md Section 9, 14, 16)
    void ResolveReferences(Scene& scene);

    // Helpers to register primitive and vector properties
    void RegisterProperty(const std::string& name, bool* val, const std::string& category = "General");
    void RegisterProperty(const std::string& name, int* val, const std::string& category = "General", int min = 0, int max = 0);
    void RegisterProperty(const std::string& name, int64_t* val, const std::string& category = "General");
    void RegisterProperty(const std::string& name, float* val, const std::string& category = "General", float min = 0.0f, float max = 0.0f);
    void RegisterProperty(const std::string& name, double* val, const std::string& category = "General");
    void RegisterProperty(const std::string& name, std::string* val, const std::string& category = "General");
    void RegisterProperty(const std::string& name, glm::vec2* val, const std::string& category = "General");
    void RegisterProperty(const std::string& name, glm::vec3* val, const std::string& category = "General");
    void RegisterProperty(const std::string& name, glm::vec4* val, const std::string& category = "General");
    void RegisterColor(const std::string& name, glm::vec3* val, const std::string& category = "General");

    // Helper to register scene object references
    void RegisterReference(const std::string& name, void* ptrAddr, ObjectRefType refType, const std::string& category = "References");

    // RespectiveObject Accessor functor (dev.md Section 15)
    RespectiveObjectAccessor GetRespectiveObject;

protected:
    EunoiaBehaviour() {
        GetRespectiveObject.behaviour = this;
    }

    EunoiaBehaviour(const EunoiaBehaviour& other)
        : m_owner(other.m_owner),
          m_scene(other.m_scene),
          m_enabled(other.m_enabled),
          m_hasStarted(other.m_hasStarted),
          m_className(other.m_className),
          m_displayName(other.m_displayName) {
        GetRespectiveObject.behaviour = this;
    }

    GameObject* m_owner = nullptr;
    Scene* m_scene = nullptr;
    bool m_enabled = true;
    bool m_hasStarted = false;
    std::string m_className = "EunoiaBehaviour";
    std::string m_displayName = "Eunoia Behaviour";
    std::vector<BehaviourProperty> m_properties;

    friend struct RespectiveObjectAccessor;
};
