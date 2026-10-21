#include "RailVehicleSpringBrake.hpp"

namespace godot {
    void RailVehicleSpringBrake::_bind_methods() {

        ClassDB::bind_method(
                D_METHOD("spring_brake_activate", "active"), &RailVehicleSpringBrake::spring_brake_activate);
        ClassDB::bind_method(
                D_METHOD("spring_brake_shut_off", "shut_off"), &RailVehicleSpringBrake::spring_brake_shut_off);
        ClassDB::bind_method(D_METHOD("spring_brake_release"), &RailVehicleSpringBrake::spring_brake_release);

        ClassDB::bind_method(D_METHOD("get_ready"), &RailVehicleSpringBrake::get_ready);
        ClassDB::bind_method(D_METHOD("get_shut_off"), &RailVehicleSpringBrake::get_shut_off);
        ClassDB::bind_method(D_METHOD("get_active"), &RailVehicleSpringBrake::get_active);
        ClassDB::bind_method(D_METHOD("get_braking"), &RailVehicleSpringBrake::get_braking);
        ClassDB::bind_method(D_METHOD("get_cylinder_pressure"), &RailVehicleSpringBrake::get_cylinder_pressure);
    }

    void RailVehicleSpringBrake::_register_commands() {
        register_command("spring_brake_activate", callable_mp(this, &RailVehicleSpringBrake::spring_brake_activate));
        register_command("spring_brake_shut_off", callable_mp(this, &RailVehicleSpringBrake::spring_brake_shut_off));
        register_command("spring_brake_release", callable_mp(this, &RailVehicleSpringBrake::spring_brake_release));
    }

    void RailVehicleSpringBrake::_unregister_commands() {
        unregister_command("spring_brake_activate");
        unregister_command("spring_brake_shut_off");
        unregister_command("spring_brake_release");
    }
} // namespace godot
