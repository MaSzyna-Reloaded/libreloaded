extends GutTest

## VehicleServer starts a vehicle's simulation anew on the controller it runs on, as that is now:
## a configuration changed since it started is kept

## The gap the vehicle is described with, and the one set on its controller as it runs [m]
const DESCRIBED_GAP:float = 5.0
const CHANGED_GAP:float = 9.0

var _vehicle:RID = RID()
var _controller:RID = RID()


func before_each() -> void:
    var description:RoadVehicleController = RoadVehicleController.new()
    description.following_gap = DESCRIBED_GAP
    _vehicle = VehicleServer.vehicle_create()
    _controller = VehicleServer.controller_create()
    RoadVehicleServer.vehicle_attach(_vehicle)
    VehicleServer.controller_configure(_controller, description)
    VehicleServer.vehicle_bind_controller(_vehicle, _controller)


func after_each() -> void:
    VehicleServer.vehicle_free(_vehicle)
    VehicleServer.controller_free(_controller)


func test_a_restarted_vehicle_keeps_its_controller_as_changed() -> void:
    var controller:RoadVehicleController = VehicleServer.vehicle_get_controller(_vehicle)
    controller.following_gap = CHANGED_GAP
    watch_signals(VehicleServer)

    VehicleServer.vehicle_restart(_vehicle)

    assert_signal_emitted_with_parameters(VehicleServer, "vehicle_controller_changed", [_vehicle])
    assert_same(VehicleServer.vehicle_get_controller(_vehicle), controller)
    assert_eq(controller.following_gap, CHANGED_GAP)
    assert_true(VehicleServer.vehicle_is_simulation_ready(_vehicle))
