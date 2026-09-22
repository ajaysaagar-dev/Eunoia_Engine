#include <EngineRHI/PipelineState.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace EngineRHI {

ComPtr<ID3DBlob> CompileShader(const ShaderDesc& desc, std::string& outError) {
    outError.clear();

    std::string sourceCode = desc.source;
    if (sourceCode.empty() && !desc.filePath.empty()) {
        std::ifstream file(desc.filePath);
        if (!file.is_open()) {
            outError = "Failed to open shader file: " + desc.filePath;
            return nullptr;
        }
        std::stringstream ss;
        ss << file.rdbuf();
        sourceCode = ss.str();
    }

    if (sourceCode.empty()) {
        outError = "Shader source code is empty.";
        return nullptr;
    }

    // Shader Caching (dev.md Priority 5)
    // Check if cached .cso exists and is newer than source file
    std::filesystem::path csoPath;
    if (!desc.filePath.empty()) {
        csoPath = desc.filePath + "." + desc.entryPoint + ".cso";
        std::error_code ec;
        if (std::filesystem::exists(csoPath, ec) && std::filesystem::exists(desc.filePath, ec)) {
            auto csoTime = std::filesystem::last_write_time(csoPath, ec);
            auto srcTime = std::filesystem::last_write_time(desc.filePath, ec);
            if (csoTime >= srcTime) {
                // Load cached CSO
                std::ifstream csoFile(csoPath, std::ios::binary | std::ios::ate);
                if (csoFile.is_open()) {
                    size_t size = static_cast<size_t>(csoFile.tellg());
                    csoFile.seekg(0, std::ios::beg);
                    ComPtr<ID3DBlob> csoBlob;
                    if (SUCCEEDED(D3DCreateBlob(size, &csoBlob))) {
                        csoFile.read(reinterpret_cast<char*>(csoBlob->GetBufferPointer()), size);
                        return csoBlob;
                    }
                }
            }
        }
    }

    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;
    UINT compileFlags = 0;
#if defined(_DEBUG) || defined(EUNOIA_D3D12_DEBUG)
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    const char* sourceName = desc.filePath.empty() ? "Shader" : desc.filePath.c_str();

    HRESULT hr = D3DCompile(
        sourceCode.c_str(),
        sourceCode.length(),
        sourceName,
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        desc.entryPoint.c_str(),
        desc.profile.c_str(),
        compileFlags,
        0,
        &shaderBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            outError = reinterpret_cast<const char*>(errorBlob->GetBufferPointer());
        } else {
            outError = "Shader compilation failed with HRESULT: " + std::to_string(hr);
        }
        return nullptr;
    }

    // Write to CSO cache if file path is available
    if (!csoPath.empty() && shaderBlob) {
        std::ofstream csoOut(csoPath, std::ios::binary);
        if (csoOut.is_open()) {
            csoOut.write(reinterpret_cast<const char*>(shaderBlob->GetBufferPointer()), shaderBlob->GetBufferSize());
            csoOut.close();
        }
    }

    return shaderBlob;
}

ComPtr<ID3D12RootSignature> CreateRootSignature(
    ID3D12Device* device,
    const void*   data,
    size_t        size
) {
    if (!device || !data || size == 0) return nullptr;

    ComPtr<ID3D12RootSignature> rootSig;
    HRESULT hr = device->CreateRootSignature(0, data, size, IID_PPV_ARGS(&rootSig));
    if (FAILED(hr)) {
        std::cerr << "[EngineRHI] Failed to create root signature: " << hr << std::endl;
        return nullptr;
    }

    return rootSig;
}

} // namespace EngineRHI
