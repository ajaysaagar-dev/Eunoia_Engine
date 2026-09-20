#pragma once
#include "Scene.h"
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>

// ============================================================================
// SceneSerializer — JSON-based Scene Save/Load System
// Saves and loads complete scene state including actors, lights, and environment.
// Format: .escene (Eunoia Scene) JSON files
// ============================================================================

class SceneSerializer {
public:

    // Save the entire scene to a JSON file
    static bool SaveScene(const Scene& scene, const std::string& filePath) {
        std::ofstream file(filePath);
        if (!file.is_open()) return false;

        file << "{\n";
        file << "  \"formatVersion\": 1,\n";

        // Extract scene name from file path
        std::filesystem::path p(filePath);
        std::string sceneName = p.stem().string();
        file << "  \"sceneName\": " << QuoteStr(sceneName) << ",\n";

        // Environment
        file << "  \"environment\": {\n";
        file << "    \"clearColor\": " << WriteVec4(scene.clearColor) << ",\n";
        file << "    \"ambientIntensity\": " << scene.ambientIntensity << ",\n";
        file << "    \"sunLight\": {\n";
        file << "      \"direction\": " << WriteVec3(scene.lightDirection) << ",\n";
        file << "      \"color\": " << WriteVec3(scene.lightColor) << ",\n";
        file << "      \"intensity\": " << scene.lightIntensity << "\n";
        file << "    },\n";
        file << "    \"shadows\": {\n";
        file << "      \"enabled\": " << BoolStr(scene.enableShadows) << ",\n";
        file << "      \"strength\": " << scene.shadowStrength << ",\n";
        file << "      \"bias\": " << scene.shadowBias << ",\n";
        file << "      \"pcfRadius\": " << scene.pcfRadius << "\n";
        file << "    },\n";
        file << "    \"grid\": {\n";
        file << "      \"show\": " << BoolStr(scene.showGrid) << ",\n";
        file << "      \"size\": " << scene.gridSize << ",\n";
        file << "      \"divisions\": " << scene.gridDivisions << ",\n";
        file << "      \"color\": " << WriteVec3(scene.gridColor) << "\n";
        file << "    }\n";
        file << "  },\n";

        // Point Lights
        file << "  \"pointLights\": [\n";
        for (size_t i = 0; i < scene.pointLights.size(); ++i) {
            const auto& pl = scene.pointLights[i];
            file << "    {\n";
            file << "      \"id\": " << pl.id << ",\n";
            file << "      \"name\": " << QuoteStr(pl.name) << ",\n";
            file << "      \"position\": " << WriteVec3(pl.position) << ",\n";
            file << "      \"color\": " << WriteVec3(pl.color) << ",\n";
            file << "      \"intensity\": " << pl.intensity << ",\n";
            file << "      \"range\": " << pl.range << ",\n";
            file << "      \"enabled\": " << BoolStr(pl.enabled) << "\n";
            file << "    }" << (i + 1 < scene.pointLights.size() ? "," : "") << "\n";
        }
        file << "  ],\n";

        // Actors
        file << "  \"actors\": [\n";
        for (size_t i = 0; i < scene.objects.size(); ++i) {
            const auto& obj = scene.objects[i];
            file << "    {\n";
            file << "      \"id\": " << obj.id << ",\n";
            file << "      \"name\": " << QuoteStr(obj.name) << ",\n";
            file << "      \"type\": " << QuoteStr(GetPrimitiveTypeName(obj.type)) << ",\n";
            file << "      \"mobility\": " << QuoteStr(MobilityToStr(obj.mobility)) << ",\n";

            // Transform
            file << "      \"transform\": {\n";
            file << "        \"position\": " << WriteVec3(obj.position) << ",\n";
            file << "        \"rotation\": " << WriteVec3(obj.rotation) << ",\n";
            file << "        \"scale\": " << WriteVec3(obj.scale) << "\n";
            file << "      },\n";

            // Material
            file << "      \"material\": {\n";
            file << "        \"materialName\": " << QuoteStr(obj.materialName) << ",\n";
            file << "        \"color\": " << WriteVec3(obj.color) << ",\n";
            file << "        \"metallic\": " << obj.metallic << ",\n";
            file << "        \"roughness\": " << obj.roughness << ",\n";
            file << "        \"normalStrength\": " << obj.normalStrength << ",\n";
            file << "        \"specular\": " << obj.specular << ",\n";
            file << "        \"emissiveColor\": " << WriteVec3(obj.emissiveColor) << ",\n";
            file << "        \"emissiveIntensity\": " << obj.emissiveIntensity << ",\n";
            file << "        \"shadingModel\": " << obj.shadingModel << ",\n";
            file << "        \"blendMode\": " << obj.blendMode << ",\n";
            file << "        \"twoSided\": " << BoolStr(obj.twoSided) << ",\n";
            file << "        \"castShadows\": " << BoolStr(obj.castShadows) << ",\n";
            file << "        \"receiveShadows\": " << BoolStr(obj.receiveShadows) << ",\n";
            file << "        \"textures\": {\n";
            file << "          \"baseColor\": " << QuoteStr(obj.baseColorTexture) << ",\n";
            file << "          \"normal\": " << QuoteStr(obj.normalTexture) << ",\n";
            file << "          \"roughness\": " << QuoteStr(obj.roughnessTexture) << ",\n";
            file << "          \"metallic\": " << QuoteStr(obj.metallicTexture) << ",\n";
            file << "          \"ao\": " << QuoteStr(obj.aoTexture) << ",\n";
            file << "          \"emission\": " << QuoteStr(obj.emissionTexture) << "\n";
            file << "        }\n";
            file << "      },\n";

            // Visibility & Animation
            file << "      \"visible\": " << BoolStr(obj.visible) << ",\n";
            file << "      \"autoRotate\": " << BoolStr(obj.autoRotate) << ",\n";
            file << "      \"autoRotateSpeed\": " << WriteVec3(obj.autoRotateSpeed) << ",\n";

            // Hierarchy, Imported Mesh & Light Proxy
            file << "      \"isImportedMesh\": " << BoolStr(obj.isImportedMesh) << ",\n";
            file << "      \"meshFilePath\": " << QuoteStr(obj.meshFilePath) << ",\n";
            file << "      \"submeshIndex\": " << obj.submeshIndex << ",\n";
            file << "      \"parentId\": " << obj.parentId << ",\n";
            file << "      \"isLight\": " << BoolStr(obj.isLight) << ",\n";
            file << "      \"lightId\": " << obj.lightId << ",\n";

            // Primitive Parameters (dev.md)
            file << "      \"paramWidth\": " << obj.params.width << ",\n";
            file << "      \"paramHeight\": " << obj.params.height << ",\n";
            file << "      \"paramDepth\": " << obj.params.depth << ",\n";
            file << "      \"paramSize\": " << obj.params.size << ",\n";
            file << "      \"paramRadius\": " << obj.params.radius << ",\n";
            file << "      \"paramRadius2\": " << obj.params.radius2 << ",\n";
            file << "      \"paramSegX\": " << obj.params.segmentsX << ",\n";
            file << "      \"paramSegY\": " << obj.params.segmentsY << ",\n";
            file << "      \"paramSegZ\": " << obj.params.segmentsZ << ",\n";
            file << "      \"paramRadialSeg\": " << obj.params.radialSegments << ",\n";
            file << "      \"paramHeightSeg\": " << obj.params.heightSegments << ",\n";
            file << "      \"paramRings\": " << obj.params.rings << ",\n";
            file << "      \"paramSectors\": " << obj.params.sectors << ",\n";
            file << "      \"paramSubdiv\": " << obj.params.subdivisions << ",\n";
            file << "      \"paramSides\": " << obj.params.sides << ",\n";
            file << "      \"paramCapTop\": " << BoolStr(obj.params.capTop) << ",\n";
            file << "      \"paramCapBottom\": " << BoolStr(obj.params.capBottom) << ",\n";

            // Light Component Properties (dev.md)
            file << "      \"lightType\": " << (int)obj.light.type << ",\n";
            file << "      \"lightEnabled\": " << BoolStr(obj.light.enabled) << ",\n";
            file << "      \"lightColor\": " << WriteVec3(obj.light.color) << ",\n";
            file << "      \"lightIntensity\": " << obj.light.intensity << ",\n";
            file << "      \"lightTemperature\": " << obj.light.temperature << ",\n";
            file << "      \"lightUseTemp\": " << BoolStr(obj.light.useTemperature) << ",\n";
            file << "      \"lightRange\": " << obj.light.range << ",\n";
            file << "      \"lightAttenuation\": " << obj.light.attenuation << ",\n";
            file << "      \"lightInnerCone\": " << obj.light.innerConeAngle << ",\n";
            file << "      \"lightOuterCone\": " << obj.light.outerConeAngle << ",\n";
            file << "      \"lightConeFalloff\": " << obj.light.coneFalloff << ",\n";
            file << "      \"lightAreaShape\": " << obj.light.areaShape << ",\n";
            file << "      \"lightAreaWidth\": " << obj.light.width << ",\n";
            file << "      \"lightAreaHeight\": " << obj.light.height << ",\n";
            file << "      \"lightAreaRadius\": " << obj.light.radius << ",\n";
            file << "      \"lightAreaLength\": " << obj.light.length << ",\n";
            file << "      \"lightTwoSided\": " << BoolStr(obj.light.twoSided) << ",\n";
            file << "      \"lightSkyColor\": " << WriteVec3(obj.light.skyColor) << ",\n";
            file << "      \"lightGroundColor\": " << WriteVec3(obj.light.groundColor) << ",\n";
            file << "      \"lightEnvMap\": " << QuoteStr(obj.light.envMapTexture) << ",\n";
            file << "      \"lightCastShadows\": " << BoolStr(obj.light.castShadows) << ",\n";
            file << "      \"lightShadowStrength\": " << obj.light.shadowStrength << ",\n";
            file << "      \"lightShadowBias\": " << obj.light.shadowBias << ",\n";
            file << "      \"lightShadowRes\": " << obj.light.shadowResolution << ",\n";
            file << "      \"lightShadowDist\": " << obj.light.shadowDistance << ",\n";
            file << "      \"lightVolumetric\": " << BoolStr(obj.light.volumetric) << ",\n";
            file << "      \"lightVolScattering\": " << obj.light.volumetricScattering << ",\n";
            file << "      \"lightVolIntensity\": " << obj.light.volumetricIntensity << ",\n";
            file << "      \"lightLayer\": " << obj.light.lightLayer << "\n";

            file << "    }" << (i + 1 < scene.objects.size() ? "," : "") << "\n";
        }
        file << "  ],\n";

        // Scene metadata
        file << "  \"nextId\": " << scene.nextId << ",\n";
        file << "  \"playAnimations\": " << BoolStr(scene.playAnimations) << "\n";

        file << "}\n";
        file.close();
        return true;
    }

