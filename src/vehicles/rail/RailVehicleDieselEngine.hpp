#pragma once
#include "RailVehicleDieselEngineUnit.hpp"
#include "RailVehicleEngine.hpp"
#include "macros.hpp"
#include "vehicles/base/VehicleCurvePointItem.hpp"

namespace godot {
    class VehicleController;

    class RailVehicleDieselEngine : public RailVehicleEngine {
            GDCLASS(RailVehicleDieselEngine, RailVehicleEngine)

        protected:
            /* The diesel engine unit, installed by the implementation that owns it */
            const RailVehicleDieselEngineUnit *diesel_engine_unit = nullptr;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;

            /* Live state, read straight from the engine's units - nothing is stored. */
            double get_rpm() const;
            bool get_oil_pump_active() const;
            bool get_oil_pump_disabled() const;
            double get_oil_pump_pressure() const;
            bool get_fuel_pump_active() const;
            bool get_fuel_pump_disabled() const;
            /* The engine overheat alarm (dizel_heat.PA, the i-malfunction lamp, Train.cpp:9210) */
            bool get_heat_malfunction() const;
            /* The pump switches' own state (FuelPump/OilPump.is_enabled) - what a two-state
             * switch flips (Train.cpp:3891, 3990) */
            bool get_fuel_pump_enabled() const;
            bool get_oil_pump_enabled() const;
            bool get_startup() const;
            bool get_ignition() const;
            bool get_spinup() const;
            double get_output_power() const;
            double get_torque() const;
            double get_fill() const;
            double get_fill_desired() const;
            double get_clutch_engagement() const;
            double get_water_temperature() const;
            double get_engine_temperature() const;
            double get_retarder_fill() const;
            double get_max_rpm() const;
            double get_idle_rpm_count() const;
            /* The cooling water's pump, its breaker, the water heater, its breaker and the link of
             * the two water circuits (WaterPump, WaterHeater, WaterCircuitsLink) */
            bool get_water_pump_enabled() const;
            bool get_water_pump_active() const;
            bool get_water_pump_breaker() const;
            bool get_water_heater_enabled() const;
            bool get_water_heater_active() const;
            bool get_water_heater_breaker() const;
            bool get_water_circuits_link() const;
            /* The main and the auxiliary water circuit's and the oil's temperature [C] - what the
             * original's driver warms the engine up by (PrepareHeating(), Driver.cpp:5040-5045) */
            double get_main_circuit_water_temperature() const;
            double get_auxiliary_circuit_water_temperature() const;
            double get_oil_temperature() const;


        private:
            static void _bind_methods();
            int turbo_position = 0;

        public:
            /* TurboPos: - the master controller position from which the turbocharger is heard; 0
             * without it (LoadFIZ_TurboPos, Mover.cpp:10714; DynObj.cpp:8267) */
            void set_turbo_position(int p_value);
            int get_turbo_position() const;

        private:
            float oil_pump_pressure_minimum = 0.0;
            float oil_pump_pressure_maximum = 0.65;
            RailVehicleController::StartMode fuel_pump_start_mode = RailVehicleController::START_MODE_MANUAL;
            RailVehicleController::StartMode oil_pump_start_mode = RailVehicleController::START_MODE_MANUAL;
            RailVehicleController::StartMode water_pump_start_mode = RailVehicleController::START_MODE_MANUAL;

            /* Engine: (Kont.), przekladnia mechaniczna */
            double mechanical_min_rpm = 0.0;
            double mechanical_max_rpm = 0.0;
            double mechanical_fuel_cutoff_rpm = 0.0;
            /* Engine: nmax [1/s]: the engine's top speed (not DList's) */
            double mechanical_nominal_max_rpm = 0.0;
            /* ShuntMode: the extra gear of a shunting mode (2Ls150), 0 without one */
            double mechanical_shunt_mode_ratio = 0.0;
            /* R_IsClutch: the retarder has its own clutch */
            bool retarder_clutch = false;
            /* R_WithIndividual */
            bool retarder_with_individual = false;
            bool torque_converter_present = false;
            bool retarder_present = false;
            double retarder_max_torque = 1.0;
            double retarder_max_power = 1.0;

            /* DList: tabela przepustnicy */
            double throttle_table_max_torque = 1.0;
            double throttle_table_max_torque_rpm = 1.0;
            double throttle_table_max_rpm_torque = 2.0;
            double throttle_table_nominal_fuel_consumption_rate = 250.0;

            /* DMList: charakterystyka momentu obrotowego */
            TypedArray<VehicleCurvePointItem> torque_table;

