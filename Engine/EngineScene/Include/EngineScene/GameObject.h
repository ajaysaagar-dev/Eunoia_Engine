#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <EngineAssets/Geometry.h>
#include <EngineScene/TransformTypes.h>
#include <EngineScene/EunoiaBehaviour.h>

enum class LightType {
    Directional,
    Point,
    Spot,
    Area,
    Sky
};

inline const char* GetLightTypeName(LightType type) {
    switch (type) {
        case LightType::Directional: return "Directional Light";
        case LightType::Point:       return "Point Light";
        case LightType::Spot:        return "Spot Light";
        case LightType::Area:        return "Area Light";
        case LightType::Sky:         return "Sky Light";
        default:                     return "Light";
    }
}

inline glm::vec3 ColorTemperatureToRGB(float kelvin) {
    float temp = glm::clamp(kelvin, 1000.0f, 40000.0f) / 100.0f;
    float red, green, blue;
    if (temp <= 66.0f) {
        red = 255.0f;
        green = 99.4708025861f * std::log(temp) - 161.1195681661f;
        if (temp <= 19.0f) {
            blue = 0.0f;
        } else {
            blue = 138.5177312231f * std::log(temp - 10.0f) - 305.0447927307f;
        }
    } else {
        red = 329.698727446f * std::pow(temp - 60.0f, -0.1332047592f);
        green = 288.1221695283f * std::pow(temp - 60.0f, -0.0755148492f);
        blue = 255.0f;
    }
    return glm::vec3(glm::clamp(red / 255.0f, 0.0f, 1.0f),
                     glm::clamp(green / 255.0f, 0.0f, 1.0f),
                     glm::clamp(blue / 255.0f, 0.0f, 1.0f));
}

struct LightComponent {
    LightType type = LightType::Point;
    bool enabled = true;
    glm::vec3 color{1.0f, 0.95f, 0.85f};
    float intensity = 2.0f;
    float temperature = 6500.0f; // Kelvin (1000K - 12000K)
    bool useTemperature = false;
    float range = 10.0f;
    float attenuation = 2.0f;

    // Direction (Directional, Spot, Hemisphere)
    glm::vec3 direction{0.0f, -1.0f, 0.0f};

    // Spot Light
    float innerConeAngle = 20.0f; // degrees
    float outerConeAngle = 45.0f; // degrees
    float coneFalloff = 1.0f;

    // Area / Rect / Disc / Tube
    int areaShape = 0; // 0 = Rectangle, 1 = Disk, 2 = Sphere, 3 = Tube
    float width = 1.0f;
    float height = 1.0f;
    float radius = 0.5f;
    float length = 1.0f;
    bool twoSided = false;

    // Sky / Environment Light
    std::string envMapTexture = "";
    float envRotation = 0.0f;
    float diffuseContribution = 1.0f;
    float specularContribution = 1.0f;
    float ambientContribution = 0.25f;
    float mipLevel = 0.0f;
    glm::vec3 lowerHemisphereColor{0.15f, 0.12f, 0.10f};

    // Hemisphere Light
    glm::vec3 skyColor{0.4f, 0.6f, 0.9f};
    glm::vec3 groundColor{0.3f, 0.2f, 0.1f};

    // Shadow properties
    bool castShadows = true;
    float shadowStrength = 0.85f;
    float shadowBias = 0.0012f;
    int shadowResolution = 2048;
    float shadowDistance = 50.0f;

    // Volumetrics
    bool volumetric = false;
    float volumetricScattering = 0.2f;
    float volumetricIntensity = 1.0f;

    // Light Channels / Layer
    uint32_t lightLayer = 1;
};

struct PrimitiveParams {
    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;
    float size = 1.0f;
    float radius = 0.5f;
    float radius2 = 0.2f;      // topRadius for Cone, minorRadius for Torus
    int segmentsX = 16;
    int segmentsY = 16;
    int segmentsZ = 16;
    int radialSegments = 24;
    int heightSegments = 1;
    int rings = 24;
    int sectors = 24;
    int subdivisions = 2;      // for Icosphere
    int sides = 4;             // for Pyramid / Prism
    bool capTop = true;
    bool capBottom = true;
};

