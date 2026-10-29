@tool
extends Node3D
class_name SceneryContentNode

## What a scenery puts in the world, registered with the servers (SceneryServer.instantiate_compiled()),
## and its scenario's declarations: the RIDs it owns, its load, its clearing and its environment. Where
## the content comes from is the subclass's: a plugin's file of a scenery, or the sectors of a
## plugin's world.

signal loaded
## What a load is doing, in the order a scenery provider goes through it: the scenery's files read
## (parsed, or its cache), tracks, traction and models registered, terrain built, objects
## instanced, vehicles built - and then, the game's own, the player's surroundings built by the
## streaming around the player's camera
enum LoadStage { FILES, INFRASTRUCTURE, TERRAIN, OBJECTS, VEHICLES, SURROUNDINGS }
## What a file of the game's data its load could not find does to the scenario (get_missing_files()):
## it may not run as written - a part of the scenery, a vehicle's description (SCENARIO) - or not
## everything of it may be drawn or heard - a model, a texture, a sound (APPEARANCE)
enum MissingFileEffect { SCENARIO, APPEARANCE }
## Emitted by the scenery provider as the load goes (progress 0..1 of the whole load, its stage, what
## it is doing now)
signal load_progress(progress:float, stage:LoadStage, message:String)
## Emitted by the scenery provider while the files are parsed: includes parsed so far and the file being
## parsed now - the progress of the files stage alone cannot tell a large include from a stall
signal load_files_parsed(count:int, filename:String)
## A load() has ended, whatever came of it: loaded, given up, or nothing to load
signal load_ended
## The content is about to be freed (a reload, an unload): whatever runs on it - its scenario -
## stops now, while everything it reaches still exists
signal unloading
## Emitted as the content is freed over frames (load(), clear()): the share of it freed so far
signal clear_progress(progress:float)
## The content freed over frames is gone - a load() goes on loading
signal cleared
## Emitted after every load with the first vehicle's vehicle_id ("" when the scenery has none)
signal scenery_loaded(first_vehicle_id:String)

## Milliseconds spent freeing content per frame while reloading
const CLEAR_BUDGET_MSEC:int = 8

var _dirty:bool = false
var _editor_dirty:bool = false

const SceneryEditor = preload("res://addons/libreloaded/editor/scenery_toolbar/scenery_editor.gd")

@export var autoload:bool = true
## The vehicles of one trainset as the player arranged them for this load (the scenery
## selector), front first, each declared one with its place in the declared trainset -
## TrainsetData.get_arranged_vehicles(). The scenery cache holds what the scenery declares; this
## only replaces it for this load. Empty: the trainsets as declared.
@export var trainset_override:Array[VehicleData] = []

## Off by default: loaded content gets no owner and stays unselectable in the editor (matches
## the model nodes' own default). A plugin's editor tool switches it on to make the content
## inspectable and selectable while authoring - see SceneryServer's attach loop for what it changes.
@export var editable_in_editor:bool = false:
    set(x):
        if not x == editable_in_editor:
            editable_in_editor = x
            _editor_dirty = true

var _loading:bool = false
## The load under way was given up (stop_loading())
var _load_given_up:bool = false
## The content of the file is here - the game's data read again loads it again
var _loaded:bool = false

## Tracks, traction and models are built directly against TrackServer/TrackRenderingServer/
## TractionRenderingServer/TractionServer/ModelRenderingServer/SignallingServer RIDs, not as scene nodes (see
## scenery_instancer.gd's instantiate()
## doc comment for why) - so unlike triangle children, they aren't cleaned up just by
## removing children from the tree. scenery_instancer.gd appends the RIDs it creates here;
## _free_owned_rids() releases them all, called before every reload and when the node is freed.
var _track_rids:Array[RID] = []
var _track_render_rids:Array[RID] = []
var _traction_rids:Array[RID] = []
var _wire_power_rids:Array[RID] = []
var _power_source_rids:Array[RID] = []
var _model_rids:Array[RID] = []
var _signalling_system_rids:Array[RID] = []
## What its provider made of the content and gave it to free (content_own()): [the RIDs, freeing one,
## setting one's scenario]
var _provider_owned:Array[Array] = []
## SceneryMeshRenderingServer's meshes of the scenery (a plugin's world's sectors)
var _mesh_rids:Array[RID] = []
## SceneryStreamingServer's providers of the scenery's region files
var _provider_rids:Array[RID] = []
var _pickable_rids:Array[RID] = []
var _event_rids:Array[RID] = []
## The event systems of its provider (ScenarioEventServer.system_create()), freed after the events
var _event_system_rids:Array[RID] = []
var _isolated_rids:Array[RID] = []
## RoadServer's roads of the scenery's road network (roads_own())
var _road_rids:Array[RID] = []
var _trainset_rids:Array[RID] = []
## The scenery's vehicles, in the order of its file
var _vehicle_rids:Array[RID] = []
## What the scenario starts - loaded here, run only by the game: the scenery's scripts
var _scenario_scripts:Array[String] = []
## The scenery's own radio, what its events say on the radio is transmitted from - one a scenery,
## gone with it
var _scenery_radio:SceneryVirtualRadio = null

