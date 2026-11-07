extends ReloadedGutTest

## A trainset as the player arranges it (TrainsetData.get_arranged_members()): its entries paired
## with the declared members by their places, each coupling by its entry's mode; and what each
## coupling mode joins of what both couplers allow (RailVehicleController.coupling_mode_get_flags())

const NONE:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_NONE
const COUPLER:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_COUPLER
const BRAKE:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_BRAKE
const FULL:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_FULL
const PERMANENT:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_PERMANENT
## An entry of the arrangement standing for a vehicle added to the trainset
const ADDED:int = -1


## The declared trainset: a member per (vehicle_id, coupling mode), front first
func _declared(vehicles:Array) -> TrainsetData:
    var trainset:TrainsetData = TrainsetData.new()
    for declared:Array in vehicles:
        var vehicle:VehicleData = VehicleData.new()
        vehicle.vehicle_id = declared[0]
        vehicle.vehicle = "dynamic/test/%s" % declared[0]
        var member:TrainsetMemberData = TrainsetMemberData.new()
        member.vehicle = vehicle
        member.coupling_mode = declared[1]
        trainset.members.append(member)
    return trainset


## The ABCD trainset: A couples to B by the coupler alone, B to C with the brake, C to D for good,
## D (the last) to nothing
func _abcd() -> TrainsetData:
    return _declared([["a", COUPLER], ["b", BRAKE], ["c", PERMANENT], ["d", NONE]])


## The arrangement: each entry a place in the declared trainset (ADDED for an added vehicle),
## reversed when it is in `reversed`
func _arranged(trainset:TrainsetData, places:Array, reversed:Array = []) -> Array[TrainsetMemberData]:
    var arranged:Array[TrainsetMemberData] = []
    for index:int in places.size():
        var place:int = places[index]
        var entry:TrainsetMemberData = TrainsetMemberData.new()
        entry.trainset_index = place
        if place == ADDED:
            entry.vehicle = VehicleData.new()
            entry.vehicle.vehicle = "dynamic/test/added"
        else:
            var declared:TrainsetMemberData = trainset.members[place]
            entry.vehicle = declared.vehicle.duplicate()
        entry.direction = TrackServer.DIRECTION_REVERSED if index in reversed else TrackServer.DIRECTION_NORMAL
        arranged.append(entry)
    return arranged


## The members' coupling modes, front first
func _modes(members:Array[TrainsetMemberData]) -> Array:
    var modes:Array = []
    for member:TrainsetMemberData in members:
        modes.append(member.coupling_mode)
    return modes


## The coupling modes of the trainset arranged as _arranged() makes it
func _arranged_modes(trainset:TrainsetData, places:Array, reversed:Array = []) -> Array:
    return _modes(trainset.get_arranged_members(_arranged(trainset, places, reversed)))


func test_each_member_couples_by_its_entrys_mode() -> void:
    var trainset:TrainsetData = _abcd()
    var arranged:Array[TrainsetMemberData] = _arranged(trainset, [0, 1, 2, 3])
    var chosen:Array = [FULL, NONE, COUPLER, NONE]
    for index:int in arranged.size():
        arranged[index].coupling_mode = chosen[index]
    assert_eq(_modes(trainset.get_arranged_members(arranged)), chosen, "the arrangement's, not the declared ones")


func test_an_added_vehicle_couples_by_its_entrys_mode() -> void:
    # MaSzyna-Reloaded/libmaszyna#305: a vehicle added in front of the declared last one
    var trainset:TrainsetData = _abcd()
    var arranged:Array[TrainsetMemberData] = _arranged(trainset, [ADDED, 3])
    arranged[0].coupling_mode = FULL
    assert_eq(_modes(trainset.get_arranged_members(arranged)), [FULL, NONE])


