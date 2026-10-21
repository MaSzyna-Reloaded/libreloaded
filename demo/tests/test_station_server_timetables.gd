extends ReloadedGutTest

## StationServer's stations and timetables: a station found by its name, with the tracks it is
## stopped or passed at, and a timetable of the stations a train calls at, its plan only

const ARRIVAL:float = 8.5
const DEPARTURE:float = 8.55

var _stations:Array[RID] = []
var _timetables:Array[RID] = []


func after_each() -> void:
    for timetable:RID in _timetables:
        StationServer.timetable_free(timetable)
    _timetables.clear()
    for station:RID in _stations:
        StationServer.station_free(station)
    _stations.clear()


func _station(station_name:String) -> RID:
    var station:RID = StationServer.station_create()
    StationServer.station_set_name(station, station_name)
    _stations.append(station)
    return station


func _timetable() -> RID:
    var timetable:RID = StationServer.timetable_create()
    _timetables.append(timetable)
    return timetable


func test_a_station_is_found_by_its_name() -> void:
    var station:RID = _station("Markowo Górne")

    assert_eq(StationServer.station_find_by_name("Markowo Górne"), station)
    assert_eq(StationServer.station_get_name(station), "Markowo Górne")
    assert_true(station in StationServer.station_list())
    assert_eq(StationServer.station_find_by_name("Markowo Dolne"), RID(), "no station of another name")


func test_a_station_is_a_station_unless_it_is_a_passenger_stop() -> void:
    var station:RID = _station("Halt")

    assert_eq(StationServer.station_get_type(station), StationServer.STATION_TYPE_STATION)
    StationServer.station_set_type(station, StationServer.STATION_TYPE_PASSENGER_STOP)
    assert_eq(StationServer.station_get_type(station), StationServer.STATION_TYPE_PASSENGER_STOP)


func test_a_renamed_station_is_found_by_its_new_name_only() -> void:
    var station:RID = _station("Old")

    StationServer.station_set_name(station, "New")

    assert_eq(StationServer.station_find_by_name("New"), station)
    assert_eq(StationServer.station_find_by_name("Old"), RID())


func test_a_station_keeps_its_stop_points() -> void:
    var station:RID = _station("Halt")
    var platform:RID = TrackServer.track_create()
    var through:RID = TrackServer.track_create()

    StationServer.station_add_stop_point(station, platform)
    StationServer.station_add_stop_point(station, through)
    StationServer.station_add_stop_point(station, platform)

    assert_eq(StationServer.station_get_stop_points(station), [platform, through] as Array[RID], "each track once")
    StationServer.station_clear_stop_points(station)
    assert_eq(StationServer.station_get_stop_points(station), [] as Array[RID])
    TrackServer.track_free(platform)
    TrackServer.track_free(through)


func test_a_timetable_holds_its_stations_and_their_times() -> void:
    var start:RID = _station("Start")
    var halt:RID = _station("Halt")
    var timetable:RID = _timetable()
    StationServer.timetable_set_train_name(timetable, "12345")

    assert_eq(StationServer.timetable_add_entry(timetable, start, StationServer.NO_TIME, ARRIVAL), 0)
    assert_eq(StationServer.timetable_add_entry(timetable, halt, ARRIVAL, DEPARTURE), 1)

    assert_eq(StationServer.timetable_get_train_name(timetable), "12345")
    assert_eq(StationServer.timetable_get_entry_count(timetable), 2)
    assert_eq(StationServer.timetable_entry_get_station(timetable, 1), halt)
    assert_eq(StationServer.timetable_entry_get_arrival(timetable, 1), ARRIVAL)
    assert_eq(StationServer.timetable_entry_get_departure(timetable, 1), DEPARTURE)
    assert_true(StationServer.timetable_entry_is_stop(timetable, 1))
    assert_false(StationServer.timetable_entry_is_stop(timetable, 0), "no arrival is planned at the start")