@export var title:String = ""
@export var category:String = ""
@export_multiline var description:String = ""
## The environment the loaded scenery sets its time, date and weather in
@export_node_path("EnvironmentNode") var environment_node_path:NodePath
@export_tool_button("Load") var load_action:Callable = self.load

## Original engine: clamp of scenario.weather.temperature (Globals.cpp:390)
const TEMPERATURE_MIN:float = -15.0
const TEMPERATURE_MAX:float = 45.0
## Original engine: clamp of a random overcast (simulationstateserializer.cpp:230)
const OVERCAST_MAX:float = 2.0
## Original engine: overcast up to 1 is the cloud cover, above it the weather turns to rain or snow
## (simulationenvironment.cpp:64-71) in two steps only - the light and the medium precipitation
## texture (opengl33precipitation.cpp:132). It has no storms, so the medium step stays at the
## precipitation the sky backend starts a storm from.
const OVERCAST_PRECIPITATION_START:float = 1.0
const OVERCAST_PRECIPITATION_MEDIUM:float = 1.35
const PRECIPITATION_LIGHT:float = 0.2
const PRECIPITATION_MEDIUM:float = 0.4
## Original engine: clamp of Global.fFogEnd (simulationstateserializer.cpp:216)
## The scenario clock of a scenery without a "time" section, 10:30 (scenario_time(),
## simulationtime.h:21 - Time.init() takes it after the load, simulationtime.cpp:30-51)
const START_TIME_DEFAULT:float = 10.5
const FOG_END_MIN:float = 10.0
const FOG_END_MAX:float = 25000.0

## vehicle_id of the first vehicle found in the loaded scenery
var first_vehicle_id:String = ""
## "time" section - scenario clock, hours; START_TIME_DEFAULT without one
var start_time:float = START_TIME_DEFAULT
## What else the loaded scenery declares about its environment; each *_defined says whether it does
var day_of_year_defined:bool = false
## "config movelight" - day of the year, 0 or less for today's date (simulationtime.cpp:45)
var day_of_year:int = 0
var temperature_defined:bool = false
## "config scenario.weather.temperature" - air temperature
var temperature:float = 0.0
var overcast_defined:bool = false
## "atmo" overcast - 0-1 cloud cover, above 1 precipitation (simulationenvironment.cpp:62)
var overcast:float = 0.0
var fog_end_defined:bool = false
## "atmo" fog range - the original's Global.fFogEnd, drawn from the declared range once per load
## like the original does (simulationstateserializer.cpp:216)
var fog_end:float = 0.0

## Initial loading (autoload) is deferred to the first _process.
func _ready() -> void:
    _dirty = autoload


func _enter_tree() -> void:
    GameDataServer.data_reload_requested.connect(_on_data_reload_requested)
    VehicleServer.vehicle_freed.connect(_on_vehicle_freed)
    # the editor keeps the environment saved with the edited scene (_load_content())
    if not Engine.is_editor_hint():
        ProjectSettings.settings_changed.connect(_apply_fog)


## A vehicle of the scenery freed by somebody else (the HUD removes a trainset) is not the
## scenery's to free any more
func _on_vehicle_freed(vehicle:RID) -> void:
    _vehicle_rids.erase(vehicle)


## A loaded scenery is the game's data - its tracks, events, drivers and vehicles are loaded again,
## from the data read again, once the load under way (if any) is done
func _on_data_reload_requested() -> void:
    if _loaded or _loading:
        _dirty = true


func _exit_tree() -> void:
    GameDataServer.data_reload_requested.disconnect(_on_data_reload_requested)
    VehicleServer.vehicle_freed.disconnect(_on_vehicle_freed)
    if not Engine.is_editor_hint():
        ProjectSettings.settings_changed.disconnect(_apply_fog)
    # a load awaits frames of the tree it has just left
    if _loading:
        SceneryServer.scenery_cancel()


