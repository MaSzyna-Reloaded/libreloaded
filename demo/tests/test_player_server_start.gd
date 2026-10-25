extends ReloadedGutTest

## PlayerServer - the player starts once where it is put (player_start()) and stops before the
## scenery goes (player_stop()); the camera follows: on foot it looks from the free view, stopped
## it follows nothing of the scenery going.

var _vehicles:Array[RID] = []


func after_each() -> void:
    PlayerServer.player_stop()
    PlayerCameraServer.camera_set_mode(PlayerCameraServer.CAMERA_MODE_FREE)
    PlayerCameraServer.camera_set_target(RID())
    for vehicle:RID in _vehicles:
        VehicleServer.vehicle_free(vehicle)
    _vehicles.clear()


## A bare vehicle - nothing of it is driven here
func _create_vehicle() -> RID:
    var vehicle:RID = VehicleServer.vehicle_create()
    _vehicles.append(vehicle)
    return vehicle


func test_the_player_started_on_foot_looks_from_the_free_view() -> void:
    PlayerCameraServer.camera_set_target(_create_vehicle())
    PlayerCameraServer.camera_set_mode(PlayerCameraServer.CAMERA_MODE_FOLLOW)
    watch_signals(PlayerServer)

    PlayerServer.player_start(RID())

    assert_signal_emit_count(PlayerServer, "player_started", 1)
    assert_true(PlayerServer.player_is_started())
    assert_false(PlayerServer.player_get_vehicle().is_valid(), "on foot")
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FREE)


func test_the_player_starts_once() -> void:
    PlayerServer.player_start(RID())
    watch_signals(PlayerServer)

    PlayerServer.player_start(RID())

    assert_engine_error("The player has started already")
    assert_signal_emit_count(PlayerServer, "player_started", 0)


func test_the_player_stopped_follows_nothing() -> void:
    PlayerServer.player_start(RID())
    PlayerCameraServer.camera_set_target(_create_vehicle())
    PlayerCameraServer.camera_set_mode(PlayerCameraServer.CAMERA_MODE_FOLLOW)
    watch_signals(PlayerServer)

    PlayerServer.player_stop()

    assert_signal_emit_count(PlayerServer, "player_stopped", 1)
    assert_false(PlayerServer.player_is_started())
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FREE)


func test_a_player_not_started_does_not_stop() -> void:
    watch_signals(PlayerServer)

    PlayerServer.player_stop()

    assert_signal_emit_count(PlayerServer, "player_stopped", 0)


## The vehicle the player is started in is the player node's to take over (its cab scene is the
## node's business); the server keeps it until the player stops or the vehicle goes
func test_the_start_vehicle_is_kept_until_the_player_stops() -> void:
    var vehicle:RID = _create_vehicle()

    PlayerServer.player_start(vehicle)

    assert_true(PlayerServer.player_is_started())
    assert_eq(PlayerServer.player_get_start_vehicle(), vehicle)
    PlayerServer.player_stop()
    assert_false(PlayerServer.player_get_start_vehicle().is_valid(), "stopped, it starts in nothing")


func test_a_start_vehicle_freed_is_no_start_vehicle() -> void:
    var vehicle:RID = _create_vehicle()
    PlayerServer.player_start(vehicle)

    VehicleServer.vehicle_free(vehicle)
    _vehicles.erase(vehicle)

    assert_false(PlayerServer.player_get_start_vehicle().is_valid())
