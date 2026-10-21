extends ReloadedGutTest

## TrackServer's track signals: a vehicle reported entering, leaving, stopping and starting off on
## a track is listed on it and announced, with the way it travels along it

var _track:RID
var _vehicle:RID


func before_each() -> void:
    _track = TrackServer.track_create()
    _vehicle = VehicleServer.vehicle_create()
    watch_signals(TrackServer)


func after_each() -> void:
    if TrackServer.track_exists(_track):
        TrackServer.track_free(_track)
    VehicleServer.vehicle_free(_vehicle)


func test_a_vehicle_entering_and_leaving_is_listed_and_announced() -> void:
    TrackServer.track_report_vehicle_entered(_track, _vehicle, TrackServer.TRAVEL_TOWARD_END)

    assert_true(TrackServer.track_is_occupied(_track))
    assert_eq(TrackServer.track_get_vehicles(_track), [_vehicle] as Array[RID])
    assert_signal_emitted_with_parameters(
            TrackServer, "track_vehicle_entered", [_track, _vehicle, TrackServer.TRAVEL_TOWARD_END])

    TrackServer.track_report_vehicle_left(_track, _vehicle)

    assert_false(TrackServer.track_is_occupied(_track))
    assert_signal_emitted_with_parameters(TrackServer, "track_vehicle_left", [_track, _vehicle])


func test_a_vehicle_put_on_a_track_travels_nowhere() -> void:
    TrackServer.track_report_vehicle_entered(_track, _vehicle)

    assert_signal_emitted_with_parameters(
            TrackServer, "track_vehicle_entered", [_track, _vehicle, TrackServer.TRAVEL_NONE])
    TrackServer.track_report_vehicle_left(_track, _vehicle)


func test_a_stop_and_a_start_are_announced() -> void:
    TrackServer.track_report_vehicle_stopped(_track, _vehicle)
    TrackServer.track_report_vehicle_started(_track, _vehicle, TrackServer.TRAVEL_TOWARD_START)

    assert_signal_emitted_with_parameters(TrackServer, "track_vehicle_stopped", [_track, _vehicle])
    assert_signal_emitted_with_parameters(
            TrackServer, "track_vehicle_started", [_track, _vehicle, TrackServer.TRAVEL_TOWARD_START])


func test_nothing_is_announced_of_a_track_that_is_gone() -> void:
    TrackServer.track_free(_track)

    TrackServer.track_report_vehicle_entered(_track, _vehicle, TrackServer.TRAVEL_TOWARD_END)
    TrackServer.track_report_vehicle_stopped(_track, _vehicle)
    TrackServer.track_report_vehicle_started(_track, _vehicle, TrackServer.TRAVEL_TOWARD_END)
    TrackServer.track_report_vehicle_left(_track, _vehicle)

    for signal_name:String in ["track_vehicle_entered", "track_vehicle_stopped", "track_vehicle_started",
            "track_vehicle_left"]:
        assert_signal_not_emitted(TrackServer, signal_name)
