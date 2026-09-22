#pragma once
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>
#include <EngineCore/EngineLogger.h>
#include <EngineScene/ScreenPrint.h>

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
    GameObject*     Camera(void* objRef) const;
    GameObject*     CameraActor(void* objRef) const;
};

// ============================================================================
// BehaviourTransformProperty & BehaviourTransform
// Exposes Location, Position, Rotation, and Scale with capitalized keywords:
// .LocalSpace(vector3) and .WorldSpace(vector3)
// ============================================================================
struct BehaviourTransformProperty {
    enum class Type { Location, Rotation, Scale };
    EunoiaBehaviour* behaviour = nullptr;
    Type type = Type::Location;

    BehaviourTransformProperty() = default;
    BehaviourTransformProperty(EunoiaBehaviour* b, Type t) : behaviour(b), type(t) {}

    // Capitalized keywords as requested:
    glm::vec3 LocalSpace() const;
    void LocalSpace(const glm::vec3& v);

    glm::vec3 WorldSpace() const;
    void WorldSpace(const glm::vec3& v);

    // Aliases & getters/setters:
    glm::vec3 GetLocalSpace() const { return LocalSpace(); }
    void SetLocalSpace(const glm::vec3& v) { LocalSpace(v); }
    glm::vec3 GetWorldSpace() const { return WorldSpace(); }
    void SetWorldSpace(const glm::vec3& v) { WorldSpace(v); }

    // Lowercase / phonetic aliases:
    glm::vec3 locaspace() const { return LocalSpace(); }
    void locaspace(const glm::vec3& v) { LocalSpace(v); }
    glm::vec3 localspace() const { return LocalSpace(); }
    void localspace(const glm::vec3& v) { LocalSpace(v); }
    glm::vec3 worldspace() const { return WorldSpace(); }
    void worldspace(const glm::vec3& v) { WorldSpace(v); }

    // Direct arithmetic & vector casting:
    operator glm::vec3() const { return LocalSpace(); }
    BehaviourTransformProperty& operator=(const glm::vec3& v) { LocalSpace(v); return *this; }
    BehaviourTransformProperty& operator+=(const glm::vec3& v) { LocalSpace(LocalSpace() + v); return *this; }
    BehaviourTransformProperty& operator-=(const glm::vec3& v) { LocalSpace(LocalSpace() - v); return *this; }
    BehaviourTransformProperty& operator*=(float s) { LocalSpace(LocalSpace() * s); return *this; }
    BehaviourTransformProperty& operator/=(float s) { LocalSpace(LocalSpace() / s); return *this; }

    float x() const { return LocalSpace().x; }
    float y() const { return LocalSpace().y; }
    float z() const { return LocalSpace().z; }
    void x(float val) { glm::vec3 v = LocalSpace(); v.x = val; LocalSpace(v); }
    void y(float val) { glm::vec3 v = LocalSpace(); v.y = val; LocalSpace(v); }
    void z(float val) { glm::vec3 v = LocalSpace(); v.z = val; LocalSpace(v); }
};

struct BehaviourTransform {
    BehaviourTransformProperty Location;
    BehaviourTransformProperty Position;
    BehaviourTransformProperty Rotation;
    BehaviourTransformProperty Scale;

    BehaviourTransformProperty& location = Location;
    BehaviourTransformProperty& position = Position;
    BehaviourTransformProperty& rotation = Rotation;
    BehaviourTransformProperty& scale = Scale;

    BehaviourTransform() = default;

    BehaviourTransform(const BehaviourTransform& other)
        : Location(other.Location),
          Position(other.Position),
          Rotation(other.Rotation),
          Scale(other.Scale) {}

    BehaviourTransform& operator=(const BehaviourTransform& other) {
        if (this != &other) {
            Location = other.Location;
            Position = other.Position;
            Rotation = other.Rotation;
            Scale = other.Scale;
        }
        return *this;
    }

    void Init(EunoiaBehaviour* b) {
        Location = BehaviourTransformProperty(b, BehaviourTransformProperty::Type::Location);
        Position = BehaviourTransformProperty(b, BehaviourTransformProperty::Type::Location);
        Rotation = BehaviourTransformProperty(b, BehaviourTransformProperty::Type::Rotation);
        Scale = BehaviourTransformProperty(b, BehaviourTransformProperty::Type::Scale);
    }
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

