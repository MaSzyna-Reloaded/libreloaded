extends ReloadedGutTest

## RoadVehicleServer drives its vehicles along RoadServer's roads: towards the velocity wanted within
## the acceleration limit, within the roads' limits, behind the vehicle ahead in its lane, past the
## ones beside it, on the way it chose where the roads part, back along a road that turns about,
## and to a stop at a dead end

## The vehicles' size [m]
const VEHICLE_LENGTH:float = 4.0
const VEHICLE_WIDTH:float = 1.8
## Their configuration: the gap kept [m], the acceleration limit [m/s2], the range looked ahead [m]
const FOLLOWING_GAP:float = 5.0
const ACCELERATION_LIMIT:float = 2.0
const OBSTACLE_RANGE:float = 250.0
## The velocity the vehicles want [km/h]
const WANTED_VELOCITY:float = 50.0
## Their place across the road, right of their way [m]
const SIDE_OFFSET:float = 1.5
## The roads' width [m]
const ROAD_WIDTH:float = 7.0
## How close a velocity or a place is taken as reached
const VELOCITY_TOLERANCE:float = 0.05
const PLACE_TOLERANCE:float = 0.1

var _roads:Array[RID] = []
var _tracks:Array[RID] = []
var _vehicles:Array[RID] = []
var _controllers:Array[RID] = []


func after_each() -> void:
    for vehicle:RID in _vehicles:
        VehicleServer.vehicle_free(vehicle)
    for controller:RID in _controllers:
        VehicleServer.controller_free(controller)
    RoadServer.road_free_many(_roads)
    for track:RID in _tracks:
        TrackServer.track_free(track)
    TrackServer.topology_rebuild()
    _vehicles.clear()
    _controllers.clear()
    _roads.clear()
    _tracks.clear()


## A road along `points`
func _road(points:Array[Vector3]) -> RID:
    var curve:Curve3D = Curve3D.new()
    for point:Vector3 in points:
        curve.add_point(point)
    var road:RID = RoadServer.road_create()
    RoadServer.road_set_curve(road, curve)
    RoadServer.road_set_width(road, ROAD_WIDTH)
    _roads.append(road)
    return road


## A straight road along +X from `start`, `length` long
func _straight(start:float, length:float) -> RID:
    var points:Array[Vector3] = [Vector3(start, 0.0, 0.0), Vector3(start + length, 0.0, 0.0)]
    return _road(points)


## A road vehicle standing `offset` along `road`, its front toward `facing`, wanting `velocity`
func _vehicle(road:RID, offset:float, facing:RoadServer.RoadEnd, velocity:float) -> RID:
    var description:RoadVehicleController = RoadVehicleController.new()
    description.dimensions_length = VEHICLE_LENGTH
    description.dimensions_width = VEHICLE_WIDTH
    description.following_gap = FOLLOWING_GAP
    description.acceleration_limit = ACCELERATION_LIMIT
    description.obstacle_range = OBSTACLE_RANGE
    var vehicle:RID = VehicleServer.vehicle_create()
    var controller:RID = VehicleServer.controller_create()
    _vehicles.append(vehicle)
    _controllers.append(controller)
    RoadVehicleServer.vehicle_attach(vehicle)
    VehicleServer.controller_configure(controller, description)
    VehicleServer.vehicle_bind_controller(vehicle, controller)
    RoadVehicleServer.vehicle_set_road(vehicle, road, offset, facing)
    VehicleServer.vehicle_send_command(vehicle, "wanted_velocity_set", velocity)
    return vehicle


func _x(vehicle:RID) -> float:
    return RoadVehicleServer.vehicle_get_transform(vehicle).origin.x


## Its front's way along the X axis
func _heading(vehicle:RID) -> float:
    return -RoadVehicleServer.vehicle_get_transform(vehicle).basis.z.x


