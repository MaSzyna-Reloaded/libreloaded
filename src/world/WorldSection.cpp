#include "WorldSection.hpp"

namespace godot {
    void WorldSection::_bind_methods() {
        BIND_PROPERTY(WorldSection, Variant::VECTOR2I, cell);
        BIND_PROPERTY_W_HINT_RES_ARRAY(WorldSection, Variant::ARRAY, layers, PROPERTY_HINT_TYPE_STRING, "WorldLayer");
    }
} // namespace godot
