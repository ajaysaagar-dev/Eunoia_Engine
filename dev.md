# Implement Complete EunoiaBehaviour System

Implement a complete Unity-style C++ Behaviour system for the Eunoia Game Engine.

Do not create a mock/demo implementation. Integrate the system into the existing engine architecture, editor, scene system, reflection/serialization system, Content Browser, Details Panel, hierarchy, object/component system, runtime game loop, and editor play-mode system.

Before changing code, inspect the existing project architecture and identify the correct existing systems for:

* Scene / Entity / GameObject management
* Components
* Reflection / RTTI / metadata
* Serialization
* Editor Details Panel / Inspector
* Hierarchy
* Content Browser
* Asset creation
* Runtime update loop
* Object IDs / GUIDs
* Rendering object types such as lights, meshes and shapes
* Editor viewport/game view
* Main editor menu bar
* Play/Stop state

Reuse the existing architecture wherever possible instead of introducing duplicate systems.

---

# 1. Base Behaviour Class

Create or integrate the base class:

```cpp
class EunoiaBehaviour
```

Every user-created behaviour must inherit from `EunoiaBehaviour`.

Example:

```cpp
class PlayerController : public EunoiaBehaviour
{
public:
    void Start() override;
    void Update(float deltaTime) override;
};
```

Provide the lifecycle:

```cpp
virtual void OnCreate() {}
virtual void OnEnable() {}
virtual void Start() {}
virtual void Update(float deltaTime) {}
virtual void FixedUpdate(float fixedDeltaTime) {}
virtual void LateUpdate(float deltaTime) {}
virtual void OnDisable() {}
virtual void OnDestroy() {}
```

Use the lifecycle functions that match the engine's existing update architecture.

The Behaviour must have access to the object/entity it is attached to.

Provide one canonical owner/entity API such as:

```cpp
GetOwner()
GetEntity()
```

using the naming convention already used by the engine.

---

# 2. Multiple Behaviours Per Object

Every scene object/entity must be able to contain multiple behaviours.

Use the existing engine container architecture or an equivalent dynamic container.

Example:

```text
Player
 ├── Transform
 ├── MeshRenderer
 ├── Collider
 ├── PlayerController
 ├── HealthBehaviour
 └── WeaponBehaviour
```

Support:

* Add multiple behaviours
* Remove behaviours
* Enable/disable individual behaviours
* Reorder behaviours where meaningful
* Execute all enabled behaviours
* Destroy behaviours safely
* Prevent invalid/dangling references

---

# 3. Behaviour Lifecycle

Integrate Behaviour execution into the actual runtime loop.

Expected lifecycle:

```text
Object Creation
    ↓
Behaviour Construction
    ↓
OnCreate()
    ↓
OnEnable()
    ↓
Start()
    ↓
-------------------------
Game Loop
    ↓
FixedUpdate()
    ↓
Update()
    ↓
LateUpdate()
-------------------------
    ↓
OnDisable()
    ↓
OnDestroy()
```

`Start()` must only execute once for each Behaviour instance unless the engine explicitly recreates that instance.

Disabled behaviours must not receive normal update callbacks.

---

# 4. User-Created Behaviour Scripts

A user must be able to create a Behaviour from the Content Browser.

Workflow:

```text
Content Browser
    ↓
Right Click
    ↓
Create
    ↓
Behaviour
```

Generate:

```text
PlayerController.h
PlayerController.cpp
```

The generated class must inherit from:

```cpp
EunoiaBehaviour
```

The Content Browser asset type must be:

```text
Behaviour
```

not generic `C++ File`, `Source`, or another generic type.

Use the project's existing C++ source-generation conventions.

---

# 5. Behaviour Registration / Discovery

Automatically discover and register Behaviour classes.

Architecture:

```text
EunoiaBehaviour
       ↓
BehaviourRegistry
       ↓
Registered Behaviour Classes
       ↓
Editor Behaviour Selector
```

Each Behaviour should have:

* Class name
* Display name
* Type ID
* Runtime factory
* Reflection metadata
* Exposed property metadata

A compiled Behaviour must become available to the editor without manually hardcoding each class.

---

# 6. Add Behaviour From Details Panel

For every selected scene object, add a:

```text
Behaviours
```

section.

Example:

```text
Behaviours
--------------------------------
PlayerController
HealthBehaviour
WeaponBehaviour

[ + Add Behaviour ]
```

The Add Behaviour menu must contain all registered Behaviour classes.

Include search/filter support.