func test_it_moves_off_within_its_acceleration_limit() -> void:
    var vehicle:RID = _vehicle(_straight(0.0, 1000.0), 10.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)

    await step(ticks(1.0))

    assert_almost_eq(VehicleServer.vehicle_get_velocity(vehicle), ACCELERATION_LIMIT, VELOCITY_TOLERANCE)
    assert_gt(_x(vehicle), 10.0, "it went toward the road's end")
    assert_almost_eq(_heading(vehicle), 1.0, VELOCITY_TOLERANCE, "its front faces the way it goes")


func test_it_keeps_to_the_limit_of_its_road() -> void:
    var road:RID = _straight(0.0, 1000.0)
    var limit:float = 20.0
    RoadServer.road_set_velocity(road, limit)
    var vehicle:RID = _vehicle(road, 10.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)

    await step(ticks(10.0))

    assert_almost_eq(VehicleServer.vehicle_get_speed(vehicle), limit, VELOCITY_TOLERANCE)


func test_it_stops_behind_a_standing_vehicle_with_its_gap() -> void:
    var road:RID = _straight(0.0, 1000.0)
    var standing:RID = _vehicle(road, 100.0, RoadServer.ROAD_END_END, 0.0)
    var following:RID = _vehicle(road, 20.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)

    await step(ticks(30.0))

    assert_almost_eq(VehicleServer.vehicle_get_velocity(following), 0.0, VELOCITY_TOLERANCE)
    var gap:float = _x(standing) - _x(following) - VEHICLE_LENGTH
    assert_true(gap >= FOLLOWING_GAP - PLACE_TOLERANCE and gap < FOLLOWING_GAP + 1.0,
            "it stands %.2f m behind, its gap is %.2f m" % [gap, FOLLOWING_GAP])


func test_vehicles_in_their_lanes_pass_each_other() -> void:
    var road:RID = _straight(0.0, 1000.0)
    var east:RID = _vehicle(road, 400.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)
    var west:RID = _vehicle(road, 600.0, RoadServer.ROAD_END_START, WANTED_VELOCITY)
    VehicleServer.vehicle_send_command(east, "side_offset_set", SIDE_OFFSET)
    VehicleServer.vehicle_send_command(west, "side_offset_set", SIDE_OFFSET)

    await step(ticks(20.0))

    assert_gt(_x(east), _x(west), "they went past each other")
    assert_ne(RoadVehicleServer.vehicle_get_transform(east).origin.z,
            RoadVehicleServer.vehicle_get_transform(west).origin.z, "each in its own lane")


func test_vehicles_in_one_lane_stop_before_each_other() -> void:
    var road:RID = _straight(0.0, 1000.0)
    var east:RID = _vehicle(road, 400.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)
    var west:RID = _vehicle(road, 600.0, RoadServer.ROAD_END_START, WANTED_VELOCITY)

    await step(ticks(30.0))

    assert_lt(_x(east), _x(west), "neither went through the other")
    assert_almost_eq(VehicleServer.vehicle_get_velocity(east), 0.0, VELOCITY_TOLERANCE)
    assert_almost_eq(VehicleServer.vehicle_get_velocity(west), 0.0, VELOCITY_TOLERANCE)


func test_it_stops_at_a_dead_end() -> void:
    var vehicle:RID = _vehicle(_straight(0.0, 100.0), 10.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)

    await step(ticks(30.0))

    assert_almost_eq(VehicleServer.vehicle_get_velocity(vehicle), 0.0, VELOCITY_TOLERANCE)
    assert_almost_eq(_x(vehicle), 100.0, PLACE_TOLERANCE, "it stands at the road's end")


func test_it_turns_about_along_a_road_back_onto_its_own() -> void:
    var road:RID = _straight(0.0, 200.0)
    var loop_points:Array[Vector3] = [Vector3(200.0, 0.0, 0.0), Vector3(230.0, 0.0, 15.0), Vector3(200.0, 0.0, 30.0),
            Vector3(200.0, 0.0, 0.0)]
    var loop:RID = _road(loop_points)
    RoadServer.road_link(road, RoadServer.ROAD_END_END, loop, RoadServer.ROAD_END_START)
    RoadServer.road_link(loop, RoadServer.ROAD_END_END, road, RoadServer.ROAD_END_END)
    var vehicle:RID = _vehicle(road, 150.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)

    var back:bool = await wait_simulated_until(
            func() -> bool: return RoadVehicleServer.vehicle_get_road(vehicle) == road and _heading(vehicle) < 0.0,
            60.0, "the vehicle back on its road")

    assert_true(back)
    await step(ticks(5.0))
    assert_lt(_x(vehicle), 200.0 - PLACE_TOLERANCE, "it goes back along its road")


