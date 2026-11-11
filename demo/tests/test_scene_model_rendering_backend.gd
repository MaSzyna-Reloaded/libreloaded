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
