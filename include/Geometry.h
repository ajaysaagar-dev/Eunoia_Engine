#pragma once
#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

struct Vertex {
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    glm::vec2 uv{0.0f, 0.0f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};

    Vertex() = default;
    Vertex(const glm::vec3& p, const glm::vec3& c)
        : position(p), normal(0.0f, 1.0f, 0.0f), uv(0.0f, 0.0f), color(c) {}
    Vertex(const glm::vec3& p, const glm::vec3& n, const glm::vec2& u, const glm::vec3& c)
        : position(p), normal(n), uv(u), color(c) {}
};

struct MeshVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv{0.0f, 0.0f};
};

struct PrimitiveMesh {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
};

class GeometryBuilder {
public:
    static PrimitiveMesh CreateCube(float size = 1.0f, int sub = 12) {
        PrimitiveMesh mesh;
        float s = size * 0.5f;

        struct FaceDef {
            glm::vec3 normal;
            glm::vec3 origin;
            glm::vec3 uAxis;
            glm::vec3 vAxis;
        };

        FaceDef faces[6] = {
            // Front (+Z)
            { {0, 0, 1},  {-s, -s,  s}, { size, 0, 0 }, { 0,  size, 0 } },
            // Back (-Z)
            { {0, 0, -1}, { s, -s, -s}, {-size, 0, 0 }, { 0,  size, 0 } },
            // Right (+X)
            { {1, 0, 0},  { s, -s,  s}, { 0, 0, -size}, { 0,  size, 0 } },
            // Left (-X)
            { {-1, 0, 0}, {-s, -s, -s}, { 0, 0,  size}, { 0,  size, 0 } },
            // Top (+Y)
            { {0, 1, 0},  {-s,  s,  s}, { size, 0, 0 }, { 0, 0, -size } },
            // Bottom (-Y)
            { {0, -1, 0}, {-s, -s, -s}, { size, 0, 0 }, { 0, 0,  size } }
        };

        for (int f = 0; f < 6; ++f) {
            uint32_t baseIdx = (uint32_t)mesh.vertices.size();
            for (int y = 0; y <= sub; ++y) {
                float ty = (float)y / (float)sub;
                for (int x = 0; x <= sub; ++x) {
                    float tx = (float)x / (float)sub;
                    glm::vec3 pos = faces[f].origin + tx * faces[f].uAxis + ty * faces[f].vAxis;
                    mesh.vertices.push_back({ pos, faces[f].normal, { tx, ty } });
                }
            }
            for (int y = 0; y < sub; ++y) {
                for (int x = 0; x < sub; ++x) {
                    uint32_t i0 = baseIdx + y * (sub + 1) + x;
                    uint32_t i1 = i0 + 1;
                    uint32_t i2 = baseIdx + (y + 1) * (sub + 1) + x;
                    uint32_t i3 = i2 + 1;

                    mesh.indices.push_back(i0);
                    mesh.indices.push_back(i1);
                    mesh.indices.push_back(i2);

                    mesh.indices.push_back(i1);
                    mesh.indices.push_back(i3);
                    mesh.indices.push_back(i2);
                }
            }
        }
        return mesh;
    }