    // Save level alias
    static bool SaveLevel(const Scene& level, const std::string& filePath) {
        return SaveScene(level, filePath);
    }

    // Load a scene from a JSON file
    static bool LoadScene(Scene& scene, const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;

        // Read entire file
        std::stringstream ss;
        ss << file.rdbuf();
        file.close();
        std::string content = ss.str();

        // Clear the scene first
        scene.objects.clear();
        scene.pointLights.clear();
        scene.selectedId = -1;

        // Parse environment
        ParseEnvironment(content, scene);

        // Parse point lights
        ParsePointLights(content, scene);

        // Parse actors
        ParseActors(content, scene);

        // Parse metadata
        scene.nextId = ParseInt(content, "\"nextId\"", 1);
        scene.playAnimations = ParseBool(content, "\"playAnimations\"", true);

        // Ensure nextId is higher than any existing actor/light ID
        for (const auto& obj : scene.objects) {
            if (obj.id >= scene.nextId) scene.nextId = obj.id + 1;
        }
        for (const auto& pl : scene.pointLights) {
            if (pl.id >= scene.nextId) scene.nextId = pl.id + 1;
        }

        // Select first actor if available
        if (!scene.objects.empty()) {
            scene.selectedId = scene.objects.front().id;
        }

        return true;
    }

private:
    // ---- JSON Writing Helpers ----
    static std::string QuoteStr(const std::string& s) {
        std::string escaped;
        escaped += '"';
        for (char c : s) {
            if (c == '"') escaped += "\\\"";
            else if (c == '\\') escaped += "\\\\";
            else if (c == '\n') escaped += "\\n";
            else escaped += c;
        }
        escaped += '"';
        return escaped;
    }

