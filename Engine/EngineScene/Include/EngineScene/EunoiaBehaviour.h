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
#include <EngineScene/TransformTypes.h>

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
using Vector2 = glm::vec2;
using Vector4 = glm::vec4;

// Global World Delta Seconds variable (dev.md: "the global var called 'World Delta Seconds' use as a DeltaTime")
inline float WorldDeltaSeconds = 0.0f;
inline float World_Delta_Seconds = 0.0f;
inline float worldDeltaSeconds = 0.0f;
inline float WorldDeltaTime = 0.0f;
#define WORLD_DELTA_SECONDS WorldDeltaSeconds

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
    void LocalSpace(float x, float y, float z) { LocalSpace(glm::vec3(x, y, z)); }
    void LocalSpace(const glm::vec3* v) { if (v) { LocalSpace(*v); delete v; } }

    glm::vec3 WorldSpace() const;
    void WorldSpace(const glm::vec3& v);
    void WorldSpace(float x, float y, float z) { WorldSpace(glm::vec3(x, y, z)); }
    void WorldSpace(const glm::vec3* v) { if (v) { WorldSpace(*v); delete v; } }

    void RelativeSpace(const glm::vec3& v) { LocalSpace(v); }
    void RelativeSpace(const glm::vec3* v) { if (v) { LocalSpace(*v); delete v; } }
    void RelativeSpace(float x, float y, float z) { LocalSpace(glm::vec3(x, y, z)); }
    glm::vec3 RelativeSpace() const { return LocalSpace(); }

    // Aliases & getters/setters:
    glm::vec3 GetLocalSpace() const { return LocalSpace(); }
    void SetLocalSpace(const glm::vec3& v) { LocalSpace(v); }
    void SetLocalSpace(float x, float y, float z) { LocalSpace(glm::vec3(x, y, z)); }
    glm::vec3 GetWorldSpace() const { return WorldSpace(); }
    void SetWorldSpace(const glm::vec3& v) { WorldSpace(v); }
    void SetWorldSpace(float x, float y, float z) { WorldSpace(glm::vec3(x, y, z)); }

    // Lowercase / phonetic aliases:
    glm::vec3 locaspace() const { return LocalSpace(); }
    void locaspace(const glm::vec3& v) { LocalSpace(v); }
    void locaspace(float x, float y, float z) { LocalSpace(glm::vec3(x, y, z)); }
    glm::vec3 localspace() const { return LocalSpace(); }
    void localspace(const glm::vec3& v) { LocalSpace(v); }
    void localspace(float x, float y, float z) { LocalSpace(glm::vec3(x, y, z)); }
    glm::vec3 worldspace() const { return WorldSpace(); }
    void worldspace(const glm::vec3& v) { WorldSpace(v); }
    void worldspace(float x, float y, float z) { WorldSpace(glm::vec3(x, y, z)); }

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

    float GetX() const { return LocalSpace().x; }
    float GetY() const { return LocalSpace().y; }
    float GetZ() const { return LocalSpace().z; }
    float X() const { return LocalSpace().x; }
    float Y() const { return LocalSpace().y; }
    float Z() const { return LocalSpace().z; }
    void X(float val) { x(val); }
    void Y(float val) { y(val); }
    void Z(float val) { z(val); }
    void SetX(float val) { x(val); }
    void SetY(float val) { y(val); }
    void SetZ(float val) { z(val); }

    void operator()(float inX, float inY, float inZ) { LocalSpace(glm::vec3(inX, inY, inZ)); }
    void operator()(const glm::vec3& v) { LocalSpace(v); }
    void operator()(const TransformVector3& v) { LocalSpace(glm::vec3(v.x, v.y, v.z)); }
    TransformVector3 operator()() const { return TransformVector3(LocalSpace()); }
};

struct BehaviourTransform {
    struct AxisProxy {
        enum class TargetType {
            WorldLocation,
            RelativeLocation,
            WorldRotation,
            RelativeRotation,
            WorldScale,
            RelativeScale
        };

