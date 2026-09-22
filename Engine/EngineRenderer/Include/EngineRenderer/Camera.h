#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <string>

class OrbitCamera {
public:
    glm::vec3 target{0.0f, 0.4f, 0.0f};
    float distance = 6.5f;
    float yaw = 45.0f;    // degrees around Y
    float pitch = 25.0f;  // degrees elevation
    float fov = 45.0f;
    bool isOrthographic = false;
    float orthoSize = 5.0f;
    float nearPlane = 0.1f;
    float farPlane = 500.0f;

    float minDistance = 0.2f;
    float maxDistance = 250.0f;

    float moveSpeed = 6.0f;
    float mouseSensitivity = 0.22f;
    bool isFlying = false;

    // View direction vector from camera to target (Forward)
    glm::vec3 GetForward() const {
        float rYaw = glm::radians(yaw);
        float rPitch = glm::radians(pitch);
        float x = -std::cos(rPitch) * std::sin(rYaw);
        float y = -std::sin(rPitch);
        float z = -std::cos(rPitch) * std::cos(rYaw);
        return glm::normalize(glm::vec3(x, y, z));
    }

    // Right vector
    glm::vec3 GetRight() const {
        float rYaw = glm::radians(yaw);
        return glm::normalize(glm::vec3(std::cos(rYaw), 0.0f, -std::sin(rYaw)));
    }

    // Up vector
    glm::vec3 GetUp() const {
        return glm::normalize(glm::cross(GetRight(), GetForward()));
    }

    glm::vec3 GetPosition() const {
        float rYaw = glm::radians(yaw);
        float rPitch = glm::radians(pitch);

        float x = target.x + distance * std::cos(rPitch) * std::sin(rYaw);
        float y = target.y + distance * std::sin(rPitch);
        float z = target.z + distance * std::cos(rPitch) * std::cos(rYaw);

        return glm::vec3(x, y, z);
    }

    glm::mat4 GetViewMatrix() const {
        glm::vec3 pos = GetPosition();
        return glm::lookAt(pos, target, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 GetProjectionMatrix(float aspectRatio) const {
        if (isOrthographic) {
            float halfH = (orthoSize > 0.01f) ? orthoSize : 5.0f;
            float halfW = halfH * aspectRatio;
            return glm::ortho(-halfW, halfW, -halfH, halfH, nearPlane, farPlane);
        }
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }

    // First-person look around in place (RMB + Mouse movement) - Inverted both horizontal and vertical directions
    void LookAround(float deltaX, float deltaY) {
        glm::vec3 curPos = GetPosition();

        yaw -= deltaX * mouseSensitivity;
        pitch += deltaY * mouseSensitivity;

        while (yaw >= 360.0f) yaw -= 360.0f;
        while (yaw < 0.0f) yaw += 360.0f;

        pitch = std::clamp(pitch, -88.0f, 88.0f);

        // Keep position identical by recalculating target along the new forward vector
        glm::vec3 forward = GetForward();
        target = curPos + forward * distance;
    }

    // Move camera and target together in 3D world (RMB + W, S, A, D, E, Q)
    void Move(const glm::vec3& deltaMove) {
        target += deltaMove;
    }

    // Orbit around pivot/selected object (Alt + LMB + Drag)
    void Orbit(float deltaX, float deltaY) {
        yaw += deltaX * 0.35f;
        pitch += deltaY * 0.35f;

        while (yaw >= 360.0f) yaw -= 360.0f;
        while (yaw < 0.0f) yaw += 360.0f;

        pitch = std::clamp(pitch, -88.0f, 88.0f);
    }

    // Pan/track camera (Alt + MMB + Drag or Middle Mouse)
    void Pan(float deltaX, float deltaY) {
        glm::vec3 right = GetRight();
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        float factor = distance * 0.002f;
        target += (-right * deltaX + up * deltaY) * factor;
    }

    // Dolly/zoom (Alt + RMB + Drag or Scroll Wheel)
    void Zoom(float delta) {
        distance -= delta * (distance * 0.10f + 0.3f);
        distance = std::clamp(distance, minDistance, maxDistance);
    }

    void Dolly(float delta) {
        distance -= delta * (distance * 0.015f + 0.05f);
        distance = std::clamp(distance, minDistance, maxDistance);
    }

    // Focus selected actor (F key)
    void FocusOn(const glm::vec3& objPos, const glm::vec3& objScale = glm::vec3(1.0f)) {
        target = objPos;
        float maxDim = std::max({objScale.x, objScale.y, objScale.z, 0.5f});
        distance = std::clamp(maxDim * 3.2f, 2.0f, 40.0f);
    }

    // Increase/decrease camera speed (RMB + Mouse Wheel Up/Down)
    void AdjustSpeed(float delta) {
        if (delta > 0.0f) {
            moveSpeed *= 1.25f;
        } else if (delta < 0.0f) {
            moveSpeed /= 1.25f;
        }
        moveSpeed = std::clamp(moveSpeed, 0.5f, 60.0f);
    }

    void SetPreset(const char* preset) {
        std::string p = preset;
        if (p == "Perspective") {
            yaw = 45.0f; pitch = 25.0f; distance = 6.5f; target = {0.0f, 0.4f, 0.0f};
        } else if (p == "Top") {
            yaw = 0.0f; pitch = 88.0f; distance = 8.0f; target = {0.0f, 0.0f, 0.0f};
        } else if (p == "Front") {
            yaw = 0.0f; pitch = 5.0f; distance = 6.5f; target = {0.0f, 0.5f, 0.0f};
        } else if (p == "Side") {
            yaw = 90.0f; pitch = 5.0f; distance = 6.5f; target = {0.0f, 0.5f, 0.0f};
        } else if (p == "Isometric") {
            yaw = 45.0f; pitch = 35.264f; distance = 7.0f; target = {0.0f, 0.4f, 0.0f};
        }
    }
};
