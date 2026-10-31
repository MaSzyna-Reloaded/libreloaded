#pragma once
#include "RailVehicleDriveUnit.hpp"
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class VehicleController;
    class RailVehicleEngine : public RailVehicleComponent {
            GDCLASS(RailVehicleEngine, RailVehicleComponent)


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_ENGINE;
            }

        protected:
            /* The drive unit, installed by the implementation that owns it */
            const RailVehicleDriveUnit *drive_unit = nullptr;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;

            /* Live state, read straight from the engine's units - nothing is stored. */
            bool get_main_switch_enabled() const;
            bool get_main_switch_closable() const;
            double get_motor_torque() const;
            double get_wheel_torque() const;
            double get_wheel_force() const;
            double get_tractive_force() const;
            double get_power() const;
            double get_rpm_count() const;
            /* The engine's turn [rad] - what a pendulum swings by (DynObj.cpp:1121-1125) */
            double get_angle() const;
            double get_rpm_ratio() const;
            int get_damage() const;
            double get_main_switch_time() const;
            bool get_main_no_power_pos() const;
            bool get_motor_overload_relay_high_threshold() const;
            bool get_relay_novolt() const;
            bool get_relay_overvoltage() const;
            bool get_relay_ground() const;
            int get_circuit_rlist_size() const;
            /* How warm the engine is [deg C]: from the moment it started (its main switch closed, or
             * its motors turning) it warms towards the air's ENGINE_WARM_RISE above it, over
             * ENGINE_WARM_UP_TIME; stopped, it cools back to the air's over ENGINE_COOL_DOWN_TIME -
             * from where it was when it started or stopped, at the simulation's clock */
            virtual double get_temperature() const;
            /* The engine's ammeters [A]: the total and the two motor branches. A vehicle without
             * an engine has none - its cab reads the powered vehicle's (TTrain::mvControlled,
             * Train.cpp:8638). */
            double get_current0() const;
            double get_current1() const;
            double get_current2() const;

            enum EngineType {
                NONE,
                DUMB,
                WHEELS_DRIVEN,
                ELECTRIC_SERIES_MOTOR,
                ELECTRIC_INDUCTION_MOTOR,
                DIESEL,
                STEAM,
                DIESEL_ELECTRIC,
                MAIN
            };

            /* AutoRelay= : automatic starting relay presence */
            enum AutoRelayMode {
                AUTO_RELAY_NO,
                AUTO_RELAY_YES,
                AUTO_RELAY_OPTIONAL,
            };


            bool main_switch(bool p_enabled);
            bool motor_overload_relay_threshold(bool p_high);
            /* The traction motors' blowers at an end switched on, or their "off" switch
             * (MotorBlowersSwitch(), MotorBlowersSwitchOff()) */
            void motor_blowers(bool p_enabled, RailVehicleController::CouplerEnd p_end);
            void motor_blowers_switch_off(bool p_enabled, RailVehicleController::CouplerEnd p_end);
            /* The traction motors' blowers at an end: switched on, switched off, working */
            bool get_motor_blowers_enabled(RailVehicleController::CouplerEnd p_end) const;
            bool get_motor_blowers_disabled(RailVehicleController::CouplerEnd p_end) const;
            bool get_motor_blowers_active(RailVehicleController::CouplerEnd p_end) const;

        private:
            static void _bind_methods();
            /* Engine: (the fields every kind of drive has) */
            int transmission_gear_teeth_motor = 0;
            int transmission_gear_teeth_wheel = 0;
            double transmission_efficiency = 1.0;

        public:
            MAKE_GS(int, transmission_gear_teeth_motor);
            MAKE_GS(int, transmission_gear_teeth_wheel);
            MAKE_GS(double, transmission_efficiency);
            /* Wheel teeth over motor teeth, 1.0 without a gear (LoadFIZ_Engine, Mover.cpp) */
            double get_transmission_ratio() const;

        private:
            double maximum_traction_force = 0.0;
            double motor_blowers_sustain_time = 0.0;
            double motor_blowers_start_velocity = -1.0;
            bool pressure_switch_present = false;
            int inverters_count = 0;
            RailVehicleController::StartMode motor_blowers_start_mode = RailVehicleController::START_MODE_MANUAL;

            AutoRelayMode cntrl_auto_relay_mode = AUTO_RELAY_NO;
            bool cntrl_has_camshaft = false;

        public:
            MAKE_GS(double, maximum_traction_force);
            MAKE_GS(double, motor_blowers_sustain_time);
            MAKE_GS(double, motor_blowers_start_velocity);
            MAKE_GS(bool, pressure_switch_present);
            MAKE_GS(int, inverters_count);
            MAKE_GS_NR(RailVehicleController::StartMode, motor_blowers_start_mode);
            MAKE_GS_NR(AutoRelayMode, cntrl_auto_relay_mode);
            MAKE_GS(bool, cntrl_has_camshaft);

        private:
            /* Cntrl. MainInitTime: how long the main circuit takes to get ready once it has power
             * [s] (MainsInitTime, Mover.cpp:10910) */
            double main_init_time = 0.0;

        public:
            void set_main_init_time(double p_value);
            double get_main_init_time() const;

        protected:
            /// Change detection for engine_start/engine_stop, compared in _do_process_component().
            /// Starts false so a vehicle coming up with the main switch already closed reports
            /// engine_start on its first tick, as it did when this was compared against the
            /// not-yet-filled state dictionary.
            bool previous_main_switch = false;

            /* How much above the air an engine running long warms [deg C], how long it takes to get
             * most of the way [s], and how long a stopped one takes to cool most of the way [s] */
            static constexpr double ENGINE_WARM_RISE = 60.0;
            static constexpr double ENGINE_WARM_UP_TIME = 1200.0;
            static constexpr double ENGINE_COOL_DOWN_TIME = 3600.0;
            /* Running or not, when it started or stopped (the simulation's time; -1: never) and how
             * warm it was then - get_temperature() goes on from them; changed in the tick */
            bool running = false;
            double running_changed_time = -1.0;
            double running_changed_temperature = 0.0;


        public:
            /* Which kind of engine this is - part of the contract, and what the simulation keys
             * its own configuration off */
            virtual EngineType get_type() const = 0;

        protected:
            void _apply_configuration() override;
            void _do_process_component(double p_delta) override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _register_commands() override;
            void _unregister_commands() override;
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleEngine::EngineType);
VARIANT_ENUM_CAST(RailVehicleEngine::AutoRelayMode);
