---
name: architecture
description: Decide where a piece of state, behaviour or API belongs in Reloaded Core's layers, and check that a change does not breach a boundary. Use before adding a class, a field, a signal, a state key or a server method; before moving logic between C++ and GDScript; when something needs a value that lives in another layer; when a node, a system and a server all look like plausible homes for the same thing; when a plugin's backend needs something of the core; and when reviewing a diff for separation of concerns. Answers "which layer owns this", "may this layer know about that one", and "is this the same mistake we already made".
---

# Reloaded Core - layers, boundaries and how they are enforced

The core is a stack of layers, each replaceable on its own. Most defects recorded in
`FINDINGS.md` are a layer that reached into another: a feature parked on a node vanished when a
second instancer appeared without nodes; a sound trigger's counters lived on the vehicle; a getter
advanced a filter, so the value depended on how often it was read.

**The question that settles every argument about placement:** *if this layer were replaced
wholesale, would this field go with it?* If yes, it belongs there. If it would have to be kept
for somebody else, it belongs to that somebody.

## The map

Read top to bottom: each layer may use the ones below it and must know nothing of the ones above.

| Layer | Who | Owns | Must never |
|---|---|---|---|
| Backend | a plugin's `VehicleImplementationServer`, registered with `VehicleServer` (`implementation_register()`) under the name a controller gives, and its implementations of the component interfaces | physical quantities, its own model of the vehicle, how its vehicles are stepped together | appear in any name, parameter or return type of the core; the core never knows which plugin simulates a vehicle |
| Vehicle model | `VehicleController` (an `Object`, any vehicle) and `RailVehicleController` (a railway one: cabs, couplers, controllers, relays), `VehicleComponent` / `RailVehicleComponent` + `RailVehicle<Domain>` interfaces | a vehicle's state, configuration and the operations that change them | name the backend; be a `Node`; hold anything only a drawing, sound or UI layer needs |
| Servers | `VehicleServer`, `RailVehicleServer`, `TrackServer`, `TractionServer`, `SignallingServer`, `SceneryStreamingServer`, `ModelRenderingServer`, `TrackRenderingServer`, `PlayerServer`, `DriverServer`, `StationServer`, and the GDScript servers (`SceneryServer`, `VehicleCatalogServer`, `TractionRenderingServer`, `ScenerySoundServer`) | handles (`RID`), the placement and the composition of what they own, their own worker threads | hand out raw pointers, or hold state a single node could own; reach a vehicle's controller to execute vehicle logic (see "Servers forward to components") |
| Systems | `CabinSystem` (a cab per vehicle RID and cab), `CabinHUDMouseSystem`, `TranscriptSystem` | cross-cutting bookkeeping keyed by handle | duplicate state the model already has; drive per-frame work that a server could do natively |
| Nodes | `RailVehicle3D`, `VehiclePhysicsNode`, `Cabin3D`, cab widgets (`BaseCabinTool3D`), `SignalHeadNode`/`SignallingSystemNode` (proxies holding a server RID) | what is drawn and what is in the scene tree | contain simulation; own a handle the server already owns; reach a vehicle by walking the tree |
| Importers | a plugin's or a tool's readers of its source data | turning that data into the core's data classes (`CompiledScenery`, `VehicleData`, `TrainsetData`, `WorldData`) | build scene trees as their output, or keep parse results in nodes; save a scripted `Resource` or `Node` into what they write - saved data holds built-in types and C++ classes only, saved by their names (`CODE_STYLE.md`, "Saved data names no script") |

### Servers forward to components

The components' API (a coupling component, say) is a layer below the server. A server offers the
**high-level API** for the high-level operations: walking through the cabins
(`person_change_cabin()`), coupling (`vehicle_couple()`), managing trainsets (`trainset_place()`)
and moving a vehicle (`vehicle_move()`). Whether a server has to wrap everything beyond these is
not settled - ask the operator before wrapping more. A vehicle server **forwards its execution to
the vehicle's component** for that domain - the base classes
(`RailVehicleBuffCoupl`, `RailVehicle<Domain>`) exist for this. The component's backend
implementation executes it: the server finds the vehicle's rail buffer and coupling component,
and the backend's implementation of it does the coupling. A server never reaches the vehicle's
controller to execute such logic.

