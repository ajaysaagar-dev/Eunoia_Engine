# EUNOIA ENGINE — IMPLEMENT A COMPLETE UNITY-STYLE C++ BEHAVIOUR / SCRIPT SYSTEM

Implement a complete Unity-style gameplay scripting system for Eunoia Engine using native C++.

The final system must provide the same overall workflow developers expect from Unity `MonoBehaviour`:

* Create a script/Behaviour from the editor.
* Script is a C++ class derived from `EunoiaBehaviour`.
* Compile the project.
* Script becomes available as an attachable Behaviour.
* Attach multiple Behaviours to any compatible GameObject/Entity.
* Select an object and edit Behaviour variables from the Details/Inspector panel.
* Primitive variables use proper editor controls.
* Object/component references use typed object pickers.
* Scene objects can be assigned from dropdowns or Hierarchy drag-and-drop.
* Script values and references are serialized with the scene/prefab.
* Behaviours execute through a proper runtime lifecycle.
* Behaviours can access their owner object and referenced objects.
* Play Mode starts the game and hides editor UI.
* Delete stops Play Mode and restores the editor.
* Scripts can be created, compiled, reloaded, attached, detached, serialized, deserialized, and executed reliably.

Do NOT implement a toy/demo system.

Integrate this into the existing Eunoia Engine architecture.

Before modifying anything, inspect the complete codebase and understand the current implementations for:

* Entity / GameObject
* Components
* Scene
* Scene serialization
* Reflection / RTTI
* Editor
* Details Panel
* Hierarchy
* Content Browser
* Asset database
* C++ build system
* Runtime loop
* Input system
* Rendering
* Object IDs / GUIDs
* Memory ownership
* Hot reload or module reload if already present
* Undo / Redo
* Prefabs, if present
* Editor viewport/game viewport
* Play / Stop mode

Reuse existing systems whenever possible.

Do not introduce a second unrelated entity system, reflection system, serializer, asset system, or scene-object system.

---

# 1. CORE ARCHITECTURE

Implement these core systems:

```text
EunoiaBehaviour
        │
        ├── Behaviour Instance
        │
        ├── Behaviour Reflection
        │
        ├── Behaviour Serialization
        │
        └── Behaviour Runtime
                │
                ▼
          BehaviourSystem
                │
                ▼
          Entity / GameObject
```

Editor side:

```text
BehaviourRegistry
       │
       ├── Behaviour Class Discovery
       ├── Behaviour Metadata
       ├── Behaviour Factory
       └── Behaviour Asset Information
```

Editor UI:

```text
Content Browser
      │
      └── Create → Behaviour
                    │
                    ▼
              .h + .cpp
                    │
                    ▼
                 Compile
                    │
                    ▼
           Behaviour Registry
                    │
                    ▼
Hierarchy → Select Object
                    │
                    ▼
              Details Panel
                    │
                    ├── Add Behaviour
                    ├── Edit properties
                    ├── Assign references
                    └── Enable / Disable
```

---

# 2. BASE CLASS: EunoiaBehaviour

Create the engine base class:

```cpp
class EunoiaBehaviour
{
public:
    virtual ~EunoiaBehaviour() = default;

    virtual void OnCreate() {}
    virtual void OnEnable() {}
    virtual void Start() {}
    virtual void FixedUpdate(float fixedDeltaTime) {}
    virtual void Update(float deltaTime) {}
    virtual void LateUpdate(float deltaTime) {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}
};
```

Adapt method declarations to the existing engine's naming and architecture where necessary, but maintain this lifecycle model.

Every user-created Behaviour must inherit from:

```cpp
EunoiaBehaviour
```

Example:

```cpp
class PlayerController : public EunoiaBehaviour
{
public:

    void Start() override
    {
    }

    void Update(float deltaTime) override
    {
    }
};
```

The base Behaviour must know its owning object/entity.

Provide a safe API such as:

```cpp
Entity* GetOwner();
const Entity* GetOwner() const;
```

or an equivalent engine-native implementation.

Do not expose raw unsafe ownership.

---

# 3. BEHAVIOUR INSTANCE DATA

Every Behaviour instance should internally maintain the information required to identify and manage it.

At minimum:

```text
Behaviour Type
Behaviour Class ID
Behaviour Instance ID
Owner Entity ID
Enabled State
Started State
Created State
Destroyed State
Reflection Metadata
Serialized Property State
```

Use the engine's existing UUID/GUID/ID system where available.

Do not use pointer addresses as persistent IDs.

---

# 4. MULTIPLE BEHAVIOURS PER OBJECT

Every Entity/GameObject must support multiple Behaviours.

Example:

```text
Player
├── Transform
├── MeshRenderer
├── CharacterController
├── PlayerMovement
├── PlayerCombat
├── PlayerHealth
└── PlayerInventory
```

The engine must support:

```cpp
object.AddBehaviour<PlayerMovement>();
object.AddBehaviour<PlayerCombat>();
object.AddBehaviour<PlayerHealth>();
```

or an equivalent engine API.

Also provide:

```cpp
RemoveBehaviour<T>();
GetBehaviour<T>();
HasBehaviour<T>();
```

and/or a generic type-based API.

Also support finding Behaviours by runtime type ID.

Do not limit an object to one Behaviour.

---

# 5. BEHAVIOUR EXECUTION MODEL

Create a central `BehaviourSystem` or integrate with the existing update architecture.

The BehaviourSystem is responsible for:

* Creating Behaviour instances.
* Initializing them.
* Calling lifecycle functions.
* Tracking enabled/disabled state.
* Calling update functions.
* Destroying Behaviours.
* Preventing invalid execution.
* Respecting Play Mode state.
* Maintaining deterministic execution order.

Do not place runtime dispatch logic directly inside random editor code.

---

# 6. EXACT LIFECYCLE MODEL

Use the following conceptual lifecycle:

