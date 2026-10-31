#include "RailVehicleWipers.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void RailVehicleWipers::_bind_methods() {
        ClassDB::bind_method(D_METHOD("switch_increase"), &RailVehicleWipers::switch_increase);
        ClassDB::bind_method(D_METHOD("switch_decrease"), &RailVehicleWipers::switch_decrease);
        BIND_PROPERTY(RailVehicleWipers, Variant::FLOAT, angle);
        BIND_PROPERTY(RailVehicleWipers, Variant::INT, default_position);
        BIND_PROPERTY(RailVehicleWipers, Variant::INT, wiper_count);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleWipers, Variant::ARRAY, positions, PROPERTY_HINT_TYPE_STRING, "RailVehicleWiperListItem");

        BIND_PROPERTY_W_HINT(RailVehicleWipers, Variant::INT, drive, PROPERTY_HINT_ENUM, "Automatic,Manual");
        BIND_ENUM_CONSTANT(DRIVE_AUTOMATIC);
        BIND_ENUM_CONSTANT(DRIVE_MANUAL);
        BIND_CONSTANT(MANUAL_WIPERS_MAX);
        ClassDB::bind_method(D_METHOD("manual_fit", "count"), &RailVehicleWipers::manual_fit);
        ClassDB::bind_method(
                D_METHOD("manual_set_position", "position", "wiper"), &RailVehicleWipers::manual_set_position);
        ClassDB::bind_method(D_METHOD("manual_get_position", "wiper"), &RailVehicleWipers::manual_get_position);
        ClassDB::bind_method(D_METHOD("manual_set_all", "position"), &RailVehicleWipers::manual_set_all);
        ClassDB::bind_method(D_METHOD("get_switch_position"), &RailVehicleWipers::get_switch_position);
        ClassDB::bind_method(D_METHOD("get_sweep_positions"), &RailVehicleWipers::get_sweep_positions);
    }

    void RailVehicleWipers::_register_commands() {
        register_command("wipers_switch_increase", callable_mp(this, &RailVehicleWipers::switch_increase));
        register_command("wipers_switch_decrease", callable_mp(this, &RailVehicleWipers::switch_decrease));
        register_command("wipers_manual_set_all", callable_mp(this, &RailVehicleWipers::manual_set_all));
        for (int wiper = 0; wiper < MANUAL_WIPERS_MAX; ++wiper) {
            register_command(
                    vformat("wipers_manual_set_%d", wiper),
                    callable_mp(this, &RailVehicleWipers::manual_set_position).bind(wiper));
        }
    }

    void RailVehicleWipers::_unregister_commands() {
        unregister_command("wipers_switch_increase");
        unregister_command("wipers_switch_decrease");
        unregister_command("wipers_manual_set_all");
        for (int wiper = 0; wiper < MANUAL_WIPERS_MAX; ++wiper) {
            unregister_command(vformat("wipers_manual_set_%d", wiper));
        }
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

    void RailVehicleWipers::set_drive(const Drive p_drive) {
        drive = p_drive;
    }

    RailVehicleWipers::Drive RailVehicleWipers::get_drive() const {
        return drive;
    }

    void RailVehicleWipers::manual_fit(const int p_count) {
        if (!get_positions().is_empty()) {
            return;
        }
        drive = DRIVE_MANUAL;
        set_wiper_count(std::clamp(p_count, 0, static_cast<int>(MANUAL_WIPERS_MAX)));
        manual_positions.resize(get_wiper_count());
        manual_positions.fill(0.0);
        apply_config();
    }

    void RailVehicleWipers::manual_set_position(const double p_position, const int p_wiper) {
        if (drive != DRIVE_MANUAL || p_wiper < 0 || p_wiper >= manual_positions.size()) {
            return;
        }
        manual_positions.set(p_wiper, std::clamp(p_position, 0.0, 1.0));
    }

    void RailVehicleWipers::manual_set_all(const double p_position) {
        for (int wiper = 0; wiper < manual_positions.size(); ++wiper) {
            manual_set_position(p_position, wiper);
        }
    }

    double RailVehicleWipers::manual_get_position(const int p_wiper) const {
        return p_wiper >= 0 && p_wiper < manual_positions.size() ? manual_positions[p_wiper] : 0.0;
    }

    void RailVehicleWipers::_fill_state_dictionary(Dictionary &p_state) const {
        p_state["wipers_switch_position"] = get_switch_position();
        p_state["wiper_positions"] = get_sweep_positions();
        for (int64_t wiper = 0; wiper < manual_positions.size(); ++wiper) {
            p_state[vformat("wipers_manual_position_%d", wiper)] = manual_positions[wiper];
        }
    }

    void RailVehicleWipers::_fill_config_dictionary(Dictionary &p_config) const {
        p_config["wipers_switch_position_max"] = std::max(0, static_cast<int>(get_positions().size()) - 1);
        p_config["wipers_angle"] = get_angle();
        p_config["wipers_drive"] = drive;
    }
} // namespace godot
