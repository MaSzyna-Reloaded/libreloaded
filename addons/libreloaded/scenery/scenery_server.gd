@tool
extends Node

## The sceneries: the providers' list of them (scenery_list()), and a scenery loaded into a
## SceneryContentNode (scenery_load()) - read by its provider into a CompiledScenery and registered
## with the servers here: its tracks, traction, power sources, lines and nodes; what only the provider
## knows it registers itself (SceneryProvider.instantiate_content(), instantiate_vehicles()).
##
## Tracks, traction and models are registered with their servers' RID-based API instead of
## TrackNormal3D/Traction3D/model nodes - a real scenery has thousands of them, and a Node per piece is
## overhead only a handful of hand-authored pieces earn. root keeps the RIDs, so they are freed on the
## next load or when it leaves the tree. What the servers get is a registration, not geometry:
## SceneryStreamingServer builds each piece only while the camera is within its range.

## Share of the loading progress taken by reading the scenery (SceneryProvider.compile())
const PARSE_PROGRESS:float = 0.5
## Where the provider's content starts (SceneryProvider.instantiate_content())
const CONTENT_PROGRESS:float = 0.55
## Where the scenery's nodes start, and where its vehicles do (SceneryProvider.instantiate_vehicles())
const OBJECTS_PROGRESS:float = 0.7
const VEHICLES_PROGRESS:float = 0.9
## Time the main thread may work through a load before it lets a frame be drawn - the loading
## screen's animations move only between frames (it was 100 ms: 10 frames a second at best)
const FRAME_BUDGET_MSEC:int = 12

static var _last_report_msec:int = 0
## Queues working for a load, so a scenery leaving the tree can stop them (scenery_cancel()) - their
## tasks are GDScript and call GDScript handlers, which must not be reached once the scripts go away
static var _active_queues:Array[WorkerTaskQueue] = []
## The registered providers' ObjectIDs, by name
var _providers:Dictionary[StringName, int] = {}
## The stations of each scenery loaded, by its node's ObjectID - kept from its CompiledScenery, which
## goes with the load
var _stations:Dictionary[int, Array] = {}

## The stations of the sceneries loaded changed (station_list())
signal station_list_changed


## A region file's lines are drawn as the scenery's own (SceneryLines), a world's geometry with the
## game's materials (WorldMeshPlacement)
func _ready() -> void:
    SceneryStreamingServer.content_set_consumer(
        SceneryStreamingProvider.CONTENT_LINES, SceneryLines.create_mesh, SceneryMeshRenderingServer.mesh_free
    )
    SceneryStreamingServer.content_set_consumer(
        SceneryStreamingProvider.CONTENT_MESHES, WorldMeshPlacement.create_mesh, SceneryMeshRenderingServer.mesh_free
    )


## A SceneryProvider, by its ObjectID, lists its sceneries under [param provider_name]
func provider_register(provider_name:StringName, provider_id:int) -> void:
    _providers[provider_name] = provider_id


func provider_unregister(provider_name:StringName) -> void:
    _providers.erase(provider_name)


## The SceneryProvider registered as [param provider_name], by its ObjectID; 0 for none
func provider_get(provider_name:StringName) -> int:
    return _providers.get(provider_name, 0)


func provider_get_list() -> PackedStringArray:
    return PackedStringArray(_providers.keys())


## Every registered provider
func _get_providers() -> Array[SceneryProvider]:
    var providers:Array[SceneryProvider] = []
    for provider_id:int in _providers.values():
        providers.append(instance_from_id(provider_id) as SceneryProvider)
    return providers


## The sceneries of every provider
func scenery_list() -> Array[SceneryInfo]:
    var sceneries:Array[SceneryInfo] = []
    for provider:SceneryProvider in _get_providers():
        sceneries.append_array(provider.list())
    return sceneries


