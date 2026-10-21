#include "PowerSourceData.hpp"

namespace godot {
    void PowerSourceData::_bind_methods() {
        // the resource's own name (resource_name), bound as the property name too: Resource's own
        // accessors, bound as this class's (godot-cpp binds a method to the class it is of)
        ClassDB::bind_method(
                D_METHOD("set_name", "name"),
                static_cast<void (PowerSourceData::*)(const String &)>(&Resource::set_name));
        ClassDB::bind_method(
                D_METHOD("get_name"), static_cast<String (PowerSourceData::*)() const>(&Resource::get_name));
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_name",
                "get_name");
        BIND_PROPERTY(PowerSourceData, Variant::VECTOR3, position);
        BIND_PROPERTY(PowerSourceData, Variant::FLOAT, nominal_voltage);
        BIND_PROPERTY(PowerSourceData, Variant::FLOAT, voltage_frequency);
        BIND_PROPERTY(PowerSourceData, Variant::FLOAT, internal_resistance);
        BIND_PROPERTY(PowerSourceData, Variant::FLOAT, max_output_current);
        BIND_PROPERTY(PowerSourceData, Variant::FLOAT, fast_fuse_timeout);
        BIND_PROPERTY(PowerSourceData, Variant::FLOAT, fast_fuse_repetition);
        BIND_PROPERTY(PowerSourceData, Variant::FLOAT, slow_fuse_timeout);
        BIND_PROPERTY(PowerSourceData, Variant::BOOL, is_section);
        BIND_PROPERTY(PowerSourceData, Variant::BOOL, recuperation);
        ClassDB::bind_method(D_METHOD("translated", "offset"), &PowerSourceData::translated);
    }

    Ref<PowerSourceData> PowerSourceData::translated(const Vector3 &p_offset) const {
        Ref<PowerSourceData> moved = duplicate();
        moved->position = position + p_offset;
        return moved;
    }
} // namespace godot
