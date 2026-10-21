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
