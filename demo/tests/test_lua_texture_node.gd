extends ReloadedGutTest

## LuaNode, LuaTextureNode, LuaVehicleTextureNode and LuaCabinTextureNode: the script runs while the
## node is in the tree, with what the node hands it. A script's assert() failing is a script error the test watches for.

const FIXTURES:String = "res://tests/fixtures/drawing"
const TEXTURE_SIZE:Vector2i = Vector2i(16, 8)
const DRAWING_SIZE:Vector2i = Vector2i(8, 4)
const ADDED_NUMBER:int = 42
## Real seconds a clock timer of 0.01 s is given to tick [s]
const CLOCK_WAIT:float = 0.2


class RecordingCabin extends ScenarioScriptCabinImplementation:
    var reads:Array = []

    func _get_control(cabin:RID, control_id:StringName) -> Variant:
        reads.append([cabin, control_id])
        return null


## What a GDScript subclass adds to the script's arguments
class ArgumentsNode extends LuaTextureNode:
    func _get_script_arguments() -> Array:
        return [ADDED_NUMBER, "added"]


var _vehicles:Array[RID] = []


func after_each() -> void:
    for vehicle:RID in _vehicles:
        VehicleServer.vehicle_free(vehicle)
    _vehicles.clear()


func _texture_node(node:LuaTextureNode, script:String) -> LuaTextureNode:
    var texture:DrawingTexture = DrawingTexture.new()
    texture.texture_size = TEXTURE_SIZE
    node.texture = texture
    node.script_path = FIXTURES.path_join(script)
    return node


func test_the_node_runs_its_script_in_the_tree() -> void:
    var node:LuaNode = LuaNode.new()
    node.script_path = FIXTURES.path_join("failing_node.lua")
    watch_signals(ScenarioScriptServer)

    add_child_autofree(node)
    assert_signal_emitted(ScenarioScriptServer, "script_error", "the script ran as the node entered the tree")


func test_the_script_gets_its_drawing_and_frees_nothing_of_it() -> void:
    var node:LuaTextureNode = _texture_node(LuaTextureNode.new(), "texture_node.lua")
    var rid:RID = node.texture.get_rid()
    watch_signals(ScenarioScriptServer)

    add_child_autofree(node)
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")
    assert_eq(node.texture.get_rid(), rid, "the texture keeps its RID with a drawing attached")
    var contexts:int = ScenarioScriptServer.context_get_rids().size()
    remove_child(node)
    assert_eq(ScenarioScriptServer.context_get_rids().size(), contexts - 1, "the context goes with the node")
    assert_eq(node.texture.get_rid(), rid, "and without one")


func test_with_a_material_the_drawing_has_its_own_size() -> void:
    var node:LuaTextureNode = _texture_node(LuaTextureNode.new(), "material_node.lua")
    node.material = ShaderMaterial.new()
    node.drawing_size = DRAWING_SIZE
    watch_signals(ScenarioScriptServer)

    add_child_autofree(node)
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")


func test_a_subclass_adds_to_the_arguments() -> void:
    var node:LuaTextureNode = _texture_node(ArgumentsNode.new(), "arguments_node.lua")
    watch_signals(ScenarioScriptServer)

    add_child_autofree(node)
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")


func test_a_vehicles_texture_gets_the_vehicle_after_the_drawing() -> void:
    var vehicle:RID = VehicleServer.vehicle_create()
    _vehicles.append(vehicle)
    VehicleServer.vehicle_set_id(vehicle, "signed")
    var node:LuaVehicleTextureNode = _texture_node(LuaVehicleTextureNode.new(), "")
    node.script_path = ""
    node.source = (
        "local drawing, vehicle = ...\n"
        + "assert(reloaded.vehicle.id(vehicle) == 'signed', 'the vehicle follows the drawing')\n"
        + "assert(reloaded.station.run(vehicle) == nil, 'a vehicle in no trainset runs no timetable')\n")
    node.set_vehicle(vehicle)
    watch_signals(ScenarioScriptServer)

    add_child_autofree(node)
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")


func test_a_script_finds_a_station_by_its_name() -> void:
    var station:RID = StationServer.station_create()
    StationServer.station_set_name(station, "Olkusz")
    var context:RID = ScenarioScriptServer.context_create(FIXTURES)
    watch_signals(ScenarioScriptServer)

    assert_true(ScenarioScriptServer.context_apply_source(context, &"station", (
        "local olkusz = reloaded.station.find('Olkusz')\n"
        + "assert(reloaded.station.name(olkusz) == 'Olkusz')\n"
        + "assert(reloaded.station.find('Nowhere') == nil)\n")))
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")
    ScenarioScriptServer.context_free(context)
    StationServer.station_free(station)


