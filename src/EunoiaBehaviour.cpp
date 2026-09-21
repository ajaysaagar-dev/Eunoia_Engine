#include "EunoiaBehaviour.h"
#include "BehaviourRegistry.h"
#include "GameObject.h"
#include "Scene.h"
#include "InputSystem.h"
#include "Camera.h"
#include <cmath>

// ============================================================================
// RespectiveObjectAccessor Implementations
// ============================================================================

LightComponent* RespectiveObjectAccessor::Light(void* objRef) const {
    if (!objRef) return nullptr;
    return reinterpret_cast<LightComponent*>(objRef);
}

PrimitiveMesh* RespectiveObjectAccessor::Mesh(void* objRef) const {
    if (!objRef) return nullptr;
    return reinterpret_cast<PrimitiveMesh*>(objRef);
}

GameObject* RespectiveObjectAccessor::Entity(void* objRef) const {
    if (!objRef) return nullptr;
    return reinterpret_cast<GameObject*>(objRef);
}

GameObject* RespectiveObjectAccessor::Actor(void* objRef) const {
    return Entity(objRef);
}

GameObject* RespectiveObjectAccessor::Shape(void* objRef) const {
    return Entity(objRef);
}

GameObject* RespectiveObjectAccessor::Transform(void* objRef) const {
    return Entity(objRef);
}

OrbitCamera* RespectiveObjectAccessor::Camera(void* objRef) const {
    if (!objRef) return nullptr;
    return reinterpret_cast<OrbitCamera*>(objRef);
}

// ============================================================================
// BehaviourTransformProperty Implementations
// ============================================================================

glm::vec3 BehaviourTransformProperty::LocalSpace() const {
    if (!behaviour) return glm::vec3(0.0f);
    GameObject* owner = behaviour->GetOwner();
    if (!owner) return glm::vec3(0.0f);
    switch (type) {
        case Type::Location: return owner->position;
        case Type::Rotation: return owner->rotation;
        case Type::Scale:    return owner->scale;
    }
    return glm::vec3(0.0f);
}

void BehaviourTransformProperty::LocalSpace(const glm::vec3& v) {
    if (!behaviour) return;
    GameObject* owner = behaviour->GetOwner();
    if (!owner) return;
    switch (type) {
        case Type::Location: owner->position = v; break;
        case Type::Rotation: owner->rotation = v; break;
        case Type::Scale:    owner->scale = v; break;
    }
}

glm::vec3 BehaviourTransformProperty::WorldSpace() const {
    if (!behaviour) return glm::vec3(0.0f);
    GameObject* owner = behaviour->GetOwner();
    if (!owner) return glm::vec3(0.0f);
    Scene* scene = behaviour->GetScene();
    if (!scene || owner->parentId == -1) {
        return LocalSpace();
    }
    glm::mat4 worldMat = scene->GetWorldMatrix(*owner);
    switch (type) {
        case Type::Location:
            return glm::vec3(worldMat[3]);
        case Type::Rotation: {
            glm::vec3 c0(worldMat[0]), c1(worldMat[1]), c2(worldMat[2]);
            float sx = glm::length(c0);
            float sy = glm::length(c1);
            float sz = glm::length(c2);
            if (sx > 1e-6f && sy > 1e-6f && sz > 1e-6f) {
                glm::mat3 rotMat(c0 / sx, c1 / sy, c2 / sz);
                float pitch = glm::degrees(std::asin(glm::clamp(-rotMat[1][2], -1.0f, 1.0f)));
                float yaw = glm::degrees(std::atan2(rotMat[0][2], rotMat[2][2]));
                float roll = glm::degrees(std::atan2(rotMat[1][0], rotMat[1][1]));
                return glm::vec3(pitch, yaw, roll);
            }
            return owner->rotation;
        }
        case Type::Scale: {
            return glm::vec3(glm::length(glm::vec3(worldMat[0])),
                             glm::length(glm::vec3(worldMat[1])),
                             glm::length(glm::vec3(worldMat[2])));
        }
    }
    return LocalSpace();
}