## Reads the scenery [param id] by [param provider] and populates [param root] with it
func scenery_load(root:SceneryContentNode, provider:SceneryProvider, id:String, parameters:Dictionary = {}) -> void:
    var measurement := SceneryLoadMeasurement.new(root)
    var compiled:CompiledScenery = await provider.compile(root, id, parameters)
    if not compiled:
        measurement.finish()
        return
    if is_load_given_up(root):
        return
    await instantiate_compiled(root, compiled, measurement, provider)


## The stations of every scenery loaded
func station_list() -> Array[StationData]:
    var stations:Array[StationData] = []
    for scenery_stations:Array in _stations.values():
        stations.append_array(scenery_stations)
    return stations


## What the server keeps of [param root]'s scenery goes with its content (SceneryContentNode frees
## it before every load and as it is freed): its stations
func scenery_release(root:SceneryContentNode) -> void:
    if _stations.erase(root.get_instance_id()):
        station_list_changed.emit()


## Stops every load in flight and joins its workers. Called by a scenery leaving the tree: the tasks
## are GDScript and a provider's parser calls GDScript handlers through a Callable, so a worker still
## inside one when the scripts are freed jumps into code that is gone (see FINDINGS.md, 2026-09-24).
## Waiting here is safe, because here the scripts are still alive.
func scenery_cancel() -> void:
    # a parse has to stop between tokens, or joining its worker means waiting out a whole file
    for provider:SceneryProvider in _get_providers():
        provider.set_parsing_cancelled(true)
    for queue:WorkerTaskQueue in _active_queues.duplicate():
        queue.drain()
    _active_queues.clear()
    # the workers are joined, so nothing is working and the next scenery may start clean
    for provider:SceneryProvider in _get_providers():
        provider.set_parsing_cancelled(false)


## A queue working for a load, joined by scenery_cancel()
static func queue_add(queue:WorkerTaskQueue) -> void:
    _active_queues.append(queue)


static func queue_remove(queue:WorkerTaskQueue) -> void:
    _active_queues.erase(queue)


## Registers what [param compiled] holds with the servers and populates root with it - the scenery
## read by [param provider] (scenery_load()), or composed by a node of its own (a plugin's world).
## [param measurement] is finished with it.
func instantiate_compiled(
    root:SceneryContentNode, compiled:CompiledScenery, measurement:SceneryLoadMeasurement, provider:SceneryProvider
) -> void:
    var world_3d:World3D = root.get_world_3d()
    await report_progress(root, PARSE_PROGRESS, SceneryContentNode.LoadStage.INFRASTRUCTURE, "Registering tracks and traction")
    var track_rids:Array[RID] = await _instantiate_infrastructure(root, world_3d, compiled)
    if is_load_given_up(root):
        return
    await provider.instantiate_content(root, compiled, track_rids)
    if is_load_given_up(root):
        return
    root._mesh_rids.append_array(SceneryLines.create_meshes(compiled.lines, world_3d.scenario))
    await report_progress(root, OBJECTS_PROGRESS, SceneryContentNode.LoadStage.OBJECTS, "Instancing objects")
    await _attach_objects(root, instantiate_cached_nodes(compiled.nodes), OBJECTS_PROGRESS, VEHICLES_PROGRESS)
    if is_load_given_up(root):
        return
    await provider.instantiate_vehicles(root, compiled)
    if is_load_given_up(root):
        return
    if Engine.is_editor_hint():
        root.SceneryEditor.update_owners(root)
    root._scenario_scripts.assign(compiled.scripts)
    _stations[root.get_instance_id()] = compiled.stations.duplicate()
    station_list_changed.emit()
    measurement.finish()
    SceneryLoadMeasurement.print_scenery(compiled)
    # what the load read and built from is let go of with the compiled scenery, and given back
    compiled = null
    ProcessMemory.release_unused()
    SceneryLoadMeasurement.print_process("SceneryMemory", "loaded")