        BehaviourTransform* parent = nullptr;
        TargetType target = TargetType::WorldLocation;

        AxisProxy() = default;
        AxisProxy(BehaviourTransform* p, TargetType t) : parent(p), target(t) {}

        void operator()(float inX, float inY, float inZ);
        void operator()(const glm::vec3& v);
        void operator()(const glm::vec3* v);
        void operator()(const TransformVector3& v);
        void operator()(const TransformVector3* v);

        TransformVector3 operator()() const;

        operator glm::vec3() const { return GetValue(); }
        operator TransformVector3() const { return TransformVector3(GetValue()); }

        void X(float val);
        void Y(float val);
        void Z(float val);

        float GetX() const;
        float GetY() const;
        float GetZ() const;

        float x() const { return GetX(); }
        float y() const { return GetY(); }
        float z() const { return GetZ(); }

        float X() const { return GetX(); }
        float Y() const { return GetY(); }
        float Z() const { return GetZ(); }

        void SetX(float val) { X(val); }
        void SetY(float val) { Y(val); }
        void SetZ(float val) { Z(val); }

        glm::vec3 GetValue() const;
        void SetValue(const glm::vec3& v);
    };

    EunoiaBehaviour* m_behaviour = nullptr;

    BehaviourTransformProperty Location;
    BehaviourTransformProperty Position;
    BehaviourTransformProperty Rotation;
    BehaviourTransformProperty Scale;

    BehaviourTransformProperty& location = Location;
    BehaviourTransformProperty& position = Position;
    BehaviourTransformProperty& rotation = Rotation;
    BehaviourTransformProperty& scale = Scale;

    AxisProxy WorldLocation{this, AxisProxy::TargetType::WorldLocation};
    AxisProxy RelativeLocation{this, AxisProxy::TargetType::RelativeLocation};
    AxisProxy WorldRotation{this, AxisProxy::TargetType::WorldRotation};
    AxisProxy RelativeRotation{this, AxisProxy::TargetType::RelativeRotation};
    AxisProxy WorldScale{this, AxisProxy::TargetType::WorldScale};
    AxisProxy RelativeScale{this, AxisProxy::TargetType::RelativeScale};

    AxisProxy WorldPosition{this, AxisProxy::TargetType::WorldLocation};
    AxisProxy RelativePosition{this, AxisProxy::TargetType::RelativeLocation};

    BehaviourTransform() : BehaviourTransform(nullptr) {}

    BehaviourTransform(EunoiaBehaviour* b) : m_behaviour(b) {
        InitProxies();
    }

    BehaviourTransform(const BehaviourTransform& other)
        : m_behaviour(other.m_behaviour),
          Location(other.Location),
          Position(other.Position),
          Rotation(other.Rotation),
          Scale(other.Scale) {
        InitProxies();
    }

    BehaviourTransform& operator=(const BehaviourTransform& other) {
        if (this != &other) {
            m_behaviour = other.m_behaviour;
            Location = other.Location;
            Position = other.Position;
            Rotation = other.Rotation;
            Scale = other.Scale;
            InitProxies();
        }
        return *this;
    }

    void InitProxies() {
        WorldLocation = AxisProxy(this, AxisProxy::TargetType::WorldLocation);
        RelativeLocation = AxisProxy(this, AxisProxy::TargetType::RelativeLocation);
        WorldRotation = AxisProxy(this, AxisProxy::TargetType::WorldRotation);
        RelativeRotation = AxisProxy(this, AxisProxy::TargetType::RelativeRotation);
        WorldScale = AxisProxy(this, AxisProxy::TargetType::WorldScale);
        RelativeScale = AxisProxy(this, AxisProxy::TargetType::RelativeScale);
        WorldPosition = AxisProxy(this, AxisProxy::TargetType::WorldLocation);
        RelativePosition = AxisProxy(this, AxisProxy::TargetType::RelativeLocation);
    }

