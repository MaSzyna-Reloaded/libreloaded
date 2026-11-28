extends ReloadedGutTest

## SceneModelRenderingBackend: a Godot scene as a model of ModelRenderingServer's, by the backend's
## name - its nodes the parts, posed, hidden and picked as the scene's meshes stand

const BACKEND:StringName = &"scene"
const MODEL:String = "res://tests/fixtures/scene_model.tscn"
## The fixture's Wheel, and its hull's half length along Z
const WHEEL_ORIGIN:Vector3 = Vector3(1.0, 0.0, 1.0)
const HULL_HALF_LENGTH:float = 2.0
const RAY_LENGTH:float = 10.0
const POSE_SHIFT:Vector3 = Vector3(0.0, 0.0, 5.0)
## How far from the camera a placement of the test is built [m], where the camera goes to leave it -
## a few of the streaming's chunks away -, and how long the streaming may take
const PLACEMENT_RANGE:float = 50.0
const CAMERA_AWAY:Vector3 = Vector3(4000.0, 0.0, 0.0)
const STREAM_TIMEOUT_SEC:float = 5.0

var _instance:RID = RID()


func after_each() -> void:
    if _instance.is_valid():
        ModelRenderingServer.instance_free(_instance)
    _instance = RID()


func _create(instancer:ModelRenderingServer.Instancer) -> RID:
    var model:Resource = ModelRenderingServer.model_load(BACKEND, MODEL)
    _instance = ModelRenderingServer.instance_create(BACKEND, model, instancer,
            ModelRenderingServer.INSTANCE_KIND_DYNAMIC)
    return _instance


## The segment from in front of the hull along -Z, through its middle
func _hit_hull() -> Dictionary:
    return ModelRenderingServer.instance_intersect_segment(_instance,
            Vector3(0.0, 0.0, RAY_LENGTH), Vector3(0.0, 0.0, -RAY_LENGTH))


func test_a_scenes_nodes_are_its_parts() -> void:
    var instance:RID = _create(ModelRenderingServer.INSTANCER_OPTIMIZED)

    assert_true(ModelRenderingServer.instance_has_part(instance, "Wheel"))
    assert_false(ModelRenderingServer.instance_has_part(instance, "Bogie"))
    assert_eq(ModelRenderingServer.instance_get_part_transform(instance, "Wheel").origin, WHEEL_ORIGIN)
    assert_eq(ModelRenderingServer.instance_has_light(instance, "headlight"), false, "a scene has no lights")
    assert_almost_eq(ModelRenderingServer.instance_get_aabb(instance).end.z, HULL_HALF_LENGTH, 0.001)


func test_its_meshes_are_picked_as_they_stand_posed() -> void:
    var instance:RID = _create(ModelRenderingServer.INSTANCER_OPTIMIZED)
    watch_signals(ModelRenderingServer)
    ModelRenderingServer.instance_build(instance)
    assert_signal_emitted_with_parameters(ModelRenderingServer, "instance_built", [instance])

    assert_almost_eq(float(_hit_hull()["distance"]), RAY_LENGTH - HULL_HALF_LENGTH, 0.001, "the hull's front")
    var poses:Dictionary = {"Body": Transform3D(Basis(), POSE_SHIFT)}
    ModelRenderingServer.instance_set_part_poses(instance, poses)
    assert_almost_eq(float(_hit_hull()["distance"]), RAY_LENGTH - HULL_HALF_LENGTH - POSE_SHIFT.z, 0.001,
            "the body posed forward")
    ModelRenderingServer.instance_set_part_visible(instance, "Body", false)
    assert_eq(_hit_hull(), {}, "a hidden part is not picked")


func test_built_as_nodes_it_is_the_scene_under_the_attached_node() -> void:
    var attached:Node3D = Node3D.new()
    add_child_autofree(attached)
    var instance:RID = _create(ModelRenderingServer.INSTANCER_NODES)
    ModelRenderingServer.instance_attach_object_instance_id(instance, attached.get_instance_id())

    ModelRenderingServer.instance_build(instance)
    ModelRenderingServer.instance_set_part_visible(instance, "Wheel", false)

    var wheel:Node3D = attached.find_child("Wheel", true, false) as Node3D
    assert_not_null(wheel, "the scene's nodes under the attached node")
    assert_false(wheel.visible, "a hidden part's node hidden")