## The trainsets and their vehicles, through the servers - no node for either
## (VehicleCatalogServer, RailVehicleServer): every vehicle is built, then each trainset stands its
## vehicles on its track and couples them (trainset_place(), "endtrainset"), and only then the
## drivers are given their orders - what a driver does first is sent along the couplers
## (simulationstateserializer.cpp:818-848). The drivers think with [param driver_implementation]
## (DriverServer.implementation_register(); empty for none), and [param trainset_placed]
## (trainset_data, driver), when given, gives the driver of a trainset standing its orders. Every
## track is registered by now.
static func instantiate_trainsets(
    root:SceneryContentNode, trainsets:Array[TrainsetData], driver_implementation:StringName,
    trainset_placed:Callable = Callable()
) -> void:
    await report_progress(root, VEHICLES_PROGRESS, SceneryContentNode.LoadStage.VEHICLES, "Instancing vehicles")
    # A trainset on a track that is not built - a road car of a scenery whose roads are not - could
    # never be placed, and still cost whole vehicles: their physics stepped, their drivers and cab
    # logic run (docs/findings-archive.md, 2026-10-03 hundreds of vehicles)
    var placed:Array[TrainsetData] = []
    var tracks:Array[RID] = []
    var vehicles:Array[Array] = []
    # the vehicles of each placed trainset, as this load arranges them (trainset_override)
    var vehicle_data:Array[Array] = []
    for trainset_data:TrainsetData in trainsets:
        var track:RID = TrackServer.track_get_rid_by_name(trainset_data.track_name)
        if not track.is_valid():
            continue
        var trainset_vehicles:Array[RID] = []
        var trainset_vehicle_data:Array[VehicleData] = trainset_data.get_arranged_vehicles(root.trainset_override)
        for data:VehicleData in trainset_vehicle_data:
            var vehicle:RID = VehicleCatalogServer.vehicle_create(data, root.get_instance_id())
            root._vehicle_rids.append(vehicle)
            trainset_vehicles.append(vehicle)
        placed.append(trainset_data)
        tracks.append(track)
        vehicles.append(trainset_vehicles)
        vehicle_data.append(trainset_vehicle_data)
    if placed.size() < trainsets.size():
        print("[SceneryLoad] %d trainsets on tracks that are not built (roads) left out" % (trainsets.size() - placed.size()))

    var vehicle_count:int = 0
    for trainset_vehicles:Array[RID] in vehicles:
        vehicle_count += trainset_vehicles.size()
    var built_count:int = 0
    for index:int in placed.size():
        var trainset_data:TrainsetData = placed[index]
        var trainset_vehicles:Array[RID] = vehicles[index]
        for vehicle:RID in trainset_vehicles:
            # reported before the wait: the trainset stands, and its driver is given its orders, in
            # the frame its last vehicle is built
            await report_progress_throttled(
                    root, lerpf(VEHICLES_PROGRESS, 1.0, float(built_count) / vehicle_count),
                    SceneryContentNode.LoadStage.VEHICLES, "Instancing vehicles")
            if is_load_given_up(root):
                return
            while not VehicleServer.vehicle_is_built(vehicle):
                await VehicleServer.vehicle_built
                if is_load_given_up(root):
                    return
            built_count += 1
        var trainset:RID = RailVehicleServer.trainset_create()
        root._trainset_rids.append(trainset)
        RailVehicleServer.trainset_set_name(trainset, trainset_data.name)
        RailVehicleServer.trainset_set_description(trainset, trainset_data.description)
        RailVehicleServer.trainset_set_player_drivable(trainset, trainset_data.player_drivable)
        RailVehicleServer.trainset_set_track(trainset, tracks[index], trainset_data.offset)
        # a vehicle that failed to load has no simulation, and the trainset stands without it
        var members:Array[int] = []
        for member:int in trainset_vehicles.size():
            if not VehicleServer.vehicle_is_simulation_ready(trainset_vehicles[member]):
                continue
            var data:VehicleData = vehicle_data[index][member]
            RailVehicleServer.trainset_add_vehicle(
                    trainset, trainset_vehicles[member], data.direction, data.gap, data.coupling)
            members.append(member)
        RailVehicleServer.trainset_place(trainset)
        # the person at the controls of a vehicle just built - the one its provider seated there
        # (VehicleData.driver_type); the player takes a vehicle over only once the scenery is
        # loaded (player.gd, _on_scenery_loaded()) - thinks with the implementation given. It drives
        # through the vehicle's cab logic, like the player, without the 3D cab
        var trainset_driver:RID = RID()
        for member:int in members:
            for driver:VehiclePerson in VehicleServer.vehicle_list_persons(
                    trainset_vehicles[member], VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER):
                trainset_driver = driver.get_person()
                DriverServer.driver_attach_implementation(trainset_driver, driver_implementation)
        # the trainset's orders go to its driver - of several, the one furthest along
        if trainset_driver.is_valid() and trainset_placed.is_valid():
            trainset_placed.call(trainset_data, trainset_driver)
    root.load_progress.emit(1.0, SceneryContentNode.LoadStage.VEHICLES, "")


