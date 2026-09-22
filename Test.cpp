#include "EunoiaBehaviour.h"
#include "BehaviourRegistry.h"
#include "GameObject.h"
#include "InputSystem.h"

// ============================================================================
// Simple Behaviour Script Example
//
// Features:
// 1. MoveSpeed property editable in the Details panel.
// 2. Print(value, time) to display on-screen debug text at the top-left!
// 3. WASD movement and Space key input.
// ============================================================================

class Test : public EunoiaBehaviour {
public:
    // --- 1. Properties (Editable in Details Panel) ---
    float MoveSpeed = 5.0f;

    Test() {
        m_className   = "Test";
        m_displayName = "Test Script";
        RegisterProperties();
    }

    void RegisterProperties() override {
        m_properties.clear();
        RegisterProperty("Move Speed", &MoveSpeed, "Settings", 0.0f, 50.0f);
    }

    std::unique_ptr<EunoiaBehaviour> Clone() const override {
        auto clone = std::make_unique<Test>(*this);
        clone->RegisterProperties();
        return clone;
    }

    // --- 2. Start (Runs once when game starts) ---
    void Start() override {
        // Print message at screen top-left for 4 seconds: Print(value, time)
        Print("Test Script Started! Press WASD to move, Space to Jump.", 4.0f);
    }

    // --- 3. Update (Runs every frame during gameplay) ---
    void Update(float deltaTime) override {
        if (!m_owner) return;

        auto& input = InputSystem::Get();

        // Print when Space key is pressed (shows for 2.0 seconds)
        if (input.IsKeyPressed(Key::Space)) {
            Print("Jump action triggered!", 2.0f);
        }

        // WASD Movement
        glm::vec3 moveDir(0.0f);
        if (input.IsKeyDown(Key::W)) moveDir.z -= 1.0f;
        if (input.IsKeyDown(Key::S)) moveDir.z += 1.0f;
        if (input.IsKeyDown(Key::A)) moveDir.x -= 1.0f;
        if (input.IsKeyDown(Key::D)) moveDir.x += 1.0f;

        if (glm::length(moveDir) > 0.001f) {
            moveDir = glm::normalize(moveDir);
            glm::vec3 delta = moveDir * (MoveSpeed * deltaTime);

            // Move actor
            Transform.Location.LocalSpace(Transform.Location.LocalSpace() + delta);

            // Print current location on screen (shows for 1.0 second)
            Print("Player Location: ", Transform.Location.LocalSpace(), 1.0f);
        }
    }
};

REGISTER_BEHAVIOUR(Test, "Test Script")