## The render layers a model is drawn on, set through the server: the scene's meshes on them; and
## the smoke of a model of no emitters, set the same way, nothing
const INTERIOR_LAYER:int = 1 << 4


func test_its_meshes_are_drawn_on_the_layers_set() -> void:
    var attached:Node3D = Node3D.new()
    add_child_autofree(attached)
    var instance:RID = _create(ModelRenderingServer.INSTANCER_NODES)
    ModelRenderingServer.instance_attach_object_instance_id(instance, attached.get_instance_id())
    ModelRenderingServer.instance_build(instance)

    ModelRenderingServer.instance_set_layer_mask(instance, INTERIOR_LAYER)
    ModelRenderingServer.instance_set_smoke_intensity(instance, 1.0)

    var meshes:Array[Node] = attached.find_children("*", "MeshInstance3D", true, false)
    assert_true(meshes.size() > 0)
    for mesh:Node in meshes:
        assert_eq((mesh as MeshInstance3D).layers, INTERIOR_LAYER, mesh.name)


## A model of two bogies of an axle each, both axles of the same name, saved where the backend loads it of
const BOGIES_MODEL:String = "user://scene_model_bogies.scn"
const AXLE_SHIFT:Vector3 = Vector3(0.0, 0.0, 1.0)


func test_a_part_is_a_node_by_its_path_or_by_a_name_no_other_has() -> void:
    var root:Node3D = Node3D.new()
    for bogie_name:String in ["Bogie_A", "Bogie_B"]:
        var bogie:Node3D = Node3D.new()
        bogie.name = bogie_name
        root.add_child(bogie)
        bogie.owner = root
        var axle:MeshInstance3D = MeshInstance3D.new()
        axle.name = "Axle"
        axle.mesh = BoxMesh.new()
        bogie.add_child(axle)
        axle.owner = root
    var scene:PackedScene = PackedScene.new()
    scene.pack(root)
    root.free()
    assert_eq(ResourceSaver.save(scene, BOGIES_MODEL), OK)
    var attached:Node3D = Node3D.new()
    add_child_autofree(attached)
    _instance = ModelRenderingServer.instance_create(BACKEND, ModelRenderingServer.model_load(BACKEND, BOGIES_MODEL),
            ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_DYNAMIC)
    ModelRenderingServer.instance_attach_object_instance_id(_instance, attached.get_instance_id())
    ModelRenderingServer.instance_build(_instance)

    assert_true(ModelRenderingServer.instance_has_part(_instance, "Bogie_B/Axle"), "by its path")
    assert_true(ModelRenderingServer.instance_has_part(_instance, "Bogie_A"), "by a name no other node has")
    assert_false(ModelRenderingServer.instance_has_part(_instance, "Axle"), "a name of two nodes names neither")
    ModelRenderingServer.instance_set_part_poses(_instance, {"Bogie_B/Axle": Transform3D(Basis(), AXLE_SHIFT)})
    var drawn_root:Node = attached.get_child(0)
    assert_eq((drawn_root.get_node("Bogie_B/Axle") as Node3D).position, AXLE_SHIFT, "the one of the path posed")
    assert_eq((drawn_root.get_node("Bogie_A/Axle") as Node3D).position, Vector3.ZERO, "the other left")


## A model of one mesh of an emissive surface and a plain one, saved where the backend loads it of
const EMISSIVE_MODEL:String = "user://scene_model_emissive.scn"
const EMISSION_ENERGY:float = 2.0