    void Init(EunoiaBehaviour* b) {
        m_behaviour = b;
        Location = BehaviourTransformProperty(b, BehaviourTransformProperty::Type::Location);
        Position = BehaviourTransformProperty(b, BehaviourTransformProperty::Type::Location);
        Rotation = BehaviourTransformProperty(b, BehaviourTransformProperty::Type::Rotation);
        Scale = BehaviourTransformProperty(b, BehaviourTransformProperty::Type::Scale);
        InitProxies();
    }

    // Direct Getters (Docs/Behaviours/Transform.md: # GET)
    TransformVector3 GetWorldLocation() const { return WorldLocation(); }
    TransformVector3 GetRelativeLocation() const { return RelativeLocation(); }
    TransformVector3 GetWorldRotation() const { return WorldRotation(); }
    TransformVector3 GetRelativeRotation() const { return RelativeRotation(); }
    TransformVector3 GetWorldScale() const { return WorldScale(); }
    TransformVector3 GetRelativeScale() const { return RelativeScale(); }

    TransformVector3 GetWorldPosition() const { return WorldLocation(); }
    TransformVector3 GetRelativePosition() const { return RelativeLocation(); }

    // Transform State (Docs/Behaviours/Transform.md: # TRANSFORM)
    TransformState GetWorldTransform() const;
    TransformState GetRelativeTransform() const;

    // Directions (Docs/Behaviours/Transform.md: # DIRECTION & # DIRECTION AXIS)
    TransformVector3 GetForwardVector() const;
    TransformVector3 GetRightVector() const;
    TransformVector3 GetUpVector() const;

    // World Translate (dev.md: Transform.WorldTranslate(0, 0, 0);)
    // 1. By delta (x, y, z) / (delta) / (new Vector3(...))
    void WorldTranslate(const glm::vec3& delta) { Location.WorldSpace(Location.WorldSpace() + delta); }
    void WorldTranslate(float x, float y, float z) { WorldTranslate(glm::vec3(x, y, z)); }
    void WorldTranslate(const glm::vec3* delta) { if (delta) { WorldTranslate(*delta); delete delta; } }

    // 2. From exact point to another location in world space:
    // Moves object such that point 'fromPoint' moves to 'toLocation' (delta = toLocation - fromPoint)
    void WorldTranslate(const glm::vec3& fromPoint, const glm::vec3& toLocation) {
        WorldTranslate(toLocation - fromPoint);
    }
    void WorldTranslate(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) {
        WorldTranslate(glm::vec3(fromX, fromY, fromZ), glm::vec3(toX, toY, toZ));
    }
    void WorldTranslate(const glm::vec3* fromPoint, const glm::vec3* toLocation) {
        if (fromPoint && toLocation) { WorldTranslate(*fromPoint, *toLocation); }
        if (fromPoint) delete fromPoint;
        if (toLocation) delete toLocation;
    }
    void WorldTranslateFromTo(const glm::vec3& fromPoint, const glm::vec3& toLocation) { WorldTranslate(fromPoint, toLocation); }
    void WorldTranslateFromTo(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) {
        WorldTranslate(fromX, fromY, fromZ, toX, toY, toZ);
    }
    void WorldTranslateFromTo(const glm::vec3* fromPoint, const glm::vec3* toLocation) { WorldTranslate(fromPoint, toLocation); }
    void WorldTranslateTo(const glm::vec3& targetLocation) { Location.WorldSpace(targetLocation); }
    void WorldTranslateTo(float x, float y, float z) { Location.WorldSpace(x, y, z); }
    void WorldTranslateTo(const glm::vec3* targetLocation) { Location.WorldSpace(targetLocation); }

