// ============================================================================
// Eunoia Engine — Primitives Plugin Implementation
// ============================================================================

#include "Primitives/IPrimitiveFactory.h"
#include "PluginAPI.h"
#include "IPlugin.h"
#include "ServiceRegistry.h"
#include "../../include/Geometry.h"

#include <iostream>

class PrimitiveFactoryImpl : public IPrimitiveFactory {
public:
    static PrimitiveMeshData ConvertMesh(const PrimitiveMesh& inMesh) {
        PrimitiveMeshData out;
        out.vertices.reserve(inMesh.vertices.size());
        for (const auto& v : inMesh.vertices) {
            out.vertices.push_back({ v.pos, v.normal, v.uv });
        }
        out.indices = inMesh.indices;
        return out;
    }

    PrimitiveMeshData CreateCube(float size, int subdivisions) override {
        return ConvertMesh(GeometryBuilder::CreateCube(size, subdivisions));
    }

    PrimitiveMeshData CreateBox(float width, float height, float depth, int subX, int subY, int subZ) override {
        return ConvertMesh(GeometryBuilder::CreateBox(width, height, depth, subX, subY, subZ));
    }

    PrimitiveMeshData CreatePlane(float width, float depth, int subX, int subZ) override {
        return ConvertMesh(GeometryBuilder::CreatePlane(width, depth, subX, subZ));
    }

    PrimitiveMeshData CreateSphere(float radius, int rings, int sectors) override {
        return ConvertMesh(GeometryBuilder::CreateSphere(radius, rings, sectors));
    }

    PrimitiveMeshData CreateCylinder(float radius, float height, int radialSegments, int heightSegments) override {
        return ConvertMesh(GeometryBuilder::CreateCylinder(radius, height, radialSegments, heightSegments));
    }

    PrimitiveMeshData CreateCone(float radius, float height, int radialSegments) override {
        return ConvertMesh(GeometryBuilder::CreateCone(radius, height, radialSegments));
    }

    PrimitiveMeshData CreateCapsule(float radius, float height, int radialSegments, int heightSegments) override {
        return ConvertMesh(GeometryBuilder::CreateCapsule(radius, height, radialSegments, heightSegments));
    }

    PrimitiveMeshData CreateTorus(float radius, float tubeRadius, int radialSegments, int tubularSegments) override {
        return ConvertMesh(GeometryBuilder::CreateTorus(radius, tubeRadius, radialSegments, tubularSegments));
    }

    PrimitiveMeshData CreatePyramid(float width, float depth, float height, int sides) override {
        return ConvertMesh(GeometryBuilder::CreatePyramid(width, depth, height, sides));
    }
};

class PrimitivesPlugin : public IPlugin {
public:
    explicit PrimitivesPlugin(const EunoiaPluginContext* /*ctx*/) {}
    ~PrimitivesPlugin() override = default;

    const char* GetName() const override { return "primitives"; }
    bool SupportsHotReload() const override { return true; }

    void OnRegister(ServiceRegistry& registry) override {
        std::cout << "[PrimitivesPlugin] Registering IPrimitiveFactory service..." << std::endl;
        registry.Register<IPrimitiveFactory>("IPrimitiveFactory", &m_factory);
    }

    void OnInit() override {
        std::cout << "[PrimitivesPlugin] Primitives plugin initialized successfully." << std::endl;
    }

    void OnShutdown() override {
        std::cout << "[PrimitivesPlugin] Shutting down Primitives plugin." << std::endl;
    }

private:
    PrimitiveFactoryImpl m_factory;
};

// Fill metadata and export plugin C ABI
static EunoiaPluginInfo s_primitivesInfo = {
    "primitives",
    "1.0.0",
    "Procedural mesh generation plugin",
    "Eunoia Team",
    EUNOIA_PLUGIN_API_VERSION,
    EunoiaPluginCategory::Primitives,
    false // dynamic tier
};

EUNOIA_DECLARE_PLUGIN(PrimitivesPlugin, &s_primitivesInfo)
