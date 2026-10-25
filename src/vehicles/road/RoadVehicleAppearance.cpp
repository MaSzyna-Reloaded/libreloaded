#include "RoadVehicleAppearance.hpp"

namespace godot {
    void RoadVehicleAppearance::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_wheels", "value"), &RoadVehicleAppearance::set_wheels);
        ClassDB::bind_method(D_METHOD("get_wheels"), &RoadVehicleAppearance::get_wheels);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "wheels"), "set_wheels", "get_wheels");
    }

    void RoadVehicleAppearance::set_wheels(const PackedStringArray &p_value) {
        wheels = p_value;
    }

    PackedStringArray RoadVehicleAppearance::get_wheels() const {
        return wheels;
    }
} // namespace godot
