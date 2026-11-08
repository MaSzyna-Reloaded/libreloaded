#include "RailVehicleTrainsetClassifier.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"
#include "vehicles/rail/RailVehicleController.hpp"

namespace godot {
    namespace {
        /* The type by whether the trainset has wagons carrying passengers and wagons carrying goods,
         * [passenger][goods] (AutoRewident(), Driver.cpp:2154-2165) */
        constexpr RailVehicleServer::TrainsetType TYPES[2][2] = {
                {RailVehicleServer::TRAINSET_TYPE_NONE, RailVehicleServer::TRAINSET_TYPE_CARGO},
                {RailVehicleServer::TRAINSET_TYPE_PASSENGER, RailVehicleServer::TRAINSET_TYPE_MIXED}};
    } // namespace

    void RailVehicleTrainsetClassifier::_bind_methods() {
        ClassDB::bind_method(D_METHOD("classify", "vehicles"), &RailVehicleTrainsetClassifier::classify);
        GDVIRTUAL_BIND(_classify, "vehicles");
    }

    RailVehicleServer::TrainsetType RailVehicleTrainsetClassifier::classify(const TypedArray<RID> &p_vehicles) const {
        if (RailVehicleServer::TrainsetType type = RailVehicleServer::TRAINSET_TYPE_NONE;
            GDVIRTUAL_CALL(_classify, p_vehicles, type)) {
            return type;
        }
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, RailVehicleServer::TRAINSET_TYPE_NONE);
        bool passenger = false;
        bool goods = false;
        for (const Variant &vehicle: p_vehicles) {
            const Ref<RailVehicleController> controller = vehicle_server->vehicle_get_controller(vehicle);
            if (controller.is_null() || controller->get_power() >= WAGON_POWER_MAX) {
                continue;
            }
            const Ref<RailVehicleBrake> brake =
                    controller->get_rail_component(RailVehicleComponentType::COMPONENT_BRAKES);
            const int delays = brake.is_valid() ? brake->get_cntrl_brake_delays() : RailVehicleBrake::BRAKE_DELAY_NONE;
            const bool carries_goods =
                    (delays & RailVehicleBrake::BRAKE_DELAY_G) != 0 && (delays & RailVehicleBrake::BRAKE_DELAY_R) == 0;
            goods = goods || carries_goods;
            passenger = passenger || !carries_goods;
        }
        return TYPES[passenger][goods];
    }
} // namespace godot