Selecting a Behaviour must instantiate and attach it to the selected object.

---

# 7. Public Behaviour Properties

Public variables inside Behaviour classes must automatically appear in the Details Panel.

Example:

```cpp
class EnemyController : public EunoiaBehaviour
{
public:

    int Health = 100;
    float MoveSpeed = 5.0f;
    bool CanAttack = true;
    std::string EnemyName = "Enemy";
};
```

Details Panel:

```text
EnemyController
--------------------------------
Health             [ 100 ]
Move Speed         [ 5.0 ]
Can Attack         [ ✓ ]
Enemy Name         [ Enemy ]
```

Do not require manually writing editor UI code for every variable.

Reuse the existing reflection/metadata system or extend it with a reusable property system.

---

# 8. Supported Primitive Types

At minimum support:

```text
bool

int
int8
int16
int32
int64

uint8
uint16
uint32
uint64

float
double

string
```

Recommended:

```text
Vector2
Vector3
Vector4
Color
Quaternion
Transform
```

Use an appropriate Details Panel control for each type.

---

# 9. Scene Object Reference Properties

Support Behaviour properties that reference engine objects/components.

Examples:

```cpp
Light* WarningLight = nullptr;
MeshRenderer* TargetMesh = nullptr;
Collider* Trigger = nullptr;
Camera* TargetCamera = nullptr;
```

The Details Panel must recognize these as object-reference properties.

Example:

```text
Warning Light
[ Select Light ▼ ]

Target Mesh
[ Select Mesh ▼ ]

Trigger
[ Select Collider ▼ ]
```

---

# 10. Type-Specific Scene Object Dropdown

Object-reference fields must be type-aware.

For:

```cpp
Light* WarningLight;
```

only compatible Light objects/components should be shown.

For:

```cpp
MeshRenderer* TargetMesh;
```

only compatible mesh-rendering objects should be shown.

For:

```cpp
Collider* Trigger;
```

only compatible collider objects should be shown.

For every other supported engine object/component type, automatically filter according to the declared type.

Never allow incompatible objects to be selected.

---

# 11. Current Scene Object Listing

Object selectors must retrieve objects from the currently active scene.

The list must update when:

* Objects are created
* Objects are destroyed
* Objects are renamed
* Scene changes
* Scene reloads
* Relevant components are added/removed

Do not hardcode scene object names.

---

# 12. Hierarchy Drag & Drop

Object references must support drag-and-drop from the Hierarchy.

Example:

```text
Hierarchy
 ├── Player
 ├── MainCamera
 ├── WarningLight
 ├── Door
 └── TriggerZone
```

Details Panel:

```text
Warning Light
[ Drop Light Here ]
```

Dragging `WarningLight` from the Hierarchy into the field must:

1. Detect the dropped object.
2. Resolve its entity/object/component.
3. Check type compatibility.
4. Assign the reference.
5. Refresh the Details Panel.
6. Persist the reference using a stable object/entity identifier.

If incompatible, reject the drop safely without changing the existing reference.

---

# 13. Object Picker

Implement a reusable typed object picker.

Example:

```text
Warning Light
[ WarningLight ▼ ]
```

Clicking opens:

```text
Search Objects...

WarningLight
TorchLight
HallLight
PlayerLight
```

Only compatible objects should appear.

Provide:

```text
None / Clear
```

for removing the assignment.

---

# 14. Behaviour Property Serialization

All exposed Behaviour variables must be serializable.

Example:

```cpp
int Health = 250;
float Speed = 7.5f;
bool Aggressive = false;
Light* DetectionLight = nullptr;
```

Save/load must preserve all values.

Never serialize raw memory pointers for scene references.

Serialize stable identifiers such as:

```text
Entity ID
Object GUID
Component ID
```

or the existing engine's persistent identifier.

Resolve those references after loading.

---

# 15. GetRespectiveObject API

Provide the required runtime API:

```text
GetRespectiveObject
```

This API must allow a Behaviour to access the object assigned to one of its object-reference properties.

Support typed access using the engine's C++ conventions.

Conceptually:

```cpp
GetRespectiveObject.Light(...)
GetRespectiveObject.Mesh(...)
GetRespectiveObject.Shape(...)
GetRespectiveObject.Entity(...)
GetRespectiveObject.Transform(...)
GetRespectiveObject.Camera(...)
GetRespectiveObject.AudioSource(...)
GetRespectiveObject.Collider(...)
```

The actual supported types must be derived from the engine's implemented object/component types.

