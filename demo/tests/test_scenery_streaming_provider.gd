extends ReloadedGutTest

## A provider supplies the content of its cells by kind as the camera comes near, each kind to its own
## consumer (SceneryStreamingServer.content_set_consumer()): a heightmap terrain's regions
## (SceneryStreamingProvider.CONTENT_HEIGHTMAP) to the consumer of their kind, which lets them go as
## the provider is freed - within the draw distance, or within the range the provider declares. What
## the streaming has prepared is built though the plan changes meanwhile, the same camera set again
## changes nothing, and the building budget grows while the camera stands and drops as it moves.

const STREAM_TIMEOUT_SEC:float = 5.0
## What the provider supplies of its one cell, the cell the camera stands in
const REGION:String = "region"
## A cell this many metres past the draw distance, and a range that reaches it
const PAST_DRAW_DISTANCE_M:float = 1500.0
const RANGE_PAST_DRAW_DISTANCE_M:float = 2500.0
## How long a slow piece takes to prepare: past a few of the streaming's looks at the camera
## (SceneryStreamingServer WATCH_INTERVAL_SEC 0.1 s), each a new plan once another piece comes
const SLOW_PRELOAD_MSEC:int = 500
## Pieces enough, each built in SLOW_BUILD_MSEC, to keep the streaming building for many frames
const SLOW_BUILD_COUNT:int = 400
const SLOW_BUILD_MSEC:int = 1
## SceneryStreamingServer BUDGET_MSEC, the budget of a moving camera, and how far it moves
const MOVING_BUDGET_MSEC:int = 4
const CAMERA_MOVE_M:float = 10.0
## Frames enough for the standing camera's budget to grow
const STANDING_FRAMES:int = 4


class HeightmapProvider:
    extends SceneryStreamingProvider

    var cell:Vector2i = Vector2i.ZERO
    var declared_range:float = 0.0

    func _get_chunk_cells() -> Array[Vector2i]:
        return [cell] as Array[Vector2i]

    func _get_range() -> float:
        return declared_range

    func _get_content_kinds() -> PackedInt32Array:
        return PackedInt32Array([CONTENT_HEIGHTMAP])

    func _chunk_load(_cell:Vector2i, kind:int) -> Array:
        return [REGION] if kind == CONTENT_HEIGHTMAP else []


func test_heightmap_regions_reach_the_consumer_of_their_kind() -> void:
    var camera:Camera3D = add_child_autoqfree(Camera3D.new())
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var adopted:Array = []
    var released:Array[RID] = []
    var region_rid:RID = rid_from_int64(rid_allocate_id())
    var adopt:Callable = func(item:Variant, _scenario:RID) -> RID:
        adopted.append(item)
        return region_rid
    var release:Callable = func(rid:RID) -> void: released.append(rid)
    SceneryStreamingServer.content_set_consumer(SceneryStreamingProvider.CONTENT_HEIGHTMAP, adopt, release)
    var provider:RID = SceneryStreamingServer.provider_register(HeightmapProvider.new(), camera.get_world_3d().scenario)

    await wait_until(func() -> bool: return adopted.size() > 0, STREAM_TIMEOUT_SEC)
    assert_eq(adopted, [REGION])
    SceneryStreamingServer.provider_free(provider)
    assert_eq(released, [region_rid] as Array[RID], "let go with its provider")
    SceneryStreamingServer.content_set_consumer(SceneryStreamingProvider.CONTENT_HEIGHTMAP, Callable(), Callable())
    SceneryStreamingServer.streaming_set_camera(0)


## A cell past the draw distance: the provider supplies it only when its range reaches it
func test_a_provider_supplies_cells_within_its_range() -> void:
    var camera:Camera3D = add_child_autoqfree(Camera3D.new())
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var adopted:Array = []
    var adopt:Callable = func(item:Variant, _scenario:RID) -> RID:
        adopted.append(item)
        return rid_from_int64(rid_allocate_id())
    SceneryStreamingServer.content_set_consumer(SceneryStreamingProvider.CONTENT_HEIGHTMAP, adopt, func(_rid:RID) -> void: pass)
    var past:float = SceneryStreamingServer.streaming_get_draw_distance() + PAST_DRAW_DISTANCE_M
    var chunk_size:float = SceneryStreamingServer.streaming_get_statistics()["chunk_size"]
    var far_cell:Vector2i = Vector2i(ceili(past / chunk_size), 0)
    var near_provider := HeightmapProvider.new()
    near_provider.cell = far_cell
    var reaching := HeightmapProvider.new()
    reaching.cell = far_cell
    reaching.declared_range = SceneryStreamingServer.streaming_get_draw_distance() + RANGE_PAST_DRAW_DISTANCE_M
    var scenario:RID = camera.get_world_3d().scenario
    var providers:Array[RID] = [
        SceneryStreamingServer.provider_register(near_provider, scenario),
        SceneryStreamingServer.provider_register(reaching, scenario)]

    await wait_until(func() -> bool: return adopted.size() > 0, STREAM_TIMEOUT_SEC)
    await wait_streaming(2)
    assert_eq(adopted, [REGION], "the reaching provider's alone")
    for provider:RID in providers:
        SceneryStreamingServer.provider_free(provider)
    SceneryStreamingServer.content_set_consumer(SceneryStreamingProvider.CONTENT_HEIGHTMAP, Callable(), Callable())
    SceneryStreamingServer.streaming_set_camera(0)