    // Relative Translate (dev.md: Transform.RelativeTranslate(0, 0, 0);)
    // 1. By delta (x, y, z) / (delta) / (new Vector3(...))
    void RelativeTranslate(const glm::vec3& delta) { Location.LocalSpace(Location.LocalSpace() + delta); }
    void RelativeTranslate(float x, float y, float z) { RelativeTranslate(glm::vec3(x, y, z)); }
    void RelativeTranslate(const glm::vec3* delta) { if (delta) { RelativeTranslate(*delta); delete delta; } }

    // 2. From exact point to another location in relative space
    void RelativeTranslate(const glm::vec3& fromPoint, const glm::vec3& toLocation) {
        RelativeTranslate(toLocation - fromPoint);
    }
    void RelativeTranslate(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) {
        RelativeTranslate(glm::vec3(fromX, fromY, fromZ), glm::vec3(toX, toY, toZ));
    }
    void RelativeTranslate(const glm::vec3* fromPoint, const glm::vec3* toLocation) {
        if (fromPoint && toLocation) { RelativeTranslate(*fromPoint, *toLocation); }
        if (fromPoint) delete fromPoint;
        if (toLocation) delete toLocation;
    }
    void RelativeTranslateFromTo(const glm::vec3& fromPoint, const glm::vec3& toLocation) { RelativeTranslate(fromPoint, toLocation); }
    void RelativeTranslateFromTo(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) {
        RelativeTranslate(fromX, fromY, fromZ, toX, toY, toZ);
    }
    void RelativeTranslateFromTo(const glm::vec3* fromPoint, const glm::vec3* toLocation) { RelativeTranslate(fromPoint, toLocation); }
    void RelativeTranslateTo(const glm::vec3& targetLocation) { Location.LocalSpace(targetLocation); }
    void RelativeTranslateTo(float x, float y, float z) { Location.LocalSpace(x, y, z); }
    void RelativeTranslateTo(const glm::vec3* targetLocation) { Location.LocalSpace(targetLocation); }

    // General Translate
    void Translate(const glm::vec3& delta) { RelativeTranslate(delta); }
    void Translate(float x, float y, float z) { RelativeTranslate(x, y, z); }
    void Translate(const glm::vec3* delta) { RelativeTranslate(delta); }
    void Translate(const glm::vec3& fromPoint, const glm::vec3& toLocation) { RelativeTranslate(fromPoint, toLocation); }
    void Translate(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) {
        RelativeTranslate(fromX, fromY, fromZ, toX, toY, toZ);
    }
    void Translate(const glm::vec3* fromPoint, const glm::vec3* toLocation) { RelativeTranslate(fromPoint, toLocation); }
    void TranslateFromTo(const glm::vec3& fromPoint, const glm::vec3& toLocation) { RelativeTranslate(fromPoint, toLocation); }
    void TranslateTo(const glm::vec3& targetLocation) { RelativeTranslateTo(targetLocation); }
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

    // DeltaTime accessors (dev.md: "the global var called 'World Delta Seconds' use as a DeltaTime")
    float DeltaTime() const { return ::WorldDeltaSeconds; }
    float GetDeltaTime() const { return ::WorldDeltaSeconds; }
    float GetWorldDeltaSeconds() const { return ::WorldDeltaSeconds; }

    // Owner / Entity access (dev.md Section 34)
    GameObject* gameObject = nullptr;
    GameObject* GetOwner() const { return m_owner; }
    GameObject* GetEntity() const { return m_owner; }
    void SetOwner(GameObject* owner) {
        m_owner = owner;
        gameObject = owner;
        Transform.Init(this);
        GetRespectiveObject.behaviour = this;
    }

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