```text
Behaviour Instance Created
        ↓
OnCreate()
        ↓
OnEnable()
        ↓
Start()
        ↓
--------------------------
Runtime Loop
        ↓
FixedUpdate()
        ↓
Update()
        ↓
LateUpdate()
--------------------------
        ↓
Disable:
OnDisable()
        ↓
Destroy:
OnDestroy()
```

Requirements:

### OnCreate

Called once after the Behaviour instance has been constructed and attached to an owner.

### OnEnable

Called when:

* Behaviour becomes enabled.
* Behaviour is enabled after being disabled.
* Owner becomes active, according to the engine's active-state system.

### Start

Called once before the first normal runtime update.

Never call `Start()` repeatedly every frame.

### FixedUpdate

Called using the engine's fixed timestep if physics/fixed-timestep support exists.

Do not simply call this every variable frame unless the engine intentionally uses a fixed update loop.

### Update

Called once per frame while:

* Play Mode is active.
* Behaviour is enabled.
* Owner is active.
* Behaviour is not destroyed.

### LateUpdate

Called after normal Update processing.

### OnDisable

Called when a Behaviour transitions from enabled to disabled.

### OnDestroy

Called exactly once when the Behaviour is permanently destroyed.

---

# 7. ENABLED AND ACTIVE STATE

Separate these concepts:

```text
Behaviour Enabled
Owner Active
Scene Active
Play Mode Active
```

A Behaviour should run only when the required conditions are satisfied.

For example:

```text
Play Mode = true
Behaviour.enabled = true
Owner.active = true
Scene.active = true
```

If the owner is disabled, the Behaviour must not continue receiving Update callbacks.

Use a state model that avoids duplicate `OnEnable()` or `OnDisable()` calls.

---

# 8. SCRIPT CREATION FROM CONTENT BROWSER

Add:

```text
Content Browser
    → Right Click
    → Create
    → Behaviour
```

The user should see:

```text
Create Behaviour
-------------------------

Name:
[ PlayerController ]

Namespace:
[ optional ]

Folder:
[ selected folder ]

[ Create ]
[ Cancel ]
```

Creating the Behaviour must generate valid C++.

Example generated files:

```text
PlayerController.h
PlayerController.cpp
```

Generated header:

```cpp
#pragma once

#include "EunoiaBehaviour.h"

class PlayerController : public EunoiaBehaviour
{
public:

    virtual void Start() override;
    virtual void Update(float deltaTime) override;
};
```

Generated source:

```cpp
#include "PlayerController.h"

void PlayerController::Start()
{
}

void PlayerController::Update(float deltaTime)
{
}
```

Adapt includes, namespaces, macros, module structure, and naming to the existing engine.

Do not produce code that requires manually fixing include paths.

---

# 9. CONTENT BROWSER ASSET TYPE

The generated script must be represented in the Content Browser as:

```text
Behaviour
```

Example:

```text
PlayerController
Type: Behaviour
```

Not:

```text
C++ File
Source File
Text File
```

The asset system must know that the class derives from `EunoiaBehaviour`.

Store metadata such as:

```text
Asset Type = Behaviour
Class Name
Class ID
Source Header
Source Implementation
Base Class = EunoiaBehaviour
```

---

# 10. BEHAVIOUR REGISTRY

Implement a `BehaviourRegistry`.

It must support:

```cpp
RegisterBehaviour(...)
UnregisterBehaviour(...)
FindBehaviour(...)
CreateBehaviourInstance(...)
IsBehaviourRegistered(...)
GetAllBehaviours(...)
```

Each Behaviour registration should contain:

```text
Class Name
Display Name
Type ID
Base Type
Factory Function
Reflection Information
Source Asset
```

Do not hardcode:

```cpp
if (className == "PlayerController")
```

for every class.

The system must be generic.

---

# 11. AUTOMATIC SCRIPT DISCOVERY

After the C++ project is compiled, Eunoia should discover available `EunoiaBehaviour` classes.

The exact implementation may use:

* Engine reflection.
* Registration macros.
* Generated metadata.
* Static registration.
* Build-time code generation.
* Module registration.

Choose the safest architecture compatible with the current engine.

The final editor workflow must be:

```text
Create Behaviour
        ↓
Generate C++
        ↓
Compile
        ↓
Behaviour detected
        ↓
Behaviour appears in Add Behaviour
```

No manual editor registration should be required for every new script.

---

# 12. C++ REGISTRATION

Where appropriate, introduce a macro or registration mechanism such as:

```cpp
EUNOIA_BEHAVIOUR(PlayerController)
```

or an equivalent implementation.

The registration must provide:

```text
Runtime type identification
Factory creation
Reflection metadata
Editor discoverability
```

Do not require users to duplicate the class name in multiple places unless unavoidable.

---

# 13. ADDING A BEHAVIOUR IN DETAILS PANEL

When a GameObject/Entity is selected, Details Panel must contain:

```text
Behaviours
---------------------------------

PlayerController

    Enabled     [ ✓ ]

    ...

Health

    Enabled     [ ✓ ]

    ...

[ + Add Behaviour ]
```

Clicking:

```text
+ Add Behaviour
```

opens all registered Behaviours.

Example:

```text
Add Behaviour
----------------------
Search...

PlayerController
PlayerHealth
PlayerCombat
DoorController
EnemyAI
CameraController
...
```

Allow search/filter.

Selecting a Behaviour creates a real instance and attaches it to the selected object.

---

# 14. MULTIPLE INSTANCES

Support multiple different Behaviours on the same object.

Prefer preventing accidental duplicate instances of the same Behaviour unless the architecture explicitly permits it.

If the engine supports multiple instances of one Behaviour class, support that intentionally.

Do not silently create duplicates.

---

# 15. REMOVE BEHAVIOUR

Every attached Behaviour should provide a context menu or remove button:

```text
...
Remove Behaviour
```

Removal must safely perform:

```text
OnDisable()
OnDestroy()
Detach
Serialize removal
Refresh inspector
```

Do not invoke lifecycle methods twice.

---

# 16. BEHAVIOUR INSPECTOR

