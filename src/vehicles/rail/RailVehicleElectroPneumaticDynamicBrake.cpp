#include "RailVehicleElectroPneumaticDynamicBrake.hpp"

namespace godot {
    void RailVehicleElectroPneumaticDynamicBrake::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("switch_ep_fuse", "value"), &RailVehicleElectroPneumaticDynamicBrake::switch_ep_fuse);

        BIND_PROPERTY(RailVehicleElectroPneumaticDynamicBrake, Variant::BOOL, ep_brake_fuse);

        BIND_ENUM_CONSTANT(NONE)
        BIND_ENUM_CONSTANT(FRONT)
        BIND_ENUM_CONSTANT(BACK)

        ClassDB::bind_method(
                D_METHOD("get_ed_braking_ep_delay"), &RailVehicleElectroPneumaticDynamicBrake::get_ed_braking_ep_delay);
        ClassDB::bind_method(
                D_METHOD("get_ep_max_brake_engagement_speed"),
                &RailVehicleElectroPneumaticDynamicBrake::get_ep_max_brake_engagement_speed);
        ClassDB::bind_method(
                D_METHOD("get_ep_min_regenerative_braking"),
                &RailVehicleElectroPneumaticDynamicBrake::get_ep_min_regenerative_braking);
        ClassDB::bind_method(D_METHOD("get_ep_force"), &RailVehicleElectroPneumaticDynamicBrake::get_ep_force);
        ClassDB::bind_method(D_METHOD("get_ep_fuse"), &RailVehicleElectroPneumaticDynamicBrake::get_ep_fuse);
    }

    void RailVehicleElectroPneumaticDynamicBrake::_register_commands() {
        register_command("switch_ep_fuse", callable_mp(this, &RailVehicleElectroPneumaticDynamicBrake::switch_ep_fuse));
        VehicleComponent::_register_commands();
    }

    void RailVehicleElectroPneumaticDynamicBrake::_unregister_commands() {
        unregister_command("switch_ep_fuse");
        VehicleComponent::_unregister_commands();
    }
} // namespace godot