## The tracks, the power sources and the traction, registered with their servers; the tracks' RIDs
## in the order of the data, which is how a provider's events find what they are aimed at
static func _instantiate_infrastructure(
    root:SceneryContentNode, world_3d:World3D, compiled:CompiledScenery
) -> Array[RID]:
    var total:float = float(maxi(compiled.tracks.size() + compiled.power_sources.size() + compiled.traction.size(), 1))
    var built_count:int = 0
    var track_rids:Array[RID] = []
    for track_data:TrackData in compiled.tracks:
        var built:Dictionary = build_track(track_data, world_3d)
        root._track_rids.append(built["track_rid"])
        track_rids.append(built["track_rid"])
        root._track_render_rids.append(built["track_render_rid"])
        built_count += 1
        await report_progress_throttled(root, lerpf(PARSE_PROGRESS, CONTENT_PROGRESS, built_count / total), SceneryContentNode.LoadStage.INFRASTRUCTURE, "Registering tracks")
        if is_load_given_up(root):
            return track_rids
    for power_source_data:PowerSourceData in compiled.power_sources:
        root._power_source_rids.append(_build_power_source(power_source_data))
        built_count += 1
    for traction_data:TractionData in compiled.traction:
        var traction_rid:RID = _build_traction(traction_data, world_3d)
        root._traction_rids.append(traction_rid)
        root._wire_power_rids.append(_build_wire_power(traction_data))
        built_count += 1
        await report_progress_throttled(root, lerpf(PARSE_PROGRESS, CONTENT_PROGRESS, built_count / total), SceneryContentNode.LoadStage.INFRASTRUCTURE, "Registering traction")
        if is_load_given_up(root):
            return track_rids
    if root._wire_power_rids.size() > 0:
        TractionServer.network_build()
    if root._track_rids.size() > 0:
        TrackServer.topology_rebuild()
    return track_rids


## The load is not to go on: its scenery is gone - the editor frees a scene it reopens, mid-load -
## or was told to stop (SceneryContentNode.stop_loading()). Asked after every wait of a load.
## `root` is untyped: a parameter typed SceneryContentNode refuses a freed scenery at the call,
## before is_instance_valid() could say so.
static func is_load_given_up(root:Variant) -> bool:
    return not is_instance_valid(root) or root.is_load_given_up()


## Reports the next loading stage and lets a frame be drawn (e.g. a loading screen) before it runs.
## `message` is a msgid the loading screen's Label translates; one composed with a value is
## translated before the value goes in.
static func report_progress(
    root:SceneryContentNode, progress:float, stage:SceneryContentNode.LoadStage, message:String
) -> void:
    root.load_progress.emit(progress, stage, message)
    await root.get_tree().process_frame
    _last_report_msec = Time.get_ticks_msec()