Logic outside the vehicle - a driver, the player, scripts - **executes through the servers**.
The components' API is fixed too, but it serves reading state on hot paths and the backend's
implementation of the forwarding, not executing logic from outside.

Debt on record (2026-10-09, `TODO.md`): `RailVehicleServer`'s coupling (`vehicle_couple()`,
`uncouple`, `is_coupled_by`, `is_coupler_automatic`, adapters,
`vehicle_get_coupler_joinable_flags()`) still goes through `RailVehicleController`, and a driver
couples by the `coupler_connect` command; nothing new is added that way.

Autoloads are listed in `demo/project.godot`; C++ singletons are registered in
`src/register_types.cpp` and documented in `doc_classes/<Class>.xml` (AGENTS.md,
"Documentation"). A singleton is reached by its typed `get_instance()` and the result is
null-checked - never by node path, never by `call("name")`.

## The core's data is the interface; plugins give backends

A game and a scenery importer speak **only the core's data and servers**: `VehicleData`,
`VehicleLivery`, `VehicleInfo`, `TrainsetData`, `CompiledScenery` and the rest of the core's data
classes, through `VehicleCatalogServer`, `SceneryServer` and the other core servers. A plugin
**supplies backends** - providers (`VehicleCatalogProvider`, `SceneryProvider`), implementations
attached to the core's servers (`VehicleImplementationServer`, `DriverImplementation`,
`SignallingImplementation`) - and its own custom classes, which stay behind those backends
(operator, 2026-10-20).