## A piece registered while another is being prepared makes a new plan; the one being prepared is
## built of what was prepared, not prepared again
func test_a_piece_prepared_is_built_though_the_plan_changes() -> void:
    var camera:Camera3D = add_child_autoqfree(Camera3D.new())
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var pieces := SlowPieces.new()
    var slow_owner:int = SceneryStreamingServer.owner_create("slow pieces", pieces.prepare, pieces.build, pieces.clear)
    var slow:RID = rid_from_int64(rid_allocate_id())
    var later:RID = rid_from_int64(rid_allocate_id())
    var streams:Array[RID] = [SceneryStreamingServer.stream_register(slow_owner, slow, Vector3.ZERO, 0.0)]
    await wait_until(func() -> bool: return pieces.get_prepared(slow) > 0, STREAM_TIMEOUT_SEC)
    streams.append(SceneryStreamingServer.stream_register(slow_owner, later, Vector3.ZERO, 0.0))

    await wait_until(func() -> bool: return pieces.built.has(slow) and pieces.built.has(later), STREAM_TIMEOUT_SEC)
    assert_eq(pieces.get_prepared(slow), 1, "prepared once")
    for stream:RID in streams:
        SceneryStreamingServer.stream_free(stream)
    SceneryStreamingServer.streaming_set_camera(0)


## A view shown anew sets the streaming's camera again: the plan and the work stay
func test_the_same_camera_set_again_changes_nothing() -> void:
    var camera:Camera3D = add_child_autoqfree(Camera3D.new())
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var revision:int = SceneryStreamingServer.streaming_get_statistics()["target_revision"]
    watch_signals(SceneryStreamingServer)
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    assert_eq(SceneryStreamingServer.streaming_get_statistics()["target_revision"], revision)
    assert_signal_not_emitted(SceneryStreamingServer, "streaming_camera_changed")
    SceneryStreamingServer.streaming_set_camera(0)


## While the camera stands the streaming builds with more of each frame, and with BUDGET_MSEC again
## as soon as it moves
func test_the_budget_grows_while_the_camera_stands_and_drops_as_it_moves() -> void:
    var camera:Camera3D = add_child_autoqfree(Camera3D.new())
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    await wait_until(func() -> bool: return not SceneryStreamingServer.streaming_get_statistics()["filling"], STREAM_TIMEOUT_SEC)
    var pieces := SlowPieces.new()
    pieces.build_msec = SLOW_BUILD_MSEC
    var slow_owner:int = SceneryStreamingServer.owner_create("slow builds", Callable(), pieces.build, pieces.clear)
    var streams:Array[RID] = []
    for index:int in SLOW_BUILD_COUNT:
        streams.append(SceneryStreamingServer.stream_register(slow_owner, rid_from_int64(rid_allocate_id()), Vector3.ZERO, 0.0))
    await wait_until(func() -> bool: return pieces.built.size() > 0, STREAM_TIMEOUT_SEC)
    await wait_idle_frames(STANDING_FRAMES)
    assert_gt(SceneryStreamingServer.streaming_get_statistics()["budget_msec"], MOVING_BUDGET_MSEC, "grown while it stands")
    camera.position += Vector3.FORWARD * CAMERA_MOVE_M
    await wait_idle_frames(1)
    assert_eq(SceneryStreamingServer.streaming_get_statistics()["budget_msec"], MOVING_BUDGET_MSEC, "back as it moves")
    assert_lt(pieces.built.size(), SLOW_BUILD_COUNT, "measured while building")
    for stream:RID in streams:
        SceneryStreamingServer.stream_free(stream)
    SceneryStreamingServer.streaming_set_camera(0)


## An owner's pieces, each taking SLOW_PRELOAD_MSEC to prepare and build_msec to build; its methods
## are what the streaming keeps of it, as of a server
class SlowPieces:
    extends RefCounted

    var built:Array[RID] = []
    var build_msec:int = 0
    var _prepared:Dictionary[RID, int] = {}
    var _mutex:Mutex = Mutex.new()

    func prepare(rid:RID) -> Variant:
        _mutex.lock()
        _prepared[rid] = _prepared.get(rid, 0) + 1
        _mutex.unlock()
        OS.delay_msec(SLOW_PRELOAD_MSEC)
        return rid

    func build(rid:RID, _preloaded:Variant) -> void:
        OS.delay_msec(build_msec)
        built.append(rid)

    func clear(_rid:RID) -> void:
        pass

    func get_prepared(rid:RID) -> int:
        _mutex.lock()
        var count:int = _prepared.get(rid, 0)
        _mutex.unlock()
        return count