    static std::string BoolStr(bool b) { return b ? "true" : "false"; }

    static std::string WriteVec3(const glm::vec3& v) {
        std::ostringstream os;
        os << "[" << v.x << ", " << v.y << ", " << v.z << "]";
        return os.str();
    }

    static std::string WriteVec4(const glm::vec4& v) {
        std::ostringstream os;
        os << "[" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "]";
        return os.str();
    }

    static std::string MobilityToStr(Mobility m) {
        switch (m) {
            case Mobility::Static: return "Static";
            case Mobility::Stationary: return "Stationary";
            case Mobility::Movable: return "Movable";
            default: return "Movable";
        }
    }

    static Mobility StrToMobility(const std::string& s) {
        if (s == "Static") return Mobility::Static;
        if (s == "Stationary") return Mobility::Stationary;
        return Mobility::Movable;
    }

    static PrimitiveType StrToPrimitive(const std::string& s) {
        if (s == "Cube") return PrimitiveType::Cube;
        if (s == "Plane") return PrimitiveType::Plane;
        if (s == "Box") return PrimitiveType::Box;
        if (s == "Sphere") return PrimitiveType::Sphere;
        if (s == "UVSphere" || s == "UV Sphere") return PrimitiveType::UVSphere;
        if (s == "Icosphere") return PrimitiveType::Icosphere;
        if (s == "Cylinder") return PrimitiveType::Cylinder;
        if (s == "Cone") return PrimitiveType::Cone;
        if (s == "Capsule") return PrimitiveType::Capsule;
        if (s == "Torus") return PrimitiveType::Torus;
        if (s == "Circle") return PrimitiveType::Circle;
        if (s == "Disc") return PrimitiveType::Disc;
        if (s == "Quad") return PrimitiveType::Quad;
        if (s == "Triangle") return PrimitiveType::Triangle;
        if (s == "Pyramid") return PrimitiveType::Pyramid;
        if (s == "Prism") return PrimitiveType::Prism;
        if (s == "Mesh" || s == "ImportedMesh") return PrimitiveType::ImportedMesh;
        if (s == "Empty" || s == "Empty Actor") return PrimitiveType::Empty;
        if (s == "DirectionalLight" || s == "Directional Light") return PrimitiveType::DirectionalLight;
        if (s == "PointLight" || s == "Point Light") return PrimitiveType::PointLight;
        if (s == "SpotLight" || s == "Spot Light") return PrimitiveType::SpotLight;
        if (s == "AreaLight" || s == "Area Light") return PrimitiveType::AreaLight;
        if (s == "SkyLight" || s == "Sky Light") return PrimitiveType::SkyLight;
        if (s == "AmbientLight" || s == "Ambient Light") return PrimitiveType::AmbientLight;
        if (s == "HemisphereLight" || s == "Hemisphere Light") return PrimitiveType::HemisphereLight;
        if (s == "TubeLight" || s == "Tube Light") return PrimitiveType::TubeLight;
        if (s == "DiscLight" || s == "Disc Light") return PrimitiveType::DiscLight;
        return PrimitiveType::Cube;
    }

    // ---- JSON Parsing Helpers ----

