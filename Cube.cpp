#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <cstdint>
#include <unordered_map>

#include "Geometry.h"
#include "GameObject.h"
#include "Level.h"
#include "LevelSerializer.h"
#include "Camera.h"
#include "EngineUI.h"
#include "TextureManager.h"
#include "AssetSystem.h"
#include "MeshImporter.h"
#include "InputSystem.h"

extern "C" {
    unsigned char *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);
    void stbi_image_free(void *retval_from_stbi_load);
}

#include "imgui.h"
#include "ImGuizmo.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_dx12.h"

// Configuration
const int WIDTH = 1280;
const int HEIGHT = 720;
const UINT FRAME_COUNT = 2;

// Real-Time Shadow Mapping Configuration (2048x2048 D32_FLOAT PCF)
const UINT SHADOW_MAP_WIDTH = 2048;
const UINT SHADOW_MAP_HEIGHT = 2048;

// HLSL Constant Buffer struct (must be 256-byte aligned in D3D12)
struct alignas(256) SceneConstantBuffer
{
	glm::mat4 mvp;
	glm::mat4 lightSpaceMatrix;
	glm::vec3 cameraPos;
	float ambientIntensity;
	glm::vec3 lightDir;
	float shadowBias;
	glm::vec3 lightColor;
	float shadowStrength;
	float pcfRadius;
	float enableShadows;
	float shadowMapSize;
	float numPointLights;
	glm::vec4 pointLightPosRange[4];
	glm::vec4 pointLightColorIntensity[4];
};

struct alignas(256) ShadowConstantBuffer
{
	glm::mat4 lightSpaceMatrix;
};

struct MaterialShaderConstants
{
	float baseColor[3];
	float metallic;
	float roughness;
	float normalStrength;
	float specular;
	float emissiveIntensity;
	float emissiveColor[3];
	float isUnlit;
	float hasAlbedoTex;
	float hasNormalTex;
	float hasRoughTex;
	float hasAoTex;
	float receiveShadows;
	float pad0;
	float pad1;
	float pad2;
};

// Window & GLFW
static GLFWwindow* g_window = nullptr;
static int g_currentWidth = WIDTH;
static int g_currentHeight = HEIGHT;

// DirectX 12 Core Objects
static IDXGIFactory4* g_dxgiFactory = nullptr;
static ID3D12Device* g_d3dDevice = nullptr;
static ID3D12CommandQueue* g_commandQueue = nullptr;
static IDXGISwapChain3* g_swapChain = nullptr;
static ID3D12DescriptorHeap* g_rtvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_dsvDescHeap = nullptr;
static ID3D12DescriptorHeap* g_srvDescHeap = nullptr;
static UINT g_rtvDescriptorSize = 0;
static UINT g_srvDescriptorSize = 0;

static ID3D12Resource* g_renderTargets[FRAME_COUNT] = {};
static ID3D12CommandAllocator* g_commandAllocators[FRAME_COUNT] = {};
static ID3D12GraphicsCommandList* g_commandList = nullptr;

static ID3D12Resource* g_depthStencilBuffer = nullptr;

// Synchronization
static UINT g_frameIndex = 0;
static HANDLE g_fenceEvent = nullptr;
static ID3D12Fence* g_fence = nullptr;
static UINT64 g_fenceValues[FRAME_COUNT] = {};

// Pipeline & Shaders
static ID3D12RootSignature* g_rootSignature = nullptr;
static ID3D12PipelineState* g_pipelineState = nullptr;

// Dynamic Scene Buffers
const size_t MAX_SCENE_VERTICES = 200000;
const size_t MAX_SCENE_INDICES  = 600000;

static ID3D12Resource* g_vertexBuffer = nullptr;
static ID3D12Resource* g_indexBuffer = nullptr;
static ID3D12Resource* g_constantBuffer = nullptr;

static D3D12_VERTEX_BUFFER_VIEW g_vertexBufferView = {};
static D3D12_INDEX_BUFFER_VIEW  g_indexBufferView = {};

static void* g_pVertexMapped = nullptr;
static void* g_pIndexMapped = nullptr;
static void* g_pConstantMapped = nullptr;

static uint32_t g_currentVertexCount = 0;
static uint32_t g_currentIndexCount = 0;

// ImGui & Engine SRV allocation tracking
static const UINT SRV_HEAP_CAPACITY = 8192; // Must match srvHeapDesc.NumDescriptors in initD3D12
static UINT g_srvDescriptorAllocIndex = 0;
static bool g_deviceLost = false;

// Game Engine State
static Scene g_scene;
static OrbitCamera g_camera;
static EngineUI g_engineUI;
static std::vector<Scene::RenderBatch> g_sceneBatches;

struct DX12GpuTexture {
	ID3D12Resource* resource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle{};
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle{};
	int width = 0;
	int height = 0;
};

static std::vector<ID3D12Resource*> g_allocatedTextures;
static std::unordered_map<std::string, DX12GpuTexture> g_gpuTextureMap;
static std::unordered_map<std::string, D3D12_GPU_DESCRIPTOR_HANDLE> g_materialDescriptorTables;

static DX12GpuTexture g_fallbackWhite;
static DX12GpuTexture g_fallbackNormal;
static DX12GpuTexture g_fallbackRoughness;
static DX12GpuTexture g_fallbackMetallic;
static DX12GpuTexture g_fallbackAO;
static DX12GpuTexture g_fallbackMissing;

// Real-Time Directional Shadow Resources
static ID3D12Resource* g_shadowDepthBuffer = nullptr;
static D3D12_CPU_DESCRIPTOR_HANDLE g_shadowDsvHandle = {};
static D3D12_CPU_DESCRIPTOR_HANDLE g_shadowSrvCpuHandle = {};
static D3D12_GPU_DESCRIPTOR_HANDLE g_shadowSrvGpuHandle = {};

static ID3D12RootSignature* g_shadowRootSignature = nullptr;
static ID3D12PipelineState* g_shadowPipelineState = nullptr;

static ID3D12Resource* g_shadowConstantBuffer = nullptr;
static void* g_pShadowConstantMapped = nullptr;

static UINT g_dsvDescriptorSize = 0;

// Dedicated Upload Fence & Allocator (avoids frame fence collision)
static ID3D12CommandAllocator* g_uploadCmdAlloc = nullptr;
static ID3D12GraphicsCommandList* g_uploadCmdList = nullptr;
static ID3D12Fence* g_uploadFence = nullptr;
static HANDLE g_uploadFenceEvent = nullptr;
static UINT64 g_uploadFenceValue = 0;

// Mouse tracking
static bool s_isLeftMouseDown = false;
static bool s_isRightMouseDown = false;
static bool s_isMiddleMouseDown = false;
static bool s_firstMouseAfterCapture = true;
static double s_lastMouseX = 0.0, s_lastMouseY = 0.0;

// D3D12 Helper Functions
inline D3D12_RESOURCE_BARRIER CreateTransitionBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after)
{
	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = resource;
	barrier.Transition.StateBefore = before;
	barrier.Transition.StateAfter = after;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	return barrier;
}

inline D3D12_HEAP_PROPERTIES CreateHeapProperties(D3D12_HEAP_TYPE type)
{
	D3D12_HEAP_PROPERTIES props = {};
	props.Type = type;
	props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	props.CreationNodeMask = 1;
	props.VisibleNodeMask = 1;
	return props;
}

inline D3D12_RESOURCE_DESC CreateBufferResourceDesc(UINT64 size)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = 0;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;
	return desc;
}

// Returns false (and logs once) if the heap is full instead of writing OOB.
bool TryAllocSrvDescriptors(UINT count, UINT& outBaseIndex)
{
	if (g_srvDescriptorAllocIndex + count > SRV_HEAP_CAPACITY)
	{
		static bool warned = false;
		if (!warned)
		{
			std::cerr << "[SRV HEAP] Exhausted (" << SRV_HEAP_CAPACITY
			          << " descriptors). Further textures/materials will use fallbacks.\n";
			g_engineUI.AddLog("LogRecovery",
				"SRV descriptor heap is full — new textures/materials will show as fallback until you restart or free assets.", 2);
			warned = true;
		}
		return false;
	}
	outBaseIndex = g_srvDescriptorAllocIndex;
	g_srvDescriptorAllocIndex += count;
	return true;
}

// ImGui SRV Descriptor Allocator Callbacks
static void ImGui_SrvAlloc(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu)
{
	UINT idx = 0;
	if (!TryAllocSrvDescriptors(1, idx))
	{
		// Safe fallback: return heap slot 0
		*out_cpu = info->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		*out_gpu = info->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		return;
	}
	D3D12_CPU_DESCRIPTOR_HANDLE cpu = info->SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpu = info->SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	cpu.ptr += (SIZE_T)idx * g_srvDescriptorSize;
	gpu.ptr += (UINT64)idx * g_srvDescriptorSize;
	*out_cpu = cpu;
	*out_gpu = gpu;
}

static void ImGui_SrvFree(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu)
{
	// Linear allocator - no-op
}

bool SafeWaitForFence(ID3D12Fence* fence, UINT64 targetValue, HANDLE eventHandle, DWORD timeoutMs = 2000, const char* context = "GPU Fence")
{
	if (!fence || !eventHandle) return true;

	if (fence->GetCompletedValue() >= targetValue) return true;

	fence->SetEventOnCompletion(targetValue, eventHandle);
	DWORD waitRes = WaitForSingleObject(eventHandle, timeoutMs);

	if (waitRes == WAIT_TIMEOUT)
	{
		std::cerr << "[RECOVERY] Engine detected stall at " << context << " (target fence: " << targetValue
		          << ", completed: " << fence->GetCompletedValue() << ", timeout: " << timeoutMs << "ms)!\n";

		HRESULT removedReason = g_d3dDevice ? g_d3dDevice->GetDeviceRemovedReason() : S_OK;
		if (FAILED(removedReason))
		{
			g_deviceLost = true;
			std::cerr << "[RECOVERY] D3D12 Device Removed reason: 0x" << std::hex << removedReason << std::dec << "\n";
			g_engineUI.AddLog("LogRecovery", "GPU Device Lost/Removed detected (0x" + std::to_string(removedReason) + "). Please save work if possible and restart editor.", 3);
		}
		else
		{
			g_engineUI.AddLog("LogRecovery", std::string("Engine stall recovered at ") + context + " (GPU took >" + std::to_string(timeoutMs) + "ms). Resumed.", 1);
		}

		// Advance fence to break out of stuck state
		fence->Signal(targetValue);
		return false;
	}

	return true;
}