func test_a_cab_screen_gets_the_vehicle_and_the_cabin_of_its_cab() -> void:
    var vehicle:RID = VehicleServer.vehicle_create()
    _vehicles.append(vehicle)
    var cabin:RID = VehicleServer.cabin_create()
    VehicleServer.vehicle_cabin_attach(vehicle, cabin)
    var cab:Cabin3D = Cabin3D.new()
    cab.set_cabin(cabin)
    var recording:RecordingCabin = RecordingCabin.new()
    var node:LuaCabinTextureNode = _texture_node(LuaCabinTextureNode.new(), "cabin_node.lua")
    node.cabin_implementation = recording
    cab.add_child(node)
    watch_signals(ScenarioScriptServer)

    add_child_autofree(cab)
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")
    assert_eq(recording.reads, [[cabin, &"screen"]], "the script reached its cab's controls")


## reloaded.time follows the clock on the wall: it runs while the simulation is paused, and stops
## when cancelled
func test_a_clock_timer_runs_while_the_simulation_is_paused() -> void:
    var context:RID = ScenarioScriptServer.context_create(FIXTURES)
    watch_signals(ScenarioScriptServer)
    SimulationServer.simulation_pause()
    assert_true(ScenarioScriptServer.context_apply_source(
            context, &"clock", "ticks = 0\ntimer = reloaded.time.every(0.01, function() ticks = ticks + 1 end)"))

    await wait_seconds(CLOCK_WAIT)
    SimulationServer.simulation_unpause()
    assert_true(ScenarioScriptServer.context_apply_source(
            context, &"check", "assert(ticks > 0, 'it ticked while paused')\nassert(reloaded.cancel(timer))"))
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")
    ScenarioScriptServer.context_free(context)


func test_the_node_runs_code_of_its_own() -> void:
    var node:LuaTextureNode = _texture_node(LuaTextureNode.new(), "")
    node.script_path = ""
    node.source = "local drawing = ...\nlocal width = reloaded.draw.size(drawing)\nassert(width == 16)"
    watch_signals(ScenarioScriptServer)

    add_child_autofree(node)
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")


func test_a_changed_script_runs_again() -> void:
    var node:LuaTextureNode = _texture_node(LuaTextureNode.new(), "texture_node.lua")
    add_child_autofree(node)
    watch_signals(ScenarioScriptServer)

    node.script_path = FIXTURES.path_join("failing_node.lua")
    assert_signal_emitted(ScenarioScriptServer, "script_error", "the new script ran")


## A context runs its code again in a runtime of its own: a module it required is read again
func test_a_context_runs_its_files_again() -> void:
    var context:RID = ScenarioScriptServer.context_create(FIXTURES)
    watch_signals(ScenarioScriptServer)
    assert_true(ScenarioScriptServer.context_run_file(context, "reload_unit.lua"))
    var files:PackedStringArray = ScenarioScriptServer.context_get_files(context)
    assert_eq(files, PackedStringArray([
            FIXTURES.path_join("reload_unit.lua"), FIXTURES.path_join("lib/reload_module.lua")]))

    ScenarioScriptServer.context_reload(context)
    assert_signal_emitted_with_parameters(ScenarioScriptServer, "context_reloaded", [context])
    assert_true(ScenarioScriptServer.context_apply_source(context, &"check", "assert(runs == 1, 'a runtime anew')"))
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")
    ScenarioScriptServer.context_free(context)


## reloaded.export_var(): the value the owner set, the default for none or for a value of another type
func test_a_script_declares_its_variables() -> void:
    var context:RID = ScenarioScriptServer.context_create(FIXTURES)
    var variables:Dictionary = {"text": "set", "count": "not a number"}
    ScenarioScriptServer.context_set_variables(context, variables)
    watch_signals(ScenarioScriptServer)
    assert_true(ScenarioScriptServer.context_apply_source(context, &"variables", (
            "assert(reloaded.export_var('text', 'default') == 'set')\n"
            + "assert(reloaded.export_var('count', 3) == 3, 'another type: the default')\n"
            + "local colour = reloaded.export_var('colour', {255, 0, 0})\n"
            + "assert(colour[1] == 255 and colour[2] == 0 and colour[4] == 255)")))

    var exports:Dictionary = ScenarioScriptServer.context_get_exports(context)
    assert_eq(exports["text"], "default")
    assert_eq(exports["count"], 3)
    assert_eq(exports["colour"], Color(1, 0, 0, 1))
    assert_signal_not_emitted(ScenarioScriptServer, "script_error")
    ScenarioScriptServer.context_free(context)


## The node's properties are its script's variables; setting one runs the script again with it
func test_the_node_shows_its_script_variables() -> void:
    var node:LuaTextureNode = _texture_node(LuaTextureNode.new(), "")
    node.script_path = ""
    node.source = "assert(reloaded.export_var('label', 'default') ~= 'wrong', 'the variable as set')"
    add_child_autofree(node)
    var names:Array[String] = []
    for property:Dictionary in node.get_property_list():
        names.append(property["name"])
    assert_has(names, "script_variables/label")
    assert_eq(node.get("script_variables/label"), "default")
    watch_signals(ScenarioScriptServer)

    node.set("script_variables/label", "wrong")
    assert_signal_emitted(ScenarioScriptServer, "script_error", "the script ran again with it")
    assert_eq(node.script_variables, {"label": "wrong"})
