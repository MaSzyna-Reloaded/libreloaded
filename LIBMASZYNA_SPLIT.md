# Splitting libmaszyna out of libreloaded

The plan for cutting this repository: **libreloaded**, the core of Rails Reloaded, and
**libmaszyna**, an open plugin of Rails Reloaded that brings MaSzyna's vehicles and sceneries
(`~/src/rails-reloaded/libmaszyna`). The operator, 2026-10-19: the core is planned as open.
Measured 2026-10-19 on `main` (`55938e5`, the tree of libmaszyna `feat/world-pck` `652c2348`); line
numbers as of that commit. Work goes on `dev/separate-libmaszyna`.

Decided by the operator:

* One shared library. libmaszyna is compiled into libreloaded's `.so` through a formal
  `ReloadedPluginServer`; a separate `.so` is out, because the Mover components inherit from core C++
  classes. libmaszyna builds only against libreloaded's headers.
* libreloaded builds, runs and passes its own tests **without** libmaszyna, C++ and GDScript alike.
* The game (`maszyna-reloaded`) holds both repositories and builds libreloaded with the plugin.
* Everything in `src/legacy/`, `addons/libmaszyna/legacy/`, everything named `Legacy`, and all code
  that ports MaSzyna or reads its formats goes to libmaszyna. libmaszyna has no `legacy`
  directories.