func test_repeated_ids_are_paired_by_their_places() -> void:
    var trainset:TrainsetData = _declared([["wagon", COUPLER], ["wagon", BRAKE], ["wagon", NONE]])
    for index:int in trainset.members.size():
        (trainset.members[index] as TrainsetMemberData).vehicle.load_name = "load_%d" % index
    var members:Array[TrainsetMemberData] = trainset.get_arranged_members(_arranged(trainset, [1, 0, 2]))
    assert_eq(members.map(func(member:TrainsetMemberData) -> String: return member.vehicle.load_name),
            ["load_1", "load_0", "load_2"], "each with its own load")


func test_an_arrangement_not_of_this_trainset_gives_the_declared_members() -> void:
    var trainset:TrainsetData = _abcd()
    var arranged:Array[TrainsetMemberData] = _arranged(trainset, [ADDED, 3])
    arranged[1].vehicle.vehicle_id = "not-d"
    var members:Array[TrainsetMemberData] = trainset.get_arranged_members(arranged)
    assert_eq(members.size(), trainset.members.size())
    assert_eq(_modes(members), [COUPLER, BRAKE, PERMANENT, NONE])


func test_an_added_vehicle_is_nobody_s_with_its_own_id() -> void:
    var trainset:TrainsetData = _abcd()
    (trainset.members[3] as TrainsetMemberData).vehicle.driver_type = VehicleData.DRIVER_HEAD
    var members:Array[TrainsetMemberData] = trainset.get_arranged_members(_arranged(trainset, [ADDED, 3]))
    assert_eq(members[0].vehicle.vehicle, "dynamic/test/added")
    assert_eq(members[0].vehicle.vehicle_id, "d_0")
    assert_eq(members[0].vehicle.driver_type, VehicleData.DRIVER_NOBODY)
    assert_eq(members[0].trainset_index, ADDED)
    assert_eq(members[1].vehicle.driver_type, VehicleData.DRIVER_HEAD, "the declared one keeps its crew")


func test_each_mode_joins_its_flags_of_what_the_couplers_allow() -> void:
    var joinable:RailVehicleController.CouplingFlags = (RailVehicleController.COUPLING_FLAG_COUPLER
            | RailVehicleController.COUPLING_FLAG_BRAKEHOSE | RailVehicleController.COUPLING_FLAG_CONTROL
            | RailVehicleController.COUPLING_FLAG_GANGWAY) as RailVehicleController.CouplingFlags
    assert_eq(RailVehicleController.coupling_mode_get_flags(NONE, joinable), RailVehicleController.COUPLING_FLAG_NONE)
    assert_eq(RailVehicleController.coupling_mode_get_flags(COUPLER, joinable),
            RailVehicleController.COUPLING_FLAG_COUPLER)
    assert_eq(RailVehicleController.coupling_mode_get_flags(BRAKE, joinable),
            RailVehicleController.COUPLING_FLAG_COUPLER | RailVehicleController.COUPLING_FLAG_BRAKEHOSE)
    assert_eq(RailVehicleController.coupling_mode_get_flags(FULL, joinable), joinable)
    assert_eq(RailVehicleController.coupling_mode_get_flags(PERMANENT, joinable),
            joinable | RailVehicleController.COUPLING_FLAG_PERMANENT)


func test_a_mode_joins_only_what_both_couplers_allow() -> void:
    var coupler_only:RailVehicleController.CouplingFlags = RailVehicleController.COUPLING_FLAG_COUPLER
    assert_eq(RailVehicleController.coupling_mode_get_flags(BRAKE, coupler_only),
            RailVehicleController.COUPLING_FLAG_COUPLER, "no brake hose to join")
    var hoses_only:RailVehicleController.CouplingFlags = (RailVehicleController.COUPLING_FLAG_BRAKEHOSE
            | RailVehicleController.COUPLING_FLAG_MAINHOSE) as RailVehicleController.CouplingFlags
    for mode:RailVehicleController.CouplingMode in [COUPLER, BRAKE, FULL, PERMANENT]:
        assert_eq(RailVehicleController.coupling_mode_get_flags(mode, hoses_only),
                RailVehicleController.COUPLING_FLAG_NONE, "no coupler to hold them together")
