#include "SoundData.hpp"

namespace godot {
    void SoundData::_bind_methods() {
        // the resource's own name (resource_name), bound as the property name too: Resource's own
        // accessors, bound as this class's (godot-cpp binds a method to the class it is of)
        ClassDB::bind_method(
                D_METHOD("set_name", "name"), static_cast<void (SoundData::*)(const String &)>(&Resource::set_name));
        ClassDB::bind_method(D_METHOD("get_name"), static_cast<String (SoundData::*)() const>(&Resource::get_name));
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_name",
                "get_name");
        BIND_PROPERTY(SoundData, Variant::VECTOR3, position);
        BIND_PROPERTY(SoundData, Variant::STRING, file);
        BIND_PROPERTY(SoundData, Variant::FLOAT, range_max);
    }
} // namespace godot