The selected Behaviour's public/exposed properties must appear in the Details Panel automatically.

Example:

```cpp
class EnemyAI : public EunoiaBehaviour
{
public:

    int Health = 100;
    float MoveSpeed = 5.0f;
    bool Patrol = true;
    std::string EnemyName = "Enemy";
};
```

Details:

```text
EnemyAI
--------------------------------

Enabled
[ ✓ ]

Health
[ 100 ]

Move Speed
[ 5.0 ]

Patrol
[ ✓ ]

Enemy Name
[ Enemy ]
```

Do not manually code inspector controls for each Behaviour class.

---

# 17. PUBLIC VARIABLE REFLECTION

Implement property reflection for Behaviour classes.

The reflection system must identify:

```text
Property Name
Property Type
Pointer/reference status
Editable status
Serializable status
Default Value
Current Value
Category
Tooltip
Display Name
```

Use existing engine reflection functionality where possible.

---

# 18. IMPORTANT C++ REFLECTION REQUIREMENT

Native C++ does not automatically expose arbitrary public member variables to a custom inspector.

Therefore implement a reliable engine-side reflection mechanism.

Possible architecture:

```cpp
EUNOIA_PROPERTY(int, Health)
EUNOIA_PROPERTY(float, MoveSpeed)
EUNOIA_PROPERTY(bool, Patrol)
```

or metadata macros/registration methods compatible with the engine.

The system must be generic and reusable across all Behaviours.

Do not implement one-off hardcoded field names.

---

# 19. PROPERTY TYPES

At minimum support:

```text
bool

char
signed char
unsigned char

short
unsigned short

int
unsigned int

long
unsigned long

long long
unsigned long long

float
double

std::string
```

Where supported by the engine also implement:

```text
Vector2
Vector3
Vector4
Color
Quaternion
Transform
```

Use proper controls in the Details Panel.

---

# 20. NUMERIC INSPECTOR CONTROLS

For integers:

```text
Health
[ 100 ]
```

For floats:

```text
Move Speed
[ 5.000 ]
```

Support:

* Text input.
* Validation.
* Numeric conversion.
* Optional drag adjustment.
* Undo/Redo.
* Clamping where metadata specifies min/max.

Do not accept invalid numeric values.

---

# 21. BOOLEAN CONTROL

Boolean:

```text
Can Attack
[ ✓ ]
```

Use checkbox/toggle behavior.

---

# 22. STRING CONTROL

String:

```text
Player Name
[ Player01 ]
```

Support editing and serialization.

---

# 23. VECTOR CONTROL

For Vector3:

```text
Position
X [ 0.00 ]
Y [ 1.00 ]
Z [ 2.00 ]
```

Use existing engine vector types.

---

# 24. COLOR CONTROL

For Color:

```text
Tint
[ Color Swatch ]
```

Support the engine's existing color picker.

---

# 25. ENUM SUPPORT

Where engine enums exist, display:

```text
Movement Mode
[ Walking ▼ ]

Walking
Running
Crouching
```

Do not display raw integer values when reflection metadata knows an enum.

---

# 26. OBJECT REFERENCES

Behaviours must support references to scene objects/components.

Example:

```cpp
class DoorController : public EunoiaBehaviour
{
public:

    Light* WarningLight = nullptr;
    MeshRenderer* DoorMesh = nullptr;
    Collider* Trigger = nullptr;
    Camera* PlayerCamera = nullptr;
};
```

The Details Panel must recognize these as typed object references.

Example:

```text
Warning Light
[ Select Light ▼ ]

Door Mesh
[ Select MeshRenderer ▼ ]

Trigger
[ Select Collider ▼ ]

Player Camera
[ Select Camera ▼ ]
```

---

# 27. TYPE-SAFE OBJECT PICKING

The inspector object picker must inspect the property's expected type.

For:

```cpp
Light*
```

show only Light-compatible objects.

For:

```cpp
MeshRenderer*
```

show only MeshRenderer-compatible objects.

For:

```cpp
Collider*
```

show only Collider-compatible objects.

For:

```cpp
Camera*
```

show only Camera-compatible objects.

Never allow incompatible assignment.

Do not solve type compatibility by blind C-style casting.

Use RTTI, engine type IDs, reflection, or a type-safe component system.

---

# 28. OBJECT PICKER UI

Clicking an object reference field should open a popup:

```text
Select Light
--------------------------------
Search...

DirectionalLight
TorchLight
HallLight
WarningLight
PlayerLight
--------------------------------
None
```

It must search the current scene.

It must update after scene changes.

It must support clearing the reference.

---

# 29. HIERARCHY DRAG AND DROP

The user must be able to drag a scene object from Hierarchy and drop it onto an object-reference field.

Example:

```text
Hierarchy

Scene
├── Player
├── MainCamera
├── WarningLight
├── Door
└── TriggerZone
```

Drag:

```text
WarningLight
```

to:

```text
DoorController
Warning Light
[ Drop Object Here ]
```

The engine must:

1. Detect the dropped hierarchy entity.
2. Resolve the entity.
3. Inspect the expected property type.
4. Resolve the requested component/object.
5. Validate compatibility.
6. Assign it.
7. Refresh the Details Panel.
8. Record the assignment for Undo/Redo.
9. Serialize the stable reference.

---

# 30. COMPONENT REFERENCE RESOLUTION

If a Hierarchy object represents an Entity but the Behaviour property expects a component such as:

```cpp
Light*
```

the system must inspect that Entity and resolve the Light component.

Example:

```text
Dropped Entity
      ↓
Has Light component?
      ↓
Yes
      ↓
Assign Light*
```

If missing:

```text
Invalid assignment
```

Do not assign the Entity itself to a Light reference.

---

# 31. ENTITY REFERENCE

Also support direct Entity/GameObject references where the engine has such a type.

Example:

```cpp
Entity* Target;
```

The picker should display scene entities.

Example:

```text
Target
[ Player ▼ ]
```

---

# 32. GETRESPECTIVEOBJECT API

