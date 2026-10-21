#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class VehicleController;
    class RailVehicleSpeedControl : public RailVehicleComponent {
            GDCLASS(RailVehicleSpeedControl, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_SPEED_CONTROL;
            }

        private:
            static void _bind_methods();

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual bool get_active() const = 0;
            virtual double get_desired_velocity() const = 0;
            virtual double get_desired_power() const = 0;
            virtual double get_selected_velocity() const = 0;
            /* The speed set by an impulse lever, in tens (NewSpeed) */
            virtual double get_set_velocity() const = 0;
            /* The speed control is on but waiting, holding no power (SpeedCtrlUnit.Standby) */
            virtual bool get_standby() const = 0;
            /* The player's buttons (OnCommand_speedcontrol*, Train.cpp:6887-6974) */
            virtual void speed_control_increase() = 0;
            virtual void speed_control_decrease() = 0;
            virtual void speed_control_power_increase() = 0;
            virtual void speed_control_power_decrease() = 0;
            virtual void speed_control_button(int p_button) = 0;
            /* The speed set outright [km/h], as a driver's order (RunCommand("SpeedCntrl"), Mover.cpp:12695) */
            virtual void speed_control_set(double p_velocity) = 0;

        protected:
            void _register_commands() override;
            void _unregister_commands() override;

        private:
            bool speed_control_enabled = false;
            /* LoadFIZ_SpeedControl's defaults (MOVER.h:1257-1286, 1933-1934) */
            double delay = 2.0;
            bool impulse_lever = false;
            int disables_on = 0;
            PackedFloat64Array preset_speeds =
                    PackedFloat64Array({30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0, 110.0, 120.0});
            double velocity_step = 5.0;
            double power_step = 0.1;
            double min_power = 0.0;
            double max_power = 1.0;
            double min_velocity = 0.0;
            double max_velocity = 120.0;
            bool brake_intervention = false;

        public:
            MAKE_GS(bool, speed_control_enabled);
            MAKE_GS(double, delay);
            MAKE_GS(bool, impulse_lever);
            MAKE_GS(int, disables_on);
            MAKE_GS(PackedFloat64Array, preset_speeds);
            MAKE_GS(double, velocity_step);
            MAKE_GS(double, power_step);
            MAKE_GS(double, min_power);
            MAKE_GS(double, max_power);
            MAKE_GS(double, min_velocity);
            MAKE_GS(double, max_velocity);
            MAKE_GS(bool, brake_intervention);
    };
} // namespace godot