* The `RailVehicle<X>` interfaces stay as the contract (facts, state getters, commands, dump) -
  decision 1 of [discussion #316](https://github.com/MaSzyna-Reloaded/libmaszyna/discussions/316).
  What moves down to the Mover adapters is the description of the Mover's model.
* Consent of the other authors of code in the core is the operator's (`TODO.md`).

Legend:

* `✔` - verified by hand in the code; the rest was measured by grep and agents' reading
* `ALARM` - a separation-of-concerns breach; the direction is decided by the operator before any
  code changes (`AGENTS.md`)
* Numbers `LS-NNN` are permanent - never renumbered or reused. A resolved item is ticked in the
  index and its entry below is deleted, **in the commit that resolves it**; an item that turns out
  to be invalid is marked "(invalid)" in the index
* An item's **Decision** is asked of the operator when its group is started, not before
* Checked before every commit on the branch: the staged diff ticks what it resolves, corrects the
  lines of what it touches, and adds no new instance of an open item

## Index

### Plugin names

- [x] [LS-001](#ls-001) `Legacy<X>` and `MaszynaLegacy<X>` → `Maszyna<X>` ✔
- [x] [LS-002](#ls-002) `E3D*` → `MaszynaE3D*`
- [x] [LS-003](#ls-003) `Mover*` adapters → `MaszynaMover*`
- [x] [LS-004](#ls-004) Saved data naming renamed classes

### Plugin mechanism

- [ ] [LS-005](#ls-005) `ReloadedPluginServer` and `ReloadedPlugin`
- [x] [LS-006](#ls-006) `register_types.cpp` registers the plugin's classes and singletons ✔
- [x] [LS-007](#ls-007) The plugin's tree in `plugins/libmaszyna/`, without `legacy` directories
- [x] [LS-008](#ls-008) Plugin pieces in the core's CMake and Makefile
- [x] [LS-009](#ls-009) The python host in the core's `.gdextension`
- [x] [LS-010](#ls-010) `MaszynaE3DRenderingServer` created in the middle of the core's boot chain

### Core reaching the plugin (ALARM)

- [x] [LS-011](#ls-011) `SceneryTrianglesSink` and the triangles classes include each other ✔
- [x] [LS-012](#ls-012) `LuaScriptContext` opens `eu07.events` and resolves paths through `MaszynaDataPath` ✔
- [x] [LS-013](#ls-013) A signal head is the lights of a model instance ✔
- [x] [LS-014](#ls-014) `ScenarioEventServer` keeps the animations' `:done` events ✔
- [x] [LS-015](#ls-015) Core servers call `MaszynaE3DRenderingServer` directly ✔
- [x] [LS-016](#ls-016) `RailVehicle3D` and `SignalHeadNode` reach `MaszynaE3DModelInstance` by name ✔
- [x] [LS-017](#ls-017) `RailVehicleRenderingServer` typed on `MaszynaE3DModel` and `MaszynaE3DSubModel`
- [x] [LS-018](#ls-018) The default vehicle controller is a plugin class ✔

### Ports in the core

- [x] [LS-019](#ls-019) `RailVehicleRenderingServer`: the `DynObj.cpp` animations ✔
- [x] [LS-020](#ls-020) `TractionServer`: the network model of `Traction.cpp`/`TractionPower.cpp`
- [x] [LS-021](#ls-021) Ports in components: wipers, mirrors, Hasler, wheels ✔
- [x] [LS-022](#ls-022) Small ports rewritten from a written spec ✔

### Cutting the layers: configuration (#316)

- [x] [LS-023](#ls-023) Every `RailVehicle*` property classified: fact or Mover model
- [x] [LS-024](#ls-024) Mover model selectors in the interfaces ✔
- [x] [LS-025](#ls-025) FIZ-shaped tables in the core ✔
- [x] [LS-026](#ls-026) Properties named after FIZ keys ✔
- [x] [LS-027](#ls-027) Mover model parameters no other layer reads
- [x] [LS-028](#ls-028) Getters whose meaning is the Mover's model

### The core without the plugin

- [ ] [LS-029](#ls-029) The core has no vehicle backend
- [x] [LS-030](#ls-030) The MaSzyna game directory in `UserSettings` and `GameDataServer`

### GDScript reaching the plugin (ALARM)

- [x] [LS-031](#ls-031) `libmaszyna.gd` registers the plugin's autoloads, types and importer
- [x] [LS-032](#ls-032) The `.scn` loader outside `legacy/`
- [x] [LS-033](#ls-033) Editor plugins working on MaSzyna data
- [x] [LS-034](#ls-034) `SmokeSourceLibrary` reads MaSzyna data
- [x] [LS-035](#ls-035) The environment pushes into E3D
- [x] [LS-036](#ls-036) Materials and the data path in tracks and the world
- [x] [LS-037](#ls-037) Sound typed on MMD and Mover classes
- [x] [LS-038](#ls-038) The world format built on importer data types
- [x] [LS-039](#ls-039) Cab control caption from `MmdCabControlCaptions` ✔
- [x] [LS-040](#ls-040) `TrainSet3D` typed on `MaszynaRailVehicle3D` ✔
- [x] [LS-069](#ls-069) The scenery streaming panel reads MaSzyna's lights (ALARM)

### Tests

- [x] [LS-041](#ls-041) `MaszynaGutTest` types Mover and E3D classes
- [x] [LS-042](#ls-042) Tests and fixtures of the plugin

### Core names

- [x] [LS-043](#ls-043) The core's own `Maszyna*` names → `Reloaded*`

### Moving out

- [x] [LS-044](#ls-044) The plugin moved to its own repository
- [x] [LS-045](#ls-045) The game's AI driver is a port of `Driver.cpp`
- [ ] [LS-046](#ls-046) The game builds with both repositories
- [ ] [LS-047](#ls-047) libreloaded's public headers for building plugins
- [ ] [LS-048](#ls-048) The plugin's addon as a `.pck`
- [ ] [LS-049](#ls-049) The plugin's class reference

### MaSzyna out of the core's text

- [ ] [LS-050](#ls-050) MaSzyna's vocabulary in the core
- [ ] [LS-051](#ls-051) Citations of the original's sources in the core

### The core's servers and their plugins' implementations

- [x] [LS-056](#ls-056) Scenery base types of the core named after MaSzyna or kept in it
- [x] [LS-057](#ls-057) No `SceneryServer`: the scenery list of each source is somebody else's
- [x] [LS-058](#ls-058) The Stations window attached to a world
- [x] [LS-060](#ls-060) `ScenarioEventServer` is the original's event system
- [x] [LS-061](#ls-061) `TrackRenderingServer` draws a track from MaSzyna's fields
- [x] [LS-062](#ls-062) `TractionRenderingServer` draws traction from MaSzyna's fields
- [x] [LS-063](#ls-063) No server of rolling stock: the vehicles are MaSzyna's directory
- [ ] [LS-064](#ls-064) The track and traction rendering servers and backends are GDScript
- [x] [LS-065](#ls-065) `ModelRenderingServer` is one backend under a singleton's name
- [x] [LS-070](#ls-070) The scenario scripts' Lua carries MaSzyna's memory cells
- [x] [LS-073](#ls-073) A trainset is the description it was placed by, not the trainset now
- [x] [LS-071](#ls-071) The core's `Timetable` is the original's `TTrainParameters`
- [x] [LS-072](#ls-072) The core's track signals are the original's track events
- [x] [LS-074](#ls-074) The game's scenery selector reads MaSzyna's files, not the providers' list

## Gates

Checked before LS-044, all clean:

* **G1** `grep -rn '#include "legacy/\|#include "maszyna/' src --include=*.cpp --include=*.hpp`
* **G2** the plugin's C++ class names (`GDCLASS` in `plugins/libmaszyna`) over `src/`, comments
  excluded
* **G3** the plugin's `class_name`s and autoload names over `addons/libreloaded` and the core's
  tests, comments excluded
* **G4** no `res://addons/libmaszyna` and no uid of a plugin file in the core's files
* **G5** `make compile-core-check` builds with `LIBRELOADED_PLUGINS=`, and the core's demo passes
  `--import` and the core's tests
* **G6** no plugin path in `CMakeLists.txt` other than the default
* **G7** no `legacy` directory or path in either tree
* **G8** no MaSzyna vocabulary and no citation of the original's sources in the core - code,
  comments and class reference (LS-050, LS-051)

## Plugin mechanism

### LS-005

**`ReloadedPluginServer` and `ReloadedPlugin`**

* **Where:** `src/plugins/` (`ReloadedPlugin`: `name`, `description`, `logo`, the stages
  `register_classes()`, `create_servers()`, `free_servers()`; `ReloadedPluginServer`:
  `plugin_get_list()`); the plugins are still compiled into the core's library
  (`cmake/reloaded_plugins.gen.cpp.in`, `plugins/libmaszyna/plugin.cmake`)
* **Rule:** operator: a plugin is a library of its own, built by anyone on libreloaded's public
  headers, found in the game's plugins directory
* **Fix:**
  * a plugin is a `.so`/`.dll` linked against `libreloaded`, with `extern "C"
    reloaded_plugin_create()`, and a `.pck` of its addon beside it - packed as one file, as a
    Minecraft mod is a `.jar`: a manifest (name, version, the libreloaded version it is built for,
    the plugins it needs) with the `.pck`, which every platform shares, and a library per platform
    (Linux x86_64, Linux arm64, Windows); a plugin of GDScript and resources alone is the `.pck`;
  * `ReloadedPluginServer` loads every plugin of `plugins/` of Rails Reloaded's game directory (not
    MaSzyna's; in the editor the project's), where a Steam DLC's depot installs one: it refuses one
    built for another libreloaded version (the game goes on without it), loads them in the order of
    their dependencies, takes its platform's library, mounts its `.pck`, and runs the stages as now;
    a plugin's autoloads, settings and HUD windows it adds itself - the game's `project.godot` knows
    none of them;
  * the manifest and the libraries are signed (Ed25519): one signed by the key built into the core
    loads at once, an unsigned one (a mod of the community) only once the player allows it, with a
    warning - its library runs with the player's rights;
  * the core exports its symbols (Linux: default visibility; Windows: MinGW only,
    `--export-all-symbols`) and the whole of godot-cpp, built with a profile: the classes the core
    uses, the ones libmaszyna uses (`ArrayMesh`, `BaseMaterial3D`, `GPUParticles3D`, `Image`,
    `ImageTexture`, `ParticleProcessMaterial`, `SpotLight3D`) and a margin of 3D and resource
    classes (`Mesh*`, `*Material*`, `*Light3D`, `Texture*`), so the plugin uses the core's one copy;
  * the generated `reloaded_plugins.gen.cpp` goes.
* **Decision:** the hooks the plugins need beyond the stages come with LS-012, LS-015; LS-019 and
  LS-020 are a plugin's server deriving from the core's and registered under its name
  (`MaszynaE3DRailVehicleRenderingServer`, `MaszynaTractionServer`).
* **Decision:** operator, 2026-10-20: plugins as DLC downloaded into the game's `plugins/`, as
  Minecraft's mods; Android (and iOS) left out for now - a downloaded library cannot be loaded there,
  their plugins are compiled in.
* **Open:** whether a `class_name` of a `.pck` mounted after the start is known to the scripts (the
  global class cache of an exported game is the export's) - a probe before the loader is designed on it.

## Ports in the core

## The core without the plugin

### LS-029

**The core has no vehicle backend**

* **Where:** `RailVehicleController` has only pure C++ virtuals; the one controller is
  `MaszynaMoverRailVehicleController`; no core test can build a vehicle without the plugin
* **Fix:** a minimal vehicle controller in the core, for the core's tests.
* **Decision:** operator, 2026-10-19: no minimal controller. What is common to every controller goes
  into `RailVehicleController` itself; the tests that need a vehicle are the plugin's.

## GDScript reaching the plugin (ALARM)

References to plugin classes in code outside `addons/libmaszyna/legacy/`, comments excluded:
`scenery/` 153 (`scenery_instancer.gd` 96), `editor/` 49, `sound/` 25, `libmaszyna.gd` 24,
`tracks/` 11, `smoke/` 10, `environment/` 5, `traction/` 4, `trainsets/` 1, `cabin/` 1.

## Tests

## Core names

## Moving out

### LS-046

**The game builds with both repositories**

* **Where:** `maszyna-reloaded` - `.gitmodules` (`vendor/libmaszyna` → MaSzyna-API-wrapper), the
  `addons/libmaszyna` symlink, `Makefile`, `project.godot`, CI, the Windows installer
* **Fix:** submodules `vendor/libreloaded` and `vendor/libmaszyna`, `LIBRELOADED_PLUGINS`, both
  addons linked, `bin/libreloaded`. Done (2026-10-21): `LIBRELOADED_PLUGINS` names the plugins'
  checkouts, `make plugin-local` / `plugin-submodule` as `libreloaded-local`; left: the submodules -
  libmaszyna has no origin yet, and `vendor/libmaszyna` is still MaSzyna-API-wrapper. The smoke of LS-034: the autoload `SmokeSourceLibrary` →
  `MaszynaSmokeSourceLibrary` (`res://addons/libmaszyna/smoke/`), and the keys of
  `settings/settings.json` `maszyna/smoke/{enabled,dynamic/*,static/*}` → `reloaded/smoke/*`
  (`maszyna/smoke/generator_mode` stays). The sceneries of LS-057: `SceneryInstancer` →
  `SceneryServer` (`scenery_list()`, `scenery_load()`, `scenery_cancel()`) and the autoload
  `MaszynaSceneryProvider`; the selector lists `SceneryInfo` (`MaszynaSceneryInfo` extends it).
* **Also:** libreloaded's CMake links its own `gut` and `gnd_sfx` into the project it builds into
  (`reloaded_link_addon()`, `CMakeLists.txt:279-281`) and so replaced the game's own links to its
  `vendor/` (2026-10-19); a project that has an addon already keeps it.
* **Also:** the game starts every scenery's scenario with libmaszyna's `MaszynaScenario`
  (`world/world.gd:75`), a world's too - which has no MaSzyna sounds or events since LS-038; a world
  gets a scenario start of its own.
* **Also:** LS-045 (2026-10-20) took the game's `ai_driver/` and its 22 test scripts to libmaszyna,
  which registers `MaszynaAIDriver` itself (`MaszynaSceneryProvider`, `"maszyna"`); in the game's
  working tree they are removed, and so is the registration in `game.gd` and
  `tests/simulation_runtime_hook.gd` - not committed. The game's HUD reads the AI's classes
  (`driving_aid.gd`, `scenario_task.gd`, `timetable_panel.gd`, `vehicle_card.gd`,
  `vehicle_selector_row.gd`); the AI's hint texts are translated by the game's catalogue
  (`translations/*.po`), libmaszyna has none of its own. The game's `tests/fixtures` still hold the
  elf fixtures copied to libmaszyna, and its `maszyna_gut_test.gd`/`maszyna_startup_test.gd` are
  older copies of libmaszyna's.
* **Also:** the game's HUD reads the core's servers: a train's timetable off StationServer
  (`TrainRun`), the trainsets off RailVehicleServer, the vehicles' side views off
  VehicleCatalogServer and the driver off `DriverServer.driver_get_state()` (`DriverState`,
  `DriverHint`). It still names libmaszyna's classes in `game_hud.gd` alone:
  `attach_environment(EnvironmentNode)` and `attach_world(MaszynaWorld)`.
  `bug_report/bug_report_snapshot.gd` does not compile (`signal_head_get_instance()` `:98`,
  `MaszynaEnvironment` `:175`).

### LS-047

**libreloaded's public headers for building plugins**

* **Where:** nothing yet; a plugin builds only inside this repository (`LIBRELOADED_PLUGINS`)
* **Rule:** operator: anyone builds libmaszyna on libreloaded's public headers
* **Fix:** a released package per core version: the core's headers, godot-cpp's headers of the
  same version and profile, `extension_api.json`, the import library for Windows (MinGW), and a
  CMake package a plugin finds; built by the core's CI in the pinned SDK image.
* **Decision:** operator, 2026-10-19: GitHub Releases of libreloaded, built by the CI.

### LS-048

**The plugin's addon as a `.pck`**

* **Where:** `plugins/libmaszyna/addons/libmaszyna/` reaches a project only by a link
  (`reloaded_link_addon`)
* **Fix:** the plugin's build packs its addon into `libmaszyna.pck`, placed beside its library in
  the plugins directory (LS-005); the plugin's autoloads are made by the plugin, not by the game's
  `project.godot` (today its `libmaszyna.gd` adds them there).

### LS-049

**The plugin's class reference**

* **Where:** `scripts/make-api-docs` documents the core only (`src/`, `addons/libreloaded`,
  `doc_classes/`)
* **Fix:** the same reference for `plugins/libmaszyna` (`src/maszyna`, `addons/libmaszyna`,
  `doc_classes/`), generated in the plugin's repository.

## MaSzyna out of the core's text

### LS-050

**MaSzyna's vocabulary in the core**

* **Where:** `src/`, `addons/libreloaded/`, `doc_classes/` on `ed6e8bc`: `E3D` 508 times in 87 files,
  `Mover` 202 in 77, `submodel` 147 in 20, `FIZ` 110 in 58, `MMD` 92 in 34, `.scn`/`.inc`/`.t3d` 40 -
  mostly in what moves to the plugin (LS-019, LS-031..LS-038), the rest in comments and the class
  reference
* **Rule:** operator: FIZ, MMD and the rest are libmaszyna's nomenclature; the core names its own
  concepts (a model's part, not a submodel; a vehicle a plugin builds from its data, not one built
  from FIZ)
* **Fix:** each stage cleans what it touches; G8 before LS-044.

### LS-051

**Citations of the original's sources in the core**

* **Where:** ~840 comments citing `File.cpp:N` of MaSzyna in the core (densest `RailVehicleServer`
  101, `track_rendering_server.gd` 46, `RailVehicleBrake.hpp` 27, `ScenarioEventServer` 27); the
  rule that asks for them, `AGENTS.md` "Code generation" (a short comment pointing to the original
  source)
* **Rule:** operator: the core points to MaSzyna nowhere, comments included
* **Fix:** the behaviour is said in the core's own words, the citations stay only in libmaszyna; the
  `AGENTS.md` rule moves to libmaszyna's own `AGENTS.md`, the core's says the opposite. The `TODO.md`
  entry on the 842 citations goes with it.


### LS-064

**The track and traction rendering servers and backends are GDScript**

* **Where:** `addons/libreloaded/tracks/track_rendering_server.gd`, `track_rendering_backend.gd`,
  libmaszyna's `maszyna_track_rendering_backend.gd` (2 148 lines: rails of a profile, trackbed,
  switch blades, road cells); `addons/libreloaded/traction/traction_rendering_server.gd`,
  `traction_rendering_backend.gd`, libmaszyna's `maszyna_traction_rendering_backend.gd`
* **Rule:** `CODE_STYLE.md`: recurring work goes to C++ first - the meshes are built on the main
  thread, interpreted, as each track comes into the camera's range
* **Fix:** the servers and the backends in C++, the core's and libmaszyna's, as `SmokeServer`.
  Done for the tracks (2026-10-20: `TrackRenderingServer`, `StandardTrackRenderingBackend`,
  `MaszynaTrackRenderingBackend`); left: the traction's server and backends.
* **Decision:** operator, 2026-10-19: after LS-062.
