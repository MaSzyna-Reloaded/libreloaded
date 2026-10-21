#include "RailVehicleHeating.hpp"
#include "vehicles/base/VehicleController.hpp"

namespace godot {
    void RailVehicleHeating::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                RailVehicleHeating, Variant::INT, heating_source, "heating", PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY(RailVehicleHeating, Variant::FLOAT, heating_max_voltage, "heating");

        ClassDB::bind_method(D_METHOD("heating", "enabled"), &RailVehicleHeating::heating);
        ClassDB::bind_method(D_METHOD("get_active"), &RailVehicleHeating::get_active);
        ClassDB::bind_method(D_METHOD("get_allowed"), &RailVehicleHeating::get_allowed);
        ClassDB::bind_method(D_METHOD("get_power"), &RailVehicleHeating::get_power);
    }

    void RailVehicleHeating::_register_commands() {
        VehicleComponent::_register_commands();
        register_command("heating", callable_mp(this, &RailVehicleHeating::heating));
    }

    void RailVehicleHeating::_unregister_commands() {
        VehicleComponent::_unregister_commands();
        unregister_command("heating");
    }
} // namespace godot