Also support a strongly typed generic implementation where appropriate:

```cpp
template<typename T>
T* GetRespectiveObject(T* assignedObject);
```

Example:

```cpp
Light* light = GetRespectiveObject(WarnLight);
MeshRenderer* mesh = GetRespectiveObject(TargetMesh);
```

Avoid duplicate object-resolution logic and prefer compile-time type safety.

---

# 16. Invalid Reference Handling

Safely handle:

```text
nullptr
Deleted object
Destroyed component
Scene unload
Scene change
Invalid GUID
Missing reference
```

Never crash due to an invalid Behaviour object reference.

The Details Panel should clearly indicate missing references.

---

# 17. Behaviour Enable / Disable

Every Behaviour must have an enabled state.

Details Panel:

```text
Enabled
[ ✓ ]
```

Disabled:

```text
Enabled
[ ]
```

Lifecycle:

```text
Enabled → Disabled
    ↓
OnDisable()

Disabled → Enabled
    ↓
OnEnable()
```

Do not recreate the Behaviour when simply toggling enabled state.

---

# 18. Behaviour Removal

Provide a context menu/button:

```text
Remove Behaviour
```

Removing a Behaviour must safely execute the required lifecycle cleanup and remove the instance and its serialized state.

---

# 19. Behaviour Reordering

Where execution order matters, allow the user to reorder behaviours in the Details Panel.

Example:

```text
Behaviours

☰ PlayerController
☰ HealthBehaviour
☰ WeaponBehaviour
☰ AnimationBehaviour
```

Runtime execution should follow the defined order if the engine supports ordered Behaviour execution.

---

# 20. Content Browser Behaviour Asset Type

The Content Browser must identify Behaviour assets as:

```text
Behaviour
```

Example:

```text
PlayerController
Type: Behaviour

EnemyAI
Type: Behaviour

DoorController
Type: Behaviour
```

Asset metadata must identify the base class as:

```text
EunoiaBehaviour
```

---

# 21. Behaviour Creation

Implement:

```text
Content Browser
→ Right Click
→ Create
→ Behaviour
```

Dialog:

```text
Create Behaviour

Name:
[ PlayerController ]

[ Create ]
[ Cancel ]
```

Generate the appropriate `.h` and `.cpp` files using the existing project/source organization.

---

# 22. Compilation / Refresh

After creation:

```text
Create Behaviour
      ↓
Generate source
      ↓
Compile
      ↓
Register Behaviour
      ↓
Refresh Reflection
      ↓
Behaviour appears in Add Behaviour
```

Compilation failures must be handled through the existing error/logging system without crashing the editor.

---

# 23. Reflection Metadata

Every exposed Behaviour property must provide reusable metadata:

```text
Property Name
Property Type
Default Value
Current Value
Editable
Serializable
Object Reference
Referenced Object Type
```

Object references must additionally contain their expected reference type.

---

# 24. Property Categories

Support property grouping where the engine's reflection system allows it.

Example:

```text
Movement
    Move Speed

Combat
    Damage

References
    Warning Light
```

---

# 25. Undo / Redo

Integrate Behaviour modifications with the existing Undo/Redo system.

Support:

```text
Property changes
Object reference assignment
Reference clearing
Behaviour addition
Behaviour removal
Behaviour reordering
Enable/disable
```

---

# 26. Prefab / Scene Integration

If the engine supports prefabs, Behaviour data must integrate with the existing prefab system.

Preserve:

```text
Behaviour class
Behaviour properties
Primitive values
Object references where valid
Enabled state
Behaviour ordering
```

Do not break existing scene serialization.

---

# 27. Editor Details Panel Refresh

Refresh the Details Panel automatically after:

* Behaviour added
* Behaviour removed
* Property edited
* Object reference changed
* Object renamed
* Object deleted
* Scene changed
* Compilation succeeds
* Behaviour registry changes

---

# 28. Performance

Do not perform full scene searches every frame for Behaviour object references.

Cache resolved references where appropriate.

Editor object-picker lists may use cached type-filtered scene data.

Runtime Behaviour updates must remain efficient.

---

# 29. Safety

Prevent:

```text
Dangling pointers
Use-after-free
Invalid casts
Duplicate registration
Double destruction
Update on destroyed objects
Start called repeatedly
Invalid scene references
Null dereference crashes
```

Follow the engine's existing memory-management model.

---

# 30. Play Mode / Editor Mode

Implement a clear editor Play Mode system.

## Default State

When the editor opens, the game must always begin in:

