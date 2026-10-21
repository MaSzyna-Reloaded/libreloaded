extends ReloadedGutTest

## StationServer's runs: a timetable run by a trainset, moved on from entry to entry, working out
## how late the train is and how long is left to its departure - as on a railway: the delay last
## recorded, or the time since the planned time of where the train has not got to

const MINUTE:float = 1.0 / 60.0
## The start, left at 8:00; a stop arrived at 8:30, left at 8:33; a station passed at 9:00
const START_DEPARTURE:float = 8.0
const STOP_ARRIVAL:float = 8.5
const STOP_DEPARTURE:float = 8.55
const PASSING:float = 9.0
const STOP_ENTRY:int = 1

var _stations:Array[RID] = []
var _timetable:RID
var _trainset:RID
var _run:RID


func before_each() -> void:
    for station_name:String in ["Start", "Stop", "Passed"]:
        var station:RID = StationServer.station_create()
        StationServer.station_set_name(station, station_name)
        _stations.append(station)
    _timetable = StationServer.timetable_create()
    StationServer.timetable_add_entry(_timetable, _stations[0], StationServer.NO_TIME, START_DEPARTURE)
    StationServer.timetable_add_entry(_timetable, _stations[1], STOP_ARRIVAL, STOP_DEPARTURE)
    StationServer.timetable_add_entry(_timetable, _stations[2], StationServer.NO_TIME, PASSING)
    _trainset = RailVehicleServer.trainset_create()
    _run = StationServer.run_create(_timetable, _trainset)


func after_each() -> void:
    if StationServer.run_find_by_trainset(_trainset) == _run:
        StationServer.run_free(_run)
    RailVehicleServer.trainset_free(_trainset)
    StationServer.timetable_free(_timetable)
    for station:RID in _stations:
        StationServer.station_free(station)
    _stations.clear()


func test_a_run_is_its_trainsets_one_at_a_time() -> void:
    assert_eq(StationServer.run_find_by_trainset(_trainset), _run)
    assert_eq(StationServer.run_get_timetable(_run), _timetable)
    assert_eq(StationServer.run_get_trainset(_run), _trainset)

    assert_eq(StationServer.run_create(_timetable, _trainset), RID())
    assert_engine_error("The trainset runs a timetable already")
    StationServer.timetable_free(_timetable)
    assert_engine_error("A trainset runs the timetable")


func test_it_stands_at_its_start_from_the_beginning_late_once_the_departure_has_passed() -> void:
    assert_eq(StationServer.run_get_current_entry(_run), 0)
    assert_ne(StationServer.run_entry_get_actual_arrival(_run, 0), StationServer.NO_TIME, "it starts there")
    assert_almost_eq(StationServer.run_get_delay(_run, START_DEPARTURE - 10.0 * MINUTE), 0.0, 0.001)
    assert_almost_eq(StationServer.run_get_delay(_run, START_DEPARTURE + 5.0 * MINUTE), 5.0, 0.001,
            "still standing five minutes after its departure")


func test_on_the_way_it_is_late_once_the_planned_arrival_has_passed() -> void:
    watch_signals(StationServer)
    StationServer.run_next_entry(_run)

    assert_signal_emitted_with_parameters(StationServer, "run_entry_changed", [_run, STOP_ENTRY])
    assert_almost_eq(StationServer.run_get_delay(_run, STOP_ARRIVAL - MINUTE), 0.0, 0.001)
    assert_almost_eq(StationServer.run_get_delay(_run, STOP_ARRIVAL + 3.0 * MINUTE), 3.0, 0.001)


func test_the_time_to_the_departure_is_the_current_stops() -> void:
    assert_eq(StationServer.run_get_time_to_departure(_run, START_DEPARTURE), 0.0, "no stop is planned at the start")
    StationServer.run_next_entry(_run)

    assert_almost_eq(StationServer.run_get_time_to_departure(_run, STOP_ARRIVAL), 3.0, 0.001)
    StationServer.run_next_entry(_run)
    assert_eq(StationServer.run_get_time_to_departure(_run, PASSING), 0.0, "where it passes")


func test_an_offset_shifts_the_planned_times() -> void:
    var trainset:RID = RailVehicleServer.trainset_create()
    var later:RID = StationServer.run_create(_timetable, trainset, 2.0)
    StationServer.run_next_entry(later)

    assert_almost_eq(StationServer.run_get_delay(later, STOP_ARRIVAL + 2.0 + 4.0 * MINUTE), 4.0, 0.001)
    assert_almost_eq(StationServer.run_get_time_to_departure(later, STOP_ARRIVAL + 2.0), 3.0, 0.001)
    StationServer.run_free(later)
    RailVehicleServer.trainset_free(trainset)


func test_midnight_is_not_a_day_away() -> void:
    var station:RID = StationServer.station_create()
    var timetable:RID = StationServer.timetable_create()
    StationServer.timetable_add_entry(timetable, station, StationServer.NO_TIME, 23.0 + 58.0 * MINUTE)
    var trainset:RID = RailVehicleServer.trainset_create()
    var run:RID = StationServer.run_create(timetable, trainset)

    assert_almost_eq(StationServer.run_get_delay(run, 3.0 * MINUTE), 5.0, 0.001, "five minutes after 23:58")
    StationServer.run_free(run)
    RailVehicleServer.trainset_free(trainset)
    StationServer.timetable_free(timetable)
    StationServer.station_free(station)


func test_it_moves_on_and_back_within_its_entries() -> void:
    StationServer.run_previous_entry(_run)
    assert_eq(StationServer.run_get_current_entry(_run), 0, "nothing before the first")
    for _step:int in 5:
        StationServer.run_next_entry(_run)
    assert_eq(StationServer.run_get_current_entry(_run), 3, "past the last, and no further")
    StationServer.run_previous_entry(_run)
    assert_eq(StationServer.run_get_current_entry(_run), 2)