    // Extract a quoted string value for a given key
    static std::string ParseString(const std::string& json, const std::string& key, const std::string& def = "") {
        size_t pos = json.find(key);
        if (pos == std::string::npos) return def;
        pos = json.find(':', pos + key.size());
        if (pos == std::string::npos) return def;
        pos = json.find('"', pos + 1);
        if (pos == std::string::npos) return def;
        pos++; // skip opening quote
        std::string result;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                pos++;
                if (json[pos] == '"') result += '"';
                else if (json[pos] == '\\') result += '\\';
                else if (json[pos] == 'n') result += '\n';
                else result += json[pos];
            } else {
                result += json[pos];
            }
            pos++;
        }
        return result;
    }

    // Extract a float value for a given key
    static float ParseFloat(const std::string& json, const std::string& key, float def = 0.0f) {
        size_t pos = json.find(key);
        if (pos == std::string::npos) return def;
        pos = json.find(':', pos + key.size());
        if (pos == std::string::npos) return def;
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        try { return std::stof(json.substr(pos, 30)); }
        catch (...) { return def; }
    }

    // Extract an int value for a given key
    static int ParseInt(const std::string& json, const std::string& key, int def = 0) {
        size_t pos = json.find(key);
        if (pos == std::string::npos) return def;
        pos = json.find(':', pos + key.size());
        if (pos == std::string::npos) return def;
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        try { return std::stoi(json.substr(pos, 20)); }
        catch (...) { return def; }
    }

    // Extract a bool value for a given key
    static bool ParseBool(const std::string& json, const std::string& key, bool def = false) {
        size_t pos = json.find(key);
        if (pos == std::string::npos) return def;
        pos = json.find(':', pos + key.size());
        if (pos == std::string::npos) return def;
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        if (pos < json.size() && json[pos] == 't') return true;
        if (pos < json.size() && json[pos] == 'f') return false;
        return def;
    }

    // Parse a JSON array of numbers: [x, y, z] or [x, y, z, w]
    static glm::vec3 ParseVec3(const std::string& json, size_t startPos) {
        glm::vec3 v(0.0f);
        size_t bracket = json.find('[', startPos);
        if (bracket == std::string::npos) return v;
        bracket++;
        try {
            size_t next;
            v.x = std::stof(json.substr(bracket), &next); bracket += next;
            while (bracket < json.size() && (json[bracket] == ',' || json[bracket] == ' ')) bracket++;
            v.y = std::stof(json.substr(bracket), &next); bracket += next;
            while (bracket < json.size() && (json[bracket] == ',' || json[bracket] == ' ')) bracket++;
            v.z = std::stof(json.substr(bracket), &next);
        } catch (...) {}
        return v;
    }

    static glm::vec4 ParseVec4(const std::string& json, size_t startPos) {
        glm::vec4 v(0.0f);
        size_t bracket = json.find('[', startPos);
        if (bracket == std::string::npos) return v;
        bracket++;
        try {
            size_t next;
            v.x = std::stof(json.substr(bracket), &next); bracket += next;
            while (bracket < json.size() && (json[bracket] == ',' || json[bracket] == ' ')) bracket++;
            v.y = std::stof(json.substr(bracket), &next); bracket += next;
            while (bracket < json.size() && (json[bracket] == ',' || json[bracket] == ' ')) bracket++;
            v.z = std::stof(json.substr(bracket), &next); bracket += next;
            while (bracket < json.size() && (json[bracket] == ',' || json[bracket] == ' ')) bracket++;
            v.w = std::stof(json.substr(bracket), &next);
        } catch (...) {}
        return v;
    }

    // Find key position within a specific region of json
    static size_t FindKeyInRegion(const std::string& json, const std::string& key, size_t regionStart, size_t regionEnd) {
        size_t pos = json.find(key, regionStart);
        if (pos == std::string::npos || pos > regionEnd) return std::string::npos;
        return pos;
    }

    // Parse a string value at a specific position (after key)
    static std::string ParseStringAt(const std::string& json, size_t keyPos, const std::string& def = "") {
        if (keyPos == std::string::npos) return def;
        size_t pos = json.find(':', keyPos);
        if (pos == std::string::npos) return def;
        pos = json.find('"', pos + 1);
        if (pos == std::string::npos) return def;
        pos++;
        std::string result;
        while (pos < json.size() && json[pos] != '"') {
            if (json[pos] == '\\' && pos + 1 < json.size()) {
                pos++;
                if (json[pos] == '"') result += '"';
                else if (json[pos] == '\\') result += '\\';
                else if (json[pos] == 'n') result += '\n';
                else result += json[pos];
            } else {
                result += json[pos];
            }
            pos++;
        }
        return result;
    }

    static float ParseFloatAt(const std::string& json, size_t keyPos, float def = 0.0f) {
        if (keyPos == std::string::npos) return def;
        size_t pos = json.find(':', keyPos);
        if (pos == std::string::npos) return def;
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        try { return std::stof(json.substr(pos, 30)); }
        catch (...) { return def; }
    }

    static int ParseIntAt(const std::string& json, size_t keyPos, int def = 0) {
        if (keyPos == std::string::npos) return def;
        size_t pos = json.find(':', keyPos);
        if (pos == std::string::npos) return def;
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        try { return std::stoi(json.substr(pos, 20)); }
        catch (...) { return def; }
    }

    static bool ParseBoolAt(const std::string& json, size_t keyPos, bool def = false) {
        if (keyPos == std::string::npos) return def;
        size_t pos = json.find(':', keyPos);
        if (pos == std::string::npos) return def;
        pos++;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
        if (pos < json.size() && json[pos] == 't') return true;
        if (pos < json.size() && json[pos] == 'f') return false;
        return def;
    }

    // ---- Section Parsers ----

    static void ParseEnvironment(const std::string& json, Scene& scene) {
        size_t envPos = json.find("\"environment\"");
        if (envPos == std::string::npos) return;

        // Find the environment object boundaries
        size_t envStart = json.find('{', envPos);
        if (envStart == std::string::npos) return;
        size_t envEnd = FindMatchingBrace(json, envStart);

        std::string envBlock = json.substr(envStart, envEnd - envStart + 1);

        // Clear color
        size_t ccPos = envBlock.find("\"clearColor\"");
        if (ccPos != std::string::npos) {
            scene.clearColor = ParseVec4(envBlock, ccPos);
        }

        // Ambient
        size_t aiPos = envBlock.find("\"ambientIntensity\"");
        if (aiPos != std::string::npos) {
            scene.ambientIntensity = ParseFloatAt(envBlock, aiPos, 0.25f);
        }

        // Sun light
        size_t sunPos = envBlock.find("\"sunLight\"");
        if (sunPos != std::string::npos) {
            size_t sunStart = envBlock.find('{', sunPos);
            size_t sunEnd = FindMatchingBrace(envBlock, sunStart);
            std::string sunBlock = envBlock.substr(sunStart, sunEnd - sunStart + 1);

            size_t dPos = sunBlock.find("\"direction\"");
            if (dPos != std::string::npos) scene.lightDirection = glm::normalize(ParseVec3(sunBlock, dPos));
            size_t cPos = sunBlock.find("\"color\"");
            if (cPos != std::string::npos) scene.lightColor = ParseVec3(sunBlock, cPos);
            size_t iPos = sunBlock.find("\"intensity\"");
            if (iPos != std::string::npos) scene.lightIntensity = ParseFloatAt(sunBlock, iPos, 1.0f);
        }

        // Shadows
        size_t shadowPos = envBlock.find("\"shadows\"");
        if (shadowPos != std::string::npos) {
            size_t sStart = envBlock.find('{', shadowPos);
            size_t sEnd = FindMatchingBrace(envBlock, sStart);
            std::string sBlock = envBlock.substr(sStart, sEnd - sStart + 1);

            size_t p;
            p = sBlock.find("\"enabled\""); if (p != std::string::npos) scene.enableShadows = ParseBoolAt(sBlock, p, true);
            p = sBlock.find("\"strength\""); if (p != std::string::npos) scene.shadowStrength = ParseFloatAt(sBlock, p, 0.85f);
            p = sBlock.find("\"bias\""); if (p != std::string::npos) scene.shadowBias = ParseFloatAt(sBlock, p, 0.0012f);
            p = sBlock.find("\"pcfRadius\""); if (p != std::string::npos) scene.pcfRadius = ParseFloatAt(sBlock, p, 1.2f);
        }

        // Grid
        size_t gridPos = envBlock.find("\"grid\"");
        if (gridPos != std::string::npos) {
            size_t gStart = envBlock.find('{', gridPos);
            size_t gEnd = FindMatchingBrace(envBlock, gStart);
            std::string gBlock = envBlock.substr(gStart, gEnd - gStart + 1);

            size_t p;
            p = gBlock.find("\"show\""); if (p != std::string::npos) scene.showGrid = ParseBoolAt(gBlock, p, true);
            p = gBlock.find("\"size\""); if (p != std::string::npos) scene.gridSize = ParseFloatAt(gBlock, p, 16.0f);
            p = gBlock.find("\"divisions\""); if (p != std::string::npos) scene.gridDivisions = ParseIntAt(gBlock, p, 16);
            p = gBlock.find("\"color\""); if (p != std::string::npos) scene.gridColor = ParseVec3(gBlock, p);
        }
    }

    static void ParsePointLights(const std::string& json, Scene& scene) {
        size_t plPos = json.find("\"pointLights\"");
        if (plPos == std::string::npos) return;

        size_t arrStart = json.find('[', plPos);
        if (arrStart == std::string::npos) return;
        size_t arrEnd = FindMatchingBracket(json, arrStart);

        std::string arrBlock = json.substr(arrStart, arrEnd - arrStart + 1);

        // Find each light object
        size_t searchPos = 0;
        while (true) {
            size_t objStart = arrBlock.find('{', searchPos);
            if (objStart == std::string::npos) break;
            size_t objEnd = FindMatchingBrace(arrBlock, objStart);
            if (objEnd == std::string::npos) break;

            std::string lightBlock = arrBlock.substr(objStart, objEnd - objStart + 1);
            PointLight pl;
            size_t p;
            p = lightBlock.find("\"id\""); if (p != std::string::npos) pl.id = ParseIntAt(lightBlock, p, 0);
            p = lightBlock.find("\"name\""); if (p != std::string::npos) pl.name = ParseStringAt(lightBlock, p, "Point Light");
            p = lightBlock.find("\"position\""); if (p != std::string::npos) pl.position = ParseVec3(lightBlock, p);
            p = lightBlock.find("\"color\""); if (p != std::string::npos) pl.color = ParseVec3(lightBlock, p);
            p = lightBlock.find("\"intensity\""); if (p != std::string::npos) pl.intensity = ParseFloatAt(lightBlock, p, 2.0f);
            p = lightBlock.find("\"range\""); if (p != std::string::npos) pl.range = ParseFloatAt(lightBlock, p, 10.0f);
            p = lightBlock.find("\"enabled\""); if (p != std::string::npos) pl.enabled = ParseBoolAt(lightBlock, p, true);

            scene.pointLights.push_back(pl);
            searchPos = objEnd + 1;
        }
    }

    static void ParseActors(const std::string& json, Scene& scene) {
        size_t actorsPos = json.find("\"actors\"");
        if (actorsPos == std::string::npos) return;

        size_t arrStart = json.find('[', actorsPos);
        if (arrStart == std::string::npos) return;
        size_t arrEnd = FindMatchingBracket(json, arrStart);

        std::string arrBlock = json.substr(arrStart, arrEnd - arrStart + 1);

        // Find each actor object
        size_t searchPos = 0;
        while (true) {
            size_t objStart = arrBlock.find('{', searchPos);
            if (objStart == std::string::npos) break;
            size_t objEnd = FindMatchingBrace(arrBlock, objStart);
            if (objEnd == std::string::npos) break;

            std::string actorBlock = arrBlock.substr(objStart, objEnd - objStart + 1);

            // Parse identity
            size_t p;
            int id = 0;
            std::string name = "Object";
            PrimitiveType type = PrimitiveType::Cube;
            glm::vec3 pos(0.0f);

            p = actorBlock.find("\"id\""); if (p != std::string::npos) id = ParseIntAt(actorBlock, p, 0);
            p = actorBlock.find("\"name\""); if (p != std::string::npos) name = ParseStringAt(actorBlock, p, "Object");
            p = actorBlock.find("\"type\""); if (p != std::string::npos) type = StrToPrimitive(ParseStringAt(actorBlock, p, "Cube"));

            // Parse transform block
            size_t tfPos = actorBlock.find("\"transform\"");
            if (tfPos != std::string::npos) {
                size_t tfStart = actorBlock.find('{', tfPos);
                size_t tfEnd = FindMatchingBrace(actorBlock, tfStart);
                std::string tfBlock = actorBlock.substr(tfStart, tfEnd - tfStart + 1);

                p = tfBlock.find("\"position\""); if (p != std::string::npos) pos = ParseVec3(tfBlock, p);
            }

            // Create the object
            glm::vec3 color(0.55f);
            GameObject obj(id, name, type, pos, color);

            // Parse mobility
            p = actorBlock.find("\"mobility\""); if (p != std::string::npos) obj.mobility = StrToMobility(ParseStringAt(actorBlock, p, "Movable"));

            // Parse remaining transform
            if (tfPos != std::string::npos) {
                size_t tfStart = actorBlock.find('{', tfPos);
                size_t tfEnd = FindMatchingBrace(actorBlock, tfStart);
                std::string tfBlock = actorBlock.substr(tfStart, tfEnd - tfStart + 1);

                p = tfBlock.find("\"rotation\""); if (p != std::string::npos) obj.rotation = ParseVec3(tfBlock, p);
                p = tfBlock.find("\"scale\""); if (p != std::string::npos) obj.scale = ParseVec3(tfBlock, p);
            }

            // Parse material block
            size_t matPos = actorBlock.find("\"material\"");
            if (matPos != std::string::npos) {
                size_t matStart = actorBlock.find('{', matPos);
                size_t matEnd = FindMatchingBrace(actorBlock, matStart);
                std::string matBlock = actorBlock.substr(matStart, matEnd - matStart + 1);

                p = matBlock.find("\"materialName\""); if (p != std::string::npos) obj.materialName = ParseStringAt(matBlock, p, "Default_Material");
                p = matBlock.find("\"color\""); if (p != std::string::npos) obj.color = ParseVec3(matBlock, p);
                p = matBlock.find("\"metallic\""); if (p != std::string::npos) obj.metallic = ParseFloatAt(matBlock, p, 0.0f);
                p = matBlock.find("\"roughness\""); if (p != std::string::npos) obj.roughness = ParseFloatAt(matBlock, p, 0.5f);
                p = matBlock.find("\"normalStrength\""); if (p != std::string::npos) obj.normalStrength = ParseFloatAt(matBlock, p, 1.0f);
                p = matBlock.find("\"specular\""); if (p != std::string::npos) obj.specular = ParseFloatAt(matBlock, p, 0.5f);
                p = matBlock.find("\"emissiveColor\""); if (p != std::string::npos) obj.emissiveColor = ParseVec3(matBlock, p);
                p = matBlock.find("\"emissiveIntensity\""); if (p != std::string::npos) obj.emissiveIntensity = ParseFloatAt(matBlock, p, 0.0f);
                p = matBlock.find("\"shadingModel\""); if (p != std::string::npos) obj.shadingModel = ParseIntAt(matBlock, p, 0);
                p = matBlock.find("\"blendMode\""); if (p != std::string::npos) obj.blendMode = ParseIntAt(matBlock, p, 0);
                p = matBlock.find("\"twoSided\""); if (p != std::string::npos) obj.twoSided = ParseBoolAt(matBlock, p, false);
                p = matBlock.find("\"castShadows\""); if (p != std::string::npos) obj.castShadows = ParseBoolAt(matBlock, p, true);
                p = matBlock.find("\"receiveShadows\""); if (p != std::string::npos) obj.receiveShadows = ParseBoolAt(matBlock, p, true);

                // Textures sub-block
                size_t texPos = matBlock.find("\"textures\"");
                if (texPos != std::string::npos) {
                    size_t texStart = matBlock.find('{', texPos);
                    size_t texEnd = FindMatchingBrace(matBlock, texStart);
                    std::string texBlock = matBlock.substr(texStart, texEnd - texStart + 1);

                    p = texBlock.find("\"baseColor\""); if (p != std::string::npos) obj.baseColorTexture = ParseStringAt(texBlock, p);
                    p = texBlock.find("\"normal\""); if (p != std::string::npos) obj.normalTexture = ParseStringAt(texBlock, p);
                    p = texBlock.find("\"roughness\""); if (p != std::string::npos) obj.roughnessTexture = ParseStringAt(texBlock, p);
                    p = texBlock.find("\"metallic\""); if (p != std::string::npos) obj.metallicTexture = ParseStringAt(texBlock, p);
                    p = texBlock.find("\"ao\""); if (p != std::string::npos) obj.aoTexture = ParseStringAt(texBlock, p);
                    p = texBlock.find("\"emission\""); if (p != std::string::npos) obj.emissionTexture = ParseStringAt(texBlock, p);
                }
            }

            // Parse visibility & animation
            p = actorBlock.find("\"visible\""); if (p != std::string::npos) obj.visible = ParseBoolAt(actorBlock, p, true);
            p = actorBlock.find("\"autoRotate\""); if (p != std::string::npos) obj.autoRotate = ParseBoolAt(actorBlock, p, false);
            p = actorBlock.find("\"autoRotateSpeed\""); if (p != std::string::npos) obj.autoRotateSpeed = ParseVec3(actorBlock, p);

            // Parse hierarchy, imported mesh & light proxy
            p = actorBlock.find("\"isImportedMesh\""); if (p != std::string::npos) obj.isImportedMesh = ParseBoolAt(actorBlock, p, false);
            p = actorBlock.find("\"meshFilePath\""); if (p != std::string::npos) obj.meshFilePath = ParseStringAt(actorBlock, p, "");
            p = actorBlock.find("\"submeshIndex\""); if (p != std::string::npos) obj.submeshIndex = ParseIntAt(actorBlock, p, -1);
            p = actorBlock.find("\"parentId\""); if (p != std::string::npos) obj.parentId = ParseIntAt(actorBlock, p, -1);
            p = actorBlock.find("\"isLight\""); if (p != std::string::npos) obj.isLight = ParseBoolAt(actorBlock, p, false);
            p = actorBlock.find("\"lightId\""); if (p != std::string::npos) obj.lightId = ParseIntAt(actorBlock, p, -1);

            // Parse Primitive Parameters
            p = actorBlock.find("\"paramWidth\""); if (p != std::string::npos) obj.params.width = ParseFloatAt(actorBlock, p, 1.0f);
            p = actorBlock.find("\"paramHeight\""); if (p != std::string::npos) obj.params.height = ParseFloatAt(actorBlock, p, 1.0f);
            p = actorBlock.find("\"paramDepth\""); if (p != std::string::npos) obj.params.depth = ParseFloatAt(actorBlock, p, 1.0f);
            p = actorBlock.find("\"paramSize\""); if (p != std::string::npos) obj.params.size = ParseFloatAt(actorBlock, p, 1.0f);
            p = actorBlock.find("\"paramRadius\""); if (p != std::string::npos) obj.params.radius = ParseFloatAt(actorBlock, p, 0.5f);
            p = actorBlock.find("\"paramRadius2\""); if (p != std::string::npos) obj.params.radius2 = ParseFloatAt(actorBlock, p, 0.2f);
            p = actorBlock.find("\"paramSegX\""); if (p != std::string::npos) obj.params.segmentsX = ParseIntAt(actorBlock, p, 16);
            p = actorBlock.find("\"paramSegY\""); if (p != std::string::npos) obj.params.segmentsY = ParseIntAt(actorBlock, p, 16);
            p = actorBlock.find("\"paramSegZ\""); if (p != std::string::npos) obj.params.segmentsZ = ParseIntAt(actorBlock, p, 16);
            p = actorBlock.find("\"paramRadialSeg\""); if (p != std::string::npos) obj.params.radialSegments = ParseIntAt(actorBlock, p, 24);
            p = actorBlock.find("\"paramHeightSeg\""); if (p != std::string::npos) obj.params.heightSegments = ParseIntAt(actorBlock, p, 1);
            p = actorBlock.find("\"paramRings\""); if (p != std::string::npos) obj.params.rings = ParseIntAt(actorBlock, p, 24);
            p = actorBlock.find("\"paramSectors\""); if (p != std::string::npos) obj.params.sectors = ParseIntAt(actorBlock, p, 24);
            p = actorBlock.find("\"paramSubdiv\""); if (p != std::string::npos) obj.params.subdivisions = ParseIntAt(actorBlock, p, 2);
            p = actorBlock.find("\"paramSides\""); if (p != std::string::npos) obj.params.sides = ParseIntAt(actorBlock, p, 4);
            p = actorBlock.find("\"paramCapTop\""); if (p != std::string::npos) obj.params.capTop = ParseBoolAt(actorBlock, p, true);
            p = actorBlock.find("\"paramCapBottom\""); if (p != std::string::npos) obj.params.capBottom = ParseBoolAt(actorBlock, p, true);

            // Parse Light Properties
            p = actorBlock.find("\"lightType\""); if (p != std::string::npos) obj.light.type = (LightType)ParseIntAt(actorBlock, p, 0);
            p = actorBlock.find("\"lightEnabled\""); if (p != std::string::npos) obj.light.enabled = ParseBoolAt(actorBlock, p, true);
            p = actorBlock.find("\"lightColor\""); if (p != std::string::npos) obj.light.color = ParseVec3(actorBlock, p);
            p = actorBlock.find("\"lightIntensity\""); if (p != std::string::npos) obj.light.intensity = ParseFloatAt(actorBlock, p, 2.0f);
            p = actorBlock.find("\"lightTemperature\""); if (p != std::string::npos) obj.light.temperature = ParseFloatAt(actorBlock, p, 6500.0f);
            p = actorBlock.find("\"lightUseTemp\""); if (p != std::string::npos) obj.light.useTemperature = ParseBoolAt(actorBlock, p, false);
            p = actorBlock.find("\"lightRange\""); if (p != std::string::npos) obj.light.range = ParseFloatAt(actorBlock, p, 10.0f);
            p = actorBlock.find("\"lightAttenuation\""); if (p != std::string::npos) obj.light.attenuation = ParseFloatAt(actorBlock, p, 2.0f);
            p = actorBlock.find("\"lightInnerCone\""); if (p != std::string::npos) obj.light.innerConeAngle = ParseFloatAt(actorBlock, p, 20.0f);
            p = actorBlock.find("\"lightOuterCone\""); if (p != std::string::npos) obj.light.outerConeAngle = ParseFloatAt(actorBlock, p, 45.0f);
            p = actorBlock.find("\"lightConeFalloff\""); if (p != std::string::npos) obj.light.coneFalloff = ParseFloatAt(actorBlock, p, 1.0f);
            p = actorBlock.find("\"lightAreaShape\""); if (p != std::string::npos) obj.light.areaShape = ParseIntAt(actorBlock, p, 0);
            p = actorBlock.find("\"lightAreaWidth\""); if (p != std::string::npos) obj.light.width = ParseFloatAt(actorBlock, p, 1.0f);
            p = actorBlock.find("\"lightAreaHeight\""); if (p != std::string::npos) obj.light.height = ParseFloatAt(actorBlock, p, 1.0f);
            p = actorBlock.find("\"lightAreaRadius\""); if (p != std::string::npos) obj.light.radius = ParseFloatAt(actorBlock, p, 0.5f);
            p = actorBlock.find("\"lightAreaLength\""); if (p != std::string::npos) obj.light.length = ParseFloatAt(actorBlock, p, 1.0f);
            p = actorBlock.find("\"lightTwoSided\""); if (p != std::string::npos) obj.light.twoSided = ParseBoolAt(actorBlock, p, false);
            p = actorBlock.find("\"lightSkyColor\""); if (p != std::string::npos) obj.light.skyColor = ParseVec3(actorBlock, p);
            p = actorBlock.find("\"lightGroundColor\""); if (p != std::string::npos) obj.light.groundColor = ParseVec3(actorBlock, p);
            p = actorBlock.find("\"lightEnvMap\""); if (p != std::string::npos) obj.light.envMapTexture = ParseStringAt(actorBlock, p, "");
            p = actorBlock.find("\"lightCastShadows\""); if (p != std::string::npos) obj.light.castShadows = ParseBoolAt(actorBlock, p, true);
            p = actorBlock.find("\"lightShadowStrength\""); if (p != std::string::npos) obj.light.shadowStrength = ParseFloatAt(actorBlock, p, 0.85f);
            p = actorBlock.find("\"lightShadowBias\""); if (p != std::string::npos) obj.light.shadowBias = ParseFloatAt(actorBlock, p, 0.0012f);
            p = actorBlock.find("\"lightShadowRes\""); if (p != std::string::npos) obj.light.shadowResolution = ParseIntAt(actorBlock, p, 2048);
            p = actorBlock.find("\"lightShadowDist\""); if (p != std::string::npos) obj.light.shadowDistance = ParseFloatAt(actorBlock, p, 50.0f);
            p = actorBlock.find("\"lightVolumetric\""); if (p != std::string::npos) obj.light.volumetric = ParseBoolAt(actorBlock, p, false);
            p = actorBlock.find("\"lightVolScattering\""); if (p != std::string::npos) obj.light.volumetricScattering = ParseFloatAt(actorBlock, p, 0.2f);
            p = actorBlock.find("\"lightVolIntensity\""); if (p != std::string::npos) obj.light.volumetricIntensity = ParseFloatAt(actorBlock, p, 1.0f);
            p = actorBlock.find("\"lightLayer\""); if (p != std::string::npos) obj.light.lightLayer = (uint32_t)ParseIntAt(actorBlock, p, 1);

            // Rebuild mesh geometry with loaded params for standard primitives
            if (!obj.isImportedMesh && !obj.isLight && !IsLightPrimitive(obj.type) && obj.type != PrimitiveType::Empty) {
                obj.RebuildMesh();
            }

            // Restore mesh geometry for imported meshes, or empty mesh for empty/light actors
            if (obj.isImportedMesh && !obj.meshFilePath.empty()) {
                ImportedModel model;
                try { model = MeshImporter::Load(obj.meshFilePath); } catch (...) {}
                if (model.valid && !model.meshes.empty()) {
                    if (obj.submeshIndex >= 0 && obj.submeshIndex < (int)model.meshes.size()) {
                        obj.mesh.vertices = model.meshes[obj.submeshIndex].vertices;
                        obj.mesh.indices  = model.meshes[obj.submeshIndex].indices;

                        glm::vec3 minBound(1e9f);
                        glm::vec3 maxBound(-1e9f);
                        for (const auto& v : obj.mesh.vertices) {
                            if (!std::isnan(v.pos.x) && !std::isnan(v.pos.y) && !std::isnan(v.pos.z) &&
                                !std::isinf(v.pos.x) && !std::isinf(v.pos.y) && !std::isinf(v.pos.z)) {
                                minBound = glm::min(minBound, v.pos);
                                maxBound = glm::max(maxBound, v.pos);
                            }
                        }
                        glm::vec3 pivot = (minBound + maxBound) * 0.5f;
                        uint32_t vCount = (uint32_t)obj.mesh.vertices.size();
                        for (uint32_t& idx : obj.mesh.indices) { if (idx >= vCount) idx = 0; }
                        for (auto& v : obj.mesh.vertices) {
                            if (std::isnan(v.pos.x)||std::isnan(v.pos.y)||std::isnan(v.pos.z)||
                                std::isinf(v.pos.x)||std::isinf(v.pos.y)||std::isinf(v.pos.z)) {
                                v.pos = glm::vec3(0.0f);
                            } else {
                                v.pos -= pivot;
                            }
                        }
                        if (obj.parentId != -1 && glm::length(obj.position) < 0.0001f && glm::length(pivot) > 0.0001f) {
                            obj.position = pivot;
                        }
                    } else if (model.meshes.size() == 1) {
                        if (!model.meshes[0].vertices.empty()) {
                            obj.mesh.vertices = model.meshes[0].vertices;
                            obj.mesh.indices  = model.meshes[0].indices;
                        } else {
                            obj.mesh = model.GetMergedMesh();
                        }
                        glm::vec3 minBound(1e9f);
                        glm::vec3 maxBound(-1e9f);
                        for (const auto& v : obj.mesh.vertices) {
                            if (!std::isnan(v.pos.x) && !std::isnan(v.pos.y) && !std::isnan(v.pos.z) &&
                                !std::isinf(v.pos.x) && !std::isinf(v.pos.y) && !std::isinf(v.pos.z)) {
                                minBound = glm::min(minBound, v.pos);
                                maxBound = glm::max(maxBound, v.pos);
                            }
                        }
                        glm::vec3 pivot = (minBound + maxBound) * 0.5f;
                        uint32_t vCount = (uint32_t)obj.mesh.vertices.size();
                        for (uint32_t& idx : obj.mesh.indices) { if (idx >= vCount) idx = 0; }
                        for (auto& v : obj.mesh.vertices) {
                            if (std::isnan(v.pos.x)||std::isnan(v.pos.y)||std::isnan(v.pos.z)||
                                std::isinf(v.pos.x)||std::isinf(v.pos.y)||std::isinf(v.pos.z)) {
                                v.pos = glm::vec3(0.0f);
                            } else {
                                v.pos -= pivot;
                            }
                        }
                        if (obj.parentId != -1 && glm::length(obj.position) < 0.0001f && glm::length(pivot) > 0.0001f) {
                            obj.position = pivot;
                        }
                    } else {
                        // submeshIndex == -1 with multiple meshes: parent/root node, keeps empty geometry
                        obj.mesh.vertices.clear();
                        obj.mesh.indices.clear();
                    }
                }
            } else if (obj.type == PrimitiveType::Empty || obj.isLight || IsLightPrimitive(obj.type)) {
                obj.isLight = (obj.type != PrimitiveType::Empty);
                obj.mesh.vertices.clear();
                obj.mesh.indices.clear();
            }

            scene.objects.push_back(obj);
            searchPos = objEnd + 1;
        }

        // Reconstruct childIds on all objects from parentId
        for (auto& o : scene.objects) {
            o.childIds.clear();
        }
        for (const auto& o : scene.objects) {
            if (o.parentId != -1) {
                GameObject* parent = scene.FindObject(o.parentId);
                if (parent) {
                    parent->childIds.push_back(o.id);
                }
            }
        }
        scene.SyncLightActors();
    }

    // ---- Brace/Bracket Matching ----

    static size_t FindMatchingBrace(const std::string& json, size_t openPos) {
        if (openPos >= json.size() || json[openPos] != '{') return std::string::npos;
        int depth = 1;
        bool inString = false;
        for (size_t i = openPos + 1; i < json.size(); ++i) {
            if (json[i] == '\\' && inString) { i++; continue; }
            if (json[i] == '"') inString = !inString;
            if (!inString) {
                if (json[i] == '{') depth++;
                else if (json[i] == '}') { depth--; if (depth == 0) return i; }
            }
        }
        return std::string::npos;
    }

    static size_t FindMatchingBracket(const std::string& json, size_t openPos) {
        if (openPos >= json.size() || json[openPos] != '[') return std::string::npos;
        int depth = 1;
        bool inString = false;
        for (size_t i = openPos + 1; i < json.size(); ++i) {
            if (json[i] == '\\' && inString) { i++; continue; }
            if (json[i] == '"') inString = !inString;
            if (!inString) {
                if (json[i] == '[') depth++;
                else if (json[i] == ']') { depth--; if (depth == 0) return i; }
            }
        }
        return std::string::npos;
    }

    static bool LoadLevel(Scene& level, const std::string& filePath) {
        return LoadScene(level, filePath);
    }
};

using LevelSerializer = SceneSerializer;
