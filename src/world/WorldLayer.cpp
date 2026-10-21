#include "WorldLayer.hpp"

namespace godot {
    void WorldLayer::_bind_methods() {
        // the resource's own name (resource_name), bound as the property name too: Resource's own
        // accessors, bound as this class's (godot-cpp binds a method to the class it is of)
        ClassDB::bind_method(
                D_METHOD("set_name", "name"), static_cast<void (WorldLayer::*)(const String &)>(&Resource::set_name));
        ClassDB::bind_method(D_METHOD("get_name"), static_cast<String (WorldLayer::*)() const>(&Resource::get_name));
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_name",
                "get_name");
        ClassDB::bind_method(D_METHOD("set_group", "value"), &WorldLayer::set_group);
        ClassDB::bind_method(D_METHOD("get_group"), &WorldLayer::get_group);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "group", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackServer.TrackTypeGroup"),
                "set_group", "get_group");
        BIND_PROPERTY_W_HINT_RES_ARRAY(WorldLayer, Variant::ARRAY, tracks, PROPERTY_HINT_TYPE_STRING, "TrackData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(WorldLayer, Variant::ARRAY, traction, PROPERTY_HINT_TYPE_STRING, "TractionData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                WorldLayer, Variant::ARRAY, power_sources, PROPERTY_HINT_TYPE_STRING, "PowerSourceData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(WorldLayer, Variant::ARRAY, meshes, PROPERTY_HINT_TYPE_STRING, "ArrayMesh");
        BIND_PROPERTY(WorldLayer, Variant::FLOAT, range_end);
        BIND_PROPERTY_W_HINT_RES_ARRAY(WorldLayer, Variant::ARRAY, models, PROPERTY_HINT_TYPE_STRING, "ModelData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(WorldLayer, Variant::ARRAY, lines, PROPERTY_HINT_TYPE_STRING, "LinesData");
        ClassDB::bind_method(D_METHOD("is_empty"), &WorldLayer::is_empty);
    }


    bool WorldLayer::is_empty() const {
        return tracks.is_empty() && traction.is_empty() && power_sources.is_empty() && meshes.is_empty() &&
               models.is_empty() && lines.is_empty();
    }
} // namespace godot
