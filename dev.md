````md
# Engine Debug Logging + DX12 Device Loss Investigation

Create a `logs.elogs` file in the project root directory.

The purpose of this file is to provide enough context for the CLI/debugging system to determine what happened immediately before an engine error or crash.

## Logging Requirements

Maintain a rolling history of the **last 50 engine actions/interactions**.

Log every important user and engine action, including:

- Mouse clicks
- Mouse button releases
- Keyboard input
- UI button clicks
- UI element interactions
- Object selection
- Object creation/deletion
- Transform changes
- Gizmo interactions
- Scene changes
- Asset loading/unloading
- Entity/component creation or deletion
- Rendering-related actions
- Play/Stop/Restart
- Project/scene operations
- Important engine state changes
- Command execution
- Any other significant engine action

Each log entry should contain:

- Timestamp
- Action/event type
- Relevant object/entity/UI element
- Important parameters/state
- Current scene/context if available

Example:

```text
[18:32:41.125] CLICK
Target: Light_01
Position: (824, 412)

[18:32:41.140] OBJECT_SELECTED
Object: Light_01
Type: PointLight

[18:32:42.021] GIZMO_MOVE
Object: Light_01
Position: (10.2, 4.5, -2.1)

[18:32:42.512] RENDER_COMMAND
Pass: ShadowPass
Object: Light_01

[18:32:43.003] ERROR
System: DirectX12
Code: 0x887A0006
Message: DXGI_ERROR_DEVICE_HUNG
````

## Rolling 50-Action History

Only keep the most recent 50 actions/events during normal operation.

When a new action is added:

```text
51st action → remove oldest entry
52nd action → remove next oldest entry
...
```

Do not allow the file to grow indefinitely.

## Error Logging

When ANY error, warning, exception, assertion failure, GPU error, DirectX error, or device-loss event occurs:

1. Record the error in `logs.elogs`.
2. Preserve the preceding 50 actions/events.
3. Record the error immediately after the relevant action history.
4. Include:

   * Error code
   * Error message
   * Subsystem
   * Timestamp
   * Current scene
   * Relevant object/resource if available
   * GPU/renderer information
   * Call site/source information if available

For the DX12 error:

```text
DXGI_ERROR_DEVICE_HUNG
0x887A0006
```

make sure the log contains the actions that happened immediately before the device loss.

## Important Debugging Behavior

The logging system must be implemented centrally so that engine systems do not need to manually create separate log files.

Use a thread-safe logger if the engine is multithreaded.

Do not significantly impact engine performance.

Do not log sensitive data or unnecessarily dump huge buffers/resources.

## CLI Error Investigation

When an engine error occurs, the CLI debugging workflow should inspect:

1. The error information.
2. The last 50 actions in `logs.elogs`.
3. The actions immediately preceding the error.
4. Related engine/renderer state.
5. Relevant source code.
6. Relevant DirectX 12 resources/commands.

Then determine the most likely cause and fix the underlying code rather than simply suppressing the error.

For example, if the log shows:

```text
SELECT Light_01
GIZMO_MOVE Light_01
UPDATE_LIGHT_BUFFER
UPDATE_DESCRIPTOR
RENDER_SHADOW
EXECUTE_COMMAND_LIST
DX12 DEVICE HUNG
```

the CLI should investigate the rendering/resource/command operations associated with those actions.

## DX12 Device Loss

Specifically investigate:

```text
[RECOVERY] Present detected device loss (0x887A0006)
```

Do not simply ignore or suppress this error.

Check for:

* Invalid DX12 resource states
* Incorrect resource barriers
* Descriptor heap errors
* Invalid descriptors
* Command allocator reuse
* Command list reuse
* Missing/wrong fence synchronization
* GPU work submitted after resource destruction
* Invalid resource lifetime
* Shader problems
* GPU timeout/TDR
* Invalid UAV/SRV/RTV/DSV usage
* Excessive GPU workloads

Enable the DirectX 12 Debug Layer and GPU-Based Validation in development builds where possible.

## Goal

The final system should make `logs.elogs` a compact **reproduction history**.

Whenever an error occurs, the CLI should be able to read:

```text
LAST 50 ACTIONS
        ↓
ERROR
        ↓
ENGINE STATE
        ↓
SOURCE CODE
        ↓
ROOT CAUSE
        ↓
FIX
```

Do not remove the logging system after fixing the current DX12 issue. It should remain as a permanent engine debugging facility.

```
```
