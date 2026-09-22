#pragma once
#include <EngineScene/EunoiaBehaviour.h>
#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <vector>
#include <cmath>

struct BehaviourClassInfo {
    std::string className;
    std::string displayName;
    std::string description;
    std::string sourceHeader;
    std::string sourceCpp;
    std::function<std::unique_ptr<EunoiaBehaviour>()> factory;
};

// ============================================================================
// Built-in Behaviours (dev.md Section 1, 7, 37)
// ============================================================================

// 1. RotatorBehaviour — Smooth continuous rotation
class RotatorBehaviour : public EunoiaBehaviour {
public:
    float rotationSpeed = 45.0f;
    glm::vec3 rotationAxis = glm::vec3(0.0f, 1.0f, 0.0f);
    bool pingPong = false;

    RotatorBehaviour() {
        m_className = "RotatorBehaviour";
        m_displayName = "Rotator Behaviour";
        RegisterProperties();
    }

    void RegisterProperties() override {
        m_properties.clear();
        RegisterProperty("Rotation Speed", &rotationSpeed, "Movement", -360.0f, 360.0f);
        RegisterProperty("Rotation Axis", &rotationAxis, "Movement");
        RegisterProperty("Ping Pong", &pingPong, "Movement");
    }

    std::unique_ptr<EunoiaBehaviour> Clone() const override {
        auto clone = std::make_unique<RotatorBehaviour>(*this);
        clone->RegisterProperties();
        return clone;
    }

    void Update(float deltaTime) override;
};

// 2. LightFlickerBehaviour — Dynamic lighting pulse/flicker
class LightFlickerBehaviour : public EunoiaBehaviour {
public:
    float minIntensity = 0.5f;
    float maxIntensity = 3.5f;
    float flickerFrequency = 8.0f;
    Light* targetLight = nullptr;

    LightFlickerBehaviour() {
        m_className = "LightFlickerBehaviour";
        m_displayName = "Light Flicker Behaviour";
        RegisterProperties();
    }

    void RegisterProperties() override {
        m_properties.clear();
        RegisterProperty("Min Intensity", &minIntensity, "Lighting", 0.0f, 20.0f);
        RegisterProperty("Max Intensity", &maxIntensity, "Lighting", 0.0f, 20.0f);
        RegisterProperty("Flicker Frequency", &flickerFrequency, "Lighting", 0.1f, 30.0f);
        RegisterReference("Target Light", &targetLight, ObjectRefType::Light, "References");
    }

    std::unique_ptr<EunoiaBehaviour> Clone() const override {
        auto clone = std::make_unique<LightFlickerBehaviour>(*this);
        clone->RegisterProperties();
        return clone;
    }

    void Update(float deltaTime) override;

private:
    float m_timeAccum = 0.0f;
};

// 3. DoorController (dev.md Section 37)
class DoorController : public EunoiaBehaviour {
public:
    float OpenSpeed = 2.5f;
    int RequiredKeys = 1;
    bool Locked = false;

    Light* WarningLight = nullptr;
    MeshRenderer* DoorMesh = nullptr;

    DoorController() {
        m_className = "DoorController";
        m_displayName = "Door Controller";
        RegisterProperties();
    }

    void RegisterProperties() override {
        m_properties.clear();
        RegisterProperty("Open Speed", &OpenSpeed, "Door Settings", 0.1f, 10.0f);
        RegisterProperty("Required Keys", &RequiredKeys, "Door Settings", 0, 10);
        RegisterProperty("Locked", &Locked, "Door Settings");
        RegisterReference("Warning Light", &WarningLight, ObjectRefType::Light, "References");
        RegisterReference("Door Mesh", &DoorMesh, ObjectRefType::Mesh, "References");
    }

    std::unique_ptr<EunoiaBehaviour> Clone() const override {
        auto clone = std::make_unique<DoorController>(*this);
        clone->RegisterProperties();
        return clone;
    }

    void Start() override;
    void Update(float deltaTime) override;

private:
    float m_currentAngle = 0.0f;
};

// 4. PlayerController (dev.md Section 1, 7)
class PlayerController : public EunoiaBehaviour {
public:
    int Health = 100;
    float MoveSpeed = 5.0f;
    bool CanAttack = true;
    std::string PlayerName = "Player";
    Camera* TargetCamera = nullptr;

    PlayerController() {
        m_className = "PlayerController";
        m_displayName = "Player Controller";
        RegisterProperties();
    }

    void RegisterProperties() override {
        m_properties.clear();
        RegisterProperty("Health", &Health, "Combat", 0, 1000);
        RegisterProperty("Move Speed", &MoveSpeed, "Movement", 0.0f, 50.0f);
        RegisterProperty("Can Attack", &CanAttack, "Combat");
        RegisterProperty("Player Name", &PlayerName, "General");
        RegisterReference("Target Camera", &TargetCamera, ObjectRefType::Camera, "References");
    }

    std::unique_ptr<EunoiaBehaviour> Clone() const override {
        auto clone = std::make_unique<PlayerController>(*this);
        clone->RegisterProperties();
        return clone;
    }

    void Start() override;
    void Update(float deltaTime) override;
};

// 5. EnemyController (dev.md Section 7)
class EnemyController : public EunoiaBehaviour {
public:
    int Health = 100;
    float MoveSpeed = 3.0f;
    bool CanAttack = true;
    std::string EnemyName = "Enemy";
    GameObject* TargetActor = nullptr;