Implement the required API concept:

```text
GetRespectiveObject
```

This is the standardized Behaviour-side mechanism for accessing assigned references.

Support a generic form and typed helpers where appropriate.

Example concept:

```cpp
GetRespectiveObject(WarningLight)
```

or:

```cpp
GetRespectiveObject.Light(WarningLight)
```

Also support typed engine objects:

```cpp
GetRespectiveObject.Light(...)
GetRespectiveObject.Mesh(...)
GetRespectiveObject.Shape(...)
GetRespectiveObject.Entity(...)
GetRespectiveObject.Transform(...)
GetRespectiveObject.Camera(...)
GetRespectiveObject.Collider(...)
GetRespectiveObject.AudioSource(...)
```

The exact types should be generated from the actual engine object/component type system.

Do not implement separate unrelated lookup logic for every type.

---

# 33. GENERIC TEMPLATE ACCESS

Prefer a generic strongly typed internal implementation:

```cpp
template<typename T>
T* GetRespectiveObject(T* reference);
```

or equivalent.

Example:

```cpp
Light* light = GetRespectiveObject(WarningLight);
Camera* camera = GetRespectiveObject(PlayerCamera);
```

The final public API should be simple for gameplay programmers.

---

# 34. OWNER ACCESS

Behaviour scripts need direct access to their owner.

Provide APIs such as:

```cpp
Entity* GetOwner();
Transform* GetTransform();
```

and where supported:

```cpp
template<typename T>
T* GetComponent();
```

Example:

```cpp
void Update(float deltaTime)
{
    auto* transform = GetTransform();

    if (transform)
    {
        // gameplay logic
    }
}
```

This should use the Behaviour's actual owner.

---

# 35. COMPONENT ACCESS FROM BEHAVIOUR

Support:

```cpp
auto* collider = GetComponent<Collider>();
auto* renderer = GetComponent<MeshRenderer>();
auto* transform = GetComponent<Transform>();
```

or an equivalent engine-native API.

Do not force the user to manually search the entire scene for their own components.

---

# 36. SERIALIZATION

All exposed Behaviour properties must serialize.

Example:

```cpp
int Health = 100;
float Speed = 5.0f;
bool Aggressive = true;
Light* WarningLight = nullptr;
```

After editing:

```text
Health = 250
Speed = 8.5
Aggressive = false
WarningLight = TorchLight
```

Save scene.

Close editor.

Reopen scene.

Expected:

```text
Health = 250
Speed = 8.5
Aggressive = false
WarningLight = TorchLight
```

Exactly preserve edited values.

---

# 37. SERIALIZING OBJECT REFERENCES

Never serialize raw pointers:

```cpp
0x000001A...
```

Do not write memory addresses into scene files.

Serialize stable references.

Example:

```text
Entity GUID
Component Type ID
Component GUID
```

or the engine's existing stable object identification.

At load:

```text
Serialized Reference
        ↓
Find Entity
        ↓
Find Component
        ↓
Resolve Pointer/Handle
        ↓
Assign Runtime Reference
```

---

# 38. SCENE REFERENCE VALIDATION

Handle:

```text
Referenced object deleted
Referenced component removed
Referenced scene unloaded
Referenced scene changed
Invalid GUID
Broken reference
Missing object
```

Never crash.

Display:

```text
Warning Light
[ Missing Reference ]
```

Allow clearing/reassigning.

---

# 39. CROSS-SCENE REFERENCES

Unless the engine explicitly supports persistent cross-scene references, do not permit invalid cross-scene serialized references.

Restrict references to the current valid scope.

If cross-scene references already exist in the engine, integrate with that system instead of creating a second one.

---

# 40. DEFAULT VALUES

A Behaviour property's initialized C++ value should become its default inspector value.

Example:

```cpp
float Speed = 5.0f;
```

Inspector initially:

```text
Speed
[ 5.0 ]
```

Do not overwrite defaults unexpectedly.

---

# 41. EDITOR PROPERTY CHANGE

When the user edits a property:

```text
Details Panel
     ↓
Reflection Property
     ↓
Behaviour Instance
     ↓
Runtime/Editor Value
```

The displayed value and actual Behaviour memory must stay synchronized.

Do not store a second unsynchronized copy.

---

# 42. UNDO / REDO

Integrate with the existing Undo/Redo system.

Support undo/redo for:

```text
Add Behaviour
Remove Behaviour
Enable/Disable Behaviour
Change int
Change float
Change bool
Change string
Change vector
Change enum
Assign object reference
Clear object reference
Reorder Behaviours
```

Undoing an object-reference assignment must restore the exact previous reference.

---

# 43. PREFAB SUPPORT

If the engine supports Prefabs:

Behaviour data must work with Prefabs.

Support:

```text
Behaviour type
Behaviour enabled state
Behaviour properties
Object references where legal
Overrides
Prefab serialization
Prefab instantiation
```

Do not silently lose Behaviour data during prefab creation or instantiation.

---

# 44. CLONING / DUPLICATION

When duplicating an Entity/GameObject:

* Duplicate Behaviour instances.
* Duplicate their serialized property values.
* Preserve internal references correctly.
* Remap references to duplicated objects when appropriate.

Example:

```text
Original:
Door → TriggerZone

Duplicated:
Door_2 → TriggerZone_2
```

If the source reference points to another object that was also duplicated, remap appropriately.

Do not leave unintended references to the original object.

---

# 45. RUNTIME INSTANTIATION

Behaviours must work on runtime-created entities.

Example:

```cpp
auto* enemy = SpawnEntity(...);

enemy->AddBehaviour<EnemyAI>();
```

The lifecycle must initialize correctly.

Do not assume every Behaviour exists only on editor-created objects.

---

# 46. RUNTIME OBJECT DESTRUCTION

When an owner Entity is destroyed:

```text
Entity Destroy
      ↓
Disable attached Behaviours
      ↓
Destroy attached Behaviours
      ↓
Release runtime data
      ↓
Destroy Entity
```

