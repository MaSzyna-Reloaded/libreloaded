#include "RailVehiclePantograph.hpp"

namespace godot {
    void RailVehiclePantograph::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                RailVehiclePantograph, Variant::INT, control, PROPERTY_HINT_ENUM,
                enum_hint(
                        {{"First", RailVehicleEnginePowerSource::PANTOGRAPH_FIRST},
                         {"Second", RailVehicleEnginePowerSource::PANTOGRAPH_SECOND}}));
        BIND_PROPERTY(RailVehiclePantograph, Variant::BOOL, has_geometry);
        BIND_PROPERTY(RailVehiclePantograph, Variant::VECTOR3, position);
        BIND_PROPERTY(RailVehiclePantograph, Variant::FLOAT, lower_length);
        BIND_PROPERTY(RailVehiclePantograph, Variant::FLOAT, upper_length);
        BIND_PROPERTY(RailVehiclePantograph, Variant::FLOAT, horizontal);
        BIND_PROPERTY(RailVehiclePantograph, Variant::FLOAT, lower_rest_angle);
        BIND_PROPERTY(RailVehiclePantograph, Variant::FLOAT, upper_rest_angle);
        BIND_PROPERTY(RailVehiclePantograph, Variant::FLOAT, slider_height);
    }
} // namespace godot