## Out of the tree the content stays, out of the world: the editor takes a scene out of the tree
## when another one's tab is shown, and all of them draw into one world. It goes with the node.
func _notification(what:int) -> void:
    match what:
        NOTIFICATION_ENTER_WORLD, NOTIFICATION_EXIT_WORLD:
            var scenario:RID = get_world_3d().scenario if what == NOTIFICATION_ENTER_WORLD else RID()
            for rid:RID in _track_render_rids:
                TrackRenderingServer.track_set_scenario(rid, scenario)
            for rid:RID in _traction_rids:
                TractionRenderingServer.set_traction_scenario(rid, scenario)
            for rid:RID in _model_rids:
                ModelRenderingServer.instance_set_scenario(rid, scenario)
            for owned:Array in _provider_owned:
                if not (owned[2] as Callable).is_valid():
                    continue
                for rid:RID in owned[0] as Array[RID]:
                    (owned[2] as Callable).call(rid, scenario)
            for rid:RID in _mesh_rids:
                SceneryMeshRenderingServer.mesh_set_scenario(rid, scenario)
            for rid:RID in _provider_rids:
                SceneryStreamingServer.provider_set_scenario(rid, scenario)
            # a vehicle still waiting for its build is not drawn yet
            for rid:RID in _vehicle_rids:
                if RailVehicleRenderingServer.vehicle_is_attached(rid):
                    RailVehicleRenderingServer.vehicle_set_scenario(rid, scenario)
                elif RoadVehicleRenderingServer.vehicle_is_attached(rid):
                    RoadVehicleRenderingServer.vehicle_set_scenario(rid, scenario)
        NOTIFICATION_PREDELETE:
            unloading.emit()
            # The planning thread calls back into GDScript (the owner's preload) and can be
            # creating rendering resources for the very RIDs freed below. It is stopped and joined
            # here, while the scripts still exist - the server's own destructor runs long after
            # they are gone.
            SceneryServer.scenery_cancel()
            SceneryStreamingServer.streaming_drain()
            _free_owned_rids()


## The scenery's trainsets (RailVehicleServer), in the order of its file
func get_trainsets() -> Array[RID]:
    return _trainset_rids


## The scenery's vehicles, in the order of its file
func get_vehicles() -> Array[RID]:
    return _vehicle_rids


## The scenery's models (ModelRenderingServer instances)
func get_models() -> Array[RID]:
    return _model_rids


## The scenery's scenario scripts, run by its scenario
func get_scenario_scripts() -> Array[String]:
    return _scenario_scripts


## [param rids] of what the provider made of the content - kept by reference, so the provider may
## add to it as it builds: they are freed with the content by [param free] (rid), and drawn into the
## node's world by [param set_scenario] (rid, scenario) - none for what is not drawn (a road network)
func content_own(rids:Array[RID], free:Callable, set_scenario:Callable) -> void:
    _provider_owned.append([rids, free, set_scenario])


## The road network its provider built of the content (RoadServer): freed with the content, all at
## once - each road freed alone looks over the links of every other one
func roads_own(roads:Array[RID]) -> void:
    _road_rids.append_array(roads)


## budget_msec > 0 spreads the freeing over frames, so whatever covers the screen (the loading
## spinner) keeps animating; 0 frees everything at once (the node freed)
func _free_owned_rids(budget_msec:int = 0) -> void:
    SceneryServer.scenery_release(self)
    # [the RIDs, freeing one, freeing them all at once (or none)]
    var groups:Array = [
        [_trainset_rids, RailVehicleServer.trainset_free, Callable()],
        [_vehicle_rids, VehicleCatalogServer.vehicle_free, Callable()],
        [_pickable_rids, SceneryHUDMouseServer.pickable_free, Callable()],
        [_event_rids, ScenarioEventServer.event_free, Callable()],
        [_event_system_rids, ScenarioEventServer.system_free, Callable()],
        [_isolated_rids, TrackServer.isolated_free, Callable()],
        # after the vehicles that drive on them, before the tracks they are laid along
        [_road_rids, Callable(), RoadServer.road_free_many],
        # the tracks all at once (both servers' track_free_many()): one by one, each emptied a
        # road cell rebuilt on the frames between and cleared every track's topology - a world of
        # tens of thousands took 99 s
        [_track_render_rids, Callable(), TrackRenderingServer.track_free_many],
        [_track_rids, Callable(), TrackServer.track_free_many],
        [_traction_rids, TractionRenderingServer.free_traction, Callable()],
        [_wire_power_rids, TractionServer.wire_free, Callable()],
        [_power_source_rids, TractionServer.power_source_free, Callable()],
        [_model_rids, ModelRenderingServer.instance_free, Callable()],
        # after the instances, whose signal heads leave the system as they go
        [_signalling_system_rids, SignallingServer.system_free, Callable()],
        [_provider_rids, SceneryStreamingServer.provider_free, Callable()],
        [_mesh_rids, SceneryMeshRenderingServer.mesh_free, Callable()],
    ]
    for owned:Array in _provider_owned:
        groups.insert(groups.size() - 1, [owned[0], owned[1], Callable()])
    var total:int = 0
    for group:Array in groups:
        total += (group[0] as Array[RID]).size()
    var freed:int = 0
    var frame_start:int = Time.get_ticks_msec()
    for group:Array in groups:
        var rids:Array[RID] = group[0]
        var free_rid:Callable = group[1]
        var free_all:Callable = group[2]
        if free_all.is_valid():
            var all:Array[RID] = rids.duplicate()
            rids.clear()
            free_all.call(all)
            freed += all.size()
            continue
        # Taken off the list before it is freed, not after the whole loop: the budgeted path
        # awaits a frame in the middle, and freeing the node during that await runs this again
        # when the node is freed over the very same RIDs - a double free.
        while rids.size() > 0:
            var rid:RID = rids.pop_back()
            if rid.is_valid():
                free_rid.call(rid)
            freed += 1
            if budget_msec > 0 and Time.get_ticks_msec() - frame_start >= budget_msec:
                clear_progress.emit(float(freed) / float(total))
                await get_tree().process_frame
                frame_start = Time.get_ticks_msec()
    _provider_owned.clear()