func test_a_timetable_says_what_train_it_is_of_and_where_it_runs() -> void:
    var origin:RID = _station("Origin")
    var destination:RID = _station("Destination")
    var timetable:RID = _timetable()

    assert_eq(StationServer.timetable_get_speed_limit(timetable), StationServer.NO_SPEED_LIMIT)
    assert_eq(StationServer.timetable_get_origin(timetable), RID())
    StationServer.timetable_set_train_category(timetable, "Ex")
    StationServer.timetable_set_train_label(timetable, "Kormoran")
    StationServer.timetable_set_origin(timetable, origin)
    StationServer.timetable_set_destination(timetable, destination)
    StationServer.timetable_set_speed_limit(timetable, 90.0)
    StationServer.timetable_set_required_locomotive(timetable, "EU07")
    StationServer.timetable_set_required_mass(timetable, 450.0)
    StationServer.timetable_set_required_brake_ratio(timetable, 65.0)

    assert_eq(StationServer.timetable_get_train_category(timetable), "Ex")
    assert_eq(StationServer.timetable_get_train_label(timetable), "Kormoran")
    assert_eq(StationServer.timetable_get_origin(timetable), origin)
    assert_eq(StationServer.timetable_get_destination(timetable), destination)
    assert_eq(StationServer.timetable_get_speed_limit(timetable), 90.0)
    assert_eq(StationServer.timetable_get_required_locomotive(timetable), "EU07")
    assert_eq(StationServer.timetable_get_required_mass(timetable), 450.0)
    assert_eq(StationServer.timetable_get_required_brake_ratio(timetable), 65.0)


func test_an_entry_keeps_what_the_timetable_says_of_the_way_to_it() -> void:
    var timetable:RID = _timetable()
    var entry:int = StationServer.timetable_add_entry(timetable, _station("Halt"), ARRIVAL, DEPARTURE)

    assert_eq(StationServer.timetable_entry_get_speed_limit(timetable, entry), StationServer.NO_SPEED_LIMIT)
    assert_eq(StationServer.timetable_entry_get_track_count(timetable, entry), 1)
    StationServer.timetable_entry_set_track_count(timetable, entry, 2)
    assert_eq(StationServer.timetable_entry_get_track_count(timetable, entry), 2)
    assert_eq(StationServer.timetable_entry_get_radio_channel(timetable, entry), StationServer.NO_RADIO_CHANNEL)
    assert_false(StationServer.timetable_entry_is_technical_stop(timetable, entry))
    assert_false(StationServer.timetable_entry_is_direction_change(timetable, entry))
    StationServer.timetable_entry_set_technical_stop(timetable, entry, true)
    StationServer.timetable_entry_set_direction_change(timetable, entry, true)
    StationServer.timetable_entry_set_speed_limit(timetable, entry, 80.0)
    StationServer.timetable_entry_set_radio_channel(timetable, entry, 3)
    StationServer.timetable_entry_set_kilometre(timetable, entry, 12.4)
    StationServer.timetable_entry_set_running_time(timetable, entry, 7.0)
    StationServer.timetable_entry_set_extra(timetable, entry, {"facilities": "R4"})
    StationServer.timetable_set_extra(timetable, {"brake_ratio": 65.0})

    assert_eq(StationServer.timetable_entry_get_speed_limit(timetable, entry), 80.0)
    assert_eq(StationServer.timetable_entry_get_radio_channel(timetable, entry), 3)
    assert_eq(StationServer.timetable_entry_get_kilometre(timetable, entry), 12.4)
    assert_eq(StationServer.timetable_entry_get_running_time(timetable, entry), 7.0)
    assert_true(StationServer.timetable_entry_is_technical_stop(timetable, entry))
    assert_true(StationServer.timetable_entry_is_direction_change(timetable, entry))
    assert_eq(StationServer.timetable_entry_get_extra(timetable, entry), {"facilities": "R4"})
    assert_eq(StationServer.timetable_get_extra(timetable), {"brake_ratio": 65.0})


func test_a_station_a_timetable_calls_at_is_not_freed() -> void:
    var station:RID = StationServer.station_create()
    var timetable:RID = StationServer.timetable_create()
    StationServer.timetable_add_entry(timetable, station, ARRIVAL, DEPARTURE)

    StationServer.station_free(station)
    assert_engine_error("A timetable calls at the station")
    assert_true(station in StationServer.station_list())

    StationServer.timetable_free(timetable)
    StationServer.station_free(station)
    assert_false(station in StationServer.station_list())