func test_it_takes_the_way_it_chose_where_the_roads_part() -> void:
    var road:RID = _straight(0.0, 50.0)
    var left_points:Array[Vector3] = [Vector3(50.0, 0.0, 0.0), Vector3(100.0, 0.0, -50.0)]
    var right_points:Array[Vector3] = [Vector3(50.0, 0.0, 0.0), Vector3(100.0, 0.0, 50.0)]
    var left:RID = _road(left_points)
    var right:RID = _road(right_points)
    RoadServer.road_link(road, RoadServer.ROAD_END_END, left, RoadServer.ROAD_END_START)
    RoadServer.road_link(road, RoadServer.ROAD_END_END, right, RoadServer.ROAD_END_START)
    var vehicle:RID = _vehicle(road, 10.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)

    var route:Array[RoadRouteSegment] = RoadVehicleServer.vehicle_trace_route(vehicle, 100.0)
    var chosen:RID = route[1].road
    var parted:bool = await wait_simulated_until(
            func() -> bool: return not RoadVehicleServer.vehicle_get_road(vehicle) == road, 30.0,
            "the vehicle past the parting")

    assert_true(parted)
    assert_true(chosen == left or chosen == right, "it chose one of the ways on")
    assert_eq(RoadVehicleServer.vehicle_get_road(vehicle), chosen, "it took the way it chose")


func test_a_vehicle_is_reported_on_the_roads_and_the_tracks_they_are_laid_along() -> void:
    var first:RID = _straight(0.0, 50.0)
    var second:RID = _straight(50.0, 500.0)
    RoadServer.road_link(first, RoadServer.ROAD_END_END, second, RoadServer.ROAD_END_START)
    var track:RID = build_track("road_test_track", 500.0)
    _tracks.append(track)
    RoadServer.road_set_track(second, track)
    var vehicle:RID = _vehicle(first, 10.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)
    watch_signals(RoadServer)

    var crossed:bool = await wait_simulated_until(
            func() -> bool: return RoadVehicleServer.vehicle_get_road(vehicle) == second, 30.0,
            "the vehicle on the next road")

    assert_true(crossed)
    assert_signal_emitted_with_parameters(RoadServer, "road_vehicle_entered", [second, vehicle, RoadServer.ROAD_END_END])
    assert_eq(RoadServer.road_get_vehicles(second), [vehicle] as Array[RID])
    assert_eq(RoadServer.road_get_vehicles(first), [] as Array[RID], "it left the first road")
    assert_eq(TrackServer.track_get_vehicles(track), [vehicle] as Array[RID], "it is on the road's track")
    assert_eq(RoadServer.track_get_road(track), second)


func test_a_ring_of_roads_of_no_length_is_a_dead_end() -> void:
    var road:RID = _straight(0.0, 100.0)
    var point:Array[Vector3] = [Vector3(100.0, 0.0, 0.0)]
    var nowhere:RID = _road(point)
    RoadServer.road_link(road, RoadServer.ROAD_END_END, nowhere, RoadServer.ROAD_END_START)
    RoadServer.road_link(nowhere, RoadServer.ROAD_END_END, nowhere, RoadServer.ROAD_END_START)
    var vehicle:RID = _vehicle(road, 10.0, RoadServer.ROAD_END_END, WANTED_VELOCITY)

    await step(ticks(30.0))

    assert_almost_eq(VehicleServer.vehicle_get_velocity(vehicle), 0.0, VELOCITY_TOLERANCE)
    assert_eq(RoadVehicleServer.vehicle_get_road(vehicle), road, "it stays on its road")
    assert_eq(RoadVehicleServer.vehicle_trace_route(vehicle, 1000.0).size(), 1, "its way ends there")