void BehaviourTransformProperty::WorldSpace(const glm::vec3& v) {
    if (!behaviour) return;
    GameObject* owner = behaviour->GetOwner();
    if (!owner) return;
    Scene* scene = behaviour->GetScene();
    if (!scene || owner->parentId == -1) {
        LocalSpace(v);
        return;
    }
    const GameObject* parent = scene->FindObjectConst(owner->parentId);
    if (!parent) {
        LocalSpace(v);
        return;
    }
    glm::mat4 pMat = scene->GetWorldMatrix(*parent);
    glm::mat4 invP = glm::inverse(pMat);
    switch (type) {
        case Type::Location: {
            owner->position = glm::vec3(invP * glm::vec4(v, 1.0f));
            break;
        }
        case Type::Rotation: {
            owner->rotation = v - parent->rotation;
            break;
        }
        case Type::Scale: {
            glm::vec3 pScale(glm::length(glm::vec3(pMat[0])),
                             glm::length(glm::vec3(pMat[1])),
                             glm::length(glm::vec3(pMat[2])));
            if (pScale.x > 1e-6f && pScale.y > 1e-6f && pScale.z > 1e-6f) {
                owner->scale = v / pScale;
            } else {
                owner->scale = v;
            }
            break;
        }
    }
}

// ============================================================================
// EunoiaBehaviour Property Registration Implementations
// ============================================================================

void EunoiaBehaviour::RegisterProperty(const std::string& name, bool* val, const std::string& category) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::Bool;
    prop.dataPtr = val;
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterProperty(const std::string& name, int* val, const std::string& category, int min, int max) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::Int;
    prop.dataPtr = val;
    if (min != max) {
        prop.hasRange = true;
        prop.minVal = (float)min;
        prop.maxVal = (float)max;
    }
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterProperty(const std::string& name, int64_t* val, const std::string& category) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::Int64;
    prop.dataPtr = val;
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterProperty(const std::string& name, float* val, const std::string& category, float min, float max) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::Float;
    prop.dataPtr = val;
    if (min != max) {
        prop.hasRange = true;
        prop.minVal = min;
        prop.maxVal = max;
    }
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterProperty(const std::string& name, double* val, const std::string& category) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::Double;
    prop.dataPtr = val;
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterProperty(const std::string& name, std::string* val, const std::string& category) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::String;
    prop.dataPtr = val;
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterProperty(const std::string& name, glm::vec2* val, const std::string& category) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::Vec2;
    prop.dataPtr = val;
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterProperty(const std::string& name, glm::vec3* val, const std::string& category) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::Vec3;
    prop.dataPtr = val;
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterProperty(const std::string& name, glm::vec4* val, const std::string& category) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::Vec4;
    prop.dataPtr = val;
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterColor(const std::string& name, glm::vec3* val, const std::string& category) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::Color3;
    prop.dataPtr = val;
    m_properties.push_back(prop);
}

void EunoiaBehaviour::RegisterReference(const std::string& name, void* ptrAddr, ObjectRefType refType, const std::string& category) {
    BehaviourProperty prop;
    prop.name = name;
    prop.displayName = name;
    prop.category = category;
    prop.type = BehaviourPropertyType::ObjectRef;
    prop.refType = refType;
    prop.dataPtr = ptrAddr;
    prop.targetId = -1;
    m_properties.push_back(prop);
}

void EunoiaBehaviour::ResolveReferences(Scene& scene) {
    m_scene = &scene;
    for (auto& prop : m_properties) {
        if (prop.type != BehaviourPropertyType::ObjectRef || !prop.dataPtr) continue;

        if (prop.targetId == -1) {
            *reinterpret_cast<void**>(prop.dataPtr) = nullptr;
            prop.isMissing = false;
            continue;
        }

        GameObject* targetObj = scene.FindObject(prop.targetId);
        if (!targetObj) {
            *reinterpret_cast<void**>(prop.dataPtr) = nullptr;
            prop.isMissing = true;
            continue;
        }

        prop.isMissing = false;
        switch (prop.refType) {
            case ObjectRefType::Actor:
                *reinterpret_cast<GameObject**>(prop.dataPtr) = targetObj;
                break;
            case ObjectRefType::Light:
                *reinterpret_cast<LightComponent**>(prop.dataPtr) = targetObj->isLight ? &targetObj->light : nullptr;
                if (!targetObj->isLight) prop.isMissing = true;
                break;
            case ObjectRefType::Mesh:
                *reinterpret_cast<PrimitiveMesh**>(prop.dataPtr) = &targetObj->mesh;
                break;
            case ObjectRefType::Camera:
                // Cameras in Eunoia are view-level or component
                *reinterpret_cast<void**>(prop.dataPtr) = nullptr;
                break;
            default:
                *reinterpret_cast<void**>(prop.dataPtr) = targetObj;
                break;
        }
    }
}