            /* WaterMin/Max/Flow/CoolingTemperature [deg C], WaterShutters */
            double cooling_water_min_temperature = -1.0;
            double cooling_water_max_temperature = -1.0;
            double cooling_water_flow_temperature = -1.0;
            double cooling_water_cooling_temperature = -1.0;
            bool cooling_water_shutters = false;
            /* WaterAuxCircuit: the cooling has a second water circuit, WaterAux*: its settings */
            bool cooling_water_aux_circuit = false;
            double cooling_water_aux_min_temperature = -1.0;
            double cooling_water_aux_max_temperature = -1.0;
            double cooling_water_aux_cooling_temperature = -1.0;
            bool cooling_water_aux_shutters = false;
            /* OilMin/MaxTemperature [deg C] */
            double cooling_oil_min_temperature = -1.0;
            double cooling_oil_max_temperature = -1.0;
            /* HeaterMin/MaxTemperature [deg C]: the water heater */
            double cooling_heater_min_temperature = -1.0;
            double cooling_heater_max_temperature = -1.0;

        public:
            MAKE_GS(float, oil_pump_pressure_minimum);
            MAKE_GS(float, oil_pump_pressure_maximum);
            MAKE_GS_NR(RailVehicleController::StartMode, fuel_pump_start_mode);
            MAKE_GS_NR(RailVehicleController::StartMode, oil_pump_start_mode);
            MAKE_GS_NR(RailVehicleController::StartMode, water_pump_start_mode);

            MAKE_GS(double, mechanical_min_rpm);
            MAKE_GS(double, mechanical_max_rpm);
            MAKE_GS(double, mechanical_fuel_cutoff_rpm);
            MAKE_GS(double, mechanical_nominal_max_rpm);
            MAKE_GS(double, mechanical_shunt_mode_ratio);
            MAKE_GS(bool, retarder_clutch);
            MAKE_GS(bool, retarder_with_individual);
            MAKE_GS(bool, torque_converter_present);
            MAKE_GS(bool, retarder_present);
            MAKE_GS(double, retarder_max_torque);
            MAKE_GS(double, retarder_max_power);

            MAKE_GS(double, throttle_table_max_torque);
            MAKE_GS(double, throttle_table_max_torque_rpm);
            MAKE_GS(double, throttle_table_max_rpm_torque);
            MAKE_GS(double, throttle_table_nominal_fuel_consumption_rate);

            MAKE_GS_NR(TypedArray<VehicleCurvePointItem>, torque_table);
            MAKE_GS(double, cooling_water_min_temperature);
            MAKE_GS(double, cooling_water_max_temperature);
            MAKE_GS(double, cooling_water_flow_temperature);
            MAKE_GS(double, cooling_water_cooling_temperature);
            MAKE_GS(bool, cooling_water_shutters);
            MAKE_GS(bool, cooling_water_aux_circuit);
            MAKE_GS(double, cooling_water_aux_min_temperature);
            MAKE_GS(double, cooling_water_aux_max_temperature);
            MAKE_GS(double, cooling_water_aux_cooling_temperature);
            MAKE_GS(bool, cooling_water_aux_shutters);
            MAKE_GS(double, cooling_oil_min_temperature);
            MAKE_GS(double, cooling_oil_max_temperature);
            MAKE_GS(double, cooling_heater_min_temperature);
            MAKE_GS(double, cooling_heater_max_temperature);

        protected:
            EngineType get_type() const override;
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            void oil_pump(bool p_enabled);
            void fuel_pump(bool p_enabled);
            /* The "off" side of a two-state pump switch (FuelPumpSwitchOff/OilPumpSwitchOff,
             * Train.cpp:3937, 3963) - an impulse switch has none */
            void oil_pump_switch_off(bool p_enabled);
            void fuel_pump_switch_off(bool p_enabled);
            /* WaterPumpSwitch(), WaterPumpBreakerSwitch(), WaterHeaterSwitch(),
             * WaterHeaterBreakerSwitch(), WaterCircuitsLinkSwitch() (Mover.cpp) */
            void water_pump(bool p_enabled);
            /* The "off" side of a two-state water pump switch (WaterPumpSwitchOff, Train.cpp:4312) */
            void water_pump_switch_off(bool p_enabled);
            void water_pump_breaker(bool p_enabled);
            void water_heater(bool p_enabled);
            void water_heater_breaker(bool p_enabled);
            void water_circuits_link(bool p_enabled);
    };
} // namespace godot
