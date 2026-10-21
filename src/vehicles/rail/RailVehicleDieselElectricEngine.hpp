#pragma once
#include "RailVehicleDieselEngine.hpp"
#include "macros.hpp"
#include "vehicles/rail/RailVehicleTractionMotorsUnit.hpp"

namespace godot {
    class RailVehicleDieselElectricEngine : public RailVehicleDieselEngine {
            GDCLASS(RailVehicleDieselElectricEngine, RailVehicleDieselEngine)
        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            /* The traction motors (RailVehicleTractionMotorsUnit) */
            double get_motor_current() const;
            /* The generator's voltage on the motors [V] (EngineVoltage) */
            double get_engine_voltage() const;
            double get_circuit_imax() const;
            bool get_dynamic_brake_active() const;
            bool get_fuse_active() const;
            bool get_motor_connectors_open() const;
            bool is_line_contactor_closed() const;
            bool is_pressure_switch_tripped() const;
            /* "zbij nadmiarowy" and the line contactors */
            void fuse_reset();
            void set_motor_connectors_open(bool p_open);

        private:
            static void _bind_methods();

            bool shunt_mode_allowed = false;
            double heating_rpm = 0.0;

        public:
            MAKE_GS(bool, shunt_mode_allowed);
            MAKE_GS(double, heating_rpm);

        protected:
            /* The traction motors driven by the generator, installed by the implementation that owns them */
            const RailVehicleTractionMotorsUnit *traction_motors_unit = nullptr;

            RailVehicleEngine::EngineType get_type() const override;
            void _register_commands() override;
            void _unregister_commands() override;
    };
} // namespace godot