Avoid callbacks into already destroyed owners.

---

# 47. SCRIPT EXECUTION ORDER

Provide deterministic Behaviour execution.

At minimum:

```text
FixedUpdate
    ↓
Update
    ↓
LateUpdate
```

For multiple Behaviours on the same object, use their defined list order or an explicit execution-order mechanism.

Do not depend on pointer/container allocation order.

If the engine supports execution priorities, integrate them.

---

# 48. EXPLICIT EXECUTION ORDER

Prefer supporting optional metadata such as:

```text
Execution Order = 100
```

Example:

```text
PlayerInput       0
PlayerMovement   100
PlayerAnimation  200
```

If not implemented initially, keep the architecture open for future execution ordering.

---

# 49. ENABLE/DISABLE FROM INSPECTOR

Each Behaviour must have:

```text
Enabled
[ ✓ ]
```

Changing it must immediately affect runtime execution.

Transition:

```text
true → false
```

calls:

```cpp
OnDisable();
```

Transition:

```text
false → true
```

calls:

```cpp
OnEnable();
```

Do not reconstruct the Behaviour.

---

# 50. GAMEOBJECT ACTIVE STATE

If an Entity/GameObject is disabled:

```text
Update must stop
```

When re-enabled:

```text
OnEnable()
```

must execute on attached enabled Behaviours according to the active-state transition rules.

Do not call `Start()` again just because the object became active again.

---

# 51. EDITOR MODE VS PLAY MODE

The editor must start in:

```text
STOPPED / EDITOR MODE
```

No gameplay simulation should run automatically.

The top menu bar should contain:

```text
[ ▶ Play ]
```

or the existing engine play button.

---

# 52. PLAY MODE

When the user presses Play:

```text
EDITOR MODE
     ↓
PLAY MODE
     ↓
Create/prepare runtime world
     ↓
Initialize Behaviours
     ↓
Run game
```

Do not run gameplay simulation while still editing.

---

# 53. PLAY MODE UI

When Play Mode starts, hide editor-only UI.

Hide:

```text
Top Editor Toolbar
Top Menu where appropriate
Hierarchy
Details Panel
Content Browser
Editor Gizmos
Editor Overlays
Scene Manipulation UI
Inspector Controls
```

Display only the Game View.

The Game View must occupy the screen/fullscreen presentation area.

Expected:

```text
+--------------------------------------------------------+
|                                                        |
|                                                        |
|                     GAME VIEW                          |
|                                                        |
|                                                        |
|                                                        |
+--------------------------------------------------------+
```

Do not render editor controls over the game.

---

# 54. GAME INPUT IN PLAY MODE

While Play Mode is active:

* Game receives keyboard input.
* Game receives mouse input.
* Existing controller/input systems remain active.
* Editor scene manipulation is disabled.
* Editor shortcuts should not accidentally modify scene data.

Game View must receive input focus.

---

# 55. STOP PLAY MODE WITH DELETE

While Play Mode is active:

```text
DELETE
```

must stop the game.

Required flow:

```text
PLAY MODE
    ↓
DELETE
    ↓
Stop Runtime
    ↓
Destroy/Cleanup Runtime Behaviours
    ↓
Restore Editor
```

Delete must work even when Game View has keyboard focus.

Do not treat Delete as gameplay input while it is the Play Mode stop key.

---

# 56. RESTORE EDITOR

After stopping:

```text
Game View
    ↓
Stop runtime
    ↓
Restore editor UI
    ↓
Restore Hierarchy
    ↓
Restore Details
    ↓
Restore Content Browser
    ↓
Restore Scene View
    ↓
STOPPED / EDITOR MODE
```

Editor state must be restored correctly.

---

# 57. PLAY MODE SCENE ISOLATION

Do not permanently modify the authoring/edit-time scene merely because gameplay code changed runtime state.

Use a runtime scene/world copy or the engine's equivalent Play Mode isolation.

Example:

```text
EDITOR SCENE
     │
     └──── Clone/Runtime World ────► PLAY MODE
                                      │
                                      └── runtime modifications
                                              │
                                              ▼
                                           STOP
                                              │
                                              ▼
                                    discard runtime state
```

Do not persist temporary runtime changes unless the existing engine intentionally supports applying them.

---

# 58. SCRIPT STATE DURING PLAY

Behaviour member variables may change during runtime.

Example:

```cpp
Health = 50;
```

If the original serialized editor value was:

```text
Health = 100
```

stopping the game should return the editor Behaviour to its original authoring state unless the engine explicitly provides an "Apply Runtime Changes" feature.

Do not accidentally save runtime-only values into the editor scene.

---

# 59. SCRIPT RECOMPILATION

When source code changes:

```text
Behaviour Source Changed
      ↓
Compile
      ↓
Detect success/failure
      ↓
Reload/refresh Behaviour metadata
```

If compilation fails:

* Keep existing engine state safe.
* Show compilation errors.
* Do not corrupt Behaviour metadata.
* Do not crash the editor.

---

# 60. SCRIPT RELOAD

If hot reload is supported, integrate with it.

When a Behaviour class is reloaded:

* Existing references must remain valid where possible.
* Serialized property values must be preserved.
* Behaviour instances must be safely reconstructed if required.
* Reflection metadata must refresh.
* Details Panel must refresh.

If true hot reload is not technically safe with the existing compiler/runtime architecture, implement a controlled reload path rather than pretending hot reload works.

---

# 61. SCRIPT API USABILITY

Gameplay programmers should be able to write:

```cpp
class EnemyAI : public EunoiaBehaviour
{
public:

    int Health = 100;
    float MoveSpeed = 3.0f;
    bool Aggressive = true;

    Entity* Target = nullptr;

    void Start() override
    {
        // initialization
    }

    void Update(float deltaTime) override
    {
        if (Target)
        {
            // gameplay logic
        }
    }
};
```

The engine should provide:

```cpp
GetOwner()
GetTransform()
GetComponent<T>()
GetBehaviour<T>()
GetRespectiveObject(...)
```

