extends ReloadedGutTest

## The map's size [px]: the camera's place, the world's origin, is drawn in its middle
const MAP_SIZE:Vector2 = Vector2(200.0, 200.0)
## The tracks' gauge [m]
const TRACK_GAUGE:float = 1.435

var _map:RailMiniMap = null
var _track:RID = RID()


## A switch parting at the world's origin, under a camera standing there, and the map of it drawn
func before_each() -> void:
    var straight:TrackCurve = TrackCurve.new()
    straight.p2 = Vector3(10.0, 0.0, 0.0)
    var diverging:TrackCurve = TrackCurve.new()
    diverging.p2 = Vector3(10.0, 0.0, 10.0)
    _track = TrackServer.track_create()
    TrackServer.track_update_curves(_track, straight, diverging)
    TrackServer.track_update(_track, TrackServer.TRACK_SWITCH, "", TRACK_GAUGE)
    TrackServer.topology_rebuild()
    var camera:Camera3D = Camera3D.new()
    add_child_autofree(camera)
    camera.make_current()
    _map = RailMiniMap.new()
    _map.size = MAP_SIZE
    add_child_autofree(_map)
    await wait_idle_frames(2)


func after_each() -> void:
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


func test_the_switch_handle_tells_the_switch_state() -> void:
    var switch_rid:RID = TrackServer.track_get_switch(_track)
    assert_string_contains(_map.get_tooltip(MAP_SIZE * 0.5), "Straight")

    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_DIVERGING)

    assert_string_contains(_map.get_tooltip(MAP_SIZE * 0.5), "Diverging")
    assert_eq(_map.get_tooltip(Vector2.ZERO), "", "no tooltip away from a switch")


func test_a_click_on_the_switch_handle_throws_the_switch() -> void:
    var switch_rid:RID = TrackServer.track_get_switch(_track)
    var click:InputEventMouseButton = InputEventMouseButton.new()
    click.button_index = MOUSE_BUTTON_LEFT
    click.pressed = true
    click.position = _map.get_global_transform_with_canvas() * (MAP_SIZE * 0.5)
    click.global_position = click.position

    get_viewport().push_input(click)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_DIVERGING)
