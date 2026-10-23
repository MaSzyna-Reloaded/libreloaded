@tool
extends SceneryContentNode
class_name World

## A world of the game's own format, described by worlds/<name>.json of the game's directory
## (read_description()), its files in its packs (mount()) - saved without a script: WorldData,
## CompiledScenery, WorldSection (CODE_STYLE.md, "Saved data names no script"). What the simulation
## needs - tracks, traction, power sources, signals - is its scenery (SCENERY_FILE), loaded whole
## with its scenario (SCENARIO_FILE: trainsets, sounds, scripts) and registered through
## SceneryServer.instantiate_compiled() by the worlds' provider (WorldSceneryProvider); what is only
## drawn is in a section a cell of SceneryStreamingServer's grid (SECTIONS_DIR), read as the camera
## comes near (WorldSectionProvider). A plugin's world divides
## it otherwise (its subclass). The node is made of its WorldData (open()); nothing of it is saved.

## Where the worlds are, under the game's directory: <name>.json describing a world, its packs in <name>/
const WORLDS_DIR:String = "worlds"
const DESCRIPTION_EXTENSION:String = "json"
const PACK_EXTENSION:String = "pck"
## The packs of what worlds share, under the game's directory: <pack>.pck, its files under
## ASSETS_RESOURCES_DIR/<pack>/
const ASSETS_DIR:String = "world_assets"
const ASSETS_RESOURCES_DIR:String = "res://world_assets"
## Where a world's files are in the packs: RESOURCES_DIR/<name>/
const RESOURCES_DIR:String = "res://worlds"
## The world (WorldData), its scenery and its scenario (CompiledScenery), in its directory
const WORLD_FILE:String = "world.res"
const SCENERY_FILE:String = "scenery.res"
const SCENARIO_FILE:String = "scenario.res"
## The sections (WorldSection), a file a cell (section_file())
const SECTIONS_DIR:String = "sections"

## The world's name: its .json and its signalling system's (get_scenery_name())
var world_name:String = ""
## The projected metres (east, north) the world's origin lies at
var origin:Vector2 = Vector2.ZERO
## The directory of the world's files - its WORLD_FILE's, in the packs or in a tool's sources
var world_dir:String = ""
## The scenario clock the world declares for its start, hours (WorldData.declared_start_time)
var declared_start_time:float = START_TIME_DEFAULT
## The cells there are sections of (WorldData.section_cells)
var section_cells:Array[Vector2i] = []
## The world's scenery and scenario; read from world_dir as it is loaded, unless given before
var scenery:CompiledScenery = null
var scenario:CompiledScenery = null


func get_scenery_name() -> String:
    return world_name


## The description of the world `filename` (worlds/<filename> of the game): {name, title,
## description, pck, world, assets, ...}; {} when there is none
static func read_description(filename:String) -> Dictionary:
    var path:String = UserSettings.get_data_dir().path_join(WORLDS_DIR).path_join(filename)
    var description:Variant = JSON.parse_string(FileAccess.get_file_as_string(path)) if FileAccess.file_exists(path) else null
    return description if description is Dictionary else {}


## Mounts the packs of what the world `description` (read_description()) shares with others
## ("assets", ASSETS_DIR) and its own ("pck"), their files under the paths they have in them; false
## when it cannot be - the editor mounts none (Godot's editor has packs off)
static func mount(description:Dictionary) -> bool:
    if Engine.is_editor_hint() or not description.has("pck"):
        return false
    for pack:String in description.get("assets", []):
        if not ProjectSettings.load_resource_pack(UserSettings.get_data_dir().path_join(ASSETS_DIR).path_join(
                pack + "." + PACK_EXTENSION)):
            push_error("World %s: no pack %s in %s" % [description.get("name", ""), pack, ASSETS_DIR])
    return ProjectSettings.load_resource_pack(UserSettings.get_data_dir().path_join(WORLDS_DIR).path_join(description["pck"]))


## The world's data the description `description` names ("world"), its packs mounted; null for none
static func read_data(description:Dictionary) -> WorldData:
    mount(description)
    var path:String = description.get("world", "")
    return load(path) as WorldData if path and ResourceLoader.exists(path) else null


## The world the description `description` describes, made of its data, not in the tree; null when
## it has none
static func open(description:Dictionary) -> World:
    var data:WorldData = read_data(description)
    if not data:
        return null
    var world:World = World.new()
    world.take_data(data, String(description["world"]).get_base_dir())
    return world


## The scenario of the world `filename` (worlds/<filename>), its packs mounted; null when it has none
static func read_scenario(filename:String) -> CompiledScenery:
    var description:Dictionary = read_description(filename)
    var path:String = String(description.get("world", "")).get_base_dir().path_join(SCENARIO_FILE)
    mount(description)
    return load(path) as CompiledScenery if description.has("world") and ResourceLoader.exists(path) else null


## The file of the section of `cell` in a world's directory
static func section_file(cell:Vector2i) -> String:
    return SECTIONS_DIR.path_join("%d_%d.res" % [cell.x, cell.y])


## The node becomes the world `data` describes, its files in `dir`; a clock the world declares none
## of stays the scenery's default
func take_data(data:WorldData, dir:String) -> void:
    world_name = data.world_name
    origin = data.origin
    world_dir = dir
    section_cells = data.section_cells
    if data.declared_start_time >= 0.0:
        declared_start_time = data.declared_start_time


func _has_source() -> bool:
    return scenery != null or not world_dir.is_empty()


## The scenery and the scenario registered whole, then the sections streamed around the camera
func _instantiate_content() -> void:
    var measurement:SceneryLoadMeasurement = SceneryLoadMeasurement.new(self)
    if not scenery and ResourceLoader.exists(world_dir.path_join(SCENERY_FILE)):
        scenery = load(world_dir.path_join(SCENERY_FILE)) as CompiledScenery
    if not scenario and ResourceLoader.exists(world_dir.path_join(SCENARIO_FILE)):
        scenario = load(world_dir.path_join(SCENARIO_FILE)) as CompiledScenery
    var compiled:CompiledScenery = CompiledScenery.new()
    if scenario:
        compiled.trainsets.append_array(scenario.trainsets)
        compiled.sounds.append_array(scenario.sounds)
        compiled.scripts.append_array(scenario.scripts)
        compiled.nodes = scenario.nodes
    if scenery:
        compiled.tracks.append_array(scenery.tracks)
        compiled.traction.append_array(scenery.traction)
        compiled.power_sources.append_array(scenery.power_sources)
        compiled.models.append_array(scenery.models)
    # the worlds' provider registers the world's models, signals and trainsets
    await SceneryServer.instantiate_compiled(self, compiled, measurement, WorldSceneryProvider)
    if is_load_given_up() or not section_cells:
        return
    _provider_rids.append(SceneryStreamingServer.provider_register(
            WorldSectionProvider.new(world_dir, section_cells), get_world_3d().scenario))


func _read_environment_declarations() -> void:
    start_time = declared_start_time