    EnemyController() {
        m_className = "EnemyController";
        m_displayName = "Enemy Controller";
        RegisterProperties();
    }

    void RegisterProperties() override {
        m_properties.clear();
        RegisterProperty("Health", &Health, "Combat", 0, 1000);
        RegisterProperty("Move Speed", &MoveSpeed, "Movement", 0.0f, 50.0f);
        RegisterProperty("Can Attack", &CanAttack, "Combat");
        RegisterProperty("Enemy Name", &EnemyName, "General");
        RegisterReference("Target Actor", &TargetActor, ObjectRefType::Actor, "References");
    }

    std::unique_ptr<EunoiaBehaviour> Clone() const override {
        auto clone = std::make_unique<EnemyController>(*this);
        clone->RegisterProperties();
        return clone;
    }

    void Start() override;
    void Update(float deltaTime) override;
};

// 6. Dynamic Script Behaviour (for user-created .cpp scripts)
class DynamicScriptBehaviour : public EunoiaBehaviour {
public:
    float MoveSpeed = 5.0f;
    bool IsActive = true;
    Light* WarningLight = nullptr;

    DynamicScriptBehaviour(const std::string& className = "CustomBehaviour") {
        m_className = className;
        m_displayName = className;
        RegisterProperties();
    }

    std::unique_ptr<EunoiaBehaviour> Clone() const override {
        auto clone = std::make_unique<DynamicScriptBehaviour>(m_className);
        clone->m_displayName = m_displayName;
        clone->MoveSpeed = MoveSpeed;
        clone->IsActive = IsActive;
        clone->WarningLight = WarningLight;
        clone->RegisterProperties();
        return clone;
    }

    void RegisterProperties() override {
        m_properties.clear();
        RegisterProperty("Move Speed", &MoveSpeed, "Locomotion", 0.0f, 50.0f);
        RegisterProperty("Is Active", &IsActive, "General");
        RegisterReference("Warning Light", &WarningLight, ObjectRefType::Light, "References");
    }

    void Start() override;
    void Update(float deltaTime) override;
};

// ============================================================================
// BehaviourRegistry Singleton (dev.md Section 5)
// ============================================================================
class BehaviourRegistry {
public:
    static BehaviourRegistry& Get() {
        static BehaviourRegistry instance;
        return instance;
    }

    template<typename T>
    void Register(const std::string& className, const std::string& displayName = "", const std::string& description = "") {
        BehaviourClassInfo info;
        info.className = className;
        info.displayName = displayName.empty() ? className : displayName;
        info.description = description;
        info.factory = [className, displayName]() -> std::unique_ptr<EunoiaBehaviour> {
            auto b = std::make_unique<T>();
            b->SetClassName(className);
            if (!displayName.empty()) b->SetDisplayName(displayName);
            b->RegisterProperties();
            return b;
        };
        m_registry[className] = info;
    }

    void RegisterCustom(const BehaviourClassInfo& info) {
        m_registry[info.className] = info;
    }

    void RegisterScriptFile(const std::string& scriptName, const std::string& cppPath) {
        if (m_registry.find(scriptName) != m_registry.end()) {
            m_registry[scriptName].sourceCpp = cppPath;
            return;
        }
        BehaviourClassInfo info;
        info.className = scriptName;
        info.displayName = scriptName;
        info.description = "User C++ Script: " + cppPath;
        info.sourceCpp = cppPath;
        info.factory = [scriptName]() -> std::unique_ptr<EunoiaBehaviour> {
            auto b = std::make_unique<DynamicScriptBehaviour>(scriptName);
            b->SetClassName(scriptName);
            b->SetDisplayName(scriptName);
            return b;
        };
        m_registry[scriptName] = info;
    }

    std::unique_ptr<EunoiaBehaviour> Create(const std::string& className) const {
        auto it = m_registry.find(className);
        if (it != m_registry.end() && it->second.factory) {
            return it->second.factory();
        }
        return nullptr;
    }

    bool Has(const std::string& className) const {
        return m_registry.find(className) != m_registry.end();
    }

    const std::unordered_map<std::string, BehaviourClassInfo>& GetAll() const {
        return m_registry;
    }

    std::vector<std::string> GetRegisteredClassNames() const {
        std::vector<std::string> names;
        for (const auto& pair : m_registry) {
            names.push_back(pair.first);
        }
        return names;
    }

private:
    BehaviourRegistry() {
        // Register built-in engine behaviours
        Register<RotatorBehaviour>("RotatorBehaviour", "Rotator Behaviour", "Smooth continuous rotation around an axis");
        Register<LightFlickerBehaviour>("LightFlickerBehaviour", "Light Flicker Behaviour", "Dynamic light pulse and flicker");
        Register<DoorController>("DoorController", "Door Controller", "Door swing and lock controller with light/mesh bindings");
        Register<PlayerController>("PlayerController", "Player Controller", "Player character controller with movement & combat stats");
        Register<EnemyController>("EnemyController", "Enemy Controller", "Enemy AI controller with target tracking");
    }

    std::unordered_map<std::string, BehaviourClassInfo> m_registry;
};

#define REGISTER_BEHAVIOUR(ClassName, DisplayName) \
    struct ClassName##_AutoRegister { \
        ClassName##_AutoRegister() { \
            BehaviourRegistry::Get().Register<ClassName>(#ClassName, DisplayName); \
        } \
    }; \
    static ClassName##_AutoRegister s_register_##ClassName;