func test_lit_only_the_emissive_surfaces_of_a_mesh_are_drawn_lit() -> void:
    var lamp:StandardMaterial3D = StandardMaterial3D.new()
    lamp.emission_enabled = true
    var wall:StandardMaterial3D = StandardMaterial3D.new()
    var mesh:ArrayMesh = ArrayMesh.new()
    for material:StandardMaterial3D in [lamp, wall]:
        mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, BoxMesh.new().get_mesh_arrays())
        mesh.surface_set_material(mesh.get_surface_count() - 1, material)
    var root:Node3D = Node3D.new()
    var interior:MeshInstance3D = MeshInstance3D.new()
    interior.name = "Interior"
    interior.mesh = mesh
    root.add_child(interior)
    interior.owner = root
    var scene:PackedScene = PackedScene.new()
    scene.pack(root)
    root.free()
    assert_eq(ResourceSaver.save(scene, EMISSIVE_MODEL), OK)
    var attached:Node3D = Node3D.new()
    add_child_autofree(attached)
    _instance = ModelRenderingServer.instance_create(BACKEND, ModelRenderingServer.model_load(BACKEND, EMISSIVE_MODEL),
            ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_DYNAMIC)
    ModelRenderingServer.instance_attach_object_instance_id(_instance, attached.get_instance_id())
    ModelRenderingServer.instance_build(_instance)

    ModelRenderingServer.instance_set_emission_energy(_instance, EMISSION_ENERGY)

    var drawn:MeshInstance3D = attached.find_child("Interior", true, false) as MeshInstance3D
    var lit:StandardMaterial3D = drawn.get_surface_override_material(0) as StandardMaterial3D
    assert_not_null(lit, "the emissive surface lit")
    assert_eq(lit.emission_energy_multiplier, EMISSION_ENERGY)
    assert_null(drawn.get_surface_override_material(1), "the plain one of its own material")


## instance_find_nearest_hit(): of the built, visible instances on a segment, the nearest - what
## hides a pickable behind it (SceneryHUDMouseServer)
func test_the_nearest_built_instance_on_a_segment_is_found() -> void:
    var model:Resource = ModelRenderingServer.model_load(BACKEND, MODEL)
    var near:RID = ModelRenderingServer.instance_create(BACKEND, model, ModelRenderingServer.INSTANCER_OPTIMIZED,
            ModelRenderingServer.INSTANCE_KIND_DYNAMIC)
    var far:RID = _create(ModelRenderingServer.INSTANCER_OPTIMIZED)
    ModelRenderingServer.instance_set_transform(near, Transform3D(Basis(), POSE_SHIFT))
    var from:Vector3 = Vector3(0.0, 0.0, RAY_LENGTH)
    var to:Vector3 = Vector3(0.0, 0.0, -RAY_LENGTH)

    assert_eq(ModelRenderingServer.instance_find_nearest_hit(from, to), {}, "nothing built yet")
    ModelRenderingServer.instance_build(near)
    ModelRenderingServer.instance_build(far)
    var hit:Dictionary = ModelRenderingServer.instance_find_nearest_hit(from, to)
    assert_eq(hit.get("instance"), near, "the one in front")
    assert_almost_eq(float(hit.get("distance", 0.0)), RAY_LENGTH - HULL_HALF_LENGTH - POSE_SHIFT.z, 0.001)
    ModelRenderingServer.instance_set_visible(near, false)
    assert_eq(ModelRenderingServer.instance_find_nearest_hit(from, to).get("instance"), far, "a hidden one hides nothing")
    ModelRenderingServer.instance_free(near)


## A placement the camera leaves is cleared by its backend: the server hears instance_cleared and
## no longer searches it - a backend's instance_get_aabb() of an instance whose model it released
## crashed the hit search (libreloaded#69)
func test_a_placement_the_camera_has_left_is_not_searched() -> void:
    var camera:Camera3D = add_child_autoqfree(Camera3D.new())
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var built:Array[RID] = []
    var cleared:Array[RID] = []
    var on_built:Callable = func(instance:RID) -> void: built.append(instance)
    var on_cleared:Callable = func(instance:RID) -> void: cleared.append(instance)
    ModelRenderingServer.instance_built.connect(on_built)
    ModelRenderingServer.instance_cleared.connect(on_cleared)
    _instance = ModelRenderingServer.instance_register(BACKEND, MODEL, PackedStringArray(), Transform3D(), 0.0,
            PLACEMENT_RANGE, camera.get_world_3d().scenario)
    var from:Vector3 = Vector3(0.0, 0.0, RAY_LENGTH)
    var to:Vector3 = Vector3(0.0, 0.0, -RAY_LENGTH)

    await wait_until(func() -> bool: return built.has(_instance), STREAM_TIMEOUT_SEC)
    assert_eq(ModelRenderingServer.instance_find_nearest_hit(from, to).get("instance"), _instance, "built near the camera")
    camera.position = CAMERA_AWAY
    await wait_until(func() -> bool: return cleared.has(_instance), STREAM_TIMEOUT_SEC)
    assert_eq(cleared, [_instance] as Array[RID], "cleared once the camera has left")
    assert_eq(ModelRenderingServer.instance_find_nearest_hit(from, to), {}, "not searched since")
    ModelRenderingServer.instance_built.disconnect(on_built)
    ModelRenderingServer.instance_cleared.disconnect(on_cleared)
    SceneryStreamingServer.streaming_set_camera(0)