    static PrimitiveMesh CreateBox(float width = 1.0f, float height = 1.0f, float depth = 1.0f, int subX = 1, int subY = 1, int subZ = 1) {
        PrimitiveMesh mesh;
        float hx = width * 0.5f, hy = height * 0.5f, hz = depth * 0.5f;
        subX = std::max(1, subX); subY = std::max(1, subY); subZ = std::max(1, subZ);

        struct FaceDef {
            glm::vec3 normal, origin, uAxis, vAxis;
            int subU, subV;
        };
        FaceDef faces[6] = {
            // Front (+Z)
            { {0, 0, 1},  {-hx, -hy,  hz}, { width, 0, 0 }, { 0,  height, 0 }, subX, subY },
            // Back (-Z)
            { {0, 0, -1}, { hx, -hy, -hz}, {-width, 0, 0 }, { 0,  height, 0 }, subX, subY },
            // Right (+X)
            { {1, 0, 0},  { hx, -hy,  hz}, { 0, 0, -depth}, { 0,  height, 0 }, subZ, subY },
            // Left (-X)
            { {-1, 0, 0}, {-hx, -hy, -hz}, { 0, 0,  depth}, { 0,  height, 0 }, subZ, subY },
            // Top (+Y)
            { {0, 1, 0},  {-hx,  hy,  hz}, { width, 0, 0 }, { 0, 0, -depth }, subX, subZ },
            // Bottom (-Y)
            { {0, -1, 0}, {-hx, -hy, -hz}, { width, 0, 0 }, { 0, 0,  depth }, subX, subZ }
        };

        for (int f = 0; f < 6; ++f) {
            uint32_t baseIdx = (uint32_t)mesh.vertices.size();
            int sU = faces[f].subU, sV = faces[f].subV;
            for (int y = 0; y <= sV; ++y) {
                float ty = (float)y / (float)sV;
                for (int x = 0; x <= sU; ++x) {
                    float tx = (float)x / (float)sU;
                    glm::vec3 pos = faces[f].origin + tx * faces[f].uAxis + ty * faces[f].vAxis;
                    mesh.vertices.push_back({ pos, faces[f].normal, { tx, ty } });
                }
            }
            for (int y = 0; y < sV; ++y) {
                for (int x = 0; x < sU; ++x) {
                    uint32_t i0 = baseIdx + y * (sU + 1) + x;
                    uint32_t i1 = i0 + 1;
                    uint32_t i2 = baseIdx + (y + 1) * (sU + 1) + x;
                    uint32_t i3 = i2 + 1;
                    mesh.indices.push_back(i0); mesh.indices.push_back(i1); mesh.indices.push_back(i2);
                    mesh.indices.push_back(i1); mesh.indices.push_back(i3); mesh.indices.push_back(i2);
                }
            }
        }
        return mesh;
    }