## report_progress() for loops over many objects - reports only after FRAME_BUDGET_MSEC
static func report_progress_throttled(
    root:SceneryContentNode, progress:float, stage:SceneryContentNode.LoadStage, message:String
) -> void:
    if Time.get_ticks_msec() - _last_report_msec < FRAME_BUDGET_MSEC:
        return
    await report_progress(root, progress, stage, message)


## Lets a frame be drawn once the main thread has worked FRAME_BUDGET_MSEC since the last one, for
## the loops of a load that report nothing of their own
static func frame_budget_wait() -> void:
    if Time.get_ticks_msec() - _last_report_msec < FRAME_BUDGET_MSEC:
        return
    await (Engine.get_main_loop() as SceneTree).process_frame
    _last_report_msec = Time.get_ticks_msec()


## Adds objects to root, reporting the object being attached (see report_progress_throttled())
## with progress going from progress_from to progress_to.
static func _attach_objects(
    root:SceneryContentNode, objects:Array, progress_from:float, progress_to:float
) -> void:
    for i:int in objects.size():
        var node:Node = objects[i] as Node
        if not node:
            continue
        root.add_child(node)
        var progress:float = lerpf(progress_from, progress_to, float(i) / float(objects.size()))
        await report_progress_throttled(root, progress, SceneryContentNode.LoadStage.OBJECTS, TranslationServer.translate("Instancing %s") % node.name)
        if is_load_given_up(root):
            return


static func instantiate_cached_nodes(packed_scene:PackedScene) -> Array:
    var objects:Array = []
    if not packed_scene:
        return objects
    var scene_root:Node = packed_scene.instantiate()
    for child:Node in scene_root.get_children():
        scene_root.remove_child(child)
        child.owner = null
        objects.append(child)
    scene_root.free()
    return objects


## Runs `task` on a WorkerTaskQueue worker while the main thread keeps drawing frames, the
## progress reported; its result, null when the load is cancelled (scenery_cancel())
static func run_on_worker(
    root:SceneryContentNode, task:Callable, progress:float, stage:SceneryContentNode.LoadStage, message:String
) -> Variant:
    var queue := WorkerTaskQueue.new()
    queue_add(queue)
    var task_id:int = queue.submit(task)
    root.load_progress.emit(progress, stage, message)
    await queue.task_finished
    _last_report_msec = Time.get_ticks_msec()
    queue_remove(queue)
    return queue.wait(task_id)


## Mirrors TrackNormal3D/TrackSwitch3D's own _create_track()/_update_track_data()/
## _update_track_rendering() (addons/libreloaded/tracks/track_normal_3d.gd,
## track_switch_3d.gd), minus the Node - see instantiate()'s doc comment for why. The rendering
## meshes are not built here: track_stream() leaves that to SceneryStreamingServer.
static func build_track(track_data:TrackData, world_3d:World3D) -> Dictionary:
    var track_rid:RID = TrackServer.track_create()
    var track_render_rid:RID = TrackRenderingServer.track_create(track_rid)
    TrackRenderingServer.track_set_scenario(track_render_rid, world_3d.scenario)

    TrackServer.track_update_curves(track_rid, track_data.curve, track_data.diverging_curve)
    TrackServer.track_update(track_rid, track_data.kind, track_data.name, track_data.width)
    TrackServer.track_update_properties(track_rid, track_data.damage, track_data.friction, track_data.joint_spacing)
    TrackServer.track_set_velocity(track_rid, track_data.speed_limit)
    TrackServer.track_set_meta(track_rid, track_data.meta)

    TrackRenderingServer.track_set_description(track_render_rid, track_data)
    TrackRenderingServer.track_set_visible(track_render_rid, track_data.visible)
    TrackRenderingServer.track_stream(track_render_rid)

    return {"track_rid": track_rid, "track_render_rid": track_render_rid}