void WaitForGpuIdle()
{
	if (!g_commandQueue || !g_fence || !g_fenceEvent) return;
	const UINT64 fence = g_fenceValues[g_frameIndex] + 1;
	g_commandQueue->Signal(g_fence, fence);
	SafeWaitForFence(g_fence, fence, g_fenceEvent, 2000, "WaitForGpuIdle");
	for (UINT i = 0; i < FRAME_COUNT; i++)
	{
		g_fenceValues[i] = fence;
	}
}

DX12GpuTexture UploadTextureToD3D12(const void* pixels, int width, int height)
{
	DX12GpuTexture result = {};
	if (!pixels || width <= 0 || height <= 0 || !g_d3dDevice || !g_uploadCmdAlloc || !g_uploadCmdList || !g_uploadFence) return result;

	UINT rowPitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
	UINT64 uploadSize = (UINT64)rowPitch * height;

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_HEAP_PROPERTIES defHeap = CreateHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
	ID3D12Resource* tex = nullptr;
	HRESULT hr = g_d3dDevice->CreateCommittedResource(&defHeap, D3D12_HEAP_FLAG_NONE, &texDesc,
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&tex));
	if (FAILED(hr)) return result;

	D3D12_RESOURCE_DESC upDesc = CreateBufferResourceDesc(uploadSize);
	D3D12_HEAP_PROPERTIES upHeap = CreateHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	ID3D12Resource* uploadBuf = nullptr;
	hr = g_d3dDevice->CreateCommittedResource(&upHeap, D3D12_HEAP_FLAG_NONE, &upDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadBuf));
	if (FAILED(hr)) {
		tex->Release();
		return result;
	}

	void* pData = nullptr;
	uploadBuf->Map(0, nullptr, &pData);
	for (int y = 0; y < height; ++y) {
		memcpy((uint8_t*)pData + y * rowPitch, (const uint8_t*)pixels + y * width * 4, width * 4);
	}
	uploadBuf->Unmap(0, nullptr);

	g_uploadCmdAlloc->Reset();
	g_uploadCmdList->Reset(g_uploadCmdAlloc, nullptr);

	D3D12_TEXTURE_COPY_LOCATION dst = {};
	dst.pResource = tex;
	dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dst.SubresourceIndex = 0;

	D3D12_TEXTURE_COPY_LOCATION src = {};
	src.pResource = uploadBuf;
	src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	src.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	src.PlacedFootprint.Footprint.Width = width;
	src.PlacedFootprint.Footprint.Height = height;
	src.PlacedFootprint.Footprint.Depth = 1;
	src.PlacedFootprint.Footprint.RowPitch = rowPitch;

	g_uploadCmdList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);

	D3D12_RESOURCE_BARRIER b = CreateTransitionBarrier(tex, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	g_uploadCmdList->ResourceBarrier(1, &b);

	g_uploadCmdList->Close();
	ID3D12CommandList* lists[] = { g_uploadCmdList };
	g_commandQueue->ExecuteCommandLists(1, lists);

	// Dedicated upload fence synchronization (does not interfere with frame fence)
	g_uploadFenceValue++;
	g_commandQueue->Signal(g_uploadFence, g_uploadFenceValue);
	SafeWaitForFence(g_uploadFence, g_uploadFenceValue, g_uploadFenceEvent, 3000, "UploadTexture");

	uploadBuf->Release();

	UINT descIdx = 0;
	if (!TryAllocSrvDescriptors(1, descIdx))
	{
		tex->Release();
		return result; // Returns empty DX12GpuTexture so caller falls back safely
	}
	D3D12_CPU_DESCRIPTOR_HANDLE cpu = g_srvDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE gpu = g_srvDescHeap->GetGPUDescriptorHandleForHeapStart();
	cpu.ptr += (SIZE_T)descIdx * g_srvDescriptorSize;
	gpu.ptr += (UINT64)descIdx * g_srvDescriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	g_d3dDevice->CreateShaderResourceView(tex, &srvDesc, cpu);

	result.resource = tex;
	result.cpuHandle = cpu;
	result.gpuHandle = gpu;
	result.width = width;
	result.height = height;

	g_allocatedTextures.push_back(tex);
	return result;
}

void InitFallbackTextures()
{
	uint32_t whitePix = 0xFFFFFFFF;
	g_fallbackWhite = UploadTextureToD3D12(&whitePix, 1, 1);

	uint8_t normalPix[4] = { 128, 128, 255, 255 };
	g_fallbackNormal = UploadTextureToD3D12(normalPix, 1, 1);

	uint32_t roughPix = 0xFFFFFFFF;
	g_fallbackRoughness = UploadTextureToD3D12(&roughPix, 1, 1);

	uint32_t metalPix = 0xFF000000;
	g_fallbackMetallic = UploadTextureToD3D12(&metalPix, 1, 1);

	uint32_t aoPix = 0xFFFFFFFF;
	g_fallbackAO = UploadTextureToD3D12(&aoPix, 1, 1);

	// Missing texture checkerboard fallback from dev.md Section 20
	CachedTexture* checker = AssetManager::Get().GetMissingTextureFallback();
	if (checker && !checker->data.empty() && checker->width > 0 && checker->height > 0) {
		g_fallbackMissing = UploadTextureToD3D12(checker->data.data(), checker->width, checker->height);
	} else {
		g_fallbackMissing = g_fallbackWhite;
	}
}

DX12GpuTexture GetOrLoadGPUTexture(const std::string& name, const DX12GpuTexture& fallback)
{
	if (name.empty() || name == "none") return fallback;

	std::string resolved = name;
	const AssetMetadata* meta = AssetRegistry::Get().FindByVirtualPath(name);
	if (!meta) {
		AssetID id = AssetID::FromString(name);
		if (id.IsValid()) meta = AssetRegistry::Get().FindById(id);
	}
	if (meta && !meta->sourcePath.empty()) {
		resolved = meta->sourcePath;
	} else {
		std::string tmResolved = TextureManager::Get().ResolvePath(name);
		if (!tmResolved.empty()) resolved = tmResolved;
	}

	auto it = g_gpuTextureMap.find(resolved);
	if (it != g_gpuTextureMap.end()) {
		return it->second;
	}

	// Single-source decoded pixels from TextureManager cache (zero duplicate disk reads)
	CachedTexture* cached = TextureManager::Get().GetTexture(resolved);
	if (!cached || !cached->valid || cached->data.empty() || cached->width <= 0 || cached->height <= 0) {
		return fallback;
	}

	DX12GpuTexture tex = UploadTextureToD3D12(cached->data.data(), cached->width, cached->height);
	if (tex.resource) {
		g_gpuTextureMap[resolved] = tex;
		return tex;
	}
	return fallback;
}

