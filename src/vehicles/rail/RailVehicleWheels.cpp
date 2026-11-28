#include "RailVehicleWheels.hpp"

namespace godot {
    void RailVehicleWheels::_bind_methods() {
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, powered_wheel_diameter);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, front_rolling_wheel_diameter);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, rear_rolling_wheel_diameter);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, axle_inertial_moment);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, track_width);
        BIND_PROPERTY(RailVehicleWheels, Variant::STRING, axle_arrangement);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, bogie_axle_spacing);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, bogie_pivot_spacing);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, minimum_curve_radius);
        BIND_PROPERTY_W_HINT(RailVehicleWheels, Variant::INT, bearing_type, PROPERTY_HINT_ENUM, "Slide,Roll");

        BIND_ENUM_CONSTANT(BOGIE_FRONT);
        BIND_ENUM_CONSTANT(BOGIE_REAR);
        BIND_ENUM_CONSTANT(BEARING_TYPE_SLIDE);
        BIND_ENUM_CONSTANT(BEARING_TYPE_ROLL);

        ClassDB::bind_method(D_METHOD("get_angle_front_deg"), &RailVehicleWheels::get_angle_front_deg);
        ClassDB::bind_method(D_METHOD("get_angle_powered_deg"), &RailVehicleWheels::get_angle_powered_deg);
        ClassDB::bind_method(D_METHOD("get_angle_rear_deg"), &RailVehicleWheels::get_angle_rear_deg);
        ClassDB::bind_method(D_METHOD("get_axle_count"), &RailVehicleWheels::get_axle_count);
        ClassDB::bind_method(D_METHOD("get_axle_angle_deg", "axle"), &RailVehicleWheels::get_axle_angle_deg);
        ClassDB::bind_method(D_METHOD("get_rotation_speed_rps"), &RailVehicleWheels::get_rotation_speed_rps);
        ClassDB::bind_method(
                D_METHOD("get_rotation_acceleration_rps2"), &RailVehicleWheels::get_rotation_acceleration_rps2);
        ClassDB::bind_method(D_METHOD("get_slipping"), &RailVehicleWheels::get_slipping);
        ClassDB::bind_method(D_METHOD("get_flat"), &RailVehicleWheels::get_flat);
    }

    const String &RailVehicleWheels::get_axle_arrangement() const {
        return axle_arrangement;
    }

    /* The arrangement read in the UIC's notation, an axle after another from the front: a letter
     * counts powered axles (A one, B two...), a digit rolling ones - of the front before any
     * powered axle, of the rear after them; what else it writes ("o", "'", brackets) counts none */
    void RailVehicleWheels::set_axle_arrangement(const String &p_axle_arrangement) {
        axle_arrangement = p_axle_arrangement;
        axle_groups.clear();
        bool powered_read = false;
        for (int64_t index = 0; index < p_axle_arrangement.length(); ++index) {
            const char32_t sign = p_axle_arrangement[index];
            int count = 0;
            AxleGroup group = AXLE_GROUP_POWERED;
            if (sign >= 'A' && sign <= 'Z') {
                count = static_cast<int>(sign - 'A') + 1;
                powered_read = true;
            } else if (sign >= '1' && sign <= '9') {
                count = static_cast<int>(sign - '0');
                group = powered_read ? AXLE_GROUP_REAR_ROLLING : AXLE_GROUP_FRONT_ROLLING;
            }
            for (int axle = 0; axle < count; ++axle) {
                axle_groups.push_back(group);
            }
        }
    }

    int RailVehicleWheels::get_axle_count() const {
        return static_cast<int>(axle_groups.size());
    }

    double RailVehicleWheels::get_axle_angle_deg(const int p_axle) const {
        if (p_axle < 0 || p_axle >= axle_groups.size()) {
            return 0.0;
        }
        switch (axle_groups[p_axle]) {
            case AXLE_GROUP_FRONT_ROLLING:
                return get_angle_front_deg();
            case AXLE_GROUP_REAR_ROLLING:
                return get_angle_rear_deg();
            default:
                return get_angle_powered_deg();
        }
    }

    double RailVehicleWheels::_get_resolved_powered_wheel_diameter() const {
        return get_powered_wheel_diameter() > 0.0 ? get_powered_wheel_diameter() : DEFAULT_WHEEL_DIAMETER;
    }

    double RailVehicleWheels::_get_resolved_front_rolling_wheel_diameter() const {
        return get_front_rolling_wheel_diameter() > 0.0 ? get_front_rolling_wheel_diameter()
                                                        : _get_resolved_powered_wheel_diameter();
    }

    double RailVehicleWheels::_get_resolved_rear_rolling_wheel_diameter() const {
        return get_rear_rolling_wheel_diameter() > 0.0 ? get_rear_rolling_wheel_diameter()
                                                       : _get_resolved_powered_wheel_diameter();
    }
} // namespace godot
