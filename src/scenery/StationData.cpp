#include "StationData.hpp"

namespace godot {
    void StationData::_bind_methods() {
        BIND_PROPERTY(StationData, Variant::STRING, station_name);
        BIND_PROPERTY(StationData, Variant::STRING, ref);
        BIND_PROPERTY(StationData, Variant::STRING, uic_ref);
        BIND_PROPERTY(StationData, Variant::VECTOR2, lat_lon);
        BIND_PROPERTY(StationData, Variant::VECTOR3, position);
    }
} // namespace godot
