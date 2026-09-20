# Fix: Eunoia-Editor randomly freezes and requires a full restart

## Root cause (confirmed by code review of `Cube.cpp`)

1. **SRV descriptor heap overflow.** `g_srvDescHeap` is created with a fixed
   `NumDescriptors = 2048` (see `initD3D12`), but three call sites increment
   `g_srvDescriptorAllocIndex` and write into the heap with **no bounds
   check**:
   - `ImGui_SrvAlloc` (~line 249)
   - `GetOrLoadGPUTexture` → `UploadTextureToD3D12` (~line 384)
   - `GetOrCreateMaterialTable` (~line 490, allocates 5 slots per unique
     material combo)

   `ImGui_SrvFree` is a no-op, so slots are **never reclaimed**. Over a long
   editing session (importing assets, duplicating objects with unique
   materials, reloading scenes), the index eventually passes 2048 and the
   code writes descriptors past the end of the heap. This is undefined
   behavior on the GPU and typically triggers a GPU page fault / driver
   TDR (device removal).

2. **Fake recovery on device loss.** `SafeWaitForFence` (~line 265) detects
   device removal via `GetDeviceRemovedReason()`, but instead of actually
   recovering it does:
   ```cpp
   fence->Signal(targetValue); // fakes completion
   return false;               // ...and the caller just carries on
   ```
   The device, swap chain, and GPU resources are never recreated. The CPU
   loop (input polling, ImGui, `glfwPollEvents`) keeps running, but every
   D3D12 call after this point silently fails on the dead device, so the
   screen simply stops updating. Nothing crashes — it just looks "stuck"
   forever, and only a fresh process (relaunch) gets a working device again.

## Task 1 (critical): bounds-check every SRV descriptor allocation

Add a single shared allocator helper and route all three call sites through
it instead of incrementing `g_srvDescriptorAllocIndex` directly.

- Add a named constant near the other `g_srv*` globals:
  ```cpp
  static const UINT SRV_HEAP_CAPACITY = 2048; // must match srvHeapDesc.NumDescriptors in initD3D12
  ```
- Add a helper function near `ImGui_SrvAlloc`/`ImGui_SrvFree`:
  ```cpp
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
  ```
- In `ImGui_SrvAlloc`, replace the direct increment with a call to
  `TryAllocSrvDescriptors(1, idx)`; if it fails, return the heap's slot 0
  handle (a safe, already-allocated descriptor) instead of an OOB one.
- In `UploadTextureToD3D12` (the function `GetOrLoadGPUTexture` calls at
  ~line 384), replace `UINT descIdx = g_srvDescriptorAllocIndex++;` with
  `TryAllocSrvDescriptors(1, descIdx)`; if it fails, return an empty/invalid
  `DX12GpuTexture` so the caller falls back to `fallback` instead of
  corrupting the heap.
- In `GetOrCreateMaterialTable` (~line 490), replace the manual
  `UINT baseIdx = g_srvDescriptorAllocIndex; g_srvDescriptorAllocIndex += 5;`
  with `TryAllocSrvDescriptors(5, baseIdx)`; if it fails, return the
  descriptor table for a previously-allocated safe default (or skip
  creating this material and use `g_fallback*` textures directly) instead
  of writing 5 descriptors OOB.
- Apply the same pattern to the shadow-map SRV allocation (~line 627).

This alone removes the memory corruption that is the actual trigger for the
GPU hang — after this, running out of slots degrades to "some textures show
as fallback," not a frozen screen.

## Task 2 (recommended): raise the heap size and reclaim unused slots

- Bump `srvHeapDesc.NumDescriptors` in `initD3D12` (~line 746) from `2048`
  to something like `8192`, and update `SRV_HEAP_CAPACITY` to match.
- Optionally implement a simple free-list: when a texture/material is no
  longer referenced (e.g. asset removed from the scene, project closed),
  push its base index onto a `std::vector<UINT> g_freeSrvSlots` and have
  `TryAllocSrvDescriptors` pop from that list before extending
  `g_srvDescriptorAllocIndex`. Not required to fix the freeze, but prevents
  the heap from filling up over long sessions even with reasonable asset
  counts.

## Task 3: make real device loss visible instead of silently freezing

In `SafeWaitForFence` (~line 265), when `GetDeviceRemovedReason()` returns a
failure code (real device loss, as opposed to an ordinary transient stall):

- Do **not** just fake-signal the fence and continue rendering 3D content.
- Set a global flag, e.g. `static bool g_deviceLost = false;` → set it
  `true` here.
- In the main loop in `main()` (~line 2016 onward), check `g_deviceLost`
  before calling `updateSceneGeometry()/updateConstantBuffer()/PreRenderUploadTextures()/renderFrame()`:
  - If set, skip those calls and instead render a plain ImGui popup/overlay
    saying something like: *"GPU device was lost (driver reset). Please
    save your work if possible and restart the editor."* This still uses
    `ImGui::Render()` + a minimal present, which does not require the lost
    device's resources.
  - This turns an indefinite silent freeze into a clear, honest message the
    user can act on, instead of guessing whether the app hung.
- (Optional, larger effort, not required for this fix): implement full
  device recreation — reinitialize `g_d3dDevice`, `g_swapChain`, all
  `g_renderTargets`, `g_commandAllocators`, pipeline state, and reload all
  GPU textures from `TextureManager`'s CPU-side cache — so the editor can
  recover without a restart at all. Flag this as a follow-up if time-boxed.

## Acceptance criteria

- Importing/loading many unique textured assets in one session no longer
  corrupts the SRV heap; once the heap is full, new assets fall back to
  placeholder textures with a single logged warning, and the app keeps
  running normally.
- If a real GPU device-removal happens (e.g. forced via driver tools or a
  deliberately malformed shader), the editor shows a clear on-screen
  "GPU device lost, please restart" message instead of freezing with no
  explanation.
- No behavior change in the normal case (heap usage well under capacity,
  no device removal).