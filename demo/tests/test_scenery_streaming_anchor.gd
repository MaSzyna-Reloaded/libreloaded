extends ReloadedGutTest

## The anchor (SceneryStreamingServer.streaming_set_anchor_position(), the player's vehicle) keeps
## what is within range of it built as the camera does: a piece far from the camera is built as the
## anchor comes within its range and prefetch, and cleared once the anchor is past its range and
## hysteresis.

const STREAM_TIMEOUT_SEC:float = 5.0
## The piece, near the corner of the chunk at the origin, and how far it is drawn
const PIECE_POSITION:Vector3 = Vector3(10.0, 0.0, 10.0)
const PIECE_RANGE:float = 200.0
## The camera far from both
const CAMERA_FAR:Vector3 = Vector3(20000.0, 0.0, 0.0)
## The anchor 500 m past the piece's chunk - within its range and the prefetch - and then 4 km past
## it, beyond its range and the hysteresis
const ANCHOR_NEAR:Vector3 = Vector3(1500.0, 0.0, 500.0)
const ANCHOR_GONE:Vector3 = Vector3(5000.0, 0.0, 500.0)


func test_what_is_within_range_of_the_anchor_is_kept_built() -> void:
    var camera:Camera3D = add_child_autoqfree(Camera3D.new())
    camera.position = CAMERA_FAR
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var pieces := Pieces.new()
    var owner_id:int = SceneryStreamingServer.owner_create("anchored pieces", Callable(), pieces.build, pieces.clear)
    var piece:RID = rid_from_int64(rid_allocate_id())
    var stream:RID = SceneryStreamingServer.stream_register(owner_id, piece, PIECE_POSITION, PIECE_RANGE)
    await wait_streaming(2)
    assert_false(pieces.built.has(piece), "far from the camera")

    SceneryStreamingServer.streaming_set_anchor_position(ANCHOR_NEAR)
    await wait_until(func() -> bool: return pieces.built.has(piece), STREAM_TIMEOUT_SEC)
    assert_true(pieces.built.has(piece), "within range of the anchor")

    SceneryStreamingServer.streaming_set_anchor_position(ANCHOR_GONE)
    await wait_until(func() -> bool: return pieces.cleared.has(piece), STREAM_TIMEOUT_SEC)
    assert_true(pieces.cleared.has(piece), "let go as the anchor went on")

    SceneryStreamingServer.streaming_clear_anchor()
    SceneryStreamingServer.stream_free(stream)
    SceneryStreamingServer.streaming_set_camera(0)


## An owner's pieces, built and cleared at once; its methods are what the streaming keeps of it
class Pieces:
    extends RefCounted

    var built:Array[RID] = []
    var cleared:Array[RID] = []

    func build(rid:RID, _preloaded:Variant) -> void:
        built.append(rid)

    func clear(rid:RID) -> void:
        cleared.append(rid)