    // Copies registered properties and target references from another behaviour instance of the same type
    void CopyPropertiesFrom(const EunoiaBehaviour& other) {
        RegisterProperties();
        for (const auto& src : other.m_properties) {
            for (auto& dst : m_properties) {
                if (dst.name == src.name) {
                    dst.targetId = src.targetId;
                    dst.isMissing = src.isMissing;
                    if (dst.dataPtr && src.dataPtr) {
                        switch (dst.type) {
                            case BehaviourPropertyType::Bool:
                                *reinterpret_cast<bool*>(dst.dataPtr) = *reinterpret_cast<const bool*>(src.dataPtr);
                                break;
                            case BehaviourPropertyType::Int:
                                *reinterpret_cast<int*>(dst.dataPtr) = *reinterpret_cast<const int*>(src.dataPtr);
                                break;
                            case BehaviourPropertyType::Int64:
                                *reinterpret_cast<int64_t*>(dst.dataPtr) = *reinterpret_cast<const int64_t*>(src.dataPtr);
                                break;
                            case BehaviourPropertyType::Float:
                                *reinterpret_cast<float*>(dst.dataPtr) = *reinterpret_cast<const float*>(src.dataPtr);
                                break;
                            case BehaviourPropertyType::Double:
                                *reinterpret_cast<double*>(dst.dataPtr) = *reinterpret_cast<const double*>(src.dataPtr);
                                break;
                            case BehaviourPropertyType::String:
                                *reinterpret_cast<std::string*>(dst.dataPtr) = *reinterpret_cast<const std::string*>(src.dataPtr);
                                break;
                            case BehaviourPropertyType::Vec2:
                                *reinterpret_cast<glm::vec2*>(dst.dataPtr) = *reinterpret_cast<const glm::vec2*>(src.dataPtr);
                                break;
                            case BehaviourPropertyType::Vec3:
                            case BehaviourPropertyType::Color3:
                                *reinterpret_cast<glm::vec3*>(dst.dataPtr) = *reinterpret_cast<const glm::vec3*>(src.dataPtr);
                                break;
                            case BehaviourPropertyType::Vec4:
                            case BehaviourPropertyType::Color4:
                                *reinterpret_cast<glm::vec4*>(dst.dataPtr) = *reinterpret_cast<const glm::vec4*>(src.dataPtr);
                                break;
                            case BehaviourPropertyType::ObjectRef:
                                *reinterpret_cast<void**>(dst.dataPtr) = *reinterpret_cast<void* const*>(src.dataPtr);
                                break;
                        }
                    }
                    break;
                }
            }
        }
    }

    // Reflection & Properties (dev.md Section 7, 8, 23, 24)
    virtual void RegisterProperties() {}
    virtual void RefreshPropertiesFromSource() {}
    virtual std::string GetSourceCppPath() const { return ""; }
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

    // Direct translation helpers on behaviour
    void WorldTranslate(float x, float y, float z) { Transform.WorldTranslate(x, y, z); }
    void WorldTranslate(const glm::vec3& delta) { Transform.WorldTranslate(delta); }
    void WorldTranslate(const glm::vec3* delta) { Transform.WorldTranslate(delta); }
    void WorldTranslate(const glm::vec3& fromPoint, const glm::vec3& toLocation) { Transform.WorldTranslate(fromPoint, toLocation); }
    void WorldTranslate(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) { Transform.WorldTranslate(fromX, fromY, fromZ, toX, toY, toZ); }
    void WorldTranslate(const glm::vec3* fromPoint, const glm::vec3* toLocation) { Transform.WorldTranslate(fromPoint, toLocation); }
    void WorldTranslateFromTo(const glm::vec3& fromPoint, const glm::vec3& toLocation) { Transform.WorldTranslateFromTo(fromPoint, toLocation); }
    void WorldTranslateFromTo(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) { Transform.WorldTranslateFromTo(fromX, fromY, fromZ, toX, toY, toZ); }
    void WorldTranslateFromTo(const glm::vec3* fromPoint, const glm::vec3* toLocation) { Transform.WorldTranslateFromTo(fromPoint, toLocation); }
    void WorldTranslateTo(const glm::vec3& targetLocation) { Transform.WorldTranslateTo(targetLocation); }
    void WorldTranslateTo(float x, float y, float z) { Transform.WorldTranslateTo(x, y, z); }
    void WorldTranslateTo(const glm::vec3* targetLocation) { Transform.WorldTranslateTo(targetLocation); }

