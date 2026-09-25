#pragma once
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// ============================================================================
// TransformVector3
// Rich 3D vector supporting both glm::vec3 operations and Transform API:
//   v.GetX(), v.GetY(), v.GetZ()
//   v.X(10), v.Y(10), v.Z(10)
//   v.x, v.y, v.z
// ============================================================================
struct TransformVector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    TransformVector3() = default;
    TransformVector3(float inX, float inY, float inZ) : x(inX), y(inY), z(inZ) {}
    TransformVector3(float scalar) : x(scalar), y(scalar), z(scalar) {}
    TransformVector3(const glm::vec3& v) : x(v.x), y(v.y), z(v.z) {}

    // Implicit conversions to and from glm::vec3
    operator glm::vec3() const { return glm::vec3(x, y, z); }
    TransformVector3& operator=(const glm::vec3& v) { x = v.x; y = v.y; z = v.z; return *this; }

    glm::vec3 ToGlm() const { return glm::vec3(x, y, z); }

    // Getters matching Docs/Behaviours/Transform.md:
    // <GameObject>.Transform.GetWorldLocation().GetX();
    // <GameObject>.Transform.GetForwardVector().GetX();
    float GetX() const { return x; }
    float GetY() const { return y; }
    float GetZ() const { return z; }

    // Setters / Getters by axis name:
    float X() const { return x; }
    float Y() const { return y; }
    float Z() const { return z; }

    void X(float val) { x = val; }
    void Y(float val) { y = val; }
    void Z(float val) { z = val; }

    void SetX(float val) { x = val; }
    void SetY(float val) { y = val; }
    void SetZ(float val) { z = val; }

    // Arithmetic
    TransformVector3 operator+(const TransformVector3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    TransformVector3 operator-(const TransformVector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    TransformVector3 operator*(float s) const { return {x * s, y * s, z * s}; }
    TransformVector3 operator/(float s) const { return {x / s, y / s, z / s}; }
    TransformVector3 operator-() const { return {-x, -y, -z}; }

    TransformVector3& operator+=(const TransformVector3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    TransformVector3& operator-=(const TransformVector3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    TransformVector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    TransformVector3& operator/=(float s) { x /= s; y /= s; z /= s; return *this; }

    bool operator==(const TransformVector3& o) const { return x == o.x && y == o.y && z == o.z; }
    bool operator!=(const TransformVector3& o) const { return !(*this == o); }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    TransformVector3 Normalized() const {
        float l = Length();
        return (l > 1e-6f) ? (*this / l) : TransformVector3(0.0f, 0.0f, 0.0f);
    }
};

inline std::ostream& operator<<(std::ostream& os, const TransformVector3& v) {
    return os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
}

// Global Vector3 alias
using Vector3 = TransformVector3;

// ============================================================================
// TransformState
// Returned by <GameObject>.Transform.GetWorldTransform() and GetRelativeTransform()
// ============================================================================
struct TransformState {
    TransformVector3 Location{0.0f, 0.0f, 0.0f};
    TransformVector3 Rotation{0.0f, 0.0f, 0.0f};
    TransformVector3 Scale{1.0f, 1.0f, 1.0f};
    glm::mat4 Matrix{1.0f};

    TransformState() = default;
    TransformState(const TransformVector3& loc, const TransformVector3& rot, const TransformVector3& sc, const glm::mat4& mat = glm::mat4(1.0f))
        : Location(loc), Rotation(rot), Scale(sc), Matrix(mat) {}

    TransformVector3 GetLocation() const { return Location; }
    TransformVector3 GetRotation() const { return Rotation; }
    TransformVector3 GetScale() const { return Scale; }

    TransformVector3 GetWorldLocation() const { return Location; }
    TransformVector3 GetWorldRotation() const { return Rotation; }
    TransformVector3 GetWorldScale() const { return Scale; }

    TransformVector3 GetRelativeLocation() const { return Location; }
    TransformVector3 GetRelativeRotation() const { return Rotation; }
    TransformVector3 GetRelativeScale() const { return Scale; }

    glm::mat4 GetMatrix() const { return Matrix; }
};
