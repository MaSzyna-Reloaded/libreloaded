extends ReloadedGutTest

## VehicleAppearance.rendering_backend: a vehicle's models are loaded and drawn by the backend its
## appearance names - a Godot scene by SceneModelRenderingBackend - not by the project's default one

const BACKEND:StringName = &"scene"
const DATA_PATH:String = "res://tests/fixtures"
const MODEL:String = "scene_model.tscn"
## The low-poly interior's scene, and its root's name
const INTERIOR:String = "scene_interior.tscn"
const INTERIOR_ROOT:String = "Interior"
## A body whose paint is a slot of replaceable textures ("body"), its frame of no slot
const LIVERY_MODEL:String = "scene_livery.tscn"
const SLOT:StringName = &"body"
const TEXTURE_SIZE:int = 4

var _vehicle:RID = RID()
## A cabin whose interior is shown in the place of the low-poly interior
var _cabin:RID = RID()


func after_each() -> void:
    if _vehicle.is_valid():
        RailVehicleRenderingServer.vehicle_detach(_vehicle)
        VehicleServer.vehicle_free(_vehicle)
    _vehicle = RID()
    if _cabin.is_valid():
        VehicleServer.cabin_free(_cabin)
    _cabin = RID()


## An editable vehicle - its models built at once - standing at the origin, drawn at a node of the
## scene, in the appearance of the fixture's scene drawn by `backend`
func _vehicle_drawn(backend:StringName, low_poly_interior:String = "", model:String = MODEL,
        replaceable_textures:Dictionary = {}, hidden_parts:PackedStringArray = []) -> RID:
    var node:Node3D = Node3D.new()
    add_child_autofree(node)
    var appearance:VehicleAppearance = VehicleAppearance.new()
    appearance.data_path = DATA_PATH
    appearance.model_filename = model
    appearance.replaceable_textures = replaceable_textures
    appearance.hidden_parts = hidden_parts
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


## The low-poly interior a tool builds anew by its own instance: the vehicle's, none of a vehicle of
## no low-poly interior
func test_the_low_poly_interior_is_the_tools_by_its_instance() -> void:
    var vehicle:RID = _vehicle_drawn(BACKEND, INTERIOR)
    var low_poly:RID = RailVehicleRenderingServer.vehicle_get_low_poly_interior(vehicle)

    assert_true(low_poly.is_valid(), "the low-poly interior built of its scene")
    assert_ne(low_poly, RailVehicleRenderingServer.vehicle_get_model(vehicle), "not the exterior")
    RailVehicleRenderingServer.vehicle_detach(vehicle)
    VehicleServer.vehicle_free(vehicle)
    _vehicle = RID()
    assert_false(RailVehicleRenderingServer.vehicle_get_low_poly_interior(_vehicle_drawn(BACKEND)).is_valid(),
            "none of a vehicle of none")


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


func test_a_cabin_shown_takes_the_place_of_the_whole_low_poly_interior() -> void:
    var vehicle:RID = _vehicle_drawn(BACKEND, INTERIOR)
    _cabin = VehicleServer.cabin_create()
    var interior:Node3D = _low_poly_interior()
    assert_not_null(interior, "the low-poly interior built as nodes")
    assert_true(interior.visible, "drawn while no cabin is shown")

    RailVehicleRenderingServer.vehicle_set_shown_cabin(vehicle, _cabin)
    assert_false(interior.visible, "hidden whole, the cabin's interior in its place")

    RailVehicleRenderingServer.vehicle_set_shown_cabin(vehicle, RID())
    assert_true(interior.visible, "drawn again as the cabin is left")


func test_a_low_poly_interior_hidden_by_a_tool_stays_hidden_as_a_cabin_is_left() -> void:
    var vehicle:RID = _vehicle_drawn(BACKEND, INTERIOR)
    _cabin = VehicleServer.cabin_create()
    var interior:Node3D = _low_poly_interior()

    RailVehicleRenderingServer.vehicle_set_low_poly_interior_visible(vehicle, false)
    assert_false(interior.visible, "hidden with no cabin shown")

    RailVehicleRenderingServer.vehicle_set_shown_cabin(vehicle, _cabin)
    RailVehicleRenderingServer.vehicle_set_shown_cabin(vehicle, RID())
    assert_false(interior.visible, "still hidden as the cabin is left")

    RailVehicleRenderingServer.vehicle_set_low_poly_interior_visible(vehicle, true)
    assert_true(interior.visible, "drawn as the tool asks again")