## Nothing in here is worth simulating while it is being torn down, and a real scenery is
## hundreds of vehicles with their cabins and sounds, all running their own _process for the
## seconds the freeing takes. RailVehicleServer steps those vehicles from its own registry,
## outside this subtree, so disabling the subtree alone leaves the heaviest part running until the
## last vehicle is freed - it is stopped here too and restored once the content is gone.
func _clear_content(budget_msec:int = 0) -> void:
    unloading.emit()
    _scenario_scripts.clear()
    _scenery_radio = null
    process_mode = Node.PROCESS_MODE_DISABLED
    VehicleServer.stepping_set_enabled(false)
    # Streaming builds content on process_frame, and the freeing below yields a frame for its
    # budget - without this it streams new content into the very RIDs being freed, which the
    # RenderingServer reports as "Initializing already initialized RID" and then aborts.
    SceneryStreamingServer.streaming_set_enabled(false)
    # the preloads in flight load the content freed below - they are stopped and joined first, as
    # when the node is freed (NOTIFICATION_PREDELETE)
    SceneryStreamingServer.streaming_drain()
    await _free_owned_rids(budget_msec)
    var frame_start:int = Time.get_ticks_msec()
    for child:Node in get_children(true):
        child.free()
        if budget_msec > 0 and Time.get_ticks_msec() - frame_start >= budget_msec:
            await get_tree().process_frame
            frame_start = Time.get_ticks_msec()
    first_vehicle_id = ""
    start_time = START_TIME_DEFAULT
    day_of_year_defined = false
    temperature_defined = false
    overcast_defined = false
    fog_end_defined = false
    SceneryStreamingServer.streaming_set_enabled(true)
    VehicleServer.stepping_set_enabled(true)
    process_mode = Node.PROCESS_MODE_INHERIT
    cleared.emit()


## Loads what the node is set to. Asked for while a load is under way, it stops that one
## (stop_loading()) and the scenery is loaded again once it has stopped.
func load() -> void:
    if _loading:
        stop_loading()
        _dirty = true
        return

    _dirty = false
    _loading = true
    _load_given_up = false
    await _clear_content(CLEAR_BUDGET_MSEC)
    _loaded = false
    if _has_source():
        await _load_content()
        _loaded = not _load_given_up
        # half a scenery is none: what the load given up had built goes
        if _load_given_up:
            await _clear_content(CLEAR_BUDGET_MSEC)
    _loading = false
    load_ended.emit()
    if _loaded:
        loaded.emit()


## Frees what is loaded; the scenery stays set to its file. A load under way is stopped instead
## (stop_loading()), which frees what it had built.
func clear() -> void:
    if _loading:
        stop_loading()
        return
    _loading = true
    await _clear_content(CLEAR_BUDGET_MSEC)
    _loaded = false
    _loading = false


## Gives the load under way up: it stops at the scenery provider's next step, and what it had
## built is freed. Nothing happens while nothing loads.
func stop_loading() -> void:
    if _loading:
        _load_given_up = true
        SceneryServer.scenery_cancel()


## Whether the load under way was given up (stop_loading())
func is_load_given_up() -> bool:
    return _load_given_up


