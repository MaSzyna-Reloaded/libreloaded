extends ReloadedGutTest

## What a scenario says of a trainset: its mission, and whether it offers it to the player - offered
## unless it says not


func test_a_trainset_keeps_its_mission_and_whether_it_is_offered() -> void:
    var trainset:RID = RailVehicleServer.trainset_create()

    assert_eq(RailVehicleServer.trainset_get_description(trainset), "")
    assert_true(RailVehicleServer.trainset_is_player_drivable(trainset), "offered unless the scenario says not")
    RailVehicleServer.trainset_set_description(trainset, "Take the express to the next station.")
    RailVehicleServer.trainset_set_player_drivable(trainset, false)
    assert_eq(RailVehicleServer.trainset_get_description(trainset), "Take the express to the next station.")
    assert_false(RailVehicleServer.trainset_is_player_drivable(trainset))
    RailVehicleServer.trainset_free(trainset)
