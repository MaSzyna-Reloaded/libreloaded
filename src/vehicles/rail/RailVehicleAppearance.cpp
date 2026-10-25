#include "RailVehicleAppearance.hpp"

namespace godot {
    void RailVehicleAppearance::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_front_bogie", "value"), &RailVehicleAppearance::set_front_bogie);
        ClassDB::bind_method(D_METHOD("get_front_bogie"), &RailVehicleAppearance::get_front_bogie);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "front_bogie"), "set_front_bogie", "get_front_bogie");
        ClassDB::bind_method(D_METHOD("set_rear_bogie", "value"), &RailVehicleAppearance::set_rear_bogie);
        ClassDB::bind_method(D_METHOD("get_rear_bogie"), &RailVehicleAppearance::get_rear_bogie);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "rear_bogie"), "set_rear_bogie", "get_rear_bogie");
        ClassDB::bind_method(
                D_METHOD("set_front_rolling_wheels", "value"), &RailVehicleAppearance::set_front_rolling_wheels);
        ClassDB::bind_method(D_METHOD("get_front_rolling_wheels"), &RailVehicleAppearance::get_front_rolling_wheels);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "front_rolling_wheels"), "set_front_rolling_wheels",
                "get_front_rolling_wheels");
        ClassDB::bind_method(D_METHOD("set_powered_wheels", "value"), &RailVehicleAppearance::set_powered_wheels);
        ClassDB::bind_method(D_METHOD("get_powered_wheels"), &RailVehicleAppearance::get_powered_wheels);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "powered_wheels"), "set_powered_wheels",
                "get_powered_wheels");
        ClassDB::bind_method(
                D_METHOD("set_rear_rolling_wheels", "value"), &RailVehicleAppearance::set_rear_rolling_wheels);
        ClassDB::bind_method(D_METHOD("get_rear_rolling_wheels"), &RailVehicleAppearance::get_rear_rolling_wheels);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "rear_rolling_wheels"), "set_rear_rolling_wheels",
                "get_rear_rolling_wheels");
    }

    void RailVehicleAppearance::set_front_bogie(const String &p_value) {
        front_bogie = p_value;
    }

    String RailVehicleAppearance::get_front_bogie() const {
        return front_bogie;
    }

    void RailVehicleAppearance::set_rear_bogie(const String &p_value) {
        rear_bogie = p_value;
    }

    String RailVehicleAppearance::get_rear_bogie() const {
        return rear_bogie;
    }

    void RailVehicleAppearance::set_front_rolling_wheels(const PackedStringArray &p_value) {
        front_rolling_wheels = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_front_rolling_wheels() const {
        return front_rolling_wheels;
    }

    void RailVehicleAppearance::set_powered_wheels(const PackedStringArray &p_value) {
        powered_wheels = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_powered_wheels() const {
        return powered_wheels;
    }

    void RailVehicleAppearance::set_rear_rolling_wheels(const PackedStringArray &p_value) {
        rear_rolling_wheels = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_rear_rolling_wheels() const {
        return rear_rolling_wheels;
    }
} // namespace godot