D3D12_GPU_DESCRIPTOR_HANDLE GetOrCreateMaterialTable(
	const std::string& albedo,
	const std::string& normal,
	const std::string& rough,
	const std::string& metal,
	const std::string& ao)
{
	std::string key = albedo + "|" + normal + "|" + rough + "|" + metal + "|" + ao;
	auto it = g_materialDescriptorTables.find(key);
	if (it != g_materialDescriptorTables.end()) {
		return it->second;
	}

	DX12GpuTexture texAlbedo = (!albedo.empty() && albedo != "none")
		? GetOrLoadGPUTexture(albedo, g_fallbackMissing)
		: g_fallbackWhite;
	DX12GpuTexture texNormal = GetOrLoadGPUTexture(normal, g_fallbackNormal);
	DX12GpuTexture texRough  = GetOrLoadGPUTexture(rough,  g_fallbackRoughness);
	DX12GpuTexture texMetal  = GetOrLoadGPUTexture(metal,  g_fallbackMetallic);
	DX12GpuTexture texAO     = GetOrLoadGPUTexture(ao,     g_fallbackAO);

	static D3D12_GPU_DESCRIPTOR_HANDLE s_fallbackTable = {};
	static bool s_hasFallbackTable = false;

	UINT baseIdx = 0;
	if (!TryAllocSrvDescriptors(5, baseIdx))
	{
		if (s_hasFallbackTable) {
			return s_fallbackTable;
		}
		return g_srvDescHeap ? g_srvDescHeap->GetGPUDescriptorHandleForHeapStart() : D3D12_GPU_DESCRIPTOR_HANDLE{};
	}

	D3D12_CPU_DESCRIPTOR_HANDLE baseCpu = g_srvDescHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE baseGpu = g_srvDescHeap->GetGPUDescriptorHandleForHeapStart();
	baseCpu.ptr += (SIZE_T)baseIdx * g_srvDescriptorSize;
	baseGpu.ptr += (UINT64)baseIdx * g_srvDescriptorSize;

	DX12GpuTexture slots[5] = { texAlbedo, texNormal, texRough, texMetal, texAO };
	for (int i = 0; i < 5; ++i) {
		D3D12_CPU_DESCRIPTOR_HANDLE dst = baseCpu;
		dst.ptr += (SIZE_T)i * g_srvDescriptorSize;
		g_d3dDevice->CopyDescriptorsSimple(1, dst, slots[i].cpuHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	if (!s_hasFallbackTable) {
		s_fallbackTable = baseGpu;
		s_hasFallbackTable = true;
	}

	g_materialDescriptorTables[key] = baseGpu;
	return baseGpu;
}

// Pre-upload all textures and material descriptor tables needed by current scene batches
// Runs BEFORE renderFrame() resets the frame command list, eliminating GPU command queue deadlocks
void PreRenderUploadTextures()
{
	for (const auto& batch : g_sceneBatches)
	{
		if (batch.indexCount == 0) continue;
		GetOrCreateMaterialTable(
			batch.albedoTex, batch.normalTex, batch.roughTex, batch.metalTex, batch.aoTex);
	}
}

int createDepthStencilView(int width, int height)
{
	if (g_depthStencilBuffer)
	{
		g_depthStencilBuffer->Release();
		g_depthStencilBuffer = nullptr;
	}

	D3D12_RESOURCE_DESC depthDesc = {};
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Alignment = 0;
	depthDesc.Width = (UINT64)width;
	depthDesc.Height = (UINT)height;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClear = {};
	depthClear.Format = DXGI_FORMAT_D32_FLOAT;
	depthClear.DepthStencil.Depth = 1.0f;
	depthClear.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES heapProps = CreateHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = g_d3dDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClear,
		IID_PPV_ARGS(&g_depthStencilBuffer)
	);
	if (FAILED(hr))
	{
		std::cerr << "Failed to create D3D12 depth stencil buffer: " << hr << std::endl;
		return EXIT_FAILURE;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = g_dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
	g_d3dDevice->CreateDepthStencilView(g_depthStencilBuffer, &dsvDesc, dsvHandle);

	return EXIT_SUCCESS;
}

int createShadowResources()
{
	if (g_shadowDepthBuffer)
	{
		g_shadowDepthBuffer->Release();
		g_shadowDepthBuffer = nullptr;
	}

	D3D12_RESOURCE_DESC depthDesc = {};
	depthDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthDesc.Alignment = 0;
	depthDesc.Width = SHADOW_MAP_WIDTH;
	depthDesc.Height = SHADOW_MAP_HEIGHT;
	depthDesc.DepthOrArraySize = 1;
	depthDesc.MipLevels = 1;
	depthDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthClear = {};
	depthClear.Format = DXGI_FORMAT_D32_FLOAT;
	depthClear.DepthStencil.Depth = 1.0f;
	depthClear.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES heapProps = CreateHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

	HRESULT hr = g_d3dDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&depthClear,
		IID_PPV_ARGS(&g_shadowDepthBuffer)
	);
	if (FAILED(hr))
	{
		std::cerr << "Failed to create D3D12 shadow depth buffer: " << hr << std::endl;
		return EXIT_FAILURE;
	}

	// Create DSV at index 1 of g_dsvDescHeap
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	g_shadowDsvHandle = g_dsvDescHeap->GetCPUDescriptorHandleForHeapStart();
	g_shadowDsvHandle.ptr += (SIZE_T)1 * g_dsvDescriptorSize;
	g_d3dDevice->CreateDepthStencilView(g_shadowDepthBuffer, &dsvDesc, g_shadowDsvHandle);

	// Create SRV in g_srvDescHeap for shadow map
	UINT descIdx = 0;
	if (!TryAllocSrvDescriptors(1, descIdx))
	{
		descIdx = 0;
	}
	g_shadowSrvCpuHandle = g_srvDescHeap->GetCPUDescriptorHandleForHeapStart();
	g_shadowSrvGpuHandle = g_srvDescHeap->GetGPUDescriptorHandleForHeapStart();
	g_shadowSrvCpuHandle.ptr += (SIZE_T)descIdx * g_srvDescriptorSize;
	g_shadowSrvGpuHandle.ptr += (UINT64)descIdx * g_srvDescriptorSize;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	g_d3dDevice->CreateShaderResourceView(g_shadowDepthBuffer, &srvDesc, g_shadowSrvCpuHandle);

	// Create Shadow Constant Buffer (upload heap)
	UINT64 shadowCbSize = (sizeof(ShadowConstantBuffer) + 255) & ~255;
	D3D12_RESOURCE_DESC cbDesc = CreateBufferResourceDesc(shadowCbSize);
	D3D12_HEAP_PROPERTIES uploadHeap = CreateHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	g_d3dDevice->CreateCommittedResource(
		&uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_shadowConstantBuffer)
	);
	D3D12_RANGE readRange = { 0, 0 };
	g_shadowConstantBuffer->Map(0, &readRange, &g_pShadowConstantMapped);

	return EXIT_SUCCESS;
}

int initD3D12(HWND hwnd)
{
	// 1. Create DXGI Factory
	UINT dxgiFactoryFlags = 0;
	HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&g_dxgiFactory));
	if (FAILED(hr))
	{
		std::cerr << "Failed to create DXGIFactory2: " << hr << std::endl;
		return EXIT_FAILURE;
	}

	// 2. Select Hardware Adapter (Prefer High Performance / Discrete GPU)
	IDXGIAdapter1* adapter = nullptr;
	for (UINT i = 0; g_dxgiFactory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			adapter->Release();
			continue;
		}

		char descStr[128];
		WideCharToMultiByte(CP_ACP, 0, desc.Description, -1, descStr, sizeof(descStr), nullptr, nullptr);
		std::cout << "DirectX 12 Adapter " << i << " : " << descStr << std::endl;

		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
		adapter->Release();
		adapter = nullptr;
	}

	// 3. Create Device
	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_d3dDevice));
	if (adapter) adapter->Release();
	if (FAILED(hr))
	{
		std::cerr << "Failed to create D3D12Device: " << hr << std::endl;
		return EXIT_FAILURE;
	}

	// 4. Create Command Queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	hr = g_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&g_commandQueue));
	if (FAILED(hr))
	{
		std::cerr << "Failed to create D3D12CommandQueue: " << hr << std::endl;
		return EXIT_FAILURE;
	}

	// 5. Create SwapChain
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FRAME_COUNT;
	swapChainDesc.Width = (UINT)g_currentWidth;
	swapChainDesc.Height = (UINT)g_currentHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	IDXGISwapChain1* sc = nullptr;
	hr = g_dxgiFactory->CreateSwapChainForHwnd(g_commandQueue, hwnd, &swapChainDesc, nullptr, nullptr, &sc);
	if (FAILED(hr))
	{
		std::cerr << "Failed to create DXGI SwapChain: " << hr << std::endl;
		return EXIT_FAILURE;
	}
	sc->QueryInterface(IID_PPV_ARGS(&g_swapChain));
	sc->Release();
	g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

	// 6. Create Descriptor Heaps (RTV, DSV, SRV)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	g_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&g_rtvDescHeap));
	g_rtvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 2; // Index 0: SwapChain Depth, Index 1: Directional Shadow Depth
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	g_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&g_dsvDescHeap));
	g_dsvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = SRV_HEAP_CAPACITY; // 8192
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	g_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&g_srvDescHeap));
	g_srvDescriptorSize = g_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// 7. Create RTVs for backbuffers
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < FRAME_COUNT; i++)
	{
		g_swapChain->GetBuffer(i, IID_PPV_ARGS(&g_renderTargets[i]));
		g_d3dDevice->CreateRenderTargetView(g_renderTargets[i], nullptr, rtvHandle);
		rtvHandle.ptr += g_rtvDescriptorSize;
	}

	// 8. Create Depth Stencil View & Shadow Map Resources
	createDepthStencilView(g_currentWidth, g_currentHeight);
	createShadowResources();

	// 9. Create Command Allocators & List
	for (UINT i = 0; i < FRAME_COUNT; i++)
	{
		g_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_commandAllocators[i]));
	}
	g_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_commandAllocators[0], nullptr, IID_PPV_ARGS(&g_commandList));
	g_commandList->Close();

	// 10. Create Fence & Event
	g_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence));
	for (UINT i = 0; i < FRAME_COUNT; i++) g_fenceValues[i] = 0;
	g_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// 11. Dedicated Upload Command Allocator, Command List, and Upload Fence
	g_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_uploadCmdAlloc));
	g_d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_uploadCmdAlloc, nullptr, IID_PPV_ARGS(&g_uploadCmdList));
	g_uploadCmdList->Close();
	g_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_uploadFence));
	g_uploadFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	g_uploadFenceValue = 0;

	return EXIT_SUCCESS;
}