    static PrimitiveMesh CreatePlane(float width = 2.0f, float depth = 2.0f, int subX = 16, int subZ = 16) {
        PrimitiveMesh mesh;
        float hx = width * 0.5f;
        float hz = depth * 0.5f;

        for (int z = 0; z <= subZ; ++z) {
            float tz = (float)z / (float)subZ;
            float posZ = -hz + tz * depth;
            for (int x = 0; x <= subX; ++x) {
                float tx = (float)x / (float)subX;
                float posX = -hx + tx * width;
                mesh.vertices.push_back({ {posX, 0.0f, posZ}, {0.0f, 1.0f, 0.0f}, {tx, tz} });
            }
        }

        for (int z = 0; z < subZ; ++z) {
            for (int x = 0; x < subX; ++x) {
                uint32_t i0 = z * (subX + 1) + x;
                uint32_t i1 = i0 + 1;
                uint32_t i2 = (z + 1) * (subX + 1) + x;
                uint32_t i3 = i2 + 1;

                mesh.indices.push_back(i0);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i1);

                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i3);
            }
        }

        return mesh;
    }

    static PrimitiveMesh CreateSphere(float radius = 0.5f, int rings = 28, int sectors = 28) {
        PrimitiveMesh mesh;

        for (int r = 0; r <= rings; ++r) {
            float v = (float)r / (float)rings;
            float phi = (float)M_PI * v - (float)M_PI * 0.5f;
            float cosPhi = std::cos(phi);
            float sinPhi = std::sin(phi);

            for (int s = 0; s <= sectors; ++s) {
                float u = (float)s / (float)sectors;
                float theta = 2.0f * (float)M_PI * u;
                float cosTheta = std::cos(theta);
                float sinTheta = std::sin(theta);

                glm::vec3 n(cosPhi * cosTheta, sinPhi, cosPhi * sinTheta);
                glm::vec3 p = n * radius;
                mesh.vertices.push_back({ p, n, { u, v } });
            }
        }

        for (int r = 0; r < rings; ++r) {
            for (int s = 0; s < sectors; ++s) {
                uint32_t first = r * (sectors + 1) + s;
                uint32_t second = (r + 1) * (sectors + 1) + s;

                mesh.indices.push_back(first);
                mesh.indices.push_back(second);
                mesh.indices.push_back(first + 1);

                mesh.indices.push_back(first + 1);
                mesh.indices.push_back(second);
                mesh.indices.push_back(second + 1);
            }
        }

        return mesh;
    }

    static PrimitiveMesh CreateUVSphere(float radius = 0.5f, int longSegments = 24, int latSegments = 16) {
        return CreateSphere(radius, latSegments, longSegments);
    }

    static PrimitiveMesh CreateIcosphere(float radius = 0.5f, int subdivisions = 2) {
        PrimitiveMesh mesh;
        subdivisions = glm::clamp(subdivisions, 0, 5);
        const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;

        std::vector<glm::vec3> positions = {
            {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
            { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
            { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
        };

        struct TriangleIndices { uint32_t a, b, c; };
        std::vector<TriangleIndices> faces = {
            {0, 11, 5}, {0, 5, 1}, {0, 1, 7}, {0, 7, 10}, {0, 10, 11},
            {1, 5, 9}, {5, 11, 4}, {11, 10, 2}, {10, 7, 6}, {7, 1, 8},
            {3, 9, 4}, {3, 4, 2}, {3, 2, 6}, {3, 6, 8}, {3, 8, 9},
            {4, 9, 5}, {2, 4, 11}, {6, 2, 10}, {8, 6, 7}, {9, 8, 1}
        };

        auto getMiddlePoint = [&](uint32_t p1, uint32_t p2, std::map<int64_t, uint32_t>& cache) -> uint32_t {
            bool firstIsSmaller = p1 < p2;
            int64_t smallerIndex = firstIsSmaller ? p1 : p2;
            int64_t greaterIndex = firstIsSmaller ? p2 : p1;
            int64_t key = (smallerIndex << 32) + greaterIndex;

            auto it = cache.find(key);
            if (it != cache.end()) return it->second;

            glm::vec3 middle = (positions[p1] + positions[p2]) * 0.5f;
            positions.push_back(middle);
            uint32_t newIdx = (uint32_t)positions.size() - 1;
            cache[key] = newIdx;
            return newIdx;
        };

        for (int i = 0; i < subdivisions; ++i) {
            std::map<int64_t, uint32_t> cache;
            std::vector<TriangleIndices> newFaces;
            for (const auto& tri : faces) {
                uint32_t a = getMiddlePoint(tri.a, tri.b, cache);
                uint32_t b = getMiddlePoint(tri.b, tri.c, cache);
                uint32_t c = getMiddlePoint(tri.c, tri.a, cache);

                newFaces.push_back({tri.a, a, c});
                newFaces.push_back({tri.b, b, a});
                newFaces.push_back({tri.c, c, b});
                newFaces.push_back({a, b, c});
            }
            faces = std::move(newFaces);
        }

        for (const auto& p : positions) {
            glm::vec3 n = glm::normalize(p);
            glm::vec3 pos = n * radius;
            float u = 0.5f + std::atan2(n.z, n.x) / (2.0f * (float)M_PI);
            float v = 0.5f - std::asin(glm::clamp(n.y, -1.0f, 1.0f)) / (float)M_PI;
            mesh.vertices.push_back({ pos, n, { u, v } });
        }

        for (const auto& tri : faces) {
            mesh.indices.push_back(tri.a);
            mesh.indices.push_back(tri.b);
            mesh.indices.push_back(tri.c);
        }

        return mesh;
    }

    static PrimitiveMesh CreateCylinder(float radius = 0.5f, float height = 1.0f, int radialSegments = 28, int heightSegments = 1, bool capTop = true, bool capBottom = true) {
        PrimitiveMesh mesh;
        radialSegments = std::max(3, radialSegments);
        heightSegments = std::max(1, heightSegments);
        float halfH = height * 0.5f;

        // Side wall
        for (int y = 0; y <= heightSegments; ++y) {
            float v = (float)y / (float)heightSegments;
            float posY = halfH - v * height;
            for (int i = 0; i <= radialSegments; ++i) {
                float u = (float)i / (float)radialSegments;
                float theta = 2.0f * (float)M_PI * u;
                float cosT = std::cos(theta);
                float sinT = std::sin(theta);
                glm::vec3 n(cosT, 0.0f, sinT);
                mesh.vertices.push_back({ {radius * cosT, posY, radius * sinT}, n, {u, v} });
            }
        }

        for (int y = 0; y < heightSegments; ++y) {
            for (int i = 0; i < radialSegments; ++i) {
                uint32_t i0 = y * (radialSegments + 1) + i;
                uint32_t i1 = i0 + 1;
                uint32_t i2 = (y + 1) * (radialSegments + 1) + i;
                uint32_t i3 = i2 + 1;

                mesh.indices.push_back(i0);
                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);

                mesh.indices.push_back(i1);
                mesh.indices.push_back(i3);
                mesh.indices.push_back(i2);
            }
        }

        // Top Cap
        if (capTop) {
            uint32_t topCenterIdx = (uint32_t)mesh.vertices.size();
            mesh.vertices.push_back({ {0.0f, halfH, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} });
            for (int i = 0; i <= radialSegments; ++i) {
                float theta = 2.0f * (float)M_PI * ((float)i / (float)radialSegments);
                mesh.vertices.push_back({ {radius * std::cos(theta), halfH, radius * std::sin(theta)}, {0.0f, 1.0f, 0.0f}, {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)} });
            }
            for (int i = 0; i < radialSegments; ++i) {
                mesh.indices.push_back(topCenterIdx);
                mesh.indices.push_back(topCenterIdx + 1 + i);
                mesh.indices.push_back(topCenterIdx + 2 + i);
            }
        }

        // Bottom Cap
        if (capBottom) {
            uint32_t botCenterIdx = (uint32_t)mesh.vertices.size();
            mesh.vertices.push_back({ {0.0f, -halfH, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f} });
            for (int i = 0; i <= radialSegments; ++i) {
                float theta = 2.0f * (float)M_PI * ((float)i / (float)radialSegments);
                mesh.vertices.push_back({ {radius * std::cos(theta), -halfH, radius * std::sin(theta)}, {0.0f, -1.0f, 0.0f}, {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)} });
            }
            for (int i = 0; i < radialSegments; ++i) {
                mesh.indices.push_back(botCenterIdx);
                mesh.indices.push_back(botCenterIdx + 2 + i);
                mesh.indices.push_back(botCenterIdx + 1 + i);
            }
        }

        return mesh;
    }

    static PrimitiveMesh CreateCone(float bottomRadius = 0.5f, float topRadius = 0.0f, float height = 1.0f, int radialSegments = 24, int heightSegments = 1, bool capBottom = true, bool capTop = false) {
        PrimitiveMesh mesh;
        radialSegments = std::max(3, radialSegments);
        heightSegments = std::max(1, heightSegments);
        float halfH = height * 0.5f;

        float slant = std::atan2(bottomRadius - topRadius, height);
        float cosSlant = std::cos(slant);
        float sinSlant = std::sin(slant);

        for (int y = 0; y <= heightSegments; ++y) {
            float v = (float)y / (float)heightSegments;
            float curY = halfH - v * height;
            float curR = topRadius + v * (bottomRadius - topRadius);

            for (int x = 0; x <= radialSegments; ++x) {
                float u = (float)x / (float)radialSegments;
                float theta = 2.0f * (float)M_PI * u;
                float cosT = std::cos(theta);
                float sinT = std::sin(theta);

                glm::vec3 n = glm::normalize(glm::vec3(cosT * cosSlant, sinSlant, sinT * cosSlant));
                glm::vec3 p(curR * cosT, curY, curR * sinT);
                mesh.vertices.push_back({ p, n, { u, v } });
            }
        }

        for (int y = 0; y < heightSegments; ++y) {
            for (int x = 0; x < radialSegments; ++x) {
                uint32_t i0 = y * (radialSegments + 1) + x;
                uint32_t i1 = i0 + 1;
                uint32_t i2 = (y + 1) * (radialSegments + 1) + x;
                uint32_t i3 = i2 + 1;

                mesh.indices.push_back(i0);
                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);

                mesh.indices.push_back(i1);
                mesh.indices.push_back(i3);
                mesh.indices.push_back(i2);
            }
        }

        if (capBottom && bottomRadius > 0.001f) {
            uint32_t botCenterIdx = (uint32_t)mesh.vertices.size();
            mesh.vertices.push_back({ {0.0f, -halfH, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f} });
            for (int i = 0; i <= radialSegments; ++i) {
                float theta = 2.0f * (float)M_PI * ((float)i / (float)radialSegments);
                mesh.vertices.push_back({ {bottomRadius * std::cos(theta), -halfH, bottomRadius * std::sin(theta)}, {0.0f, -1.0f, 0.0f}, {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)} });
            }
            for (int i = 0; i < radialSegments; ++i) {
                mesh.indices.push_back(botCenterIdx);
                mesh.indices.push_back(botCenterIdx + 2 + i);
                mesh.indices.push_back(botCenterIdx + 1 + i);
            }
        }

        if (capTop && topRadius > 0.001f) {
            uint32_t topCenterIdx = (uint32_t)mesh.vertices.size();
            mesh.vertices.push_back({ {0.0f, halfH, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} });
            for (int i = 0; i <= radialSegments; ++i) {
                float theta = 2.0f * (float)M_PI * ((float)i / (float)radialSegments);
                mesh.vertices.push_back({ {topRadius * std::cos(theta), halfH, topRadius * std::sin(theta)}, {0.0f, 1.0f, 0.0f}, {0.5f + 0.5f * std::cos(theta), 0.5f + 0.5f * std::sin(theta)} });
            }
            for (int i = 0; i < radialSegments; ++i) {
                mesh.indices.push_back(topCenterIdx);
                mesh.indices.push_back(topCenterIdx + 1 + i);
                mesh.indices.push_back(topCenterIdx + 2 + i);
            }
        }

        return mesh;
    }

    static PrimitiveMesh CreateCapsule(float radius = 0.3f, float height = 0.8f, int radialSegments = 20, int heightSegments = 2, int hemisphereSegments = 8) {
        PrimitiveMesh mesh;
        radialSegments = std::max(3, radialSegments);
        heightSegments = std::max(1, heightSegments);
        hemisphereSegments = std::max(2, hemisphereSegments);
        float halfH = height * 0.5f;

        int totalRings = hemisphereSegments * 2 + heightSegments;

        for (int r = 0; r <= totalRings; ++r) {
            float curY = 0.0f;
            float curR = radius;
            float nY = 0.0f;
            float nR = 1.0f;

            if (r <= hemisphereSegments) {
                float phi = (float)M_PI * 0.5f * (1.0f - (float)r / (float)hemisphereSegments);
                curY = halfH + radius * std::sin(phi);
                curR = radius * std::cos(phi);
                nY = std::sin(phi);
                nR = std::cos(phi);
            } else if (r <= hemisphereSegments + heightSegments) {
                float t = (float)(r - hemisphereSegments) / (float)heightSegments;
                curY = halfH - t * height;
                curR = radius;
                nY = 0.0f;
                nR = 1.0f;
            } else {
                int bIdx = r - (hemisphereSegments + heightSegments);
                float phi = -(float)M_PI * 0.5f * ((float)bIdx / (float)hemisphereSegments);
                curY = -halfH + radius * std::sin(phi);
                curR = radius * std::cos(phi);
                nY = std::sin(phi);
                nR = std::cos(phi);
            }

            float v = (float)r / (float)totalRings;
            for (int s = 0; s <= radialSegments; ++s) {
                float u = (float)s / (float)radialSegments;
                float theta = 2.0f * (float)M_PI * u;
                float cosT = std::cos(theta);
                float sinT = std::sin(theta);

                glm::vec3 n(nR * cosT, nY, nR * sinT);
                glm::vec3 p(curR * cosT, curY, curR * sinT);
                mesh.vertices.push_back({ p, glm::normalize(n), { u, v } });
            }
        }

        for (int r = 0; r < totalRings; ++r) {
            for (int s = 0; s < radialSegments; ++s) {
                uint32_t i0 = r * (radialSegments + 1) + s;
                uint32_t i1 = i0 + 1;
                uint32_t i2 = (r + 1) * (radialSegments + 1) + s;
                uint32_t i3 = i2 + 1;

                mesh.indices.push_back(i0);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i1);

                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i3);
            }
        }

        return mesh;
    }

    static PrimitiveMesh CreatePyramid(float baseWidth = 1.0f, float baseDepth = 1.0f, float height = 1.0f, int sides = 4) {
        PrimitiveMesh mesh;
        glm::vec3 apex(0.0f, height, 0.0f);
        sides = std::max(3, sides);

        std::vector<glm::vec3> baseCorners;
        if (sides == 4) {
            float sx = baseWidth * 0.5f;
            float sz = baseDepth * 0.5f;
            baseCorners = {
                {-sx, 0.0f,  sz},
                { sx, 0.0f,  sz},
                { sx, 0.0f, -sz},
                {-sx, 0.0f, -sz}
            };
        } else {
            float r = baseWidth * 0.5f;
            for (int i = 0; i < sides; ++i) {
                float a = 2.0f * (float)M_PI * ((float)i / (float)sides);
                baseCorners.push_back({ r * std::cos(a), 0.0f, r * std::sin(a) });
            }
        }

        for (int i = 0; i < sides; ++i) {
            glm::vec3 c0 = baseCorners[i];
            glm::vec3 c1 = baseCorners[(i + 1) % sides];
            glm::vec3 normal = glm::normalize(glm::cross(c1 - c0, apex - c0));

            uint32_t start = (uint32_t)mesh.vertices.size();
            mesh.vertices.push_back({ c0, normal, {0.0f, 0.0f} });
            mesh.vertices.push_back({ c1, normal, {1.0f, 0.0f} });
            mesh.vertices.push_back({ apex, normal, {0.5f, 1.0f} });

            mesh.indices.push_back(start);
            mesh.indices.push_back(start + 1);
            mesh.indices.push_back(start + 2);
        }

        glm::vec3 baseNormal(0.0f, -1.0f, 0.0f);
        uint32_t centerIdx = (uint32_t)mesh.vertices.size();
        mesh.vertices.push_back({ {0.0f, 0.0f, 0.0f}, baseNormal, {0.5f, 0.5f} });
        for (int i = 0; i < sides; ++i) {
            mesh.vertices.push_back({ baseCorners[i], baseNormal, {0.5f + 0.5f * (baseCorners[i].x / (baseWidth * 0.5f + 0.0001f)), 0.5f + 0.5f * (baseCorners[i].z / (baseDepth * 0.5f + 0.0001f))} });
        }
        for (int i = 0; i < sides; ++i) {
            mesh.indices.push_back(centerIdx);
            mesh.indices.push_back(centerIdx + 1 + ((i + 1) % sides));
            mesh.indices.push_back(centerIdx + 1 + i);
        }

        return mesh;
    }

    static PrimitiveMesh CreateDisc(float radius = 0.5f, int segments = 32) {
        PrimitiveMesh mesh;
        segments = std::max(3, segments);
        glm::vec3 normal(0.0f, 1.0f, 0.0f);

        mesh.vertices.push_back({ {0.0f, 0.0f, 0.0f}, normal, {0.5f, 0.5f} });

        for (int i = 0; i <= segments; ++i) {
            float theta = 2.0f * (float)M_PI * ((float)i / (float)segments);
            float cosT = std::cos(theta);
            float sinT = std::sin(theta);
            mesh.vertices.push_back({ {radius * cosT, 0.0f, radius * sinT}, normal, {0.5f + 0.5f * cosT, 0.5f + 0.5f * sinT} });
        }

        for (int i = 1; i <= segments; ++i) {
            mesh.indices.push_back(0);
            mesh.indices.push_back(i);
            mesh.indices.push_back(i + 1);
        }

        return mesh;
    }

    static PrimitiveMesh CreateCircle(float radius = 0.5f, int segments = 32) {
        return CreateDisc(radius, segments);
    }

    static PrimitiveMesh CreateQuad(float width = 1.0f, float height = 1.0f) {
        PrimitiveMesh mesh;
        float hx = width * 0.5f;
        float hy = height * 0.5f;
        glm::vec3 n(0.0f, 1.0f, 0.0f);

        mesh.vertices.push_back({ {-hx, 0.0f,  hy}, n, {0.0f, 1.0f} });
        mesh.vertices.push_back({ { hx, 0.0f,  hy}, n, {1.0f, 1.0f} });
        mesh.vertices.push_back({ { hx, 0.0f, -hy}, n, {1.0f, 0.0f} });
        mesh.vertices.push_back({ {-hx, 0.0f, -hy}, n, {0.0f, 0.0f} });

        mesh.indices = { 0, 1, 2, 0, 2, 3 };
        return mesh;
    }

    static PrimitiveMesh CreateTriangle(float baseWidth = 1.0f, float height = 1.0f) {
        PrimitiveMesh mesh;
        float hx = baseWidth * 0.5f;
        float hy = height * 0.5f;
        glm::vec3 n(0.0f, 1.0f, 0.0f);

        mesh.vertices.push_back({ {-hx, 0.0f,  hy}, n, {0.0f, 0.0f} });
        mesh.vertices.push_back({ { hx, 0.0f,  hy}, n, {1.0f, 0.0f} });
        mesh.vertices.push_back({ {0.0f, 0.0f, -hy}, n, {0.5f, 1.0f} });

        mesh.indices = { 0, 1, 2 };
        return mesh;
    }

    static PrimitiveMesh CreatePrism(float radius = 0.5f, float height = 1.0f, int sides = 3) {
        PrimitiveMesh mesh;
        sides = std::max(3, sides);
        float halfH = height * 0.5f;

        for (int i = 0; i < sides; ++i) {
            float a0 = 2.0f * (float)M_PI * ((float)i / (float)sides);
            float a1 = 2.0f * (float)M_PI * ((float)(i + 1) / (float)sides);

            glm::vec3 p0_top(radius * std::cos(a0),  halfH, radius * std::sin(a0));
            glm::vec3 p1_top(radius * std::cos(a1),  halfH, radius * std::sin(a1));
            glm::vec3 p0_bot(radius * std::cos(a0), -halfH, radius * std::sin(a0));
            glm::vec3 p1_bot(radius * std::cos(a1), -halfH, radius * std::sin(a1));

            glm::vec3 normal = glm::normalize(glm::cross(p1_bot - p0_bot, p0_top - p0_bot));
            uint32_t base = (uint32_t)mesh.vertices.size();

            mesh.vertices.push_back({ p0_bot, normal, {0.0f, 0.0f} });
            mesh.vertices.push_back({ p1_bot, normal, {1.0f, 0.0f} });
            mesh.vertices.push_back({ p1_top, normal, {1.0f, 1.0f} });
            mesh.vertices.push_back({ p0_top, normal, {0.0f, 1.0f} });

            mesh.indices.push_back(base + 0);
            mesh.indices.push_back(base + 1);
            mesh.indices.push_back(base + 2);
            mesh.indices.push_back(base + 0);
            mesh.indices.push_back(base + 2);
            mesh.indices.push_back(base + 3);
        }

        uint32_t topCenter = (uint32_t)mesh.vertices.size();
        mesh.vertices.push_back({ {0.0f, halfH, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.5f, 0.5f} });
        for (int i = 0; i <= sides; ++i) {
            float a = 2.0f * (float)M_PI * ((float)i / (float)sides);
            mesh.vertices.push_back({ {radius * std::cos(a), halfH, radius * std::sin(a)}, {0.0f, 1.0f, 0.0f}, {0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a)} });
        }
        for (int i = 0; i < sides; ++i) {
            mesh.indices.push_back(topCenter);
            mesh.indices.push_back(topCenter + 1 + i);
            mesh.indices.push_back(topCenter + 2 + i);
        }

        uint32_t botCenter = (uint32_t)mesh.vertices.size();
        mesh.vertices.push_back({ {0.0f, -halfH, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.5f, 0.5f} });
        for (int i = 0; i <= sides; ++i) {
            float a = 2.0f * (float)M_PI * ((float)i / (float)sides);
            mesh.vertices.push_back({ {radius * std::cos(a), -halfH, radius * std::sin(a)}, {0.0f, -1.0f, 0.0f}, {0.5f + 0.5f * std::cos(a), 0.5f + 0.5f * std::sin(a)} });
        }
        for (int i = 0; i < sides; ++i) {
            mesh.indices.push_back(botCenter);
            mesh.indices.push_back(botCenter + 2 + i);
            mesh.indices.push_back(botCenter + 1 + i);
        }

        return mesh;
    }

    static PrimitiveMesh CreateTorus(float rMain = 0.6f, float rTube = 0.2f, int mainSeg = 24, int tubeSeg = 16) {
        PrimitiveMesh mesh;

        for (int i = 0; i <= mainSeg; ++i) {
            float u = 2.0f * (float)M_PI * (float)i / (float)mainSeg;
            float cosU = std::cos(u);
            float sinU = std::sin(u);

            for (int j = 0; j <= tubeSeg; ++j) {
                float v = 2.0f * (float)M_PI * (float)j / (float)tubeSeg;
                float cosV = std::cos(v);
                float sinV = std::sin(v);

                glm::vec3 p(
                    (rMain + rTube * cosV) * cosU,
                    rTube * sinV,
                    (rMain + rTube * cosV) * sinU
                );

                glm::vec3 n(cosV * cosU, sinV, cosV * sinU);
                mesh.vertices.push_back({ p, glm::normalize(n), {(float)i / mainSeg, (float)j / tubeSeg} });
            }
        }

        for (int i = 0; i < mainSeg; ++i) {
            for (int j = 0; j < tubeSeg; ++j) {
                uint32_t i0 = i * (tubeSeg + 1) + j;
                uint32_t i1 = (i + 1) * (tubeSeg + 1) + j;
                uint32_t i2 = i * (tubeSeg + 1) + (j + 1);
                uint32_t i3 = (i + 1) * (tubeSeg + 1) + (j + 1);

                mesh.indices.push_back(i0);
                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);

                mesh.indices.push_back(i2);
                mesh.indices.push_back(i1);
                mesh.indices.push_back(i3);
            }
        }

        return mesh;
    }

    static void AppendGrid(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices, float size = 16.0f, int divisions = 16, glm::vec3 color = {0.35f, 0.35f, 0.4f}) {
        float step = size / (float)divisions;
        float half = size * 0.5f;
        float lineThick = 0.015f;

        for (int i = 0; i <= divisions; ++i) {
            float p = -half + i * step;
            glm::vec3 c = (i == divisions / 2) ? glm::vec3(0.6f, 0.6f, 0.7f) : color;

            // X-line quad
            uint32_t idx = (uint32_t)outVertices.size();
            outVertices.push_back({ {-half, -0.005f, p - lineThick}, c });
            outVertices.push_back({ { half, -0.005f, p - lineThick}, c });
            outVertices.push_back({ { half, -0.005f, p + lineThick}, c });
            outVertices.push_back({ {-half, -0.005f, p + lineThick}, c });

            outIndices.push_back(idx + 0);
            outIndices.push_back(idx + 1);
            outIndices.push_back(idx + 2);
            outIndices.push_back(idx + 0);
            outIndices.push_back(idx + 2);
            outIndices.push_back(idx + 3);

            // Z-line quad
            idx = (uint32_t)outVertices.size();
            outVertices.push_back({ {p - lineThick, -0.005f, -half}, c });
            outVertices.push_back({ {p + lineThick, -0.005f, -half}, c });
            outVertices.push_back({ {p + lineThick, -0.005f,  half}, c });
            outVertices.push_back({ {p - lineThick, -0.005f,  half}, c });

            outIndices.push_back(idx + 0);
            outIndices.push_back(idx + 1);
            outIndices.push_back(idx + 2);
            outIndices.push_back(idx + 0);
            outIndices.push_back(idx + 2);
            outIndices.push_back(idx + 3);
        }
    }

    // Append camera-facing line ribbon for clean, crisp, constant-width wireframe/outline edges
    static void AppendCameraFacingLine(
        std::vector<Vertex>& outVertices,
        std::vector<uint32_t>& outIndices,
        const glm::vec3& p1,
        const glm::vec3& p2,
        const glm::vec3& cameraPos,
        float thickness,
        const glm::vec3& color)
    {
        glm::vec3 dir = p2 - p1;
        float len = glm::length(dir);
        if (len < 0.0001f) return;

        glm::vec3 mid = (p1 + p2) * 0.5f;
        glm::vec3 toCam = cameraPos - mid;
        float distToCam = glm::length(toCam);
        if (distToCam < 0.0001f) return;
        toCam /= distToCam;

        glm::vec3 side = glm::cross(dir, toCam);
        float sideLen = glm::length(side);
        if (sideLen < 0.0001f) {
            side = glm::vec3(0.0f, 1.0f, 0.0f);
        } else {
            side /= sideLen;
        }

        float halfWidth = glm::clamp(distToCam * 0.0022f, 0.008f, 0.035f) * thickness;
        glm::vec3 bias = toCam * (halfWidth * 0.5f + 0.003f);

        glm::vec3 v0 = p1 - side * halfWidth + bias;
        glm::vec3 v1 = p2 - side * halfWidth + bias;
        glm::vec3 v2 = p2 + side * halfWidth + bias;
        glm::vec3 v3 = p1 + side * halfWidth + bias;

        uint32_t baseIdx = (uint32_t)outVertices.size();
        outVertices.push_back({ v0, color });
        outVertices.push_back({ v1, color });
        outVertices.push_back({ v2, color });
        outVertices.push_back({ v3, color });

        outIndices.push_back(baseIdx + 0);
        outIndices.push_back(baseIdx + 1);
        outIndices.push_back(baseIdx + 2);
        outIndices.push_back(baseIdx + 0);
        outIndices.push_back(baseIdx + 2);
        outIndices.push_back(baseIdx + 3);
    }
};