## A model of a hull's collision box (vehicle.collision.body) and a door's (vehicle.collision.interior)
const COLLISION_MODEL:String = "res://tests/fixtures/scene_model_collision.tscn"
## The fixture's Door, its box's half depth along X; how far a pose of the door moves it
const DOOR_ORIGIN:Vector3 = Vector3(3.0, 0.0, 0.0)
const DOOR_HALF_DEPTH:float = 0.05
const DOOR_SHIFT:Vector3 = Vector3(0.0, 0.0, 2.0)
const HULL_HALF_WIDTH:float = 1.0
## A point of the hull a ray along X through misses the door
const BESIDE_DOOR:Vector3 = Vector3(0.0, 0.0, -1.5)
## The layers of the roles' shapes (ModelRenderingServer)
const BODY_LAYER_SETTING:String = "reloaded/physics/vehicle_body_layer"
const INTERIOR_LAYER_SETTING:String = "reloaded/physics/vehicle_interior_layer"
## How long a kinematic body may take to stand where it is put
const PHYSICS_TIMEOUT_SEC:float = 1.0


func _collision_instance(space:RID) -> RID:
    var model:Resource = ModelRenderingServer.model_load(BACKEND, COLLISION_MODEL)
    _instance = ModelRenderingServer.instance_create(BACKEND, model, ModelRenderingServer.INSTANCER_OPTIMIZED,
            ModelRenderingServer.INSTANCE_KIND_DYNAMIC)
    ModelRenderingServer.instance_set_space(_instance, space)
    ModelRenderingServer.instance_build(_instance)
    return _instance


## What a ray of the layers of `mask` from +X along -X through `at` hits first in `space`
func _ray_hit(space:RID, at:Vector3, mask:int) -> Dictionary:
    var query:PhysicsRayQueryParameters3D = PhysicsRayQueryParameters3D.create(
            at + Vector3.RIGHT * RAY_LENGTH, at - Vector3.RIGHT * RAY_LENGTH, mask)
    return PhysicsServer3D.space_get_direct_state(space).intersect_ray(query)


func _space() -> RID:
    var node:Node3D = add_child_autofree(Node3D.new())
    return node.get_world_3d().space


func test_a_models_collision_shapes_stand_in_the_space_on_their_roles_layers() -> void:
    var space:RID = _space()
    _collision_instance(space)
    await wait_physics_frames(2)
    var body_layer:int = ProjectSettings.get_setting(BODY_LAYER_SETTING)
    var interior_layer:int = ProjectSettings.get_setting(INTERIOR_LAYER_SETTING)

    var door:Dictionary = _ray_hit(space, DOOR_ORIGIN, interior_layer)
    assert_almost_eq(Vector3(door.get("position", Vector3.INF)).x, DOOR_ORIGIN.x + DOOR_HALF_DEPTH, 0.001,
            "the door's box where the door stands, on the interior's layer")
    var hull:Dictionary = _ray_hit(space, BESIDE_DOOR, body_layer)
    assert_almost_eq(Vector3(hull.get("position", Vector3.INF)).x, HULL_HALF_WIDTH, 0.001, "the hull's on the body's")
    assert_eq(_ray_hit(space, BESIDE_DOOR, interior_layer), {}, "the hull on no other layer")


