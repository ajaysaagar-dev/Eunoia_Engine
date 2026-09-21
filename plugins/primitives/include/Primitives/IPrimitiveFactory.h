#pragma once
// ============================================================================
// Eunoia Engine — IPrimitiveFactory Interface
// ============================================================================
// Service interface published by the Primitives plugin into ServiceRegistry.
// Allows other plugins (Scene, Editor, Renderer) to generate procedural meshes
// without direct compile-time coupling to geometry generation code.
// ============================================================================

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

// Forward-compatible primitive vertex representation
struct PrimitiveVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv{0.0f, 0.0f};
};

struct PrimitiveMeshData {
    std::vector<PrimitiveVertex> vertices;
    std::vector<uint32_t> indices;
};

class IPrimitiveFactory {
public:
    virtual ~IPrimitiveFactory() = default;

    virtual PrimitiveMeshData CreateCube(float size = 1.0f, int subdivisions = 12) = 0;
    virtual PrimitiveMeshData CreateBox(float width = 1.0f, float height = 1.0f, float depth = 1.0f, int subX = 1, int subY = 1, int subZ = 1) = 0;
    virtual PrimitiveMeshData CreatePlane(float width = 2.5f, float depth = 2.5f, int subX = 16, int subZ = 16) = 0;
    virtual PrimitiveMeshData CreateSphere(float radius = 0.6f, int rings = 24, int sectors = 24) = 0;
    virtual PrimitiveMeshData CreateCylinder(float radius = 0.5f, float height = 1.2f, int radialSegments = 24, int heightSegments = 1) = 0;
    virtual PrimitiveMeshData CreateCone(float radius = 0.5f, float height = 1.2f, int radialSegments = 24) = 0;
    virtual PrimitiveMeshData CreateCapsule(float radius = 0.35f, float height = 0.8f, int radialSegments = 20, int heightSegments = 2) = 0;
    virtual PrimitiveMeshData CreateTorus(float radius = 0.6f, float tubeRadius = 0.22f, int radialSegments = 24, int tubularSegments = 16) = 0;
    virtual PrimitiveMeshData CreatePyramid(float width = 1.0f, float depth = 1.0f, float height = 1.2f, int sides = 4) = 0;
};