or equivalent APIs.

---

# 62. SCRIPT ACCESS TO OTHER BEHAVIOURS

Support:

```cpp
auto* health = GetBehaviour<HealthBehaviour>();
```

from an owner entity/Behaviour when appropriate.

Also support retrieving attached Behaviours generically by type.

Do not use hardcoded class names.

---

# 63. SAFE BEHAVIOUR CASTING

Do not perform unsafe casts like:

```cpp
reinterpret_cast<PlayerController*>(...);
```

Use:

* type IDs
* RTTI
* engine reflection
* template-based typed retrieval

as appropriate.

---

# 64. LOGGING

Add clear logging for major Behaviour system actions.

Examples:

```text
[Behaviour] Registered: PlayerController
[Behaviour] Added PlayerController to Entity 'Player'
[Behaviour] Removed PlayerController from Entity 'Player'
[Behaviour] Started PlayerController on Entity 'Player'
[Behaviour] Assigned Light 'WarningLight' to DoorController::WarningLight
[Behaviour] Cleared reference DoorController::WarningLight
[Behaviour] Invalid reference: DoorController::WarningLight
[Behaviour] Compilation failed for EnemyAI
```

Do not log Update() every frame.

---

# 65. ERROR HANDLING

The Behaviour system must never crash because of:

* Missing Behaviour type.
* Invalid script class.
* Broken reference.
* Deleted scene object.
* Deleted component.
* Failed compilation.
* Failed reflection lookup.
* Failed serialization.
* Scene reload.
* Object destruction.
* Play/Stop cycles.

Recover gracefully and display useful diagnostics.

---

# 66. CONTENT BROWSER UX

The Content Browser must show Behaviour files with:

```text
Icon: Behaviour
Type: Behaviour
Name: PlayerController
```

The user should be able to:

```text
Create
Rename
Delete
Open/Edit
Find in source
```

using the existing Content Browser functionality.

Do not break source-file management.

---

# 67. DETAILS PANEL UX

Recommended layout:

```text
Selected Object
----------------------------------------

Transform
...

Mesh Renderer
...

Behaviours
========================================

PlayerController
----------------------------------------
Enabled       [ ✓ ]

Movement
    Speed      [ 5.0 ]

Combat
    Damage     [ 20 ]

References
    Target     [ Player ▼ ]

----------------------------------------

EnemyAI
----------------------------------------
Enabled       [ ✓ ]

Health        [ 100 ]
Aggressive    [ ✓ ]

Target
[ EnemyTarget ▼ ]

----------------------------------------

[ + Add Behaviour ]
```

Make the Behaviour section visually consistent with existing component sections.

---

# 68. COLLAPSIBLE BEHAVIOURS

Each Behaviour section should be collapsible.

Example:

```text
▼ PlayerController
▼ EnemyAI
▶ HealthBehaviour
```

Use the existing Details Panel style.

---

# 69. SEARCHING BEHAVIOURS

The Add Behaviour dialog must support search.

For example:

```text
Search: camera
```

returns:

```text
CameraController
PlayerCameraBehaviour
CinematicCamera
```

Do not scan and instantiate every Behaviour merely for search.

Use registry metadata.

---

# 70. SERIALIZED PROPERTY VERSIONING

Design Behaviour serialization so the system can evolve.

For example:

```text
Behaviour Type ID
Property Name/ID
Property Type
Serialized Value
```

Do not make serialization dependent on raw memory layout.

If properties change between versions, handle missing/new fields safely.

---

# 71. PROPERTY IDENTIFIERS

Prefer stable property IDs or names.

Do not serialize:

```text
raw memory offset
```

as the permanent identity of a property.

The serialized representation should remain readable and upgradeable.

---

# 72. OBJECT REFERENCE IDs

Object references must use stable identifiers.

Example:

```text
DoorController
   WarningLight
      EntityGUID = ...
      ComponentGUID = ...
```

At runtime resolve:

```text
GUID → Entity → Component → Pointer/Handle
```

Do not save pointer values.

---

# 73. SCENE OBJECT PICKER PERFORMANCE

Do not iterate the entire scene every frame.

Maintain cached indexes such as:

```text
Entity Registry
Light Registry
Mesh Registry
Collider Registry
Camera Registry
```

or use the existing scene/component registry.

Update those indexes when the scene changes.

---

# 74. DRAG & DROP VALIDATION

When an object is dropped onto a property:

```text
Dragged Entity
       ↓
Expected Type
       ↓
Compatible?
   ┌───┴───┐
  YES     NO
   ↓       ↓
Assign   Reject
```

Do not silently assign invalid values.

---

# 75. NONE / CLEAR SUPPORT

Every object reference field must have a clear option.

Example:

```text
Target
[ Player ▼ ] [ X ]
```

Clicking X:

```text
Target
[ None ]
```

The serialized reference must become null/empty.

---

# 76. DEFAULT INSPECTOR VALUES

Newly attached Behaviours should use their C++ defaults.

Example:

```cpp
int Damage = 25;
float Speed = 4.0f;
bool IsBoss = false;
```

New instance:

```text
Damage = 25
Speed = 4.0
Is Boss = false
```

Do not accidentally initialize everything to zero unless zero is actually the class default.

---

# 77. RUNTIME / EDITOR PROPERTY SEPARATION

Distinguish between:

```text
Authoring State
Runtime State
```

Editor serialized values belong to authoring state.

Runtime modifications belong to runtime state.

Stopping Play Mode should discard runtime-only state unless explicitly applied.

---

# 78. EDITOR WORLD AND RUNTIME WORLD

If the existing engine already has separate worlds, use them.

If not, introduce a clean abstraction:

```text
World
 ├── Editor World
 └── Runtime World
```

Do not duplicate all engine functionality.

The purpose is to safely isolate Play Mode.

---

# 79. BEHAVIOUR TEST SCENE

Create an internal test scene or automated test where practical.

Test:

```text
Player Entity
 ├── PlayerController
 ├── HealthBehaviour
 └── CameraController
```