enum class PrimitiveType {
    Cube,
    Plane,
    Box,
    Sphere,
    UVSphere,
    Icosphere,
    Cylinder,
    Cone,
    Capsule,
    Torus,
    Circle,
    Disc,
    Quad,
    Triangle,
    Pyramid,
    Prism,
    ImportedMesh,
    Empty,
    // Lights
    DirectionalLight,
    PointLight,
    SpotLight,
    AreaLight,
    SkyLight,
    Camera
};

inline const char* GetPrimitiveTypeName(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Cube:             return "Cube";
        case PrimitiveType::Plane:            return "Plane";
        case PrimitiveType::Box:              return "Box";
        case PrimitiveType::Sphere:           return "Sphere";
        case PrimitiveType::UVSphere:         return "UV Sphere";
        case PrimitiveType::Icosphere:        return "Icosphere";
        case PrimitiveType::Cylinder:         return "Cylinder";
        case PrimitiveType::Cone:             return "Cone";
        case PrimitiveType::Capsule:          return "Capsule";
        case PrimitiveType::Torus:            return "Torus";
        case PrimitiveType::Circle:           return "Circle";
        case PrimitiveType::Disc:             return "Disc";
        case PrimitiveType::Quad:             return "Quad";
        case PrimitiveType::Triangle:         return "Triangle";
        case PrimitiveType::Pyramid:          return "Pyramid";
        case PrimitiveType::Prism:            return "Prism";
        case PrimitiveType::ImportedMesh:     return "Mesh";
        case PrimitiveType::Empty:            return "Empty Actor";
        case PrimitiveType::DirectionalLight: return "Directional Light";
        case PrimitiveType::PointLight:       return "Point Light";
        case PrimitiveType::SpotLight:        return "Spot Light";
        case PrimitiveType::AreaLight:        return "Area Light";
        case PrimitiveType::SkyLight:         return "Sky Light";
        case PrimitiveType::Camera:           return "Camera";
        default:                              return "Object";
    }
}

inline bool IsLightPrimitive(PrimitiveType type) {
    return type == PrimitiveType::DirectionalLight ||
           type == PrimitiveType::PointLight ||
           type == PrimitiveType::SpotLight ||
           type == PrimitiveType::AreaLight ||
           type == PrimitiveType::SkyLight;
}

inline bool IsCameraPrimitive(PrimitiveType type) {
    return type == PrimitiveType::Camera;
}

struct CameraComponent {
    bool isOrthographic = false;
    float fov = 60.0f;             // Vertical FOV in degrees
    float orthoSize = 5.0f;        // Orthographic half-height
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;
    float aspectRatio = 16.0f / 9.0f;
};

enum class Mobility {
    Static,
    Stationary,
    Movable
};

struct GameObject;

// ============================================================================
// GameObjectTransform
// Supports dev.md requirements:
//   <GameObject>.Transform.WorldLocation(new Vector3(0, 0, 0));
//   <GameObject>.Transform.WorldRotation(new Vector3(0, 0, 0));
//   <GameObject>.Transform.WorldScale(new Vector3(0, 0, 0));
//   <GameObject>.Transform.RelativeLocation(new Vector3(0, 0, 0));
//   <GameObject>.Transform.RelativeRotation(new Vector3(0, 0, 0));
//   <GameObject>.Transform.RelativeScale(new Vector3(0, 0, 0));
// ============================================================================
struct GameObjectTransform {
    struct AxisProxy {
        enum class TargetType {
            WorldLocation,
            RelativeLocation,
            WorldRotation,
            RelativeRotation,
            WorldScale,
            RelativeScale
        };

        GameObjectTransform* parent = nullptr;
        TargetType target = TargetType::WorldLocation;

        AxisProxy() = default;
        AxisProxy(GameObjectTransform* p, TargetType t) : parent(p), target(t) {}

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

    GameObject* gameObject = nullptr;

    AxisProxy WorldLocation{this, AxisProxy::TargetType::WorldLocation};
    AxisProxy RelativeLocation{this, AxisProxy::TargetType::RelativeLocation};
    AxisProxy WorldRotation{this, AxisProxy::TargetType::WorldRotation};
    AxisProxy RelativeRotation{this, AxisProxy::TargetType::RelativeRotation};
    AxisProxy WorldScale{this, AxisProxy::TargetType::WorldScale};
    AxisProxy RelativeScale{this, AxisProxy::TargetType::RelativeScale};

    AxisProxy WorldPosition{this, AxisProxy::TargetType::WorldLocation};
    AxisProxy RelativePosition{this, AxisProxy::TargetType::RelativeLocation};

