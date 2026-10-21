#include "RailVehicleElectricInductionEngine.hpp"

namespace godot {
    void RailVehicleElectricInductionEngine::_bind_methods() {
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, pole_pairs);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, initial_force);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, max_power);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, max_braking_force);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, max_braking_power);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, motor_max_current);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, nominal_voltage);
        ClassDB::bind_method(D_METHOD("get_force_max"), &RailVehicleElectricInductionEngine::get_force_max);
        ClassDB::bind_method(D_METHOD("get_force_full"), &RailVehicleElectricInductionEngine::get_force_full);
        ClassDB::bind_method(D_METHOD("get_field_current"), &RailVehicleElectricInductionEngine::get_field_current);
        ClassDB::bind_method(D_METHOD("get_motor_voltage"), &RailVehicleElectricInductionEngine::get_motor_voltage);
    }

    RailVehicleEngine::EngineType RailVehicleElectricInductionEngine::get_type() const {
        return RailVehicleEngine::EngineType::ELECTRIC_INDUCTION_MOTOR;
    }

    void RailVehicleElectricInductionEngine::_fill_state_dictionary(Dictionary &p_state) const {
        RailVehicleElectricEngine::_fill_state_dictionary(p_state);
        if (!is_simulation_ready()) {
            return;
        }
        p_state["inverters"] = get_inverters();
        p_state["force_max"] = get_force_max();
        p_state["force_full"] = get_force_full();
        p_state["field_current"] = get_field_current();
        p_state["motor_voltage"] = get_motor_voltage();
    }

    void RailVehicleElectricInductionEngine::set_nominal_voltage(const double p_value) {
        nominal_voltage = p_value;
    }
    double RailVehicleElectricInductionEngine::get_nominal_voltage() const {
        return nominal_voltage;
    }
} // namespace godot