## The files of the game's data its last load could not find, by what their absence does: none
## while nothing is loaded. A provider's node answers it from what its readers gave up on; the
## game tells the player (#14)
func get_missing_files() -> Dictionary[String, MissingFileEffect]:
    var missing:Dictionary[String, MissingFileEffect] = {}
    return missing


## The scenery's name: what its signalling system is known by (SignallingServer.system_get_rid_by_name(),
## a scenario's Lua)
func get_scenery_name() -> String:
    return ""


## Whether there is anything to load (a file, sectors)
func _has_source() -> bool:
    return false


## Builds the content with the servers, from what the subclass holds
func _instantiate_content() -> void:
    pass


func _load_content() -> void:
    await _instantiate_content()
    # the load was given up: nothing of it is announced
    if is_load_given_up():
        return
    first_vehicle_id = _find_driver_vehicle_id()
    _read_environment_declarations()
    # a scenery loaded in the editor must not rewrite the environment saved with the edited scene
    if not Engine.is_editor_hint():
        _apply_environment_declarations()
    scenery_loaded.emit(first_vehicle_id)


## The scenario's declarations of its environment (start_time, the *_defined values), read once the
## content is built; the subclass's, from where its scenario declares them
func _read_environment_declarations() -> void:
    pass


func _process(delta: float) -> void:
    if _dirty and not _loading:
        _dirty = false
        _process_dirty(delta)
    if _editor_dirty:
        _editor_dirty = false
        if Engine.is_editor_hint():
            SceneryEditor.update_owners(self)

func _process_dirty(_delta: float) -> void:
    self.load()


func _apply_environment_declarations() -> void:
    var environment_node:EnvironmentNode = get_node_or_null(environment_node_path) as EnvironmentNode
    if not environment_node:
        return
    environment_node.use_system_time = false
    environment_node.current_time = start_time
    if day_of_year_defined:
        var date:Dictionary = (
            Time.get_date_dict_from_system() if day_of_year <= 0
            else _date_from_day_of_year(day_of_year, environment_node.year))
        environment_node.use_system_time = false
        environment_node.day = date["day"]
        environment_node.month = date["month"]
    if temperature_defined:
        environment_node.temperature = temperature
    _apply_fog()
    if overcast_defined:
        environment_node.cloudiness = clampf(overcast, 0.0, 1.0)
        environment_node.precipitation = _precipitation_from_overcast(overcast)


## The scenery's fog, with FOG_SCENERY_DISTANCE_FACTOR_SETTING as it is now - applied again when the
## setting changes while the scenery stands
func _apply_fog() -> void:
    var environment_node:EnvironmentNode = get_node_or_null(environment_node_path) as EnvironmentNode
    if not fog_end_defined or not environment_node:
        return
    # Original engine: the fog has no density of its own, its range is
    # fFogEnd / max(1, Overcast * 2) (opengl33renderer.cpp:4685); see FOG_CURVE_SETTING for how
    # that range becomes the distance of a complete fog
    var fog_range:float = fog_end / maxf(1.0, overcast * 2.0 if overcast_defined else 0.0)
    environment_node.fog_distance = fog_range * float(ProjectSettings.get_setting(
        SkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_SETTING,
        SkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_DEFAULT))
    environment_node.fog_density = 1.0


func _precipitation_from_overcast(value:float) -> float:
    if value <= OVERCAST_PRECIPITATION_START:
        return 0.0
    if value < OVERCAST_PRECIPITATION_MEDIUM:
        return PRECIPITATION_LIGHT
    return PRECIPITATION_MEDIUM


func _date_from_day_of_year(year_day:int, year:int) -> Dictionary:
    var new_year:int = Time.get_unix_time_from_datetime_dict({"year": year, "month": 1, "day": 1})
    return Time.get_date_dict_from_unix_time(new_year + (year_day - 1) * Units.SECONDS_PER_DAY)


## The player belongs in a vehicle with a driver, not in whatever vehicle the scenery declares
## first (VehicleData.driver_type)
func _find_driver_vehicle_id() -> String:
    var reverse_driver_vehicle_id:String = ""
    for vehicle:RID in get_vehicles():
        var data:VehicleData = VehicleCatalogServer.vehicle_get_data(vehicle)
        if data.driver_type == VehicleData.DriverType.DRIVER_HEAD:
            return data.vehicle_id
        if data.driver_type == VehicleData.DriverType.DRIVER_REAR and not reverse_driver_vehicle_id:
            reverse_driver_vehicle_id = data.vehicle_id
    if reverse_driver_vehicle_id:
        return reverse_driver_vehicle_id
    return VehicleCatalogServer.vehicle_get_data(get_vehicles()[0]).vehicle_id if get_vehicles() else ""
