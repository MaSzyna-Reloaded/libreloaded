#include "SignalBoxData.hpp"

namespace godot {
    void SignalBoxData::_bind_methods() {
        BIND_PROPERTY(SignalBoxData, Variant::STRING, box_name);
        BIND_PROPERTY(SignalBoxData, Variant::STRING, ref);
        BIND_PROPERTY(SignalBoxData, Variant::STRING, kind);
        BIND_PROPERTY(SignalBoxData, Variant::VECTOR2, lat_lon);
        BIND_PROPERTY(SignalBoxData, Variant::VECTOR3, position);
        BIND_PROPERTY(SignalBoxData, Variant::STRING, station_ref);
    }
} // namespace godot