* A plugin takes the core's data as it is. It never converts it into a subclass of its own to
  make it fit (a `VehicleData` copied into a plugin's subclass by property list left every world
  vehicle empty) and never hands its subclass back for the caller to cast
  (`VehicleCatalogServer.vehicle_get_data(rid)` cast to a plugin's class gave the caller null).
* What a vehicle is, is `VehicleData.vehicle` + `livery` of its `vehicle_provider`; how a provider
  spells that in its own files is the provider's business.
* Code outside a plugin that needs something of a vehicle - its side view, its liveries, its
  description, its appearance - asks the catalogue (`VehicleCatalogServer.vehicle_get_*`,
  `livery_list`). A question the catalogue cannot answer is a new method of the core's API that
  the providers implement, decided by the operator - never a reach into the plugin.

## A breach is an alarm, not a problem to solve quietly

When the work at hand would break a boundary - a field in a layer that does not own it, a lower
layer calling or naming an upper one, two layers each calling the other, a copy of state kept "for
convenience" - **stop and report it to the operator before writing it**: which layer, which field
or call, which rule it breaks, and the options you see. Do not invent a workaround (a shim, a
cached copy, a signal relayed through a third party to hide the dependency, a string `call()`), and
do not redesign in silence halfway through a task. The operator decides.

**Never a shortcut to make it build or pass - ABSOLUTE.** A compiler or a test refusing a reach
into another layer (a private member, a missing API, a type that is not this layer's) is the
boundary doing its job. Widening the access (`private` to `public`, a constant exposed), a cast to
the other layer's class, a string `call()`, a copy of the owner's rule, or a test changed to read
elsewhere only moves the rule out of its owner: two places then decide one thing, and they drift
apart without a signal. The owner gets a method answering the very question - on 2026-10-20
`StationServer` needed to know whether a vehicle stands, and the fix was
`RailVehicleServer.vehicle_is_standing()`, not a public `RailVehicleServer::STANDING_SPEED` for it
to compare speeds with. Ask whose rule it is before writing the fix.

Examples of what must raise the alarm:
* a camera server holding what the player drives (the player's state, not the view's);
* the player's server calling the camera server while the camera server reads the player's -
  a cycle: one of them must only read and listen;
* a HUD component keeping its own copy of a vehicle RID another server owns.

## The five boundaries that carry the design

**1. The backend never appears in the core.** No core header names a plugin's class or a
backend's type. A component reaches its backend only as `VehicleServer` was given it: the
vehicle hands every component its `VehicleImplementationServer`
(`VehicleComponent::attach_implementation()`), and an implementation's component takes what it
needs from it in its own `_implementation_changed()` - no `dynamic_cast`, one lifetime (from the
simulation's start to its release). How the backend steps its vehicles together - in which
phases, with what sub-steps - is its own business (`VehicleImplementationServer::stepping_advance()`).

**2. A public API takes handles, not pointers.** `VehicleServer` exposes `vehicle_create`,
`controller_create`/`controller_configure`, `vehicle_bind_controller`, `vehicle_dump_state`, and
`RailVehicleServer` `vehicle_attach`, `vehicle_set_track`, `vehicle_get_transform` - `RID` in,
`Variant` out. Their controller pointers are private helpers, which is the rule working: pointers
stay inside one class. What a consumer would have wanted the controller for comes out under the
handle instead - the controller's moves and commands are relayed by `VehicleServer`
(`vehicle_moved`, `vehicle_command_received`), its rail events by `RailVehicleServer`
(`vehicle_driver_cabin_changed`, `vehicle_trainset_changed`, `vehicle_coupler_*`).

**3. State has one owner and one writer, and a getter only reads.** A value reaches other layers
as a typed getter of the component that owns it, and configuration as that component's (or the
controller's) properties - a reader takes the component once (`vehicle_component_get()`) and calls
it. The same values are also published by name through `_fill_state_dictionary` /
`_fill_config_dictionary` (`VehicleComponent.hpp:108,122`), composed by the server into a dump that
is rebuilt after every step or command (the config dump on every call). The dump is for readers
driven by a name out of the data - the cab (CabinSystem, no second cache in front of the server's),
sound triggers, the console, tests - and **never for a hot path**: a sound, a driver, a camera
reading it per frame composes hundreds of keys for one value (2026-09-30, sound system at x8).
Change detection,
filters and flag consumption belong in the tick, never in a fetch or a getter - the four
violations of this cost a day each (`FINDINGS.md`, 2026-09-22).

**4. Ordering is an event, never a retry.** The simulation steps before anything reads it
(`SimulationServer` ticks on the SceneTree's `process_frame`, before any node's `_process`, and
`VehicleServer` steps the vehicles on its `simulation_advanced`), and a value that is not there
yet is an ordering defect: react to `simulation_configured`, do not raise a "try again next frame"
flag.

**5. A vehicle is its handle, never its id or name.** Everything that holds a vehicle holds its
`RailVehicleServer` RID and commands it with `vehicle_send_command(vehicle_rid, ...)`; a vehicle
keeps its own commands (`VehicleController::register_command`). Its id (`vehicle_id`, the scenery
name in lower case) may be empty or repeated, and it lives only in the server's id registry:
`vehicle_get_rid_by_id` leaves `""` and `"none"` unmapped and gives a repeated id to the later
vehicle, as the original `basic_table::insert` does (`Names.h:27-40`). Only what knows a vehicle by
its id alone - a scenario, an event, the console - goes through it. Its name (`vehicle_get_name()`,
as the scenery writes it) is only shown to the player; nothing is found or compared by it. A
registry keyed by that name elsewhere is how the second of two same-named vehicles lost its
commands and its cab (TrainSystem, removed 2026-09-26).

## Where a new thing goes

* **A value the simulation produces** - a component's own field, published through its
  `_fill_state_dictionary`. Before reusing a config getter for it, find who writes that field: a
  value the simulation writes is state, a value only the core writes is configuration, and
  confusing the two is how a battery reported a voltage that never moved.
* **A value only one consumer needs** - that consumer's layer. Sound bookkeeping lives in the
  sound system; what a gauge shows lives in the cab; a counter a trigger compares against is not
  the vehicle's business.
* **A handle, a placement, a composition** - the server that owns the thing. The body transform is
  composed by `RailVehicleServer` from the two bogie pivots, not by the node that draws it; two
  writers agreed on every straight track in the game and disagreed on curves.
* **Anything a scenery places in the world** (a model, a track, a wire, a light, a smoke emitter,
  a sound) - a server that registers it with `SceneryStreamingServer` (`owner_create` +
  `stream_register`) and builds it only while the camera is in its range. Never a node per
  placement built at load: a scenery has tens of thousands of them. Scenery sounds are event
  pairs of `ScenerySoundServer`'s one `SfxPlayer3D` and one `SfxBank`, placed by
  `SfxSpatialConfig.position`, not a player each (2026-09-29).
* **Something an instancer must honour** - the server, not the node that happens to create the
  instance. The moment a second instancer appeared without nodes, every feature parked on a node
  silently stopped existing.
* **A new component** - a `RailVehicle<Domain>` interface in the core, its implementation in the
  backend's plugin, added with `add_component()` and reached with
  `get_component(VehicleComponentType::TYPE)`.
* **Behaviour that differs between variants of one server object** - an implementation, not a subclass
  of the object: an interface class of `GDVIRTUAL`s whose C++ virtuals forward to the script
  (`SignallingImplementation`), implemented in C++ or GDScript, attached after `*_create()`
  (`system_attach_implementation`). Every callback carries the owner's RID, since a `Resource` is
  shared; the implementation changes state only through the server's API. A plugin's behaviour
  is one such implementation, never the base.
* **Something that lives as long as another server's handle** - follow the owner's freed signal
  (`ModelRenderingServer.instance_freed` drops `SceneryHUDMouseServer`'s pickables on that
  instance), not a validity check.
* **Configuration of a handle** - the server's struct under the RID (`SignalHeadData`), set by
  `<thing>_set_<field>` and announced by `<thing>_config_changed`. What the data already knows
  is derived by the server, never typed in by the user (a signal head's light count,
  `signal_head_set_light_count()`, comes from its model). Shared data describing a *kind* of
  thing is a `Resource` handed to the server (`signal_head_set_kind(rid, SignalHeadKind)`), not
  fields copied onto each handle. Where the source data has no such description, the importer
  derives it from the data, and equal kinds become one resource - the source's encoding is read
  once there and never reaches the server.
* **A node over a handle** - its properties are proxies: getters read the server, setters call
  it, the server's change signals are relayed filtered to the node's RID and refresh the property
  list (`SignalHeadNode`). The node keeps only what the scene sets before the handle exists
  (`kind`, the initial `aspect`) and hands it over once. A node-typed export in C++ is an
  `ObjectID`, not a `Node *` - the node it names may be freed first. An accessor on a native
  subclass needs a name the native class does not have (`ReloadedModel3D.get_model_instance()`,
  not `get_instance()`, on a `VisualInstance3D`).
* **Recurring work** - C++ on `process_frame`, then a `Timer`, then `_process` with an
  accumulator. Never a bare per-frame `_process`, never a loop in one, and never wiring
  (`connect`, `get_node`, a path) inside one.

## Before committing a change to any of this

1. Read every name the diff adds to a core file: no plugin's class, no backend's type.
2. `grep -n '\*' <server>.hpp` - no pointer crossed a public boundary.
3. Read every `get_*` the diff adds: it returns and does nothing else.
4. Read every `_process` the diff touches: no loop, no lookup, no wiring, nothing while idle.
   `grep -n 'vehicle_dump_' <changed files>` - no dump on a per-frame, per-step or per-tick path.
5. Ask the placement question of each new field, out loud, and write the answer in the commit.
6. `grep -rn 'vehicle_id' addons src` - an id is never a key a vehicle is held, cached or
   commanded by; only `vehicle_get_rid_by_id` turns an id into a vehicle.
7. `grep -rn 'emit\.call_deferred\|call_deferred(.emit_signal' <changed files>` - nothing; a
   signal is emitted where its event happens, never deferred (`CODE_STYLE.md`).
8. Read every `.hpp` the diff touches: declarations only - a function body goes to its `.cpp`
   (a template and the `MAKE_*` accessors excepted, `CODE_STYLE.md`).

**And verify against the code, never against a document.** `TODO.md`'s stage list was written when
the first part of each stage landed and went stale; this file will too. Every count and line
number here was measured, and the next reader measures again - a claim about this codebase that
nobody re-ran is a guess with a citation.