## A model of a scenery registered with ModelRenderingServer under [param model_name] - its
## ModelData.model, or what its provider makes of it - and drawn in [param world_3d], its lights as
## the data gives them (ModelData.lights_state, lights_colors)
static func build_model(model_data:ModelData, model_name:String, world_3d:World3D) -> RID:
    var model_rid:RID = ModelRenderingServer.instance_register(
        model_data.rendering_backend,
        model_name,
        model_data.skins,
        Transform3D(Basis.from_euler(model_data.rotation), model_data.position),
        model_data.range_min,
        model_data.range_max,
        world_3d.scenario,
    )
    if model_rid.is_valid() and model_data.lights_state:
        ModelRenderingServer.instance_set_lights_state(model_rid, model_data.lights_state)
    if model_rid.is_valid() and model_data.lights_colors:
        ModelRenderingServer.instance_set_lights_colors(model_rid, model_data.lights_colors)
    return model_rid


## The signal head of a model of a signal_head_kind, under the model's name: the instance
## [param model_rid] draws its lights, and frees it as it goes. RID() for a model of no kind
static func build_signal_head(model_rid:RID, model_data:ModelData) -> RID:
    if not model_data.signal_head_kind:
        return RID()
    var signal_head:RID = SignallingServer.signal_head_create()
    ModelRenderingServer.instance_attach_signal_head(model_rid, signal_head)
    SignallingServer.signal_head_set_kind(signal_head, model_data.signal_head_kind)
    SignallingServer.signal_head_set_name(signal_head, model_data.name)
    return signal_head


## Mirrors Traction3D's own _enter_tree()/_update() (addons/libreloaded/traction/
## traction_3d.gd), minus the Node. contact_p1/p2/support_p1/p2 are already absolute
## world coordinates read straight from the .scn (same as track curve points), so the traction's
## own transform is identity - there's nothing local left to place.
static func _build_traction(traction_data:TractionData, world_3d:World3D) -> RID:
    var traction_rid:RID = TractionRenderingServer.create_traction()
    TractionRenderingServer.set_traction_description(traction_rid, traction_data)
    TractionRenderingServer.set_traction_transform(traction_rid, Transform3D.IDENTITY)
    TractionRenderingServer.set_traction_visible(traction_rid, traction_data.visible)
    TractionRenderingServer.set_traction_scenario(traction_rid, world_3d.scenario)
    TractionRenderingServer.stream_traction(traction_rid)
    return traction_rid


## Mirrors _build_traction() - a tractionpowersource node has no visual representation, so this
## only ever registers electrical data against TractionServer.
static func _build_power_source(power_source_data:PowerSourceData) -> RID:
    var power_source_rid:RID = TractionServer.power_source_create()
    TractionServer.power_source_set_params(
        power_source_rid,
        power_source_data.name,
        power_source_data.nominal_voltage,
        power_source_data.voltage_frequency,
        power_source_data.internal_resistance,
        power_source_data.max_output_current,
        power_source_data.fast_fuse_timeout,
        power_source_data.fast_fuse_repetition,
        power_source_data.slow_fuse_timeout,
        power_source_data.recuperation,
        false,
        power_source_data.is_section,
    )
    return power_source_rid


## Registers a traction wire's electrical data (as opposed to _build_traction()'s visual mesh)
## against TractionServer - a second, purely-electrical RID for the same wire span.
static func _build_wire_power(traction_data:TractionData) -> RID:
    var wire_rid:RID = TractionServer.wire_create()
    TractionServer.wire_set_params(
        wire_rid,
        traction_data.contact_p1,
        traction_data.contact_p2,
        traction_data.power_supply_name,
        traction_data.nominal_voltage,
        traction_data.max_current,
        traction_data.resistivity,
    )
    # the span this one shares its running with, which a pantograph cannot reach along the chain
    TractionServer.wire_set_parallel(wire_rid, traction_data.parallel)
    return wire_rid