Verify:

* Multiple Behaviours.
* Primitive properties.
* Scene references.
* Object picker.
* Drag/drop.
* Serialization.
* Runtime execution.
* Enable/disable.
* Play/Stop.

Do not leave a test scene as a required runtime dependency.

---

# 80. AUTOMATED VALIDATION

Where feasible, add automated tests for:

### Behaviour creation

```text
Create Behaviour
→ Registry contains Behaviour
```

### Behaviour attachment

```text
Entity
→ Add Behaviour
→ Has Behaviour
```

### Lifecycle

```text
OnCreate once
OnEnable correctly
Start once
Update while active
OnDisable correctly
OnDestroy once
```

### Serialization

```text
Set property
→ Save
→ Load
→ Value restored
```

### Object reference

```text
Assign Light
→ Save
→ Load
→ Resolve Light
```

### Invalid reference

```text
Delete Light
→ Behaviour remains safe
→ Reference becomes invalid/null
```

---

# 81. EXAMPLE FINAL SCRIPT

The system must support a script like:

```cpp
#pragma once

#include "EunoiaBehaviour.h"

class DoorController : public EunoiaBehaviour
{
public:

    float OpenSpeed = 2.5f;
    int RequiredKeys = 1;
    bool Locked = true;

    Light* WarningLight = nullptr;
    MeshRenderer* DoorMesh = nullptr;
    Collider* Trigger = nullptr;

    void Start() override;

    void Update(float deltaTime) override;
};
```

Implementation:

```cpp
#include "DoorController.h"

void DoorController::Start()
{
    auto* light = GetRespectiveObject(WarningLight);

    if (light)
    {
        // Configure light.
    }
}

void DoorController::Update(float deltaTime)
{
    if (!Locked)
    {
        // Door gameplay.
    }
}
```

The editor must automatically display:

```text
DoorController
--------------------------------

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

Trigger
[ TriggerZone ▼ ]
```

---

# 82. EXACT USER WORKFLOW

The final Eunoia workflow must be:

```text
1. Open Eunoia Engine
        ↓
2. Editor starts in STOPPED mode
        ↓
3. Content Browser
        ↓
4. Right Click
        ↓
5. Create
        ↓
6. Behaviour
        ↓
7. Name it PlayerController
        ↓
8. Engine creates .h and .cpp
        ↓
9. Compile project
        ↓
10. Behaviour automatically registered
        ↓
11. Select Player in Hierarchy
        ↓
12. Details Panel
        ↓
13. Add Behaviour
        ↓
14. Select PlayerController
        ↓
15. Behaviour is attached
        ↓
16. Public variables appear automatically
        ↓
17. Edit values
        ↓
18. Assign scene references using dropdown
        ↓
19. Or drag object from Hierarchy
        ↓
20. Save scene
        ↓
21. Click Play
        ↓
22. Editor UI disappears
        ↓
23. Game View becomes fullscreen
        ↓
24. Behaviour lifecycle starts
        ↓
25. Game runs
        ↓
26. Press DELETE
        ↓
27. Runtime stops
        ↓
28. Runtime Behaviour state discarded
        ↓
29. Editor UI returns
        ↓
30. Back to STOPPED / EDITOR MODE
```

---

# 83. REQUIRED FILE/CLASS ORGANIZATION

Use the existing engine directory structure, but conceptually implement:

```text
Runtime/
    Behaviour/
        EunoiaBehaviour.h
        EunoiaBehaviour.cpp
        BehaviourSystem.h
        BehaviourSystem.cpp
        BehaviourRegistry.h
        BehaviourRegistry.cpp
        BehaviourReflection.h
        BehaviourReflection.cpp
        BehaviourSerializer.h
        BehaviourSerializer.cpp
        ObjectReference.h
        ObjectReference.cpp

Editor/
    Behaviour/
        BehaviourAsset.h
        BehaviourAsset.cpp
        BehaviourCreator.h
        BehaviourCreator.cpp
        BehaviourEditor.h
        BehaviourEditor.cpp
        BehaviourInspector.h
        BehaviourInspector.cpp
        ObjectReferencePicker.h
        ObjectReferencePicker.cpp
```

Do not blindly create these exact files if the engine has an established architecture. Place them where the existing project structure dictates.

---

# 84. CODE QUALITY REQUIREMENTS

Use modern C++ where compatible with the project.

Prefer:

```text
RAII
Smart pointers
Strong types
constexpr where useful
Templates where appropriate
Centralized ownership
Stable IDs
Validated references
```

Avoid:

```text
Global raw pointers
Hardcoded class lists
Hardcoded scene names
Pointer serialization
Unsafe casts
Duplicated reflection code
Per-class inspector implementations
Per-class object-picker implementations
```

---

# 85. MEMORY OWNERSHIP

Define clear ownership:

```text
Entity
 └── Behaviour instances
```

A Behaviour should not independently own the Entity it is attached to.

Avoid reference cycles.

If runtime references use raw pointers for performance, ensure the owning scene/entity system invalidates them safely.

Prefer handles/IDs where appropriate.

---

# 86. EDITOR/ENGINE BOUNDARY

Keep editor-only systems separate from runtime systems.

Runtime Behaviour must not depend directly on:

```text
ImGui
Editor windows
Content Browser
Hierarchy UI
Details Panel
```

Editor code should inspect and manipulate runtime/reflection data through engine APIs.

This is important so packaged games do not need editor UI systems.

---

# 87. PACKAGE / BUILD REQUIREMENT

Behaviours are development-time C++ source code but their compiled functionality must be available in the final game build.

The build pipeline must include user Behaviour source in the game build.

Do not make Behaviour execution dependent on the editor executable.

---

# 88. SHIPPING BUILD REQUIREMENT

Ensure the Behaviour system can run without:

```text
Content Browser
Details Panel
Hierarchy
Editor UI
Editor reflection widgets
```

The runtime only requires:

