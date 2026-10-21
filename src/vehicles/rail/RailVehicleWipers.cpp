#include "RailVehicleWipers.hpp"
#include <algorithm>

namespace godot {
    void RailVehicleWipers::_bind_methods() {
        ClassDB::bind_method(D_METHOD("switch_increase"), &RailVehicleWipers::switch_increase);
        ClassDB::bind_method(D_METHOD("switch_decrease"), &RailVehicleWipers::switch_decrease);
        BIND_PROPERTY(RailVehicleWipers, Variant::FLOAT, angle);
        BIND_PROPERTY(RailVehicleWipers, Variant::INT, default_position);
        BIND_PROPERTY(RailVehicleWipers, Variant::INT, wiper_count);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleWipers, Variant::ARRAY, positions, PROPERTY_HINT_TYPE_STRING, "RailVehicleWiperListItem");

        ClassDB::bind_method(D_METHOD("get_switch_position"), &RailVehicleWipers::get_switch_position);
        ClassDB::bind_method(D_METHOD("get_sweep_positions"), &RailVehicleWipers::get_sweep_positions);
    }

    void RailVehicleWipers::_register_commands() {
        register_command("wipers_switch_increase", callable_mp(this, &RailVehicleWipers::switch_increase));
        register_command("wipers_switch_decrease", callable_mp(this, &RailVehicleWipers::switch_decrease));
    }

    void RailVehicleWipers::_unregister_commands() {
        unregister_command("wipers_switch_increase");
        unregister_command("wipers_switch_decrease");
    }

    int RailVehicleWipers::get_default_position() const {
        return default_position;
    }

    void RailVehicleWipers::set_default_position(const int p_value) {
        default_position = p_value;
        _set_switch_position(default_position);
    }

    TypedArray<RailVehicleWiperListItem> RailVehicleWipers::get_positions() const {
        return positions;
    }

    void RailVehicleWipers::set_positions(const TypedArray<RailVehicleWiperListItem> &p_value) {
        positions = p_value;
        _set_switch_position(default_position);
    }

    void RailVehicleWipers::_set_switch_position(const int p_position) {
        switch_position = std::clamp(p_position, 0, std::max(0, static_cast<int>(get_positions().size()) - 1));
    }

    void RailVehicleWipers::switch_increase() {
        _set_switch_position(switch_position + 1);
    }

    void RailVehicleWipers::switch_decrease() {
        _set_switch_position(switch_position - 1);
    }

    int RailVehicleWipers::get_switch_position() const {
        return switch_position;
    }

    void RailVehicleWipers::_fill_state_dictionary(Dictionary &p_state) const {
        p_state["wipers_switch_position"] = get_switch_position();
        p_state["wiper_positions"] = get_sweep_positions();
    }

    void RailVehicleWipers::_fill_config_dictionary(Dictionary &p_config) const {
        p_config["wipers_switch_position_max"] = std::max(0, static_cast<int>(get_positions().size()) - 1);
        p_config["wipers_angle"] = get_angle();
    }
} // namespace godot
