#pragma once
// ============================================================================
// EngineRenderer::MaterialSystem — forwarding header
// Material table creation / lookup (was GetOrCreateMaterialTable in Cube.cpp).
// ============================================================================

#include <string>
#include <unordered_map>

namespace EngineRenderer {

/// Opaque handle to a material table entry (GPU descriptor table index)
using MaterialTableIndex = uint32_t;
constexpr MaterialTableIndex kInvalidMaterialTable = UINT32_MAX;

// Forward declaration; implemented in EngineRenderer/src/MaterialSystem.cpp
class MaterialSystem {
public:
    static MaterialSystem& Get();

    MaterialTableIndex GetOrCreate(const std::string& materialName);
    bool               Contains(const std::string& materialName) const;
    void               Clear();

private:
    std::unordered_map<std::string, MaterialTableIndex> m_table;
    MaterialTableIndex m_nextIndex = 0;
};

} // namespace EngineRenderer
