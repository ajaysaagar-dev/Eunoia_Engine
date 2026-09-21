#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Geometry.h"
#include "EunoiaBehaviour.h"

enum class LightType {
    Directional,
    Point,
    Spot,
    Area,
    Sky,
    Ambient,
    Hemisphere,
    Tube,
    Disc
};

inline const char* GetLightTypeName(LightType type) {
    switch (type) {
        case LightType::Directional: return "Directional Light";
        case LightType::Point:       return "Point Light";
        case LightType::Spot:        return "Spot Light";
        case LightType::Area:        return "Area Light";
        case LightType::Sky:         return "Sky Light";
        case LightType::Ambient:     return "Ambient Light";
        case LightType::Hemisphere:  return "Hemisphere Light";
        case LightType::Tube:        return "Tube Light";
        case LightType::Disc:        return "Disc Light";
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
    AmbientLight,
    HemisphereLight,
    TubeLight,
    DiscLight
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
        case PrimitiveType::AmbientLight:     return "Ambient Light";
        case PrimitiveType::HemisphereLight:  return "Hemisphere Light";
        case PrimitiveType::TubeLight:        return "Tube Light";
        case PrimitiveType::DiscLight:        return "Disc Light";
        default:                              return "Object";
    }
}

inline bool IsLightPrimitive(PrimitiveType type) {
    return type == PrimitiveType::DirectionalLight ||
           type == PrimitiveType::PointLight ||
           type == PrimitiveType::SpotLight ||
           type == PrimitiveType::AreaLight ||
           type == PrimitiveType::SkyLight ||
           type == PrimitiveType::AmbientLight ||
           type == PrimitiveType::HemisphereLight ||
           type == PrimitiveType::TubeLight ||
           type == PrimitiveType::DiscLight;
}

enum class Mobility {
    Static,
    Stationary,
    Movable
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
    glm::vec2 uvScale{1.0f, 1.0f};

    bool visible = true;
    bool autoRotate = false;
    glm::vec3 autoRotateSpeed{0.0f, 45.0f, 0.0f}; // deg/sec

    std::string meshFilePath = "";
    bool isImportedMesh = false;
    int submeshIndex = -1;           // -1 for root/group or single-mesh, >= 0 for submesh index

    // Hierarchy (parent-child)
    int parentId = -1;               // -1 = root
    std::vector<int> childIds;       // ordered child IDs

    // Light actor proxy: when isLight==true this object represents a PointLight
    // Light actor proxy: when isLight==true this object represents a Light
    // and uses 'lightId' to index into scene.pointLights (if local light)
    bool isLight = false;
    int  lightId  = -1;              // index into Scene::pointLights
    LightComponent light;
    PrimitiveParams params;

    PrimitiveMesh mesh;

    // Attached Behaviours (dev.md Section 1, 2)
    std::vector<std::shared_ptr<EunoiaBehaviour>> behaviours;

    GameObject() = default;

    GameObject(const GameObject& other) {
        CopyFrom(other);
    }

    GameObject& operator=(const GameObject& other) {
        if (this != &other) {
            CopyFrom(other);
        }
        return *this;
    }

    GameObject(GameObject&&) noexcept = default;
    GameObject& operator=(GameObject&&) noexcept = default;

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
        uvScale = other.uvScale;
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
        params = other.params;
        mesh = other.mesh;

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
        : id(objId), name(objName), type(objType), position(pos), color(col) {
        if (IsLightPrimitive(type)) {
            isLight = true;
            switch (type) {
                case PrimitiveType::DirectionalLight: light.type = LightType::Directional; light.castShadows = true; break;
                case PrimitiveType::PointLight:       light.type = LightType::Point;       light.castShadows = true; break;
                case PrimitiveType::SpotLight:        light.type = LightType::Spot;        light.castShadows = true; break;
                case PrimitiveType::AreaLight:        light.type = LightType::Area;        light.castShadows = false; break;
                case PrimitiveType::SkyLight:         light.type = LightType::Sky;         light.castShadows = false; break;
                case PrimitiveType::AmbientLight:     light.type = LightType::Ambient;     light.castShadows = false; break;
                case PrimitiveType::HemisphereLight:  light.type = LightType::Hemisphere;  light.castShadows = false; break;
                case PrimitiveType::TubeLight:        light.type = LightType::Tube;        light.castShadows = false; break;
                case PrimitiveType::DiscLight:        light.type = LightType::Disc;        light.castShadows = false; break;
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
                if (IsLightPrimitive(type) || type == PrimitiveType::Empty || type == PrimitiveType::ImportedMesh) {
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

