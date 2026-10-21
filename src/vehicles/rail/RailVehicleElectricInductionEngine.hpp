#pragma once
#include "RailVehicleElectricEngine.hpp"
#include "RailVehicleInverter.hpp"
#include "macros.hpp"
#include "vehicles/base/VehicleCurvePointItem.hpp"

namespace godot {
    class VehicleController;

    class RailVehicleElectricInductionEngine : public RailVehicleElectricEngine {
            GDCLASS(RailVehicleElectricInductionEngine, RailVehicleElectricEngine)
        public:
            static void _bind_methods();

        protected:
            EngineType get_type() const override;
            void _fill_state_dictionary(Dictionary &p_state) const override;

        public:
            /// The state of each inverter (TMoverParameters::Inverters)
            virtual TypedArray<RailVehicleInverter> get_inverters() const = 0;
            /* The force the motors are set to [kN], negative when braking (eimv[eimv_Fmax]) */
            virtual double get_force_max() const = 0;
            /* The full force at the current speed [kN] (eimv[eimv_Fful]) */
            virtual double get_force_full() const = 0;
            /* The motors' field current [A] (eimv[eimv_If]) */
            virtual double get_field_current() const = 0;
            /* The voltage the inverters feed the motors with [V] (eimv[eimv_U]) */
            virtual double get_motor_voltage() const = 0;

        private:
            double pole_pairs = 0.0;
            double initial_force = 0.0;
            double max_power = 0.0;
            double max_braking_force = 0.0;
            double max_braking_power = 0.0;
            double motor_max_current = 0.0;

        public:
            MAKE_GS(double, pole_pairs);
            MAKE_GS(double, initial_force);
            MAKE_GS(double, max_power);
            MAKE_GS(double, max_braking_force);
            MAKE_GS(double, max_braking_power);
            MAKE_GS(double, motor_max_current);

        private:
            /* The rest of the FIZ Engine: line of an induction motor. */
            double nominal_voltage = 0.0; // Volt

        public:
            void set_nominal_voltage(double p_value);
            double get_nominal_voltage() const;
    };
} // namespace godot