    // Owner / Entity access (dev.md Section 34)
    GameObject* GetOwner() const { return m_owner; }
    GameObject* GetEntity() const { return m_owner; }
    void SetOwner(GameObject* owner) { m_owner = owner; }

    Scene* GetScene() const { return m_scene; }
    void SetScene(Scene* scene) { m_scene = scene; }

    // -------------------------------------------------------------------------
    // GetBehaviour<T>() / HasBehaviour<T>() — access sibling behaviours on the
    // same owner (dev.md Section 62, 63)
    // -------------------------------------------------------------------------
    template<typename T>
    T* GetBehaviour() const;

    template<typename T>
    bool HasBehaviour() const;

    // -------------------------------------------------------------------------
    // GetTransform() — returns BehaviourTransform reference (dev.md Section 34)
    // -------------------------------------------------------------------------
    BehaviourTransform& GetTransform() { return Transform; }
    const BehaviourTransform& GetTransform() const { return Transform; }

    // -------------------------------------------------------------------------
    // GetComponent<T>() — typed component access on the owner (dev.md Section 35)
    // Specialisations for LightComponent, PrimitiveMesh, OrbitCamera defined
    // after the class.
    // -------------------------------------------------------------------------
    template<typename T>
    T* GetComponent() const { return nullptr; }

    // -------------------------------------------------------------------------
    // SpawnGameObject / DestroyGameObject — runtime object creation (dev.md §45)
    // -------------------------------------------------------------------------
    GameObject* SpawnGameObject(const std::string& name, const glm::vec3& position = glm::vec3(0.0f));
    void        DestroyGameObject(int objectId);

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

    // Transform accessors (Capitalized keywords: Transform.Location, Transform.Position, Transform.Rotation, Transform.Scale)
    BehaviourTransform Transform;
    BehaviourTransform& transform = Transform;

    // On-screen Print function (prints in game view screen top-left with time)
    template<typename T>
    void Print(const T& value, float time) {
        ::Print(value, time);
    }
    template<typename T>
    void Print(const T& value) {
        ::Print(value);
    }
    template<typename T1, typename T2, typename... Rest>
    void Print(const T1& v1, const T2& v2, const Rest&... rest) {
        ::Print(v1, v2, rest...);
    }

protected:
    EunoiaBehaviour() {
        GetRespectiveObject.behaviour = this;
        Transform.Init(this);
    }

    EunoiaBehaviour(const EunoiaBehaviour& other)
        : Transform(other.Transform),
          transform(Transform),
          m_owner(other.m_owner),
          m_scene(other.m_scene),
          m_enabled(other.m_enabled),
          m_hasStarted(other.m_hasStarted),
          m_className(other.m_className),
          m_displayName(other.m_displayName),
          m_properties(other.m_properties) {
        GetRespectiveObject.behaviour = this;
        Transform.Init(this);
    }

    EunoiaBehaviour& operator=(const EunoiaBehaviour& other) {
        if (this != &other) {
            m_owner = other.m_owner;
            m_scene = other.m_scene;
            m_enabled = other.m_enabled;
            m_hasStarted = other.m_hasStarted;
            m_className = other.m_className;
            m_displayName = other.m_displayName;
            m_properties = other.m_properties;
            GetRespectiveObject.behaviour = this;
            Transform = other.Transform;
            Transform.Init(this);
        }
        return *this;
    }

    GameObject* m_owner = nullptr;
    Scene* m_scene = nullptr;
    bool m_enabled = true;
    bool m_hasStarted = false;
    std::string m_className = "EunoiaBehaviour";
    std::string m_displayName = "Eunoia Behaviour";
    std::vector<BehaviourProperty> m_properties;

    friend struct RespectiveObjectAccessor;
    friend struct BehaviourTransformProperty;
};

// ============================================================================
// GetComponent<T> Specialisations (dev.md Section 35)
// ============================================================================
template<> LightComponent* EunoiaBehaviour::GetComponent<LightComponent>() const;
template<> PrimitiveMesh*  EunoiaBehaviour::GetComponent<PrimitiveMesh>() const;