int createShadersAndPipeline()
{
	// 1. Root Signature (CBV b0, 32-bit constants b1, Descriptor Table t0-t4 for materials, Descriptor Table t5 for shadow map, Samplers s0 and s1)
	D3D12_DESCRIPTOR_RANGE srvRange = {};
	srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvRange.NumDescriptors = 5;
	srvRange.BaseShaderRegister = 0;
	srvRange.RegisterSpace = 0;
	srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE shadowSrvRange = {};
	shadowSrvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	shadowSrvRange.NumDescriptors = 1;
	shadowSrvRange.BaseShaderRegister = 5;
	shadowSrvRange.RegisterSpace = 0;
	shadowSrvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER rootParams[4] = {};
	// 0: CBV b0 (Frame Constants)
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	rootParams[0].Descriptor.ShaderRegister = 0;
	rootParams[0].Descriptor.RegisterSpace = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// 1: 32-bit Constants b1 (20 floats Material Constants)
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[1].Constants.ShaderRegister = 1;
	rootParams[1].Constants.RegisterSpace = 0;
	rootParams[1].Constants.Num32BitValues = 20;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// 2: Descriptor Table t0-t4 (5 SRVs for Material Textures)
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 3: Descriptor Table t5 (1 SRV for Directional Shadow Map)
	rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
	rootParams[3].DescriptorTable.pDescriptorRanges = &shadowSrvRange;
	rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
	// Sampler 0: s0 (Anisotropic wrap for materials)
	staticSamplers[0].Filter = D3D12_FILTER_ANISOTROPIC;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].MipLODBias = 0.0f;
	staticSamplers[0].MaxAnisotropy = 16;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Sampler 1: s1 (Hardware PCF Comparison Sampler for Shadow Map)
	staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].MipLODBias = 0.0f;
	staticSamplers[1].MaxAnisotropy = 1;
	staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplers[1].MinLOD = 0.0f;
	staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[1].ShaderRegister = 1;
	staticSamplers[1].RegisterSpace = 0;
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	rootSigDesc.NumParameters = 4;
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 2;
	rootSigDesc.pStaticSamplers = staticSamplers;
	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* serializedRootSig = nullptr;
	ID3DBlob* errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSig, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob)
		{
			std::cerr << "RootSig serialization error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
			errorBlob->Release();
		}
		return EXIT_FAILURE;
	}
	g_d3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&g_rootSignature));
	serializedRootSig->Release();

	// 2. Compile Main HLSL Shaders with Hardware PBR, 2K Textures & 16-Tap PCF Real-Time Shadows
	const char* hlslSource = R"(
		cbuffer FrameConstants : register(b0)
		{
			float4x4 mvp;
			float4x4 lightSpaceMatrix;
			float3 cameraPos;
			float ambientIntensity;
			float3 lightDir;
			float shadowBias;
			float3 lightColor;
			float shadowStrength;
			float pcfRadius;
			float enableShadows;
			float shadowMapSize;
			float numPointLights;
			float4 pointLightPosRange[4];
			float4 pointLightColorIntensity[4];
		};

		cbuffer MaterialConstants : register(b1)
		{
			float3 baseColor;
			float metallic;
			float roughness;
			float normalStrength;
			float specular;
			float emissiveIntensity;
			float3 emissiveColor;
			float isUnlit;
			float hasAlbedoTex;
			float hasNormalTex;
			float hasRoughTex;
			float hasAoTex;
			float receiveShadows;
			float pad0;
			float pad1;
			float pad2;
		};

		Texture2D g_albedoTex : register(t0);
		Texture2D g_normalTex : register(t1);
		Texture2D g_roughTex  : register(t2);
		Texture2D g_metalTex  : register(t3);
		Texture2D g_aoTex     : register(t4);
		Texture2D g_shadowMap : register(t5);

		SamplerState g_sampler : register(s0);
		SamplerComparisonState g_shadowSampler : register(s1);

		struct VSInput
		{
			float3 position : POSITION;
			float3 normal   : NORMAL;
			float2 uv       : TEXCOORD;
			float3 color    : COLOR;
		};

		struct PSInput
		{
			float4 position    : SV_POSITION;
			float3 worldPos    : WORLD_POS;
			float3 normal      : NORMAL;
			float2 uv          : TEXCOORD;
			float3 color       : COLOR;
			float4 shadowCoord : SHADOW_COORD;
		};

		PSInput VSMain(VSInput input)
		{
			PSInput output;
			output.position = mul(mvp, float4(input.position, 1.0f));
			output.worldPos = input.position;
			output.normal   = input.normal;
			output.uv       = input.uv;
			output.color    = input.color;
			output.shadowCoord = mul(lightSpaceMatrix, float4(input.position, 1.0f));
			return output;
		}

		#define M_PI 3.14159265359f

		float CalculateShadow(float4 shadowCoord, float3 N, float3 L)
		{
			if (enableShadows < 0.5f || receiveShadows < 0.5f) return 1.0f;

			float3 projCoords = shadowCoord.xyz / shadowCoord.w;

			// Outside frustum test
			if (projCoords.z > 1.0f || projCoords.z < 0.0f ||
				projCoords.x < -1.0f || projCoords.x > 1.0f ||
				projCoords.y < -1.0f || projCoords.y > 1.0f)
			{
				return 1.0f;
			}

			// Map NDC [-1, 1] to Texture UV [0, 1] (D3D texture coordinates: V=0 is top)
			float2 shadowUV;
			shadowUV.x = projCoords.x * 0.5f + 0.5f;
			shadowUV.y = -projCoords.y * 0.5f + 0.5f;

			float currentDepth = projCoords.z;

			// Slope-scaled normal bias
			float cosTheta = saturate(dot(N, L));
			float bias = max(shadowBias * (1.0f - cosTheta), shadowBias * 0.25f);

			// 16-tap PCF kernel for smooth soft shadows
			float shadow = 0.0f;
			float texelSize = 1.0f / shadowMapSize;

			[unroll]
			for (int x = -1; x <= 2; ++x)
			{
				[unroll]
				for (int y = -1; y <= 2; ++y)
				{
					float2 offset = float2(x, y) * texelSize * pcfRadius;
					shadow += g_shadowMap.SampleCmpLevelZero(g_shadowSampler, shadowUV + offset, currentDepth - bias);
				}
			}
			shadow /= 16.0f;

			return lerp(1.0f - shadowStrength, 1.0f, shadow);
		}

		float4 PSMain(PSInput input) : SV_TARGET
		{
			if (isUnlit > 0.5f)
			{
				float3 col = input.color * baseColor;
				if (hasAlbedoTex > 0.5f)
				{
					col = g_albedoTex.Sample(g_sampler, input.uv).rgb;
				}
				return float4(col + emissiveColor * emissiveIntensity, 1.0f);
			}

			float3 N = normalize(input.normal);
			if (hasNormalTex > 0.5f && normalStrength > 0.01f)
			{
				float3 nSample = g_normalTex.Sample(g_sampler, input.uv).rgb * 2.0f - 1.0f;
				nSample.xy *= normalStrength;
				float3 up = abs(N.y) < 0.999f ? float3(0, 1, 0) : float3(1, 0, 0);
				float3 T = normalize(cross(up, N));
				float3 B = cross(N, T);
				N = normalize(T * nSample.x + B * nSample.y + N * nSample.z);
			}

			float3 albedo = input.color * baseColor;
			if (hasAlbedoTex > 0.5f)
			{
				albedo = g_albedoTex.Sample(g_sampler, input.uv).rgb;
			}

			float rough = roughness;
			if (hasRoughTex > 0.5f)
			{
				rough = g_roughTex.Sample(g_sampler, input.uv).r;
			}
			rough = clamp(rough, 0.04f, 1.0f);

			float metal = metallic;
			if (hasAlbedoTex > 0.5f && metallic > 0.0f)
			{
				metal = clamp(g_metalTex.Sample(g_sampler, input.uv).r, 0.0f, 1.0f);
			}

			float ao = 1.0f;
			if (hasAoTex > 0.5f)
			{
				ao = clamp(g_aoTex.Sample(g_sampler, input.uv).r, 0.05f, 1.0f);
			}

			float3 V = normalize(cameraPos - input.worldPos);
			float3 L = normalize(lightDir);
			float3 H = normalize(L + V);

			float NdotL = max(dot(N, L), 0.0f);
			float NdotV = max(dot(N, V), 0.001f);
			float NdotH = max(dot(N, H), 0.0f);
			float VdotH = max(dot(V, H), 0.0f);

			float3 F0 = lerp(float3(0.04f * specular * 2.0f, 0.04f * specular * 2.0f, 0.04f * specular * 2.0f), albedo, metal);
			float3 F = F0 + (1.0f - F0) * pow(clamp(1.0f - VdotH, 0.0f, 1.0f), 5.0f);

			float a = rough * rough;
			float a2 = a * a;
			float denomD = (NdotH * NdotH * (a2 - 1.0f) + 1.0f);
			float D = a2 / (M_PI * denomD * denomD + 0.0001f);

			float k = ((rough + 1.0f) * (rough + 1.0f)) / 8.0f;
			float g1V = NdotV / (NdotV * (1.0f - k) + k);
			float g1L = NdotL / (NdotL * (1.0f - k) + k);
			float G = g1V * g1L;

			float3 specBRDF = (D * F * G) / max(4.0f * NdotV * NdotL, 0.001f);
			float3 kS = F;
			float3 kD = (1.0f - kS) * (1.0f - metal);
			float3 diffBRDF = kD * albedo;

			// Directional Sun Light with Shadows
			float shadowFactor = CalculateShadow(input.shadowCoord, N, L);
			float3 directLit = (diffBRDF + specBRDF) * NdotL * lightColor * shadowFactor;

			float3 ambientDiff = albedo * ambientIntensity * (1.0f - metal) * ao;
			float3 ambF = F0 + (max(1.0f - rough, F0) - F0) * pow(clamp(1.0f - NdotV, 0.0f, 1.0f), 5.0f);
			float3 ambientSpec = ambF * ambientIntensity * lerp(1.0f, 0.15f, rough) * ao;

			// Multiple Point Lights with Physical Inverse-Square Falloff
			float3 pointLightsContribution = float3(0, 0, 0);
			int numLights = min((int)numPointLights, 4);
			for (int i = 0; i < numLights; ++i)
			{
				float3 pPos = pointLightPosRange[i].xyz;
				float pRange = pointLightPosRange[i].w;
				float3 pCol = pointLightColorIntensity[i].xyz;
				float pIntensity = pointLightColorIntensity[i].w;

				float3 toLight = pPos - input.worldPos;
				float dist = length(toLight);
				if (dist < pRange && dist > 0.001f)
				{
					float3 pL = toLight / dist;
					float3 pH = normalize(pL + V);
					float pNdotL = max(dot(N, pL), 0.0f);
					float pNdotH = max(dot(N, pH), 0.0f);
					float pVdotH = max(dot(V, pH), 0.0f);

					float atten = saturate(1.0f - (dist / pRange));
					atten = (atten * atten) / (dist * dist + 1.0f);

					float3 pF = F0 + (1.0f - F0) * pow(clamp(1.0f - pVdotH, 0.0f, 1.0f), 5.0f);
					float pDenomD = (pNdotH * pNdotH * (a2 - 1.0f) + 1.0f);
					float pD = a2 / (M_PI * pDenomD * pDenomD + 0.0001f);
					float pg1L = pNdotL / (pNdotL * (1.0f - k) + k);
					float pG = g1V * pg1L;
					float3 pSpec = (pD * pF * pG) / max(4.0f * NdotV * pNdotL, 0.001f);
					float3 pkD = (1.0f - pF) * (1.0f - metal);
					float3 pDiff = pkD * albedo;

					pointLightsContribution += (pDiff + pSpec) * pNdotL * pCol * pIntensity * atten;
				}
			}

			float3 emissive = emissiveColor * emissiveIntensity;
			float3 litColor = ambientDiff + ambientSpec + directLit + pointLightsContribution + emissive;
			return float4(saturate(litColor), 1.0f);
		}
	)";

	ID3DBlob* vertexShader = nullptr;
	ID3DBlob* pixelShader = nullptr;

	hr = D3DCompile(hlslSource, strlen(hlslSource), nullptr, nullptr, nullptr, "VSMain", "vs_5_0", 0, 0, &vertexShader, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob) { std::cerr << "VS compile error: " << (char*)errorBlob->GetBufferPointer() << std::endl; errorBlob->Release(); }
		return EXIT_FAILURE;
	}

	hr = D3DCompile(hlslSource, strlen(hlslSource), nullptr, nullptr, nullptr, "PSMain", "ps_5_0", 0, 0, &pixelShader, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob) { std::cerr << "PS compile error: " << (char*)errorBlob->GetBufferPointer() << std::endl; errorBlob->Release(); }
		return EXIT_FAILURE;
	}

	// 3. Input Layout (Position, Normal, UV, Color)
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	// 4. Graphics Pipeline State Object (PSO) for Main Lit Pass
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputElementDescs, 4 };
	psoDesc.pRootSignature = g_rootSignature;
	psoDesc.VS = { vertexShader->GetBufferPointer(), vertexShader->GetBufferSize() };
	psoDesc.PS = { pixelShader->GetBufferPointer(), pixelShader->GetBufferSize() };

	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE; // Double sided for editor
	psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	psoDesc.RasterizerState.DepthClipEnable = TRUE;

	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	psoDesc.DepthStencilState.StencilEnable = FALSE;

	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.SampleDesc.Count = 1;

	hr = g_d3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&g_pipelineState));
	vertexShader->Release();
	pixelShader->Release();

	if (FAILED(hr))
	{
		std::cerr << "Failed to create D3D12 Graphics Pipeline State: " << hr << std::endl;
		return EXIT_FAILURE;
	}

	// 5. Shadow Pass Root Signature & Pipeline State
	D3D12_ROOT_PARAMETER shadowRootParam = {};
	shadowRootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	shadowRootParam.Descriptor.ShaderRegister = 0;
	shadowRootParam.Descriptor.RegisterSpace = 0;
	shadowRootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	D3D12_ROOT_SIGNATURE_DESC shadowRootSigDesc = {};
	shadowRootSigDesc.NumParameters = 1;
	shadowRootSigDesc.pParameters = &shadowRootParam;
	shadowRootSigDesc.NumStaticSamplers = 0;
	shadowRootSigDesc.pStaticSamplers = nullptr;
	shadowRootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob* serializedShadowRootSig = nullptr;
	hr = D3D12SerializeRootSignature(&shadowRootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedShadowRootSig, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob) { std::cerr << "Shadow RootSig serialization error: " << (char*)errorBlob->GetBufferPointer() << std::endl; errorBlob->Release(); }
		return EXIT_FAILURE;
	}
	g_d3dDevice->CreateRootSignature(0, serializedShadowRootSig->GetBufferPointer(), serializedShadowRootSig->GetBufferSize(), IID_PPV_ARGS(&g_shadowRootSignature));
	serializedShadowRootSig->Release();

	const char* shadowHlsl = R"(
		cbuffer ShadowConstants : register(b0)
		{
			float4x4 lightSpaceMatrix;
		};

		struct VSInput
		{
			float3 position : POSITION;
			float3 normal   : NORMAL;
			float2 uv       : TEXCOORD;
			float3 color    : COLOR;
		};

		float4 VSShadow(VSInput input) : SV_POSITION
		{
			return mul(lightSpaceMatrix, float4(input.position, 1.0f));
		}
	)";

	ID3DBlob* shadowVS = nullptr;
	hr = D3DCompile(shadowHlsl, strlen(shadowHlsl), nullptr, nullptr, nullptr, "VSShadow", "vs_5_0", 0, 0, &shadowVS, &errorBlob);
	if (FAILED(hr))
	{
		if (errorBlob) { std::cerr << "Shadow VS compile error: " << (char*)errorBlob->GetBufferPointer() << std::endl; errorBlob->Release(); }
		return EXIT_FAILURE;
	}

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = {};
	shadowPsoDesc.InputLayout = { inputElementDescs, 4 };
	shadowPsoDesc.pRootSignature = g_shadowRootSignature;
	shadowPsoDesc.VS = { shadowVS->GetBufferPointer(), shadowVS->GetBufferSize() };
	shadowPsoDesc.PS = { nullptr, 0 }; // Depth-only pass
	shadowPsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	shadowPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	shadowPsoDesc.RasterizerState.FrontCounterClockwise = FALSE;
	shadowPsoDesc.RasterizerState.DepthClipEnable = TRUE;
	shadowPsoDesc.RasterizerState.DepthBias = 50;
	shadowPsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
	shadowPsoDesc.DepthStencilState.DepthEnable = TRUE;
	shadowPsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	shadowPsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	shadowPsoDesc.DepthStencilState.StencilEnable = FALSE;
	shadowPsoDesc.SampleMask = UINT_MAX;
	shadowPsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	shadowPsoDesc.NumRenderTargets = 0;
	shadowPsoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	shadowPsoDesc.SampleDesc.Count = 1;

	hr = g_d3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&g_shadowPipelineState));
	shadowVS->Release();
	if (FAILED(hr))
	{
		std::cerr << "Failed to create shadow PSO: " << hr << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int createDynamicBuffers()
{
	// 1. Vertex Buffer
	UINT64 vertexBufferSize = MAX_SCENE_VERTICES * sizeof(Vertex);
	D3D12_HEAP_PROPERTIES uploadHeap = CreateHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC vBufferDesc = CreateBufferResourceDesc(vertexBufferSize);

	g_d3dDevice->CreateCommittedResource(
		&uploadHeap, D3D12_HEAP_FLAG_NONE, &vBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_vertexBuffer)
	);

	D3D12_RANGE readRange = { 0, 0 };
	g_vertexBuffer->Map(0, &readRange, &g_pVertexMapped);

	g_vertexBufferView.BufferLocation = g_vertexBuffer->GetGPUVirtualAddress();
	g_vertexBufferView.StrideInBytes = sizeof(Vertex);
	g_vertexBufferView.SizeInBytes = (UINT)vertexBufferSize;

	// 2. Index Buffer
	UINT64 indexBufferSize = MAX_SCENE_INDICES * sizeof(uint32_t);
	D3D12_RESOURCE_DESC iBufferDesc = CreateBufferResourceDesc(indexBufferSize);

	g_d3dDevice->CreateCommittedResource(
		&uploadHeap, D3D12_HEAP_FLAG_NONE, &iBufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_indexBuffer)
	);

	g_indexBuffer->Map(0, &readRange, &g_pIndexMapped);

	g_indexBufferView.BufferLocation = g_indexBuffer->GetGPUVirtualAddress();
	g_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	g_indexBufferView.SizeInBytes = (UINT)indexBufferSize;

	// 3. Constant Buffer
	UINT64 cbSize = (sizeof(SceneConstantBuffer) + 255) & ~255;
	D3D12_RESOURCE_DESC cbDesc = CreateBufferResourceDesc(cbSize);

	g_d3dDevice->CreateCommittedResource(
		&uploadHeap, D3D12_HEAP_FLAG_NONE, &cbDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&g_constantBuffer)
	);

	g_constantBuffer->Map(0, &readRange, &g_pConstantMapped);

	return EXIT_SUCCESS;
}

