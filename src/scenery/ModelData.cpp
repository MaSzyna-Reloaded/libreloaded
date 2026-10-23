#include "ModelData.hpp"

namespace godot {
    void ModelData::_bind_methods() {
        // the resource's own name (resource_name), bound as the property name too: Resource's own
        // accessors, bound as this class's (godot-cpp binds a method to the class it is of)
        ClassDB::bind_method(
                D_METHOD("set_name", "name"), static_cast<void (ModelData::*)(const String &)>(&Resource::set_name));
        ClassDB::bind_method(D_METHOD("get_name"), static_cast<String (ModelData::*)() const>(&Resource::get_name));
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_name",
                "get_name");
        BIND_PROPERTY(ModelData, Variant::STRING, model);
        BIND_PROPERTY(ModelData, Variant::STRING_NAME, rendering_backend);
        BIND_PROPERTY(ModelData, Variant::PACKED_STRING_ARRAY, skins);
        BIND_PROPERTY(ModelData, Variant::VECTOR3, position);
        BIND_PROPERTY(ModelData, Variant::VECTOR3, rotation);
        BIND_PROPERTY(ModelData, Variant::FLOAT, range_min);
        BIND_PROPERTY(ModelData, Variant::FLOAT, range_max);
        BIND_PROPERTY_W_HINT(
                ModelData, Variant::OBJECT, signal_head_kind, PROPERTY_HINT_RESOURCE_TYPE, "SignalHeadKind");
        BIND_PROPERTY(ModelData, Variant::DICTIONARY, lights_state);
        BIND_PROPERTY(ModelData, Variant::DICTIONARY, lights_colors);
        ClassDB::bind_method(D_METHOD("translated", "offset"), &ModelData::translated);
    }

    Ref<ModelData> ModelData::translated(const Vector3 &p_offset) const {
        Ref<ModelData> moved = duplicate();
        moved->position = position + p_offset;
        return moved;
    }
} // namespace godot
