#include "WorldData.hpp"

namespace godot {
    void WorldData::_bind_methods() {
        BIND_PROPERTY(WorldData, Variant::STRING, world_name);
        BIND_PROPERTY(WorldData, Variant::VECTOR2, origin);
        BIND_PROPERTY(WorldData, Variant::FLOAT, declared_start_time);
        BIND_PROPERTY_W_HINT(WorldData, Variant::ARRAY, section_cells, PROPERTY_HINT_ARRAY_TYPE, "Vector2i");
    }
} // namespace godot