void updateSceneGeometry()
{
	static std::vector<Vertex> sceneVertices;
	static std::vector<uint32_t> sceneIndices;

	g_scene.BuildSceneMesh(sceneVertices, sceneIndices, g_sceneBatches, g_camera.GetPosition());

	g_currentVertexCount = (uint32_t)std::min(sceneVertices.size(), MAX_SCENE_VERTICES);
	g_currentIndexCount  = (uint32_t)std::min(sceneIndices.size(), MAX_SCENE_INDICES);

	if (g_pVertexMapped && g_currentVertexCount > 0)
	{
		memcpy(g_pVertexMapped, sceneVertices.data(), g_currentVertexCount * sizeof(Vertex));
	}
	if (g_pIndexMapped && g_currentIndexCount > 0)
	{
		memcpy(g_pIndexMapped, sceneIndices.data(), g_currentIndexCount * sizeof(uint32_t));
	}
}

void updateConstantBuffer()
{
	if (!g_pConstantMapped) return;

	g_scene.SyncLightPositionsFromActors();

	float aspect = (g_currentHeight > 0) ? ((float)g_currentWidth / (float)g_currentHeight) : 1.777f;
	glm::mat4 model = glm::mat4(1.0f);
	glm::mat4 view  = g_camera.GetViewMatrix();
	glm::mat4 proj  = g_camera.GetProjectionMatrix(aspect);

	// Light View-Projection for Directional Sun Light
	glm::vec3 lightDir = glm::normalize(g_scene.lightDirection);
	glm::vec3 sceneCenter(0.0f, 0.0f, 0.0f);
	glm::vec3 lightPos = sceneCenter + lightDir * 18.0f;
	glm::vec3 up = (std::abs(lightDir.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 lightView = glm::lookAt(lightPos, sceneCenter, up);
	float orthoHalfSize = 14.0f;
	glm::mat4 lightProj = glm::ortho(-orthoHalfSize, orthoHalfSize, -orthoHalfSize, orthoHalfSize, 1.0f, 40.0f);
	glm::mat4 lightSpaceMatrix = lightProj * lightView;

	if (g_pShadowConstantMapped)
	{
		ShadowConstantBuffer scb = {};
		scb.lightSpaceMatrix = lightSpaceMatrix;
		memcpy(g_pShadowConstantMapped, &scb, sizeof(scb));
	}

	SceneConstantBuffer cb = {};
	cb.mvp = proj * view * model;
	cb.lightSpaceMatrix = lightSpaceMatrix;
	cb.cameraPos = g_camera.GetPosition();
	cb.ambientIntensity = g_scene.ambientIntensity;
	cb.lightDir = lightDir;
	cb.shadowBias = g_scene.shadowBias;
	cb.lightColor = g_scene.lightColor * g_scene.lightIntensity;
	cb.shadowStrength = g_scene.shadowStrength;
	cb.pcfRadius = g_scene.pcfRadius;
	cb.enableShadows = g_scene.enableShadows ? 1.0f : 0.0f;
	cb.shadowMapSize = (float)SHADOW_MAP_WIDTH;

	// Fill Point Lights (up to 4)
	int activePointLights = 0;
	for (size_t i = 0; i < g_scene.pointLights.size() && activePointLights < 4; ++i)
	{
		const auto& pl = g_scene.pointLights[i];
		if (!pl.enabled) continue;
		cb.pointLightPosRange[activePointLights] = glm::vec4(pl.position, pl.range);
		cb.pointLightColorIntensity[activePointLights] = glm::vec4(pl.color, pl.intensity);
		activePointLights++;
	}
	cb.numPointLights = (float)activePointLights;

	memcpy(g_pConstantMapped, &cb, sizeof(cb));
}

void resizeBuffers(int width, int height)
{
	if (width <= 0 || height <= 0) return;

	WaitForGpuIdle();

	for (UINT i = 0; i < FRAME_COUNT; i++)
	{
		if (g_renderTargets[i])
		{
			g_renderTargets[i]->Release();
			g_renderTargets[i] = nullptr;
		}
	}

	g_swapChain->ResizeBuffers(FRAME_COUNT, (UINT)width, (UINT)height, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < FRAME_COUNT; i++)
	{
		g_swapChain->GetBuffer(i, IID_PPV_ARGS(&g_renderTargets[i]));
		g_d3dDevice->CreateRenderTargetView(g_renderTargets[i], nullptr, rtvHandle);
		rtvHandle.ptr += g_rtvDescriptorSize;
	}

	createDepthStencilView(width, height);
}

void renderFrame()
{
	g_commandAllocators[g_frameIndex]->Reset();
	g_commandList->Reset(g_commandAllocators[g_frameIndex], nullptr);

	// ----------------------------------------------------
	// PASS 1: Directional Shadow Depth Map Pass (2048x2048 D32)
	// ----------------------------------------------------
	if (!g_deviceLost && g_currentIndexCount > 0 && g_shadowDepthBuffer && g_shadowPipelineState && g_scene.enableShadows)
	{
		D3D12_RESOURCE_BARRIER shadowBarrier = CreateTransitionBarrier(
			g_shadowDepthBuffer,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE
		);
		g_commandList->ResourceBarrier(1, &shadowBarrier);

		D3D12_VIEWPORT shadowViewport = { 0.0f, 0.0f, (float)SHADOW_MAP_WIDTH, (float)SHADOW_MAP_HEIGHT, 0.0f, 1.0f };
		D3D12_RECT shadowScissor = { 0, 0, (LONG)SHADOW_MAP_WIDTH, (LONG)SHADOW_MAP_HEIGHT };
		g_commandList->RSSetViewports(1, &shadowViewport);
		g_commandList->RSSetScissorRects(1, &shadowScissor);

		g_commandList->ClearDepthStencilView(g_shadowDsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
		g_commandList->OMSetRenderTargets(0, nullptr, FALSE, &g_shadowDsvHandle);

		g_commandList->SetGraphicsRootSignature(g_shadowRootSignature);
		g_commandList->SetPipelineState(g_shadowPipelineState);
		g_commandList->SetGraphicsRootConstantBufferView(0, g_shadowConstantBuffer->GetGPUVirtualAddress());

		g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
		g_commandList->IASetIndexBuffer(&g_indexBufferView);

		for (const auto& batch : g_sceneBatches)
		{
			if (batch.indexCount == 0 || !batch.castShadows || batch.isUnlit) continue;
			g_commandList->DrawIndexedInstanced(batch.indexCount, 1, batch.startIndex, 0, 0);
		}

		D3D12_RESOURCE_BARRIER shadowReadBarrier = CreateTransitionBarrier(
			g_shadowDepthBuffer,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
		);
		g_commandList->ResourceBarrier(1, &shadowReadBarrier);
	}

	// ----------------------------------------------------
	// PASS 2: Main Scene PBR Lit Pass with Shadows & Point Lights
	// ----------------------------------------------------
	D3D12_RESOURCE_BARRIER barrier = CreateTransitionBarrier(
		g_renderTargets[g_frameIndex],
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	g_commandList->ResourceBarrier(1, &barrier);

	// Set Viewport & Scissor
	D3D12_VIEWPORT viewport = { 0.0f, 0.0f, (float)g_currentWidth, (float)g_currentHeight, 0.0f, 1.0f };
	D3D12_RECT scissor = { 0, 0, (LONG)g_currentWidth, (LONG)g_currentHeight };
	g_commandList->RSSetViewports(1, &viewport);
	g_commandList->RSSetScissorRects(1, &scissor);

	// Clear RTV & DSV
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_rtvDescHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (SIZE_T)g_frameIndex * g_rtvDescriptorSize;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = g_dsvDescHeap->GetCPUDescriptorHandleForHeapStart();

	const float clearColor[4] = { g_scene.clearColor.r, g_scene.clearColor.g, g_scene.clearColor.b, 1.0f };
	g_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	g_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	g_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

	// Set Descriptor Heap (for ImGui and scene resources)
	ID3D12DescriptorHeap* descriptorHeaps[] = { g_srvDescHeap };
	g_commandList->SetDescriptorHeaps(1, descriptorHeaps);

	// 1. Draw 3D Scene Primitives with Hardware PBR, Full Texture Resolution & Real-Time Shadows
	if (!g_deviceLost && g_currentIndexCount > 0)
	{
		g_commandList->SetGraphicsRootSignature(g_rootSignature);
		g_commandList->SetPipelineState(g_pipelineState);
		g_commandList->SetGraphicsRootConstantBufferView(0, g_constantBuffer->GetGPUVirtualAddress());
		g_commandList->SetGraphicsRootDescriptorTable(3, g_shadowSrvGpuHandle);

		g_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		g_commandList->IASetVertexBuffers(0, 1, &g_vertexBufferView);
		g_commandList->IASetIndexBuffer(&g_indexBufferView);

		for (const auto& batch : g_sceneBatches)
		{
			if (batch.indexCount == 0) continue;

			MaterialShaderConstants matConsts = {};
			matConsts.baseColor[0] = batch.baseColor.r;
			matConsts.baseColor[1] = batch.baseColor.g;
			matConsts.baseColor[2] = batch.baseColor.b;
			matConsts.metallic = batch.metallic;
			matConsts.roughness = batch.roughness;
			matConsts.normalStrength = batch.normalStrength;
			matConsts.specular = batch.specular;
			matConsts.emissiveIntensity = batch.emissiveIntensity;
			matConsts.emissiveColor[0] = batch.emissiveColor.r;
			matConsts.emissiveColor[1] = batch.emissiveColor.g;
			matConsts.emissiveColor[2] = batch.emissiveColor.b;
			matConsts.isUnlit = batch.isUnlit ? 1.0f : 0.0f;
			matConsts.hasAlbedoTex = (!batch.albedoTex.empty() && batch.albedoTex != "none") ? 1.0f : 0.0f;
			matConsts.hasNormalTex = (!batch.normalTex.empty() && batch.normalTex != "none") ? 1.0f : 0.0f;
			matConsts.hasRoughTex  = (!batch.roughTex.empty() && batch.roughTex != "none")  ? 1.0f : 0.0f;
			matConsts.hasAoTex     = (!batch.aoTex.empty() && batch.aoTex != "none")        ? 1.0f : 0.0f;
			matConsts.receiveShadows = batch.receiveShadows ? 1.0f : 0.0f;

			g_commandList->SetGraphicsRoot32BitConstants(1, 20, &matConsts, 0);

			D3D12_GPU_DESCRIPTOR_HANDLE tableHandle = GetOrCreateMaterialTable(
				batch.albedoTex, batch.normalTex, batch.roughTex, batch.metalTex, batch.aoTex);
			g_commandList->SetGraphicsRootDescriptorTable(2, tableHandle);

			g_commandList->DrawIndexedInstanced(batch.indexCount, 1, batch.startIndex, 0, 0);
		}
	}

	// 2. Render Unreal Engine 5 UI with Dear ImGui
	ImDrawData* drawData = ImGui::GetDrawData();
	if (drawData)
	{
		ImGui_ImplDX12_RenderDrawData(drawData, g_commandList);
	}

	// Transition BackBuffer to Present
	barrier = CreateTransitionBarrier(
		g_renderTargets[g_frameIndex],
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	g_commandList->ResourceBarrier(1, &barrier);

	g_commandList->Close();

	ID3D12CommandList* commandLists[] = { g_commandList };
	g_commandQueue->ExecuteCommandLists(1, commandLists);

	HRESULT hrPresent = g_swapChain->Present(1, 0);
	if (hrPresent == DXGI_ERROR_DEVICE_REMOVED || hrPresent == DXGI_ERROR_DEVICE_RESET)
	{
		g_deviceLost = true;
		HRESULT reason = g_d3dDevice ? g_d3dDevice->GetDeviceRemovedReason() : hrPresent;
		std::cerr << "[RECOVERY] Present detected device loss (0x" << std::hex << reason << std::dec << ")\n";
		g_engineUI.AddLog("LogRecovery", "GPU Device Lost/Removed detected on Present. Please save work and restart editor.", 3);
	}

	// Signal fence for current frame
	const UINT64 currentFenceValue = g_fenceValues[g_frameIndex] + 1;
	g_commandQueue->Signal(g_fence, currentFenceValue);
	g_fenceValues[g_frameIndex] = currentFenceValue;

	// Advance to next backbuffer
	g_frameIndex = g_swapChain->GetCurrentBackBufferIndex();

	// Wait if the next backbuffer is still in-flight
	if (g_fence->GetCompletedValue() < g_fenceValues[g_frameIndex])
	{
		g_fence->SetEventOnCompletion(g_fenceValues[g_frameIndex], g_fenceEvent);
		SafeWaitForFence(g_fence, g_fenceValues[g_frameIndex], g_fenceEvent, 2000, "renderFrame frame fence");
	}
}

void cleanUpD3D12()
{
	WaitForGpuIdle();

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	if (g_vertexBuffer) { g_vertexBuffer->Unmap(0, nullptr); g_vertexBuffer->Release(); }
	if (g_indexBuffer) { g_indexBuffer->Unmap(0, nullptr); g_indexBuffer->Release(); }
	if (g_constantBuffer) { g_constantBuffer->Unmap(0, nullptr); g_constantBuffer->Release(); }
	if (g_shadowConstantBuffer) { g_shadowConstantBuffer->Unmap(0, nullptr); g_shadowConstantBuffer->Release(); }

	for (auto* tex : g_allocatedTextures) {
		if (tex) tex->Release();
	}
	g_allocatedTextures.clear();
	g_gpuTextureMap.clear();
	g_materialDescriptorTables.clear();

	if (g_depthStencilBuffer) g_depthStencilBuffer->Release();
	if (g_shadowDepthBuffer) g_shadowDepthBuffer->Release();

	for (UINT i = 0; i < FRAME_COUNT; i++)
	{
		if (g_renderTargets[i]) g_renderTargets[i]->Release();
		if (g_commandAllocators[i]) g_commandAllocators[i]->Release();
	}

	if (g_commandList) g_commandList->Release();
	if (g_pipelineState) g_pipelineState->Release();
	if (g_rootSignature) g_rootSignature->Release();
	if (g_shadowPipelineState) g_shadowPipelineState->Release();
	if (g_shadowRootSignature) g_shadowRootSignature->Release();

	if (g_rtvDescHeap) g_rtvDescHeap->Release();
	if (g_dsvDescHeap) g_dsvDescHeap->Release();
	if (g_srvDescHeap) g_srvDescHeap->Release();

	if (g_uploadCmdList) g_uploadCmdList->Release();
	if (g_uploadCmdAlloc) g_uploadCmdAlloc->Release();
	if (g_uploadFence) g_uploadFence->Release();
	if (g_uploadFenceEvent) CloseHandle(g_uploadFenceEvent);

	if (g_fence) g_fence->Release();
	if (g_fenceEvent) CloseHandle(g_fenceEvent);

	if (g_swapChain) g_swapChain->Release();
	if (g_commandQueue) g_commandQueue->Release();
	if (g_d3dDevice) g_d3dDevice->Release();
	if (g_dxgiFactory) g_dxgiFactory->Release();
}

// GLFW Callbacks
static void frameBufferResizeCallback(GLFWwindow* window, int width, int height)
{
	g_currentWidth = width;
	g_currentHeight = height;
	resizeBuffers(width, height);
}

static bool RayTriangleIntersect(
	const glm::vec3& orig, const glm::vec3& dir,
	const glm::vec3& v0, const glm::vec3& v1, const glm::vec3& v2,
	float& t)
{
	const float EPSILON = 1e-7f;
	glm::vec3 edge1 = v1 - v0;
	glm::vec3 edge2 = v2 - v0;
	glm::vec3 h = glm::cross(dir, edge2);
	float a = glm::dot(edge1, h);
	if (a > -EPSILON && a < EPSILON) return false;

	float f = 1.0f / a;
	glm::vec3 s = orig - v0;
	float u = f * glm::dot(s, h);
	if (u < 0.0f || u > 1.0f) return false;

	glm::vec3 q = glm::cross(s, edge1);
	float v = f * glm::dot(dir, q);
	if (v < 0.0f || u + v > 1.0f) return false;

	float localT = f * glm::dot(edge2, q);
	if (localT > EPSILON)
	{
		t = localT;
		return true;
	}
	return false;
}

static void PickObjectAtCursor(float mouseX, float mouseY)
{
	if (g_currentWidth <= 0 || g_currentHeight <= 0) return;

	float ndcX = (2.0f * mouseX) / (float)g_currentWidth - 1.0f;
	float ndcY = 1.0f - (2.0f * mouseY) / (float)g_currentHeight;

	float aspect = (float)g_currentWidth / (float)g_currentHeight;
	glm::mat4 proj = g_camera.GetProjectionMatrix(aspect);
	glm::mat4 view = g_camera.GetViewMatrix();
	glm::mat4 vp = proj * view;
	glm::mat4 invVP = glm::inverse(vp);

	glm::vec4 nearPointWorld = invVP * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
	glm::vec4 farPointWorld  = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);

	if (nearPointWorld.w != 0.0f) nearPointWorld /= nearPointWorld.w;
	if (farPointWorld.w != 0.0f)  farPointWorld  /= farPointWorld.w;

	glm::vec3 rayOrigin = glm::vec3(nearPointWorld);
	glm::vec3 rayDir = glm::normalize(glm::vec3(farPointWorld - nearPointWorld));

	// 1. Check if user clicked on a Light Sprite Billboard in screen space
	int closestLightId = -1;
	float closestLightDist2D = 25.0f; // pixel tolerance for clicking icon
	for (const auto& obj : g_scene.objects)
	{
		if (!obj.visible || !obj.isLight) continue;
		glm::vec3 worldPos = g_scene.GetWorldPosition(obj);
		glm::vec4 clip = vp * glm::vec4(worldPos, 1.0f);
		if (clip.w <= 0.05f) continue;
		glm::vec3 ndcLight = glm::vec3(clip) / clip.w;
		if (ndcLight.z < -1.0f || ndcLight.z > 1.0f) continue;

		float screenX = (ndcLight.x * 0.5f + 0.5f) * (float)g_currentWidth;
		float screenY = ((1.0f - ndcLight.y) * 0.5f) * (float)g_currentHeight;

		float dist2D = std::hypot(mouseX - screenX, mouseY - screenY);
		if (dist2D < closestLightDist2D)
		{
			closestLightDist2D = dist2D;
			closestLightId = obj.id;
		}
	}

	if (closestLightId != -1)
	{
		g_scene.selectedId = closestLightId;
		GameObject* hitObj = g_scene.FindObject(closestLightId);
		if (hitObj)
		{
			g_engineUI.AddLog("LogActor", "Selected Light: " + hitObj->name, 0);
		}
		return;
	}

	int closestId = -1;
	float closestDist = 1e9f;

	for (const auto& obj : g_scene.objects)
	{
		if (!obj.visible || obj.mesh.vertices.empty()) continue;

		glm::mat4 model = g_scene.GetWorldMatrix(obj);
		glm::mat4 invModel = glm::inverse(model);

		glm::vec3 localRayOrig = glm::vec3(invModel * glm::vec4(rayOrigin, 1.0f));
		glm::vec3 localRayDir  = glm::normalize(glm::vec3(invModel * glm::vec4(rayDir, 0.0f)));

		const auto& verts = obj.mesh.vertices;
		const auto& inds  = obj.mesh.indices;

		for (size_t i = 0; i + 2 < inds.size(); i += 3)
		{
			const glm::vec3& v0 = verts[inds[i]].pos;
			const glm::vec3& v1 = verts[inds[i + 1]].pos;
			const glm::vec3& v2 = verts[inds[i + 2]].pos;

			float t = 0.0f;
			if (RayTriangleIntersect(localRayOrig, localRayDir, v0, v1, v2, t))
			{
				glm::vec3 localHit = localRayOrig + localRayDir * t;
				glm::vec3 worldHit = glm::vec3(model * glm::vec4(localHit, 1.0f));
				float worldDist = glm::dot(worldHit - rayOrigin, rayDir);

				if (worldDist > 0.0f && worldDist < closestDist)
				{
					closestDist = worldDist;
					closestId = obj.id;
				}
			}
		}
	}

	if (closestId != -1)
	{
		g_scene.selectedId = closestId;
		GameObject* hitObj = g_scene.FindObject(closestId);
		if (hitObj)
		{
			g_engineUI.AddLog("LogActor", "Selected Actor: " + hitObj->name, 0);
		}
	}
	else
	{
		// Clicked empty background: deselect
		g_scene.selectedId = -1;
	}
}

static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		s_isLeftMouseDown = (action == GLFW_PRESS);
		if (action == GLFW_PRESS)
		{
			bool isAlt = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
			              glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
			bool isPopupOpen = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId);
			if (!isAlt && !g_camera.isFlying && !io.WantCaptureMouse && !isPopupOpen && !ImGuizmo::IsOver() && !ImGuizmo::IsUsing())
			{
				double mouseX, mouseY;
				glfwGetCursorPos(window, &mouseX, &mouseY);
				PickObjectAtCursor((float)mouseX, (float)mouseY);
			}
		}
	}
	else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
	{
		s_isMiddleMouseDown = (action == GLFW_PRESS);
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			s_isRightMouseDown = true;
			bool isAlt = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
			              glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);
			if (!isAlt && !io.WantCaptureMouse)
			{
				g_camera.isFlying = true;
				s_firstMouseAfterCapture = true;
				glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			}
		}
		else if (action == GLFW_RELEASE)
		{
			s_isRightMouseDown = false;
			g_camera.isFlying = false;
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}

static void cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (s_firstMouseAfterCapture && g_camera.isFlying)
	{
		s_lastMouseX = xpos;
		s_lastMouseY = ypos;
		s_firstMouseAfterCapture = false;
		return;
	}

	double deltaX = xpos - s_lastMouseX;
	double deltaY = ypos - s_lastMouseY;
	s_lastMouseX = xpos;
	s_lastMouseY = ypos;

	ImGuiIO& io = ImGui::GetIO();
	if (ImGuizmo::IsUsing()) return;

	bool isAlt = (glfwGetKey(window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS ||
	              glfwGetKey(window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS);

	// 1. In-Editor Fly Camera (Hold RMB + Mouse Movement -> Look around)
	if (g_camera.isFlying && s_isRightMouseDown && !isAlt)
	{
		g_camera.LookAround((float)deltaX, (float)deltaY);
		return;
	}

	if (io.WantCaptureMouse || ImGuizmo::IsOver()) return;

	// 2. Alt Navigation (Unreal Engine Maya-style viewport controls)
	if (isAlt)
	{
		if (s_isLeftMouseDown)
		{
			// Alt + LMB + Drag: Orbit around pivot/selected object
			g_camera.Orbit((float)deltaX, (float)deltaY);
		}
		else if (s_isRightMouseDown)
		{
			// Alt + RMB + Drag: Dolly/zoom
			g_camera.Dolly((float)(deltaX - deltaY));
		}
		else if (s_isMiddleMouseDown)
		{
			// Alt + MMB + Drag: Pan/track
			g_camera.Pan((float)deltaX, (float)deltaY);
		}
	}
	else
	{
		// Non-Alt navigation
		if (s_isMiddleMouseDown)
		{
			g_camera.Pan((float)deltaX, (float)deltaY);
		}
	}
}

static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	InputSystem::ScrollCallback(window, xoffset, yoffset);

	ImGuiIO& io = ImGui::GetIO();

	if (s_isRightMouseDown && g_camera.isFlying)
	{
		// RMB + Mouse Wheel Up/Down: Increase/Decrease camera speed
		g_camera.AdjustSpeed((float)yoffset);
		g_engineUI.cameraSpeed = g_camera.moveSpeed;
	}
	else
	{
		if (io.WantCaptureMouse) return;
		g_camera.Zoom((float)yoffset);
	}
}

static void dropCallback(GLFWwindow* window, int count, const char** paths)
{
	g_engineUI.HandleFileDrop(paths, count);
}

int main()
{
	// 1. Initialize GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW!" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	g_window = glfwCreateWindow(WIDTH, HEIGHT, "Eunoia-Editor", nullptr, nullptr);
	if (!g_window)
	{
		std::cerr << "Failed to create GLFW window!" << std::endl;
		glfwTerminate();
		return -1;
	}

	HWND hwnd = glfwGetWin32Window(g_window);
	ShowWindow(hwnd, SW_SHOW);
	UpdateWindow(hwnd);

	int fbWidth = 0, fbHeight = 0;
	glfwGetFramebufferSize(g_window, &fbWidth, &fbHeight);
	if (fbWidth > 0 && fbHeight > 0)
	{
		g_currentWidth = fbWidth;
		g_currentHeight = fbHeight;
	}

	glfwSetFramebufferSizeCallback(g_window, frameBufferResizeCallback);
	glfwSetMouseButtonCallback(g_window, mouseButtonCallback);
	glfwSetCursorPosCallback(g_window, cursorPosCallback);
	glfwSetScrollCallback(g_window, scrollCallback);
	glfwSetDropCallback(g_window, dropCallback);

	// 2. Initialize DirectX 12
	if (initD3D12(hwnd) != EXIT_SUCCESS)
	{
		std::cerr << "DirectX 12 initialization failed!" << std::endl;
		return -1;
	}

	if (createShadersAndPipeline() != EXIT_SUCCESS)
	{
		std::cerr << "DirectX 12 pipeline creation failed!" << std::endl;
		return -1;
	}

	if (createDynamicBuffers() != EXIT_SUCCESS)
	{
		std::cerr << "DirectX 12 buffer creation failed!" << std::endl;
		return -1;
	}

	// 3. Initialize Dear ImGui for DirectX 12
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.IniFilename = nullptr; // Prevent stale off-screen positions from overriding responsive layout

	g_engineUI.SetupTheme();

	ImGui_ImplGlfw_InitForOther(g_window, true);

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = g_d3dDevice;
	init_info.CommandQueue = g_commandQueue;
	init_info.NumFramesInFlight = FRAME_COUNT;
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	init_info.SrvDescriptorHeap = g_srvDescHeap;
	init_info.SrvDescriptorAllocFn = ImGui_SrvAlloc;
	init_info.SrvDescriptorFreeFn  = ImGui_SrvFree;

	ImGui_ImplDX12_Init(&init_info);

	InitFallbackTextures();
	DX12GpuTexture lightIcon = GetOrLoadGPUTexture("resources/icons/light.png", g_fallbackWhite);
	g_engineUI.lightIconGpuHandle = lightIcon.gpuHandle.ptr;
	InputSystem::Get().SetupDefaultActions();

	auto lastTime = std::chrono::high_resolution_clock::now();
	float frameCount = 0.0f;
	float fpsTimer = 0.0f;
	float currentFps = 60.0f;
	float currentFrameTimeMs = 16.6f;

	// 4. Main Event & Render Loop
	while (!glfwWindowShouldClose(g_window))
	{
		InputSystem::Get().BeginFrame();
		glfwPollEvents();
		InputSystem::Get().Update(g_window);

		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(
			currentTime - lastTime).count();
		lastTime = currentTime;

		if (deltaTime > 0.1f) deltaTime = 0.1f;

		// Process In-Editor Fly Mode (Hold RMB + W, S, A, D, E, Q)
		if (s_isRightMouseDown && g_camera.isFlying)
		{
			float speed = g_camera.moveSpeed;
			if (glfwGetKey(g_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
			    glfwGetKey(g_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
			{
				speed *= 2.5f; // Fast fly with Shift
			}

			glm::vec3 deltaMove(0.0f);
			glm::vec3 forward = g_camera.GetForward();
			glm::vec3 right   = g_camera.GetRight();
			glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

			if (glfwGetKey(g_window, GLFW_KEY_W) == GLFW_PRESS) deltaMove += forward;
			if (glfwGetKey(g_window, GLFW_KEY_S) == GLFW_PRESS) deltaMove -= forward;
			if (glfwGetKey(g_window, GLFW_KEY_D) == GLFW_PRESS) deltaMove += right;
			if (glfwGetKey(g_window, GLFW_KEY_A) == GLFW_PRESS) deltaMove -= right;
			if (glfwGetKey(g_window, GLFW_KEY_E) == GLFW_PRESS) deltaMove += worldUp;
			if (glfwGetKey(g_window, GLFW_KEY_Q) == GLFW_PRESS) deltaMove -= worldUp;

			if (glm::length(deltaMove) > 0.001f)
			{
				deltaMove = glm::normalize(deltaMove) * speed * deltaTime;
				g_camera.Move(deltaMove);
			}
		}

		frameCount += 1.0f;
		fpsTimer += deltaTime;
		if (fpsTimer >= 0.5f)
		{
			currentFps = frameCount / fpsTimer;
			currentFrameTimeMs = (fpsTimer / frameCount) * 1000.0f;
			frameCount = 0.0f;
			fpsTimer = 0.0f;
		}

		// Update 3D Scene animations
		g_scene.Update(deltaTime);

		// New ImGui Frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// Render Unreal Engine 5 Editor UI & 3D Gizmos
		bool shouldExit = false;
		g_engineUI.Render(g_scene, g_camera, currentFps, currentFrameTimeMs,
		                  g_currentVertexCount, g_currentIndexCount, shouldExit);
		if (shouldExit)
		{
			glfwSetWindowShouldClose(g_window, GLFW_TRUE);
		}

		// Device Loss Notification Overlay (dev.md Task 3)
		if (g_deviceLost)
		{
			ImGui::OpenPopup("GPU Device Lost");
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
			ImGui::SetNextWindowSize(ImVec2(520, 240), ImGuiCond_Appearing);
			if (ImGui::BeginPopupModal("GPU Device Lost", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
			{
				ImGui::TextColored(ImVec4(1.0f, 0.25f, 0.25f, 1.0f), "CRITICAL: GPU Device Was Lost (Driver Reset)");
				ImGui::Separator();
				ImGui::Spacing();
				ImGui::TextWrapped("The graphics driver reset or the GPU device was removed. 3D rendering has been halted to prevent the editor from freezing.");
				ImGui::Spacing();
				ImGui::TextWrapped("You can still save your level to avoid losing changes before restarting the editor.");
				ImGui::Spacing();
				ImGui::Separator();
				ImGui::Spacing();
				if (ImGui::Button("Save Current Level", ImVec2(170, 32)))
				{
					g_engineUI.SaveLevel(g_scene);
				}
				ImGui::SameLine();
				if (ImGui::Button("Restart / Exit", ImVec2(140, 32)))
				{
					glfwSetWindowShouldClose(g_window, GLFW_TRUE);
				}
				ImGui::EndPopup();
			}
		}

		ImGui::Render();

		// Update 3D Scene GPU Buffers AFTER gizmo has applied any transform changes
		// When device is lost, skip 3D scene updates to prevent crashes/stalls
		if (g_deviceLost)
		{
			try {
				renderFrame();
			} catch (...) {}
		}
		else
		{
			try {
				updateSceneGeometry();
				updateConstantBuffer();

				// Pre-upload all scene batch textures before the frame command list begins recording
				PreRenderUploadTextures();

				// Render DirectX 12 Frame
				renderFrame();
			} catch (const std::exception& ex) {
				std::cerr << "[RECOVERY] Frame exception: " << ex.what() << "\n";
				g_engineUI.AddLog("LogRecovery", std::string("Frame exception caught, skipping frame: ") + ex.what(), 3);
			} catch (...) {
				std::cerr << "[RECOVERY] Unknown frame exception\n";
				g_engineUI.AddLog("LogRecovery", "Unknown frame exception caught, skipping frame", 3);
			}
		}
	}

	// 5. Cleanup
	cleanUpD3D12();

	glfwDestroyWindow(g_window);
	glfwTerminate();

	return EXIT_SUCCESS;
}