func test_a_collision_shape_moves_with_its_parts_pose() -> void:
    var space:RID = _space()
    var instance:RID = _collision_instance(space)
    var interior_layer:int = ProjectSettings.get_setting(INTERIOR_LAYER_SETTING)
    await wait_physics_frames(2)
    ModelRenderingServer.instance_set_part_poses(instance, {"Door": Transform3D(Basis(), DOOR_SHIFT)})
    # a kinematic body moves where it is put by the steps of the physics
    await wait_until(func() -> bool: return _ray_hit(space, DOOR_ORIGIN, interior_layer).is_empty(),
            PHYSICS_TIMEOUT_SEC)

    assert_eq(_ray_hit(space, DOOR_ORIGIN, interior_layer), {}, "gone from where the door stood")
    assert_false(_ray_hit(space, DOOR_ORIGIN + DOOR_SHIFT, interior_layer).is_empty(), "where the pose puts it")


func test_a_model_in_no_space_collides_with_nothing() -> void:
    var space:RID = _space()
    var instance:RID = _collision_instance(RID())
    await wait_physics_frames(2)
    var body_layer:int = ProjectSettings.get_setting(BODY_LAYER_SETTING)
    assert_eq(_ray_hit(space, Vector3.ZERO, body_layer), {}, "no space set")

    ModelRenderingServer.instance_set_space(instance, space)
    await wait_physics_frames(2)
    assert_false(_ray_hit(space, Vector3.ZERO, body_layer).is_empty(), "in the space once set")


## A model's nodes turned by channels (the model format's rotation_control): a crank by an axle's
## angle, the coupling rod on its pin turned back by the same angle, so it keeps level as its pin
## goes round; a lever of no channel's value at its default, within its limits
const RODS_MODEL:String = "res://tests/fixtures/scene_model_rods.tscn"
const CRANK_ORIGIN:Vector3 = Vector3(0.0, 0.5, 0.0)
const CRANK_PIN:Vector3 = Vector3(0.0, 0.0, 0.25)
const AXLE_CHANNEL:String = "axle.1.angle"
const LEVER_LIMIT:float = 0.25
const ROD_HALF_LENGTH:float = 1.0
const QUARTER_TURNS:int = 4


func _rods_instance(instancer:ModelRenderingServer.Instancer) -> RID:
    var model:Resource = ModelRenderingServer.model_load(BACKEND, RODS_MODEL)
    _instance = ModelRenderingServer.instance_create(BACKEND, model, instancer,
            ModelRenderingServer.INSTANCE_KIND_DYNAMIC)
    return _instance


func test_a_coupling_rod_keeps_level_on_its_crank_pin() -> void:
    var attached:Node3D = Node3D.new()
    add_child_autofree(attached)
    var instance:RID = _rods_instance(ModelRenderingServer.INSTANCER_NODES)
    ModelRenderingServer.instance_attach_object_instance_id(instance, attached.get_instance_id())
    ModelRenderingServer.instance_build(instance)
    var rod:Node3D = attached.find_child("Rod", true, false) as Node3D

    for quarter:int in QUARTER_TURNS:
        var angle:float = quarter * PI / 2.0
        ModelRenderingServer.instance_set_channels(instance, {AXLE_CHANNEL: angle})
        var pin:Vector3 = CRANK_ORIGIN + Basis(Vector3.RIGHT, angle) * CRANK_PIN
        assert_true(rod.global_transform.origin.is_equal_approx(pin), "the rod on the pin at %d deg" % (quarter * 90))
        assert_true(rod.global_transform.basis.is_equal_approx(Basis()), "the rod level at %d deg" % (quarter * 90))


func test_a_part_of_no_channels_value_stands_at_its_default_within_its_limits() -> void:
    var attached:Node3D = Node3D.new()
    add_child_autofree(attached)
    var instance:RID = _rods_instance(ModelRenderingServer.INSTANCER_NODES)
    ModelRenderingServer.instance_attach_object_instance_id(instance, attached.get_instance_id())
    ModelRenderingServer.instance_build(instance)

    var lever:Node3D = attached.find_child("Lever", true, false) as Node3D
    assert_true(lever.transform.basis.is_equal_approx(Basis(Vector3.UP, LEVER_LIMIT)), "the default clamped to the limit")


