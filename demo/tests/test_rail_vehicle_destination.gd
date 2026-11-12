extends ReloadedGutTest

## Where a rail vehicle is to go: a station and a track of it, set by whoever knows - the scenario
## as it places the vehicle - and by its trainset's timetable: assigning one sends the trainset's
## vehicles to its destination, letting go of it leaves them with none.

const TRACK_LENGTH:float = 100.0
const VEHICLE_OFFSET:float = 10.0

var _track:RID
var _vehicle:RID
var _stations:Array[RID] = []
var _timetables:Array[RID] = []


func before_each() -> void:
    _track = build_track("destination", TRACK_LENGTH)
    _vehicle = VehicleServer.vehicle_create()
    RailVehicleServer.vehicle_attach(_vehicle)
    RailVehicleServer.vehicle_set_track(_vehicle, _track, VEHICLE_OFFSET, TrackServer.DIRECTION_NORMAL)


func after_each() -> void:
    var run:RID = StationServer.run_find_by_trainset(RailVehicleServer.vehicle_get_trainset(_vehicle))
    if run.is_valid():
        StationServer.run_free(run)
    VehicleServer.vehicle_free(_vehicle)
    for timetable:RID in _timetables:
        StationServer.timetable_free(timetable)
    for station:RID in _stations:
        StationServer.station_free(station)
    _timetables.clear()
    _stations.clear()
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


func _create_station(station_name:String) -> RID:
    var station:RID = StationServer.station_create()
    StationServer.station_set_name(station, station_name)
    _stations.append(station)
    return station


## A timetable from `origin` to `destination`
func _create_timetable(origin:RID, destination:RID) -> RID:
    var timetable:RID = StationServer.timetable_create()
    StationServer.timetable_add_entry(timetable, origin, StationServer.NO_TIME, 8.0)
    StationServer.timetable_set_origin(timetable, origin)
    StationServer.timetable_set_destination(timetable, destination)
    _timetables.append(timetable)
    return timetable


func test_a_vehicle_has_no_destination_until_one_is_set() -> void:
    var station:RID = _create_station("Yard")

    assert_false(RailVehicleServer.vehicle_get_destination_station(_vehicle).is_valid())
    RailVehicleServer.vehicle_set_destination(_vehicle, station, _track)
    assert_eq(RailVehicleServer.vehicle_get_destination_station(_vehicle), station)
    assert_eq(RailVehicleServer.vehicle_get_destination_track(_vehicle), _track)


func test_no_station_is_no_destination_and_no_track() -> void:
    RailVehicleServer.vehicle_set_destination(_vehicle, _create_station("Yard"), _track)

    RailVehicleServer.vehicle_set_destination(_vehicle, RID(), _track)
    assert_false(RailVehicleServer.vehicle_get_destination_station(_vehicle).is_valid())
    assert_false(RailVehicleServer.vehicle_get_destination_track(_vehicle).is_valid(), "a track of no station")


func test_assigning_a_timetable_sends_the_vehicles_to_its_destination() -> void:
    var destination:RID = _create_station("Terminus")
    RailVehicleServer.vehicle_set_destination(_vehicle, _create_station("Yard"), _track)

    StationServer.run_create(_create_timetable(_create_station("Start"), destination),
            RailVehicleServer.vehicle_get_trainset(_vehicle))
    assert_eq(RailVehicleServer.vehicle_get_destination_station(_vehicle), destination)
    assert_false(RailVehicleServer.vehicle_get_destination_track(_vehicle).is_valid(), "the timetable names no track")


func test_letting_go_of_the_timetable_leaves_no_destination() -> void:
    var run:RID = StationServer.run_create(_create_timetable(_create_station("Start"), _create_station("Terminus")),
            RailVehicleServer.vehicle_get_trainset(_vehicle))

    StationServer.run_free(run)
    assert_false(RailVehicleServer.vehicle_get_destination_station(_vehicle).is_valid())


func test_another_timetable_sends_the_vehicles_to_its_own_destination() -> void:
    var trainset:RID = RailVehicleServer.vehicle_get_trainset(_vehicle)
    var start:RID = _create_station("Start")
    StationServer.run_free(StationServer.run_create(_create_timetable(start, _create_station("First")), trainset))
    var second:RID = _create_station("Second")

    StationServer.run_create(_create_timetable(start, second), trainset)
    assert_eq(RailVehicleServer.vehicle_get_destination_station(_vehicle), second)
