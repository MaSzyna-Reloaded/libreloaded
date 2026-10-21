extends GutTest

## VehicleServer's state of a vehicle's build: built when whoever builds it says so, announced once

var _vehicles:Array[RID] = []


func after_each() -> void:
    for vehicle:RID in _vehicles:
        VehicleServer.vehicle_free(vehicle)
    _vehicles.clear()


func _create_vehicle() -> RID:
    var vehicle:RID = VehicleServer.vehicle_create()
    _vehicles.append(vehicle)
    return vehicle


func test_a_vehicle_is_built_when_its_builder_says_so() -> void:
    var vehicle:RID = _create_vehicle()
    watch_signals(VehicleServer)

    assert_false(VehicleServer.vehicle_is_built(vehicle))
    VehicleServer.vehicle_set_built(vehicle)
    assert_true(VehicleServer.vehicle_is_built(vehicle))
    assert_signal_emitted_with_parameters(VehicleServer, "vehicle_built", [vehicle])
    # built without its simulation: no controller was bound, as of a vehicle whose data was not read
    assert_false(VehicleServer.vehicle_is_simulation_ready(vehicle))
