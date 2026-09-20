#pragma once
// ============================================================================
// EngineRHI::PipelineState — HLSL shader compilation and PSO creation helpers
// ============================================================================

#include <d3d12.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <string>

namespace EngineRHI {

using Microsoft::WRL::ComPtr;

struct ShaderDesc {
    std::string source;       // HLSL source code as string (or empty if from file)
    std::string filePath;     // path to .hlsl file (used when source is empty)
    std::string entryPoint;
    std::string profile;      // e.g. "vs_5_1", "ps_5_1"
};

/// Compile a shader from source or file. Returns null on failure.
ComPtr<ID3DBlob> CompileShader(const ShaderDesc& desc, std::string& outError);

/// Helper: create a root signature from a serialized blob.
ComPtr<ID3D12RootSignature> CreateRootSignature(
    ID3D12Device* device,
    const void*   data,
    size_t        size
);

} // namespace EngineRHI