    GameObjectTransform() : GameObjectTransform(nullptr) {}
    GameObjectTransform(GameObject* go) : gameObject(go) {
        InitProxies();
    }
    GameObjectTransform(const GameObjectTransform& other) : gameObject(other.gameObject) {
        InitProxies();
    }
    GameObjectTransform& operator=(const GameObjectTransform& other) {
        if (this != &other) {
            gameObject = other.gameObject;
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

    glm::vec3 GetWorldLocationInternal() const;
    void SetWorldLocationInternal(const glm::vec3& v);
    glm::vec3 GetRelativeLocationInternal() const;
    void SetRelativeLocationInternal(const glm::vec3& v);
    glm::vec3 GetWorldRotationInternal() const;
    void SetWorldRotationInternal(const glm::vec3& v);
    glm::vec3 GetRelativeRotationInternal() const;
    void SetRelativeRotationInternal(const glm::vec3& v);
    glm::vec3 GetWorldScaleInternal() const;
    void SetWorldScaleInternal(const glm::vec3& v);
    glm::vec3 GetRelativeScaleInternal() const;
    void SetRelativeScaleInternal(const glm::vec3& v);

    // World Translate (dev.md: <GameObject>.Transform.WorldTranslate(0, 0, 0);)
    // 1. By delta (x, y, z) / (delta) / (new Vector3(...))
    void WorldTranslate(const glm::vec3& delta);
    void WorldTranslate(float x, float y, float z) { WorldTranslate(glm::vec3(x, y, z)); }
    void WorldTranslate(const glm::vec3* delta) { if (delta) { WorldTranslate(*delta); delete delta; } }

    // 2. From exact point to another location in world space:
    // Moves object such that point 'fromPoint' moves to 'toLocation' (delta = toLocation - fromPoint)
    void WorldTranslate(const glm::vec3& fromPoint, const glm::vec3& toLocation);
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
    void WorldTranslateTo(const glm::vec3& targetLocation) { WorldLocation(targetLocation); }
    void WorldTranslateTo(float x, float y, float z) { WorldLocation(x, y, z); }
    void WorldTranslateTo(const glm::vec3* targetLocation) { WorldLocation(targetLocation); }

    // Relative Translate (dev.md: <GameObject>.Transform.RelativeTranslate(0, 0, 0);)
    // 1. By delta (x, y, z) / (delta) / (new Vector3(...))
    void RelativeTranslate(const glm::vec3& delta);
    void RelativeTranslate(float x, float y, float z) { RelativeTranslate(glm::vec3(x, y, z)); }
    void RelativeTranslate(const glm::vec3* delta) { if (delta) { RelativeTranslate(*delta); delete delta; } }

    // 2. From exact point to another location in relative space
    void RelativeTranslate(const glm::vec3& fromPoint, const glm::vec3& toLocation);
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
    void RelativeTranslateTo(const glm::vec3& targetLocation) { RelativeLocation(targetLocation); }
    void RelativeTranslateTo(float x, float y, float z) { RelativeLocation(x, y, z); }
    void RelativeTranslateTo(const glm::vec3* targetLocation) { RelativeLocation(targetLocation); }

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

struct GameObject {
    int id = 0;
    std::string name = "Object";
    PrimitiveType type = PrimitiveType::Cube;
    Mobility mobility = Mobility::Movable;

    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f}; // Euler angles in degrees
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    glm::vec3 color{0.55f, 0.55f, 0.55f}; // Default gray

    // Surface Textures (from dev.md)
    std::string baseColorTexture = "";
    std::string normalTexture = "";
    std::string roughnessTexture = "";
    std::string metallicTexture = "";
    std::string aoTexture = "";
    std::string emissionTexture = "";
    std::string opacityTexture = "";

    // PBR Material properties
    std::string materialName = "Default_Material";
    float metallic = 0.0f;
    float roughness = 0.5f;
    float normalStrength = 1.0f;
    float specular = 0.5f;
    glm::vec3 emissiveColor{0.0f, 0.0f, 0.0f};
    float emissiveIntensity = 0.0f;
    float opacity = 1.0f;
    float opacityMaskClipValue = 0.333f;
    int shadingModel = 0; // 0 = Default Lit (PBR), 1 = Unlit, 2 = Subsurface
    int blendMode = 0;    // 0 = Opaque, 1 = Masked, 2 = Translucent
    bool twoSided = false;
    bool castShadows = true;
    bool receiveShadows = true;
    bool meshClusterCulling = false; // dev.md: Mesh Cluster Culling (OFF by default)
    glm::vec2 uvScale{1.0f, 1.0f};
    bool normalMapYFlip = false;     // Invert normal green/Y channel (OpenGL/Blender convention)
    int metallicChannel = 0;         // 0=R, 1=G, 2=B, 3=A
    int roughnessChannel = 1;        // 0=R, 1=G, 2=B, 3=A
    int aoChannel = 0;               // 0=R, 1=G, 2=B, 3=A
    int materialDebugMode = 0;       // 0=Final PBR, 1..10 diagnostic modes

    bool visible = true;
    bool autoRotate = false;
    glm::vec3 autoRotateSpeed{0.0f, 45.0f, 0.0f}; // deg/sec

    std::string meshFilePath = "";
    bool isImportedMesh = false;
    int submeshIndex = -1;           // -1 for root/group or single-mesh, >= 0 for submesh index

    // Hierarchy (parent-child)
    int parentId = -1;               // -1 = root
    std::vector<int> childIds;       // ordered child IDs

    // Scene & Transform accessors (dev.md)
    Scene* scene = nullptr;
    GameObjectTransform Transform{this};
    GameObjectTransform& transform = Transform;

    // Light actor proxy: when isLight==true this object represents a PointLight
    // Light actor proxy: when isLight==true this object represents a Light
    // and uses 'lightId' to index into scene.pointLights (if local light)
    bool isLight = false;
    int  lightId  = -1;              // index into Scene::pointLights
    LightComponent light;
    bool isCamera = false;
    CameraComponent camera;
    PrimitiveParams params;

    PrimitiveMesh mesh;

    // Attached Behaviours (dev.md Section 1, 2)
    std::vector<std::shared_ptr<EunoiaBehaviour>> behaviours;

    GameObject() : Transform(this) {}

    GameObject(const GameObject& other) : Transform(this) {
        CopyFrom(other);
    }

    GameObject& operator=(const GameObject& other) {
        if (this != &other) {
            CopyFrom(other);
        }
        return *this;
    }

    GameObject(GameObject&& other) noexcept : Transform(this) {
        CopyFrom(other);
    }

    GameObject& operator=(GameObject&& other) noexcept {
        if (this != &other) {
            CopyFrom(other);
        }
        return *this;
    }

    void CopyFrom(const GameObject& other) {
        id = other.id;
        name = other.name;
        type = other.type;
        mobility = other.mobility;
        position = other.position;
        rotation = other.rotation;
        scale = other.scale;
        color = other.color;
        baseColorTexture = other.baseColorTexture;
        normalTexture = other.normalTexture;
        roughnessTexture = other.roughnessTexture;
        metallicTexture = other.metallicTexture;
        aoTexture = other.aoTexture;
        emissionTexture = other.emissionTexture;
        opacityTexture = other.opacityTexture;
        materialName = other.materialName;
        metallic = other.metallic;
        roughness = other.roughness;
        normalStrength = other.normalStrength;
        specular = other.specular;
        emissiveColor = other.emissiveColor;
        emissiveIntensity = other.emissiveIntensity;
        opacity = other.opacity;
        opacityMaskClipValue = other.opacityMaskClipValue;
        shadingModel = other.shadingModel;
        blendMode = other.blendMode;
        twoSided = other.twoSided;
        castShadows = other.castShadows;
        receiveShadows = other.receiveShadows;
        meshClusterCulling = other.meshClusterCulling;
        uvScale = other.uvScale;
        normalMapYFlip = other.normalMapYFlip;
        metallicChannel = other.metallicChannel;
        roughnessChannel = other.roughnessChannel;
        aoChannel = other.aoChannel;
        materialDebugMode = other.materialDebugMode;
        visible = other.visible;
        autoRotate = other.autoRotate;
        autoRotateSpeed = other.autoRotateSpeed;
        meshFilePath = other.meshFilePath;
        isImportedMesh = other.isImportedMesh;
        submeshIndex = other.submeshIndex;
        parentId = other.parentId;
        childIds = other.childIds;
        isLight = other.isLight;
        lightId = other.lightId;
        light = other.light;
        isCamera = other.isCamera;
        camera = other.camera;
        params = other.params;
        mesh = other.mesh;
        scene = other.scene;
        Transform.gameObject = this;
        behaviours.clear();
        for (const auto& b : other.behaviours) {
            if (b) {
                auto clone = b->Clone();
                clone->SetOwner(this);
                behaviours.push_back(std::move(clone));
            }
        }
    }

    void AddBehaviour(std::shared_ptr<EunoiaBehaviour> b) {
        if (!b) return;
        b->SetOwner(this);
        behaviours.push_back(b);
        b->OnCreate();
    }

    bool RemoveBehaviour(size_t index) {
        if (index >= behaviours.size()) return false;
        if (behaviours[index]) {
            behaviours[index]->OnDisable();
            behaviours[index]->OnDestroy();
        }
        behaviours.erase(behaviours.begin() + index);
        return true;
    }

    bool ReorderBehaviour(size_t fromIdx, size_t toIdx) {
        if (fromIdx >= behaviours.size() || toIdx >= behaviours.size() || fromIdx == toIdx) return false;
        std::swap(behaviours[fromIdx], behaviours[toIdx]);
        return true;
    }

    template<typename T>
    T* GetBehaviour() {
        for (auto& b : behaviours) {
            if (auto casted = dynamic_cast<T*>(b.get())) {
                return casted;
            }
        }
        return nullptr;
    }

    GameObject(int objId, const std::string& objName, PrimitiveType objType, glm::vec3 pos, glm::vec3 col = {0.55f, 0.55f, 0.55f})
        : id(objId), name(objName), type(objType), position(pos), color(col), Transform(this) {
        if (type == PrimitiveType::Camera) {
            isCamera = true;
        }
        if (IsLightPrimitive(type)) {
            isLight = true;
            switch (type) {
                case PrimitiveType::DirectionalLight: light.type = LightType::Directional; light.castShadows = true; break;
                case PrimitiveType::PointLight:       light.type = LightType::Point;       light.castShadows = true; break;
                case PrimitiveType::SpotLight:        light.type = LightType::Spot;        light.castShadows = true; break;
                case PrimitiveType::AreaLight:        light.type = LightType::Area;        light.castShadows = true; break;
                case PrimitiveType::SkyLight:         light.type = LightType::Sky;         light.castShadows = false; light.intensity = 1.0f; break;
                default: break;
            }
            light.color = col;
        }

        // Set default primitive parameters
        if (type == PrimitiveType::Plane) { params.width = 2.5f; params.depth = 2.5f; params.segmentsX = 16; params.segmentsZ = 16; }
        else if (type == PrimitiveType::Box) { params.width = 1.0f; params.height = 1.0f; params.depth = 1.0f; params.segmentsX = 1; params.segmentsY = 1; params.segmentsZ = 1; }
        else if (type == PrimitiveType::Sphere || type == PrimitiveType::UVSphere) { params.radius = 0.6f; params.rings = 24; params.sectors = 24; }
        else if (type == PrimitiveType::Icosphere) { params.radius = 0.6f; params.subdivisions = 2; }
        else if (type == PrimitiveType::Cylinder) { params.radius = 0.5f; params.height = 1.2f; params.radialSegments = 24; params.heightSegments = 1; }
        else if (type == PrimitiveType::Cone) { params.radius = 0.5f; params.radius2 = 0.0f; params.height = 1.2f; params.radialSegments = 24; params.heightSegments = 1; }
        else if (type == PrimitiveType::Capsule) { params.radius = 0.35f; params.height = 0.8f; params.radialSegments = 20; params.heightSegments = 2; }
        else if (type == PrimitiveType::Torus) { params.radius = 0.6f; params.radius2 = 0.22f; params.radialSegments = 24; params.segmentsY = 16; }
        else if (type == PrimitiveType::Circle || type == PrimitiveType::Disc) { params.radius = 0.6f; params.radialSegments = 32; }
        else if (type == PrimitiveType::Quad) { params.width = 1.5f; params.height = 1.5f; }
        else if (type == PrimitiveType::Triangle) { params.width = 1.2f; params.height = 1.2f; }
        else if (type == PrimitiveType::Pyramid) { params.width = 1.0f; params.depth = 1.0f; params.height = 1.2f; params.sides = 4; }
        else if (type == PrimitiveType::Prism) { params.radius = 0.6f; params.height = 1.2f; params.sides = 3; }

        RebuildMesh();
    }

    void RebuildMesh() {
        switch (type) {
            case PrimitiveType::Cube:
                mesh = GeometryBuilder::CreateCube(params.size, params.segmentsX);
                break;
            case PrimitiveType::Plane:
                mesh = GeometryBuilder::CreatePlane(params.width, params.depth, params.segmentsX, params.segmentsZ);
                break;
            case PrimitiveType::Box:
                mesh = GeometryBuilder::CreateBox(params.width, params.height, params.depth, params.segmentsX, params.segmentsY, params.segmentsZ);
                break;
            case PrimitiveType::Sphere:
                mesh = GeometryBuilder::CreateSphere(params.radius, params.rings, params.sectors);
                break;
            case PrimitiveType::UVSphere:
                mesh = GeometryBuilder::CreateUVSphere(params.radius, params.sectors, params.rings);
                break;
            case PrimitiveType::Icosphere:
                mesh = GeometryBuilder::CreateIcosphere(params.radius, params.subdivisions);
                break;
            case PrimitiveType::Cylinder:
                mesh = GeometryBuilder::CreateCylinder(params.radius, params.height, params.radialSegments, params.heightSegments, params.capTop, params.capBottom);
                break;
            case PrimitiveType::Cone:
                mesh = GeometryBuilder::CreateCone(params.radius, params.radius2, params.height, params.radialSegments, params.heightSegments, params.capBottom, params.capTop);
                break;
            case PrimitiveType::Capsule:
                mesh = GeometryBuilder::CreateCapsule(params.radius, params.height, params.radialSegments, params.heightSegments, std::max(2, params.rings / 3));
                break;
            case PrimitiveType::Torus:
                mesh = GeometryBuilder::CreateTorus(params.radius, params.radius2 > 0.001f ? params.radius2 : 0.2f, params.radialSegments, params.segmentsY);
                break;
            case PrimitiveType::Circle:
                mesh = GeometryBuilder::CreateCircle(params.radius, params.radialSegments);
                break;
            case PrimitiveType::Disc:
                mesh = GeometryBuilder::CreateDisc(params.radius, params.radialSegments);
                break;
            case PrimitiveType::Quad:
                mesh = GeometryBuilder::CreateQuad(params.width, params.height);
                break;
            case PrimitiveType::Triangle:
                mesh = GeometryBuilder::CreateTriangle(params.width, params.height);
                break;
            case PrimitiveType::Pyramid:
                mesh = GeometryBuilder::CreatePyramid(params.width, params.depth, params.height, params.sides);
                break;
            case PrimitiveType::Prism:
                mesh = GeometryBuilder::CreatePrism(params.radius, params.height, params.sides);
                break;
            case PrimitiveType::ImportedMesh:
            case PrimitiveType::Empty:
            default:
                if (IsLightPrimitive(type) || IsCameraPrimitive(type) || type == PrimitiveType::Empty || type == PrimitiveType::ImportedMesh) {
                    mesh.vertices.clear();
                    mesh.indices.clear();
                }
                break;
        }
    }

    glm::mat4 GetLocalMatrix() const {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, position);
        model = glm::rotate(model, glm::radians(rotation.x), glm::vec3(1, 0, 0));
        model = glm::rotate(model, glm::radians(rotation.y), glm::vec3(0, 1, 0));
        model = glm::rotate(model, glm::radians(rotation.z), glm::vec3(0, 0, 1));
        model = glm::scale(model, scale);
        return model;
    }

    glm::mat4 GetModelMatrix() const {
        return GetLocalMatrix();
    }

    // Direct Transform Translation helpers (dev.md)
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

    void Update(float dt) {
        if (mobility == Mobility::Movable && autoRotate) {
            rotation += autoRotateSpeed * dt;
            if (rotation.x > 360.0f) rotation.x -= 360.0f;
            if (rotation.y > 360.0f) rotation.y -= 360.0f;
            if (rotation.z > 360.0f) rotation.z -= 360.0f;
        }
    }
};

// ============================================================================
// EunoiaBehaviour::GetBehaviour / HasBehaviour inline implementations
// (Defined here where GameObject is a complete type)
// ============================================================================
template<typename T>
inline T* EunoiaBehaviour::GetBehaviour() const {
    if (!m_owner) return nullptr;
    return m_owner->template GetBehaviour<T>();
}

template<typename T>
inline bool EunoiaBehaviour::HasBehaviour() const {
    return GetBehaviour<T>() != nullptr;
}