    void RelativeTranslate(float x, float y, float z) { Transform.RelativeTranslate(x, y, z); }
    void RelativeTranslate(const glm::vec3& delta) { Transform.RelativeTranslate(delta); }
    void RelativeTranslate(const glm::vec3* delta) { Transform.RelativeTranslate(delta); }
    void RelativeTranslate(const glm::vec3& fromPoint, const glm::vec3& toLocation) { Transform.RelativeTranslate(fromPoint, toLocation); }
    void RelativeTranslate(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) { Transform.RelativeTranslate(fromX, fromY, fromZ, toX, toY, toZ); }
    void RelativeTranslate(const glm::vec3* fromPoint, const glm::vec3* toLocation) { Transform.RelativeTranslate(fromPoint, toLocation); }
    void RelativeTranslateFromTo(const glm::vec3& fromPoint, const glm::vec3& toLocation) { Transform.RelativeTranslateFromTo(fromPoint, toLocation); }
    void RelativeTranslateFromTo(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) { Transform.RelativeTranslateFromTo(fromX, fromY, fromZ, toX, toY, toZ); }
    void RelativeTranslateFromTo(const glm::vec3* fromPoint, const glm::vec3* toLocation) { Transform.RelativeTranslateFromTo(fromPoint, toLocation); }
    void RelativeTranslateTo(const glm::vec3& targetLocation) { Transform.RelativeTranslateTo(targetLocation); }
    void RelativeTranslateTo(float x, float y, float z) { Transform.RelativeTranslateTo(x, y, z); }
    void RelativeTranslateTo(const glm::vec3* targetLocation) { Transform.RelativeTranslateTo(targetLocation); }

    void Translate(float x, float y, float z) { Transform.Translate(x, y, z); }
    void Translate(const glm::vec3& delta) { Transform.Translate(delta); }
    void Translate(const glm::vec3* delta) { Transform.Translate(delta); }
    void Translate(const glm::vec3& fromPoint, const glm::vec3& toLocation) { Transform.Translate(fromPoint, toLocation); }
    void Translate(float fromX, float fromY, float fromZ, float toX, float toY, float toZ) { Transform.Translate(fromX, fromY, fromZ, toX, toY, toZ); }
    void Translate(const glm::vec3* fromPoint, const glm::vec3* toLocation) { Transform.Translate(fromPoint, toLocation); }
    void TranslateFromTo(const glm::vec3& fromPoint, const glm::vec3& toLocation) { Transform.TranslateFromTo(fromPoint, toLocation); }
    void TranslateTo(const glm::vec3& targetLocation) { Transform.TranslateTo(targetLocation); }

    // Direct Transform Getters (Docs/Behaviours/Transform.md: # GET, # TRANSFORM, # DIRECTION)
    TransformVector3 GetWorldLocation() const { return Transform.GetWorldLocation(); }
    TransformVector3 GetRelativeLocation() const { return Transform.GetRelativeLocation(); }
    TransformVector3 GetWorldRotation() const { return Transform.GetWorldRotation(); }
    TransformVector3 GetRelativeRotation() const { return Transform.GetRelativeRotation(); }
    TransformVector3 GetWorldScale() const { return Transform.GetWorldScale(); }
    TransformVector3 GetRelativeScale() const { return Transform.GetRelativeScale(); }

    TransformState GetWorldTransform() const { return Transform.GetWorldTransform(); }
    TransformState GetRelativeTransform() const { return Transform.GetRelativeTransform(); }

    TransformVector3 GetForwardVector() const { return Transform.GetForwardVector(); }
    TransformVector3 GetRightVector() const { return Transform.GetRightVector(); }
    TransformVector3 GetUpVector() const { return Transform.GetUpVector(); }

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
          gameObject(other.gameObject),
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
            gameObject = other.gameObject;
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