// ============================================================================
// Built-in Behaviour Logic
// ============================================================================

void RotatorBehaviour::Update(float deltaTime) {
    if (!m_owner) return;
    glm::vec3 normAxis = glm::length(rotationAxis) > 0.001f ? glm::normalize(rotationAxis) : glm::vec3(0, 1, 0);
    m_owner->rotation += normAxis * (rotationSpeed * deltaTime);
    if (m_owner->rotation.x >= 360.0f) m_owner->rotation.x -= 360.0f;
    if (m_owner->rotation.y >= 360.0f) m_owner->rotation.y -= 360.0f;
    if (m_owner->rotation.z >= 360.0f) m_owner->rotation.z -= 360.0f;
}

void LightFlickerBehaviour::Update(float deltaTime) {
    m_timeAccum += deltaTime * flickerFrequency;
    float noise = (std::sin(m_timeAccum) + std::sin(m_timeAccum * 2.3f) + std::cos(m_timeAccum * 3.7f)) / 3.0f; // [-1, 1]
    float norm = (noise + 1.0f) * 0.5f; // [0, 1]
    float currentIntensity = minIntensity + norm * (maxIntensity - minIntensity);

    if (targetLight) {
        targetLight->intensity = currentIntensity;
    } else if (m_owner && m_owner->isLight) {
        m_owner->light.intensity = currentIntensity;
    }
}

void DoorController::Start() {
    auto* light = GetRespectiveObject.Light(WarningLight);
    if (light) {
        light->color = Locked ? glm::vec3(1.0f, 0.1f, 0.1f) : glm::vec3(0.1f, 1.0f, 0.1f);
    }
    AddEngineLog("LogBehaviour", "DoorController::Start initialized on " + (m_owner ? m_owner->name : "Unknown"), 0);
}

void DoorController::Update(float deltaTime) {
    if (!m_owner) return;
    float targetAngle = Locked ? 0.0f : 90.0f;
    m_currentAngle += (targetAngle - m_currentAngle) * std::min(1.0f, OpenSpeed * deltaTime);
    m_owner->rotation.y = m_currentAngle;

    auto* light = GetRespectiveObject.Light(WarningLight);
    if (light) {
        light->color = Locked ? glm::vec3(1.0f, 0.1f, 0.1f) : glm::vec3(0.1f, 1.0f, 0.1f);
    }
}

void PlayerController::Start() {
    AddEngineLog("LogBehaviour", "PlayerController::Start — Player \"" + PlayerName + "\" entered level.", 0);
}

void PlayerController::Update(float deltaTime) {
    if (!m_owner) return;
    auto& input = InputSystem::Get();

    glm::vec3 moveDir(0.0f);
    if (input.IsKeyDown(Key::W) || input.IsKeyDown(Key::Up))    moveDir.z -= 1.0f;
    if (input.IsKeyDown(Key::S) || input.IsKeyDown(Key::Down))  moveDir.z += 1.0f;
    if (input.IsKeyDown(Key::A) || input.IsKeyDown(Key::Left))  moveDir.x -= 1.0f;
    if (input.IsKeyDown(Key::D) || input.IsKeyDown(Key::Right)) moveDir.x += 1.0f;

    if (glm::length(moveDir) > 0.001f) {
        moveDir = glm::normalize(moveDir);
        m_owner->position += moveDir * (MoveSpeed * deltaTime);
    }
}

void EnemyController::Start() {
    AddEngineLog("LogBehaviour", "EnemyController::Start — " + EnemyName + " searching for target.", 0);
}

void EnemyController::Update(float deltaTime) {
    if (!m_owner) return;
    GameObject* target = GetRespectiveObject.Actor(TargetActor);
    if (target) {
        glm::vec3 dir = target->position - m_owner->position;
        float dist = glm::length(dir);
        if (dist > 1.5f) {
            dir = glm::normalize(dir);
            m_owner->position += dir * (MoveSpeed * deltaTime);
            float targetYaw = glm::degrees(std::atan2(dir.x, -dir.z));
            m_owner->rotation.y = targetYaw;
        }
    }
}

