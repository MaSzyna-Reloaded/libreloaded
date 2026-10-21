extends GutTest
class_name ReloadedGutTest

## What every test of the core may use: the simulation stepped tick by tick and waited on, a track to
## stand on, a driver's implementation. A plugin's tests extend it with what its own things need.

## ResourceCache's switch: simulation_runtime_hook.gd turns it off, so a test never reads what a
## previous run compiled; a test of the cache itself turns it on for its duration
const CACHE_ENABLED_SETTING:String = "reloaded/cache/enabled"
## Standard gauge of build_track() [m]
const BUILT_TRACK_GAUGE:float = 1.435
## Real seconds a scenery fixture may take to load: parsing and building is the machine's work, not
## simulated time - a fixture loads in under 0.1 s, this only catches a hang
const LOAD_TIMEOUT:float = 5.0
## A few of the streaming's camera watch intervals (SceneryStreamingServer WATCH_INTERVAL_SEC 0.1 s)
const STREAMING_WATCH_SEC:float = 0.3
## One step of the test's clock [simulated s]: the simulation's tick to come (#301, 30 Hz). The run
## has no SimulationRuntime - no frame moves the simulation; a test steps it tick by tick, as a step
## debugger does, and what happens does not depend on how fast the machine is
const TICK:float = 1.0 / 30.0


## A script leaves the simulation as the hook set it: running, at speed 1, there already - a speed
## only set back (simulation_speed) leaves the clock at the old one, and every script after it runs
## at that until the speed change closes (FINDINGS.md); SimulationServer.simulation_reset_speed()
## sets both. Asserted here, so the script that leaves it is the one that goes red.
func after_all() -> void:
    assert_false(SimulationServer.simulation_is_paused(), "the script left the simulation paused")
    assert_eq(SimulationServer.simulation_speed, 1.0, "the script left the simulation's speed set")
    assert_eq(SimulationServer.simulation_get_current_speed(), 1.0,
            "the script left the simulation's clock at another speed")


## The ticks `seconds` of simulated time take, the last one whole
static func ticks(seconds:float) -> int:
    return ceili(seconds / TICK)


## `count` ticks of the simulation, one after the other - a frame between them for what runs on the
## frames (the keys, a knob a key holds)
func step(count:int) -> void:
    for _tick:int in count:
        SimulationServer.simulation_advance(TICK)
        await Engine.get_main_loop().process_frame


## Steps the simulation tick by tick until `done`, for at most `simulated_seconds` - worked out from
## the data of what is waited for. Not coming about by then fails the test there, saying `what`;
## true when it came
func wait_simulated_until(done:Callable, simulated_seconds:float, what:String) -> bool:
    for _tick:int in ticks(simulated_seconds):
        if done.call():
            return true
        SimulationServer.simulation_advance(TICK)
        await Engine.get_main_loop().process_frame
    if done.call():
        return true
    fail_test("%s did not come about within %.2f simulated seconds (%d ticks)" % [
            what, simulated_seconds, ticks(simulated_seconds)])
    return false


## The loading that `loaded` announces is done; one that does not end within LOAD_TIMEOUT fails the
## test there, saying `what`
func wait_loaded(loaded:Signal, what:String) -> bool:
    if await wait_for_signal(loaded, LOAD_TIMEOUT):
        return true
    fail_test("%s did not load within %.0f s" % [what, LOAD_TIMEOUT])
    return false


func wait_idle_frames(frames, message = ""):
    while frames > 0:
        await Engine.get_main_loop().process_frame
        frames -= 1


## The streaming looks at its camera, anchor and content every 0.1 s of real time
## (SceneryStreamingServer WATCH_INTERVAL_SEC) and applies what it planned per frame: a change of
## them is waited for by both
func wait_streaming(frames:int) -> void:
    await wait_seconds(STREAMING_WATCH_SEC)
    await wait_idle_frames(frames)


## The person becomes a driver thinking with `implementation` (null: no driver any more): it is
## registered with DriverServer under a name of its own, which the driver is declared with
func attach_driver_implementation(driver:RID, implementation:DriverImplementation) -> void:
    if not implementation:
        DriverServer.driver_attach_implementation(driver, &"")
        return
    var implementation_name:StringName = StringName("test_%d" % implementation.get_instance_id())
    DriverServer.implementation_register(implementation_name, implementation)
    DriverServer.driver_attach_implementation(driver, implementation_name)


## The person build_vehicle() seated as the DRIVER of the vehicle
func get_vehicle_driver(vehicle_rid:RID) -> RID:
    var drivers:Array[VehiclePerson] = VehicleServer.vehicle_list_persons(
            vehicle_rid, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    return drivers[0].get_person()


## A straight track along +X to stand vehicles on; the test frees it (TrackServer.track_free(), then
## topology_rebuild())
func build_track(track_name:String, length:float) -> RID:
    var curve:TrackCurve = TrackCurve.new()
    curve.p2 = Vector3(length, 0.0, 0.0)
    var track:RID = TrackServer.track_create()
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, TrackServer.TRACK_NORMAL, track_name, BUILT_TRACK_GAUGE)
    TrackServer.topology_rebuild()
    return track


## A vehicle a test stood on a track (a plugin's build_rail_vehicle()), gone: the node first - it lets go of its controller - then
## its physics node, which takes the vehicle out of RailVehicleServer (vehicle_freed)
func free_rail_vehicle(vehicle:RailVehicle3D) -> void:
    var physics_node:Node = vehicle.get_node(vehicle.controller_path)
    remove_child(vehicle)
    vehicle.free()
    physics_node.free()