```text
EunoiaBehaviour
Behaviour runtime
Behaviour registration/type metadata needed at runtime
Scene serialization
Object references
Game loop
```

---

# 89. EDITOR-ONLY PROPERTY METADATA

Where possible, distinguish:

```text
Runtime Metadata
Editor Metadata
```

so shipping builds do not carry unnecessary editor-only data.

---

# 90. SECURITY / VALIDATION

Do not execute arbitrary source code merely because a `.cpp` file appears in the Content Browser.

Only compiled/registered Behaviour classes are valid runtime Behaviours.

Failed or unregistered classes must not instantiate.

---

# 91. NO HARDCODED DEMO LOGIC

Do not implement fake behaviour registration like:

```cpp
RegisterBehaviour<PlayerController>();
RegisterBehaviour<EnemyAI>();
RegisterBehaviour<DoorController>();
```

as the only discovery mechanism.

The architecture must support arbitrary future user-created Behaviours.

---

# 92. NO MANUAL INSPECTOR IMPLEMENTATIONS

Do not implement:

```cpp
DrawPlayerControllerInspector()
DrawEnemyAIInspector()
DrawDoorControllerInspector()
```

for individual classes.

The inspector must be reflection-driven and generic.

---

# 93. NO HARDCODED SCENE OBJECT TYPES

Do not write:

```cpp
if (propertyName == "WarningLight")
```

or:

```cpp
if (propertyType == "Light*")
```

throughout random UI code.

Centralize type metadata and use the engine's runtime type/reflection system.

---

# 94. FINAL ACCEPTANCE CRITERIA

The implementation is complete only when ALL of these work:

```text
[ ] EunoiaBehaviour base class exists.
[ ] User classes can inherit from it.
[ ] Behaviour lifecycle is implemented.
[ ] Multiple Behaviours can be attached to one Entity.
[ ] Behaviour enabled state works.
[ ] Owner active state works.
[ ] Start runs once.
[ ] FixedUpdate works.
[ ] Update works.
[ ] LateUpdate works.
[ ] OnDisable works.
[ ] OnDestroy works.

[ ] Content Browser can create Behaviour.
[ ] Behaviour generates valid .h and .cpp.
[ ] Behaviour appears as asset type "Behaviour".
[ ] Behaviour source compiles.
[ ] Compiled Behaviour is automatically discovered.
[ ] Behaviour Registry works.
[ ] Add Behaviour list works.
[ ] Search/filter works.
[ ] Behaviour can be removed.
[ ] Behaviour list can support multiple Behaviours.

[ ] Public variables are reflected.
[ ] int works.
[ ] float works.
[ ] bool works.
[ ] string works.
[ ] vectors work where supported.
[ ] colors work where supported.
[ ] enums work where supported.
[ ] Property defaults work.
[ ] Property serialization works.
[ ] Inspector updates correctly.

[ ] Entity references work.
[ ] Component references work.
[ ] Light references work.
[ ] MeshRenderer references work.
[ ] Collider references work.
[ ] Camera references work.
[ ] Other engine types work through the same generic mechanism.
[ ] Typed object picker works.
[ ] Search in object picker works.
[ ] None/Clear works.
[ ] Current scene objects appear.
[ ] Type filtering works.
[ ] Hierarchy drag-and-drop works.
[ ] Invalid drag-and-drop is rejected.
[ ] Object references serialize by stable IDs.
[ ] References restore after loading.
[ ] Deleted references are safely invalidated.
[ ] GetRespectiveObject works.
[ ] GetOwner/GetComponent/GetBehaviour works.

[ ] Undo/Redo works.
[ ] Prefabs work where supported.
[ ] Duplication/remapping works where supported.
[ ] Runtime-created entities can use Behaviours.
[ ] Runtime destruction is safe.
[ ] No dangling references.
[ ] No double destruction.
[ ] No duplicate lifecycle calls.
[ ] No editor crashes from bad scripts.
[ ] Compilation failures are handled safely.

[ ] Editor starts in STOPPED mode.
[ ] Play button starts Play Mode.
[ ] Game simulation starts.
[ ] Game input works.
[ ] Editor UI is hidden during Play Mode.
[ ] Game View is fullscreen.
[ ] Hierarchy is hidden during Play Mode.
[ ] Details Panel is hidden during Play Mode.
[ ] Content Browser is hidden during Play Mode.
[ ] Editor gizmos are hidden during Play Mode.
[ ] DELETE stops Play Mode.
[ ] DELETE works with Game View focused.
[ ] Runtime world is cleaned up.
[ ] Runtime Behaviour state is discarded.
[ ] Editor UI is restored.
[ ] Editor returns to STOPPED mode.
[ ] Repeated Play → Delete → Play cycles work correctly.

[ ] Engine compiles successfully.
[ ] Game build compiles successfully.
[ ] Existing engine systems are not broken.
[ ] No placeholder core systems remain.
```

---

# 95. IMPORTANT IMPLEMENTATION RULE

Do not stop after implementing the base class.

Do not stop after implementing the inspector.

Do not stop after implementing script generation.

Implement the entire pipeline:

```text
C++ Source
    ↓
Behaviour Class
    ↓
Compile
    ↓
Registration
    ↓
Reflection
    ↓
Content Browser
    ↓
Add Behaviour
    ↓
Entity Attachment
    ↓
Details Inspector
    ↓
Property Editing
    ↓
Scene Object References
    ↓
Serialization
    ↓
Runtime Resolution
    ↓
Behaviour Lifecycle
    ↓
Play Mode
    ↓
Fullscreen Game
    ↓
DELETE
    ↓
Stop
    ↓
Restore Editor
```

The final system should feel like a native part of Eunoia Engine rather than a separate plugin or temporary editor feature.

Use the existing project architecture first, make the implementation generic, maintainable, type-safe, serialized, and runtime-safe, then build the engine and fix all implementation errors before finishing.

Do not leave TODOs, mocked lists, hardcoded example Behaviours, placeholder inspectors, or fake runtime execution for any core requirement above.