void DynamicScriptBehaviour::Start() {
    AddEngineLog("LogBehaviour", m_className + "::Start called on " + (GetOwner() ? GetOwner()->name : "Unknown"), 0);
}

void DynamicScriptBehaviour::Update(float deltaTime) {
    if (!m_owner || !m_enabled || !IsActive) return;
    auto& input = InputSystem::Get();

    float currentSpeed = MoveSpeed;
    if (input.IsKeyDown(Key::LShift) || input.IsActionDown("Sprint")) {
        currentSpeed *= 1.75f;
    }

    // Heading vectors from owner rotation (yaw)
    float yawRad = glm::radians(m_owner->rotation.y);
    glm::vec3 forward(std::sin(yawRad), 0.0f, -std::cos(yawRad));
    glm::vec3 right(std::cos(yawRad), 0.0f, std::sin(yawRad));

    glm::vec3 moveDir(0.0f);
    // W: Move Forward
    if (input.IsKeyDown(Key::W) || input.IsKeyDown(Key::Up) || input.IsActionDown("MoveForward")) {
        moveDir += forward;
    }
    // S: Move Backward
    if (input.IsKeyDown(Key::S) || input.IsKeyDown(Key::Down) || input.IsActionDown("MoveBackward")) {
        moveDir -= forward;
    }
    // A: Strafe Left
    if (input.IsKeyDown(Key::A) || input.IsKeyDown(Key::Left) || input.IsActionDown("MoveLeft")) {
        moveDir -= right;
    }
    // D: Strafe Right
    if (input.IsKeyDown(Key::D) || input.IsKeyDown(Key::Right) || input.IsActionDown("MoveRight")) {
        moveDir += right;
    }
    // Space: Ascend / Jump
    if (input.IsKeyDown(Key::Space) || input.IsActionDown("Jump")) {
        moveDir.y += 1.0f;
    }
    // C or LCtrl: Descend / Crouch
    if (input.IsKeyDown(Key::LCtrl) || input.IsKeyDown(Key::C)) {
        moveDir.y -= 1.0f;
    }

    if (glm::length(moveDir) > 0.001f) {
        moveDir = glm::normalize(moveDir);
        m_owner->position += moveDir * (currentSpeed * deltaTime);
        Transform.Location.LocalSpace(m_owner->position);
    }
}

// ============================================================================
// GetComponent<T> Specialisations (dev.md Section 35)
// Implementations live here to avoid circular includes between
// EunoiaBehaviour.h ↔ GameObject.h.
// ============================================================================

template<>
LightComponent* EunoiaBehaviour::GetComponent<LightComponent>() const {
    if (!m_owner || !m_owner->isLight) return nullptr;
    return &(m_owner->light);
}

template<>
PrimitiveMesh* EunoiaBehaviour::GetComponent<PrimitiveMesh>() const {
    if (!m_owner || m_owner->isLight) return nullptr;
    return &(m_owner->mesh);
}

// ============================================================================
// SpawnGameObject / DestroyGameObject implementations (dev.md Section 45)
// ============================================================================

GameObject* EunoiaBehaviour::SpawnGameObject(const std::string& name, const glm::vec3& position) {
    if (!m_scene) {
        AddEngineLog("LogBehaviour", "[Behaviour] SpawnGameObject: no active scene - call only during Play Mode", 1);
        return nullptr;
    }
    GameObject& obj = m_scene->AddObject(PrimitiveType::Empty, position, glm::vec3(0.55f));
    obj.name = name;
    AddEngineLog("LogBehaviour", "[Behaviour] SpawnGameObject: created '" + name + "' (ID " + std::to_string(obj.id) + ")", 0);
    return &obj;
}

void EunoiaBehaviour::DestroyGameObject(int objectId) {
    if (!m_scene) return;
    m_scene->RemoveObject(objectId);
    AddEngineLog("LogBehaviour", "[Behaviour] DestroyGameObject: removed object ID " + std::to_string(objectId), 0);
}
