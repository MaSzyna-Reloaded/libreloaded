# LS-056 - the core's own descriptions of tracks, traction and power sources

Scope: `LIBMASZYNA_SPLIT.md`, LS-056. The operator (2026-10-19): the core has a description of its own
of tracks, of traction and of traction power; MaSzyna's fields (`tex_height`, `material1/2`, ...) are
only libmaszyna's, in a subclass of the core's description (`TrackData` and `MaszynaTrackData
extends TrackData`, and so on); a world's PCK holds only the core's. Today the three are libmaszyna's
`MaszynaTrackData`, `MaszynaTractionData` and `MaszynaPowerSourceData`
(`plugins/libmaszyna/addons/libmaszyna/scenery/`), the `.scn`'s shape, handed to the core's servers by
`SceneryServer.build_track()`, `_build_traction()`, `_build_wire_power()` and
`_build_power_source()`. Measured on `782ff43`; decided by the operator the same day.

## Track - `TrackData` (core), `MaszynaTrackData extends TrackData` (libmaszyna)

| field today | read by (outside the importer) | core `TrackData` | libmaszyna `MaszynaTrackData` |
|---|---|---|---|
| `type` | `TrackServer.track_update()` | `kind` | |
| `track_name` | `TrackServer`, event factory, scenery info, MMD sound events | `name` | |
| `curve`, `diverging_curve` | `TrackServer.track_update_curves()` | `curve`, `diverging_curve` | |
| `width` | `TrackServer`, track rendering | `width` | |
| `friction` | `TrackServer.track_update_properties()` | `friction` | |
| `sound_distance` | `running_sound_model.gd:279` | `joint_spacing` | |
| `damage_flag` | `RailVehicleServer.cpp:1758` (derailment) | `damage` | |
| `visible` | track rendering | `visible` | |
| `parameters["velocity"]` | `TrackServer.track_set_velocity()` | `speed_limit` | |
| `environment` | `running_sound_model.gd:304` | | `meta["environment"]` (`EnvironmentType`); BRIDGE, TUNNEL also `structure` |
| `quality_flag` | `running_sound_model.gd:85` | | `meta["quality"]` |
| `material1`, `material2` | track rendering | | as today |
| `tex_length`, `tex_height`, `tex_width`, `tex_slope` | track rendering | | as today |
| `railprofile`, `parameters["trackbed"]` | track rendering | | as today |
| `parameters["isolated"]`, `parameters["friction"]`, `events` | event factory | | as today (LS-060) |
| `length` | importer only | | as today |

`environment` and `quality_flag` leave `TrackServer` (`track_update_properties()`,
`track_get_environment()`, `track_get_quality_flag()`); the operator: in a `meta` dictionary, which
`TrackServer` keeps per track (`track_set_meta()`, `track_get_meta()`) and the running sound reads.

### What a track is - the core's features in place of the look

The operator (2026-10-19): the description says what a segment is and what it has, never how it is
drawn or with which texture; the renderer (a plugin's) picks the materials and the section
for the features. A `.scn` still gives its `material1/2`, `tex_*` in `MaszynaTrackData` - that is
MaSzyna's format.

| field | values | note |
|---|---|---|
| `structure` | NONE, BRIDGE, TUNNEL | a feature of the segment, not the sound's `environment` |
| `bed` (a rail track) | CRUSHED_STONE, GRAVEL, DIRT, SLAB, EMBEDDED | what the bed is made of: crushed stone ballast, gravel, earth, a concrete slab, embedded in a street |
| `surface` (a road) | ASPHALT, CONCRETE, PAVING, GRAVEL, DIRT | |
| `centre_line` (a road) | bool | |
| `shoulder` (a road) | bool | |
| `meta` | Dictionary | what a plugin keeps of its own (`TrackServer` keeps it per track too) |

A world's importer writes the features in place of `material1/2` and `tex_*`; the look of an asphalt
or a dirt road is the renderer's.

## Traction - `TractionData` (core), `MaszynaTractionData extends TractionData` (libmaszyna)

| field today | read by | core `TractionData` | libmaszyna `MaszynaTractionData` |
|---|---|---|---|
| `contact_p1`, `contact_p2`, `support_p1`, `support_p2` | `TractionRenderingServer`, `TractionServer.wire_set_params()` | yes | |
| `wire_thickness`, `wires`, `wire_offset`, `min_height`, `segment_length` | `TractionRenderingServer` | yes | |
| `visible` | `TractionRenderingServer` | yes | |
| `power_supply_name`, `nominal_voltage`, `max_current`, `resistivity`, `parallel` | `TractionServer` | yes | |
| `damage_flag` | `_get_traction_material()` (PATINA) | yes | |
| - | - | `meta` (Dictionary) | |
| `material` (copper, aluminium) | `_get_traction_material()` | | yes |

## Power source - `PowerSourceData` (core)

Every field (`name`, `position`, `nominal_voltage`, `voltage_frequency`, `internal_resistance`,
`max_output_current`, `fast_fuse_timeout`, `fast_fuse_repetition`, `slow_fuse_timeout`, `is_section`,
`recuperation`) is the core's; `position` is read by nothing and stays.

## What follows from it

* Drawing a track from MaSzyna's fields goes to libmaszyna (LS-061); the core draws a `TrackData`
  with a look of its own.
* The inner class `TrackData` of `track_rendering_server.gd:91` becomes `TrackGeometry`.
* libmaszyna's importer writes its subclasses; the worlds already exported hold `Maszyna*Data`
  resources and are exported again (no migration, LS-043).

## The rest of a scenery's types

The operator, 2026-10-19: no `Scenery` prefix where the name says it already (`CompiledScenery`);
a vehicle of a trainset stripped of MaSzyna's is `VehicleData`; a model is found by its name and by
the backend it is the one of (`model_provider`, LS-065).

| today (libmaszyna) | core | libmaszyna (extends the core's) |
|---|---|---|
| `MaszynaCompiledScenery` | `CompiledScenery`: `nodes`, `tracks`, `traction`, `power_sources`, `models`, `sounds`, `lines`, `trainsets`, `scripts`, `stations`, `signal_boxes` | `MaszynaCompiledScenery`: the `.scn` cache (`format_version`, `source_path`, `parameters_hash`, `dependencies`), `events`, `memcells`, `launchers`, `isolated_sections`, `region_files`, `triangle_chunks` |
| `MaszynaModelData` | `ModelData`: one model in the game - `name`, `model`, `model_provider`, `skins`, `position`, `rotation`, `range_min/max`, `signal_head_kind` | `data_path`, `lights`, `light_colors` |
| `MaszynaDynamicData` | `VehicleData`: `vehicle_id`, `name`, `direction`, `gap`, `coupling`, `velocity`, `driver_type`, `load_name/amount`, `trainset_index` | `MaszynaVehicleData`: `data_path`, `file_name`, `skin` |
| `MaszynaTrainsetData` | `TrainsetData` (`dynamics` → `vehicles`) | - |
| `MaszynaLinesData`, `MaszynaSoundData`, `MaszynaStationData`, `MaszynaSignalBoxData` | `LinesData`, `SoundData`, `StationData`, `SignalBoxData` | - |
| `MaszynaLinesGeometry` (C++) | `SceneryLinesGeometry` | - |
| `maszyna_scenery_content_node.gd` | `SceneryContentNode` | - |

Until the rolling stock has its server (LS-063), `MaszynaSceneryProvider` builds MaSzyna's vehicles only
(a `VehicleData` taken as `MaszynaVehicleData`); until LS-065, a model of no `.scn` is registered
with the E3D server by its name alone.
