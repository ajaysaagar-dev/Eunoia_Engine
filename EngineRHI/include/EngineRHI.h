#pragma once
// ============================================================================
// EngineRHI — Rendering Hardware Interface module
//
// This is the ONLY place in the codebase that should include <d3d12.h> and
// <dxgi1_4.h>. All higher-level modules (EngineRenderer, Editor) depend on
// the types declared in this module rather than directly on D3D12 headers.
//
// Module headers:
//   EngineRHI/Device.h          — adapter selection, device + debug layer
//   EngineRHI/SwapChain.h       — swapchain creation, Present, RTV management
//   EngineRHI/CommandContext.h  — command list/allocator reset/close helpers
//   EngineRHI/Fence.h           — SafeWaitForFence, device-loss detection
//   EngineRHI/DescriptorAllocator.h — bounds-checked SRV/RTV/DSV allocator
//   EngineRHI/UploadHeap.h      — UploadTextureToD3D12, upload fence
//   EngineRHI/PipelineState.h   — shader compile + PSO creation helpers
// ============================================================================

#include "EngineRHI/Device.h"
#include "EngineRHI/SwapChain.h"
#include "EngineRHI/CommandContext.h"
#include "EngineRHI/Fence.h"
#include "EngineRHI/DescriptorAllocator.h"
#include "EngineRHI/UploadHeap.h"
#include "EngineRHI/PipelineState.h"