func _texture() -> Texture2D:
    return ImageTexture.create_from_image(Image.create(TEXTURE_SIZE, TEXTURE_SIZE, false, Image.FORMAT_RGBA8))


## What a mesh of the editable vehicle's model is drawn with in place of its own material
func _drawn_material(mesh:String) -> BaseMaterial3D:
    var node:MeshInstance3D = find_child(mesh, true, false) as MeshInstance3D
    return node.get_surface_override_material(0) as BaseMaterial3D if node else null


func test_a_livery_replaces_the_textures_of_the_materials_of_its_slot() -> void:
    var base_color:Texture2D = _texture()
    var orm:Texture2D = _texture()
    var textures:Dictionary = {SLOT: {"base_color": base_color, "orm": orm}}
    _vehicle_drawn(BACKEND, "", LIVERY_MODEL, textures)

    var paint:BaseMaterial3D = _drawn_material("Body")
    assert_not_null(paint, "the body's paint drawn of the livery")
    if paint:
        assert_eq(paint.albedo_texture, base_color, "its base colour")
        assert_eq(paint.roughness_texture, orm, "its roughness of the occlusion-roughness-metalness one")
        assert_eq(paint.roughness_texture_channel, BaseMaterial3D.TEXTURE_CHANNEL_GREEN)
    assert_null(_drawn_material("Frame"), "a material of no slot its own")


func test_another_livery_replaces_the_textures_of_the_vehicle_drawn() -> void:
    var vehicle:RID = _vehicle_drawn(BACKEND, "", LIVERY_MODEL, {SLOT: {"base_color": _texture()}})
    var other:Texture2D = _texture()

    RailVehicleRenderingServer.vehicle_set_replaceable_textures(vehicle, {SLOT: {"base_color": other}})
    var paint:BaseMaterial3D = _drawn_material("Body")
    assert_eq(paint.albedo_texture if paint else null, other, "the other livery's")

    RailVehicleRenderingServer.vehicle_set_replaceable_textures(vehicle, {})
    assert_null(_drawn_material("Body"), "the model's own, of none")


func test_a_part_the_appearance_hides_is_not_drawn() -> void:
    _vehicle_drawn(BACKEND, INTERIOR, LIVERY_MODEL, {}, PackedStringArray(["Frame", "Seats"]))

    assert_false((find_child("Frame", true, false) as Node3D).visible, "the exterior's part hidden")
    assert_true((find_child("Body", true, false) as Node3D).visible, "the others drawn")
    assert_false((find_child("Seats", true, false) as Node3D).visible, "the low-poly interior's part hidden")


func test_other_parts_hidden_are_hidden_on_the_vehicle_drawn_the_first_drawn_again() -> void:
    var vehicle:RID = _vehicle_drawn(BACKEND, INTERIOR, LIVERY_MODEL, {}, PackedStringArray(["Frame"]))

    RailVehicleRenderingServer.vehicle_set_hidden_parts(vehicle, PackedStringArray(["Body", "Seats"]))

    assert_true((find_child("Frame", true, false) as Node3D).visible, "the part hidden before drawn again")
    assert_false((find_child("Body", true, false) as Node3D).visible, "the exterior's other part hidden")
    assert_false((find_child("Seats", true, false) as Node3D).visible, "and the low-poly interior's")


## The node the models of a vehicle drawn in detail are built under: named after its model's file
const HOLDER:String = "scene_model"


func test_models_built_as_nodes_are_hidden_with_the_vehicles_scenario() -> void:
    var vehicle:RID = _vehicle_drawn(BACKEND)
    var holder:Node3D = find_child(HOLDER, true, false) as Node3D
    assert_not_null(holder, "built as nodes, an editable vehicle")
    if not holder:
        return
    var scenario:RID = holder.get_world_3d().scenario
    assert_true(holder.visible, "drawn in its world")

    RailVehicleRenderingServer.vehicle_set_scenario(vehicle, RID())
    assert_false(holder.visible, "of no scenario, as the optimized instances are drawn in none")
    RailVehicleRenderingServer.vehicle_set_scenario(vehicle, scenario)
    assert_true(holder.visible, "drawn again in its world")
