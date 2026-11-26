extends ReloadedGutTest

## VehicleAppearance.rendering_backend: a vehicle's models are loaded and drawn by the backend its
## appearance names - a Godot scene by SceneModelRenderingBackend - not by the project's default one

const BACKEND:StringName = &"scene"
const DATA_PATH:String = "res://tests/fixtures"
const MODEL:String = "scene_model.tscn"
## The low-poly interior's scene, and its root's name
const INTERIOR:String = "scene_interior.tscn"
const INTERIOR_ROOT:String = "Interior"

var _vehicle:RID = RID()


func after_each() -> void:
    if _vehicle.is_valid():
        RailVehicleRenderingServer.vehicle_detach(_vehicle)
        VehicleServer.vehicle_free(_vehicle)
    _vehicle = RID()


## An editable vehicle - its models built at once - standing at the origin, drawn at a node of the
## scene, in the appearance of the fixture's scene drawn by `backend`
func _vehicle_drawn(backend:StringName, low_poly_interior:String = "") -> RID:
    var node:Node3D = Node3D.new()
    add_child_autofree(node)
    var appearance:VehicleAppearance = VehicleAppearance.new()
    appearance.data_path = DATA_PATH
    appearance.model_filename = MODEL
    appearance.low_poly_model_filename = low_poly_interior
    appearance.rendering_backend = backend
    _vehicle = VehicleServer.vehicle_create()
    RailVehicleRenderingServer.vehicle_attach(_vehicle, node.get_instance_id())
    RailVehicleRenderingServer.vehicle_set_editable(_vehicle, true)
    RailVehicleRenderingServer.vehicle_set_appearance(_vehicle, appearance)
    RailVehicleRenderingServer.vehicle_set_transform(_vehicle, Transform3D())
    return _vehicle


func test_the_models_are_the_ones_of_the_backend_the_appearance_names() -> void:
    var vehicle:RID = _vehicle_drawn(BACKEND)
    var model:RID = RailVehicleRenderingServer.vehicle_get_model(vehicle)

    assert_true(model.is_valid(), "the exterior built of the scene")
    assert_true(ModelRenderingServer.instance_has_part(model, "Wheel"), "the scene's nodes are its parts")


## The low-poly interior's own scene, drawn as nodes under the vehicle's node - an editable vehicle's
func _low_poly_interior() -> Node3D:
    return find_child(INTERIOR_ROOT, true, false) as Node3D


func test_a_low_poly_interior_is_hidden_and_drawn_as_a_tool_asks() -> void:
    var vehicle:RID = _vehicle_drawn(BACKEND, INTERIOR)
    var interior:Node3D = _low_poly_interior()
    assert_not_null(interior, "the low-poly interior built as nodes")
    assert_true(interior.visible, "drawn")

    RailVehicleRenderingServer.vehicle_set_low_poly_interior_visible(vehicle, false)
    assert_false(interior.visible, "hidden as the tool asks")

    RailVehicleRenderingServer.vehicle_set_low_poly_interior_visible(vehicle, true)
    assert_true(interior.visible, "drawn again")