```text
STOPPED / EDITOR MODE
```

The user edits the scene normally.

The game simulation must not automatically run.

Example top menu bar:

```text
File   Edit   View   Window   Help

                 [ ▶ Play ]
```

The initial state must be:

```text
Editor Mode
Game Running = false
```

---

# 31. Play Button

Add/use the existing top menu bar Play button.

Example:

```text
[ ▶ Play ]
```

When the user clicks Play:

```text
Editor Mode
    ↓
Play Mode
    ↓
Start Game
    ↓
Run Behaviours
    ↓
Run Physics
    ↓
Run Game Simulation
```

All Behaviour lifecycle methods must participate in the game simulation.

For example:

```text
Start()
FixedUpdate()
Update()
LateUpdate()
```

must run during Play Mode according to the engine loop.

---

# 32. Play Mode UI Transition

When Play is clicked, transition the editor into a game-only presentation mode.

The editor UI must be hidden, including:

```text
Top menu bar
Toolbar
Hierarchy
Details Panel
Content Browser
Scene editor controls
Editor gizmos
Editor overlays
Other editor-only panels
```

Only the running game view should remain visible.

Expected result:

```text
+--------------------------------------------------+
|                                                  |
|                                                  |
|                  GAME VIEW                       |
|                                                  |
|                                                  |
|                                                  |
+--------------------------------------------------+
```

The game view must occupy the available screen area/fullscreen presentation area.

Do not render editor UI elements over the game unless they are explicitly part of the game itself.

---

# 33. Play Mode Input

When Play Mode is active:

* Game input must be routed to the running game.
* Editor selection/input behavior must be disabled.
* Editor shortcuts must not accidentally modify the scene.
* Mouse and keyboard input should behave as game input according to the existing input system.

The editor must not continue manipulating scene objects while the game is running.

---

# 34. Stop Game With Delete Key

While Play Mode is active, pressing:

```text
DELETE
```

must immediately stop the game and return the application to Editor Mode.

Required transition:

```text
Play Mode
    ↓
DELETE
    ↓
Stop Game
    ↓
Destroy/cleanup runtime state
    ↓
Restore editor state
    ↓
Editor Mode
```

The Delete key is the required Play Mode stop key.

It must work even when the game view has input focus.

Do not interpret Delete as a gameplay/editor action while it is being used to stop Play Mode.

---

# 35. Restore Editor After Stop

When the Delete key stops Play Mode:

```text
Game Simulation
      ↓
Stop
      ↓
Restore Editor UI
      ↓
Show Hierarchy
      ↓
Show Details Panel
      ↓
Show Content Browser
      ↓
Show Toolbar/Menu
      ↓
Restore Scene Editor
```

Return to the normal stopped/editor state.

The editor scene must not become permanently corrupted by runtime modifications.

Use the engine's existing scene/runtime-copy architecture if available.

---

# 36. Play Mode State Safety

Play Mode must safely handle:

```text
Behaviour creation
Behaviour destruction
Scene object destruction
Physics state
Rendering state
Audio state
Input state
Object references
Timers
Runtime allocations
```

When stopping, clean up runtime-only state correctly.

Avoid leaving dangling runtime pointers or Behaviour instances in the editor scene.

---

# 37. Behaviour Runtime Example

The final system must support code similar to:

```cpp
#include "EunoiaBehaviour.h"

class DoorController : public EunoiaBehaviour
{
public:

    float OpenSpeed = 2.5f;
    int RequiredKeys = 1;
    bool Locked = true;

    Light* WarningLight = nullptr;
    MeshRenderer* DoorMesh = nullptr;

    void Start() override
    {
        auto* light = GetRespectiveObject.Light(WarningLight);

        if (light)
        {
            // Configure light.
        }
    }

    void Update(float deltaTime) override
    {
        if (!Locked)
        {
            // Door logic.
        }
    }
};
```

Details Panel:

```text
DoorController
------------------------------------

Enabled
[ ✓ ]

Open Speed
[ 2.5 ]

Required Keys
[ 1 ]

Locked
[ ✓ ]

Warning Light
[ WarningLight ▼ ]

Door Mesh
[ DoorMesh ▼ ]
```

Hierarchy drag-and-drop:

```text
Hierarchy
WarningLight
    ↓ drag
Details Panel
Warning Light
    ↓ drop
DoorController.WarningLight
```

Play workflow:

```text
Editor
  ↓
Click [ ▶ Play ]
  ↓
Hide editor UI
  ↓
Fullscreen Game View
  ↓
Game runs
  ↓
Press DELETE
  ↓
Stop Game
  ↓
Restore Editor UI
  ↓
Editor Mode
```

---

# 38. Required Editor Workflow

Final workflow:

```text
CONTENT BROWSER
      │
      └── Create → Behaviour
                     │
                     ▼
              C++ Behaviour
                     │
                     ▼
                Compile
                     │
                     ▼
             Behaviour Registry
                     │
                     ▼
HIERARCHY ──► Select Object
                     │
                     ▼
               DETAILS PANEL
                     │
                     ├── Add Behaviour
                     │
                     ├── Primitive Inputs
                     │
                     ├── Object Dropdowns
                     │
                     └── Hierarchy Drag & Drop
                              │
                              ▼
                        Serialized Data

TOP MENU BAR
      │
      └── [ ▶ Play ]
              │
              ▼
          Play Mode
              │
              ▼
      Hide Editor UI
              │
              ▼
       Fullscreen Game
              │
              ▼
       Press [DELETE]
              │
              ▼
        Stop Play Mode
              │
              ▼
       Restore Editor UI
```

---

# 39. Final Verification Checklist

Verify all of the following:

```text
[ ] EunoiaBehaviour exists.
[ ] C++ classes can inherit from EunoiaBehaviour.
[ ] Behaviour lifecycle works.
[ ] Multiple Behaviours can be attached to one object.
[ ] Behaviours can be enabled/disabled.
[ ] Behaviours can be removed.
[ ] Behaviours can be reordered where supported.
[ ] Behaviour can be created from Content Browser.
[ ] Behaviour asset type displays as "Behaviour".
[ ] Generated C++ Behaviour compiles.
[ ] Compiled Behaviour is automatically registered.
[ ] Registered Behaviour appears in Add Behaviour.
[ ] Public primitive properties appear in Details Panel.
[ ] Integer input works.
[ ] Float input works.
[ ] Boolean input works.
[ ] String input works.
[ ] Supported vector/color/etc. types work.
[ ] Object-reference properties are recognized.
[ ] Typed object dropdown works.
[ ] Current scene objects are listed.
[ ] Dropdown filters by requested type.
[ ] Hierarchy drag-and-drop works.
[ ] Invalid drag-and-drop is rejected safely.
[ ] Object references serialize using stable IDs.
[ ] References survive scene save/load.
[ ] Deleted references become safely invalid.
[ ] GetRespectiveObject API works.
[ ] Undo/Redo works.
[ ] Inspector refreshes correctly.
[ ] No dangling pointers occur.

[ ] Editor starts in STOPPED / EDITOR MODE.
[ ] Game does not automatically run.
[ ] Top menu bar contains/uses Play control.
[ ] Clicking Play starts Play Mode.
[ ] Behaviour lifecycle executes during Play Mode.
[ ] Game input is active during Play Mode.
[ ] Editor UI is hidden during Play Mode.
[ ] Game View occupies the screen/fullscreen presentation.
[ ] Hierarchy is inaccessible during Play Mode.
[ ] Details Panel is inaccessible during Play Mode.
[ ] Content Browser is hidden during Play Mode.
[ ] Editor gizmos/overlays are hidden during Play Mode.
[ ] DELETE key stops Play Mode.
[ ] DELETE works while Game View has focus.
[ ] Runtime state is cleaned up on Stop.
[ ] Editor UI is restored after Stop.
[ ] Editor returns to STOPPED / EDITOR MODE.
[ ] Existing editor scene remains safe after Play/Stop.
[ ] No crashes occur when repeatedly playing/stopping.
```

---

# 40. Implementation Requirement

Implement this as a real integrated Eunoia Engine system.

Do not stop after creating only:

```text
EunoiaBehaviour.h
```

All required runtime, reflection, serialization, Behaviour registry, Content Browser, Details Panel, object picker, hierarchy drag-and-drop, Play Mode, fullscreen game presentation, Delete-to-stop behavior, and editor restoration must be implemented and connected to the existing engine architecture.

After implementation, build/compile the engine and fix all compilation errors introduced by the implementation.

Then test the complete workflow from:

```text
Create Behaviour
→ Compile
→ Add Behaviour
→ Edit Properties
→ Assign Scene Objects
→ Save Scene
→ Enter Play
→ Fullscreen Game View
→ Run Behaviour
→ Press DELETE
→ Stop Game
→ Restore Editor
```

Do not leave placeholder implementations for any of these core requirements.
