extends ReloadedGutTest

## A turntable track: taken off the tracks it meets (track_disconnect()), turned to other tracks
## (track_update_curves(), track_curves_changed) with the vehicles on it, and joined to the ones its
## ends meet there (track_connect())

## The tracks' width [m]
const TRACK_WIDTH:float = 1.435
## Where the turntable track lies: across the gap between the approach tracks, its middle the
## pivot it turns about
const TABLE_START:Vector3 = Vector3(10.0, 0.0, 0.0)
const TABLE_END:Vector3 = Vector3(20.0, 0.0, 0.0)
const PIVOT:Vector3 = Vector3(15.0, 0.0, 0.0)
## How far the turned track's ends and the tracks they meet lie off the pivot, square to the
## approach tracks [m]
const HALF_LENGTH:float = 5.0
const SIDING_LENGTH:float = 10.0
## Where a vehicle stands on the turntable track, from its start - off the pivot, so that turning
## moves it [m]
const VEHICLE_OFFSET:float = 2.5
const POSITION_TOLERANCE:float = 0.01

var _tracks:Array[RID] = []
var _table:RID
var _approach_in:RID
var _approach_out:RID
var _siding_left:RID
var _siding_right:RID


func before_each() -> void:
    _approach_in = _create_track(Vector3.ZERO, TABLE_START)
    _table = _create_track(TABLE_START, TABLE_END, TrackServer.TRACK_TABLE)
    _approach_out = _create_track(TABLE_END, Vector3(30.0, 0.0, 0.0))
    _siding_left = _create_track(_turned_start(), _turned_start() + Vector3(0.0, 0.0, SIDING_LENGTH))
    _siding_right = _create_track(_turned_end(), _turned_end() - Vector3(0.0, 0.0, SIDING_LENGTH))
    TrackServer.topology_rebuild()
    watch_signals(TrackServer)


func after_each() -> void:
    for track:RID in _tracks:
        TrackServer.track_free(track)
    _tracks.clear()
    TrackServer.topology_rebuild()


func _create_track(p1:Vector3, p2:Vector3, type:TrackServer.TrackType = TrackServer.TRACK_NORMAL) -> RID:
    var track:RID = TrackServer.track_create()
    TrackServer.track_update_curves(track, _curve(p1, p2), null)
    TrackServer.track_update(track, type, "", TRACK_WIDTH)
    _tracks.append(track)
    return track


func _curve(p1:Vector3, p2:Vector3) -> TrackCurve:
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = p1
    curve.p2 = p2
    return curve


func _turned_start() -> Vector3:
    return PIVOT + Vector3(0.0, 0.0, HALF_LENGTH)


func _turned_end() -> Vector3:
    return PIVOT - Vector3(0.0, 0.0, HALF_LENGTH)


func _connected(track:RID, endpoint:TrackServer.EndpointIndex) -> Array[RID]:
    var connected:Array[RID] = []
    for connection:TrackEndpointRef in TrackServer.track_get_endpoint_connections(track, endpoint):
        connected.append(connection.track_rid)
    return connected


func test_a_disconnected_track_meets_no_track() -> void:
    TrackServer.track_disconnect(_table)

    assert_eq(_connected(_table, TrackServer.CURVE1_P1), [] as Array[RID])
    assert_eq(_connected(_table, TrackServer.CURVE1_P2), [] as Array[RID])
    assert_eq(_connected(_approach_in, TrackServer.CURVE1_P2), [] as Array[RID], "the track it met is free as well")
    assert_signal_emitted(TrackServer, "tracks_changed")


func test_a_turned_track_is_drawn_and_found_where_it_is_now() -> void:
    TrackServer.track_disconnect(_table)

    TrackServer.track_update_curves(_table, _curve(_turned_start(), _turned_end()), null)

    assert_signal_emitted_with_parameters(TrackServer, "track_curves_changed", [_table])
    var near_turned_start:Rect2 = Rect2(Vector2(_turned_start().x, _turned_start().z), Vector2.ZERO).grow(1.0)
    assert_has(TrackServer.tracks_find_in_aabb(near_turned_start), _table)


## A scenery gives each of its tracks its curves once, before it is drawn: no change to announce,
## or every track would be drawn at once, past the streaming that draws it near the camera
func test_a_track_given_its_first_curves_announces_no_change() -> void:
    var track:RID = TrackServer.track_create()
    _tracks.append(track)

    TrackServer.track_update_curves(track, _curve(Vector3(0.0, 0.0, 50.0), Vector3(10.0, 0.0, 50.0)), null)

    assert_signal_not_emitted(TrackServer, "track_curves_changed")


func test_a_turned_track_joins_the_tracks_its_ends_meet() -> void:
    TrackServer.track_disconnect(_table)
    TrackServer.track_update_curves(_table, _curve(_turned_start(), _turned_end()), null)

    TrackServer.track_connect(_table)

    assert_eq(_connected(_table, TrackServer.CURVE1_P1), [_siding_left] as Array[RID])
    assert_eq(_connected(_table, TrackServer.CURVE1_P2), [_siding_right] as Array[RID])
    assert_eq(_connected(_approach_in, TrackServer.CURVE1_P2), [] as Array[RID], "the track it left stays free")


func test_a_joined_end_stays_as_it_is() -> void:
    TrackServer.track_connect(_table)

    assert_eq(_connected(_table, TrackServer.CURVE1_P1), [_approach_in] as Array[RID])
    assert_eq(_connected(_table, TrackServer.CURVE1_P2), [_approach_out] as Array[RID])
    assert_signal_not_emitted(TrackServer, "tracks_changed")


func test_a_vehicle_turns_with_the_track_it_stands_on() -> void:
    var vehicle:RID = VehicleServer.vehicle_create()
    RailVehicleServer.vehicle_attach(vehicle)
    RailVehicleServer.vehicle_set_track(vehicle, _table, VEHICLE_OFFSET, TrackServer.DIRECTION_NORMAL)
    TrackServer.track_disconnect(_table)

    TrackServer.track_update_curves(_table, _curve(_turned_start(), _turned_end()), null)

    var origin:Vector3 = VehicleServer.vehicle_get_transform(vehicle).origin
    assert_almost_eq(origin.x, PIVOT.x, POSITION_TOLERANCE, "on the turned track")
    assert_almost_eq(origin.z, _turned_start().z - VEHICLE_OFFSET, POSITION_TOLERANCE, "where it stood along it")
    VehicleServer.vehicle_free(vehicle)