func test_channels_set_before_the_build_pose_the_meshes_drawn_optimized() -> void:
    var instance:RID = _rods_instance(ModelRenderingServer.INSTANCER_OPTIMIZED)
    ModelRenderingServer.instance_set_channels(instance, {AXLE_CHANNEL: PI})
    ModelRenderingServer.instance_build(instance)

    # the rod half a turn round: its pin under the crank, the rod still along Z
    var pin:Vector3 = CRANK_ORIGIN + Basis(Vector3.RIGHT, PI) * CRANK_PIN
    var hit:Dictionary = ModelRenderingServer.instance_intersect_segment(
            instance, pin + Vector3(0.0, 0.0, 2.0 * ROD_HALF_LENGTH), pin - Vector3(0.0, 0.0, 2.0 * ROD_HALF_LENGTH))
    assert_almost_eq(float(hit.get("distance", -1.0)), ROD_HALF_LENGTH, 0.001, "the rod's end where the pin took it")


func test_a_part_driven_by_its_data_turns_by_its_channel_alone() -> void:
    var attached:Node3D = Node3D.new()
    add_child_autofree(attached)
    var instance:RID = _rods_instance(ModelRenderingServer.INSTANCER_NODES)
    ModelRenderingServer.instance_attach_object_instance_id(instance, attached.get_instance_id())
    ModelRenderingServer.instance_build(instance)
    var crank:Node3D = attached.find_child("Crank", true, false) as Node3D

    # the crank posed as a wheel too, by the same angle: turned once, as the rod on it
    ModelRenderingServer.instance_set_part_poses(instance, {"Crank": Transform3D(Basis(Vector3.RIGHT, PI / 2.0))})
    ModelRenderingServer.instance_set_channels(instance, {AXLE_CHANNEL: PI / 2.0})
    assert_true(crank.transform.basis.is_equal_approx(Basis(Vector3.RIGHT, PI / 2.0)), "a quarter turn, not a half")


## A model's headlight dimmed by its channel (the model format's light_control): off of no value, at
## its own energy times the level, and its lens (light_id) lit at the same level
const LIGHTS_MODEL:String = "res://tests/fixtures/scene_model_lights.tscn"
const HEADLIGHT_CHANNEL:String = "headlight.front.left.level"
const HEADLIGHT_ENERGY:float = 8.0
const LENS_EMISSION:float = 4.0
const DIMMED:float = 0.5


func test_a_headlight_and_its_lens_shine_at_the_level_of_their_channel() -> void:
    var attached:Node3D = Node3D.new()
    add_child_autofree(attached)
    var model:Resource = ModelRenderingServer.model_load(BACKEND, LIGHTS_MODEL)
    _instance = ModelRenderingServer.instance_create(BACKEND, model, ModelRenderingServer.INSTANCER_NODES,
            ModelRenderingServer.INSTANCE_KIND_DYNAMIC)
    ModelRenderingServer.instance_attach_object_instance_id(_instance, attached.get_instance_id())
    ModelRenderingServer.instance_build(_instance)
    var light:SpotLight3D = attached.find_child("Headlight", true, false) as SpotLight3D
    var lens:MeshInstance3D = attached.find_child("Lens", true, false) as MeshInstance3D
    var lens_emission:Callable = func() -> float:
        var drawn:BaseMaterial3D = lens.get_surface_override_material(0) as BaseMaterial3D
        return drawn.emission_energy_multiplier if drawn else -1.0

    assert_false(light.visible, "off of no value: its default")
    assert_almost_eq(lens_emission.call(), 0.0, 0.001, "the lens dark")

    ModelRenderingServer.instance_set_channels(_instance, {HEADLIGHT_CHANNEL: DIMMED})
    assert_true(light.visible)
    assert_almost_eq(light.light_energy, HEADLIGHT_ENERGY * DIMMED, 0.001, "its energy times the level")
    assert_almost_eq(lens_emission.call(), LENS_EMISSION * DIMMED, 0.001, "the lens at the same level")

    ModelRenderingServer.instance_set_channels(_instance, {HEADLIGHT_CHANNEL: 0.0})
    assert_false(light.visible, "off at 0")
