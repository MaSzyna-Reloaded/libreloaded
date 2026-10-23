#include "TrackData.hpp"

namespace godot {
    void TrackData::_bind_methods() {
        BIND_ENUM_CONSTANT(STRUCTURE_NONE);
        BIND_ENUM_CONSTANT(STRUCTURE_BRIDGE);
        BIND_ENUM_CONSTANT(STRUCTURE_TUNNEL);
        BIND_ENUM_CONSTANT(BED_CRUSHED_STONE);
        BIND_ENUM_CONSTANT(BED_GRAVEL);
        BIND_ENUM_CONSTANT(BED_DIRT);
        BIND_ENUM_CONSTANT(BED_SLAB);
        BIND_ENUM_CONSTANT(BED_EMBEDDED);
        BIND_ENUM_CONSTANT(SURFACE_ASPHALT);
        BIND_ENUM_CONSTANT(SURFACE_CONCRETE);
        BIND_ENUM_CONSTANT(SURFACE_PAVING);
        BIND_ENUM_CONSTANT(SURFACE_GRAVEL);
        BIND_ENUM_CONSTANT(SURFACE_DIRT);
        BIND_ENUM_CONSTANT(SLEEPER_CONCRETE);
        BIND_ENUM_CONSTANT(SLEEPER_WOOD);
        BIND_ENUM_CONSTANT(SLEEPER_STEEL);

        ClassDB::bind_method(D_METHOD("set_kind", "value"), &TrackData::set_kind);
        ClassDB::bind_method(D_METHOD("get_kind"), &TrackData::get_kind);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "kind", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackServer.TrackType"),
                "set_kind", "get_kind");
        // the resource's own name (resource_name), bound as the property name too: Resource's own
        // accessors, bound as this class's (godot-cpp binds a method to the class it is of)
        ClassDB::bind_method(
                D_METHOD("set_name", "name"), static_cast<void (TrackData::*)(const String &)>(&Resource::set_name));
        ClassDB::bind_method(D_METHOD("get_name"), static_cast<String (TrackData::*)() const>(&Resource::get_name));
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_name",
                "get_name");
        BIND_PROPERTY_W_HINT(TrackData, Variant::OBJECT, curve, PROPERTY_HINT_RESOURCE_TYPE, "TrackCurve");
        BIND_PROPERTY_W_HINT(TrackData, Variant::OBJECT, diverging_curve, PROPERTY_HINT_RESOURCE_TYPE, "TrackCurve");
        BIND_PROPERTY(TrackData, Variant::FLOAT, width);
        BIND_PROPERTY(TrackData, Variant::FLOAT, friction);
        BIND_PROPERTY(TrackData, Variant::FLOAT, joint_spacing);
        BIND_PROPERTY_W_HINT(TrackData, Variant::INT, damage, PROPERTY_HINT_FLAGS, "No rail:128");
        BIND_PROPERTY(TrackData, Variant::BOOL, visible);
        BIND_PROPERTY(TrackData, Variant::FLOAT, speed_limit);
        ClassDB::bind_method(D_METHOD("set_structure", "value"), &TrackData::set_structure);
        ClassDB::bind_method(D_METHOD("get_structure"), &TrackData::get_structure);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "structure", PROPERTY_HINT_ENUM, "None,Bridge,Tunnel",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackData.Structure"),
                "set_structure", "get_structure");
        ClassDB::bind_method(D_METHOD("set_bed", "value"), &TrackData::set_bed);
        ClassDB::bind_method(D_METHOD("get_bed"), &TrackData::get_bed);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "bed", PROPERTY_HINT_ENUM, "Crushed Stone,Gravel,Dirt,Slab,Embedded",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackData.Bed"),
                "set_bed", "get_bed");
        ClassDB::bind_method(D_METHOD("set_surface", "value"), &TrackData::set_surface);
        ClassDB::bind_method(D_METHOD("get_surface"), &TrackData::get_surface);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "surface", PROPERTY_HINT_ENUM, "Asphalt,Concrete,Paving,Gravel,Dirt",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackData.Surface"),
                "set_surface", "get_surface");
        BIND_PROPERTY(TrackData, Variant::BOOL, centre_line);
        BIND_PROPERTY(TrackData, Variant::BOOL, shoulder);
        BIND_PROPERTY(TrackData, Variant::DICTIONARY, meta);
        BIND_PROPERTY(TrackData, Variant::FLOAT, formation_height);
        BIND_PROPERTY(TrackData, Variant::FLOAT, formation_width);
        BIND_PROPERTY(TrackData, Variant::FLOAT, formation_slope);
        BIND_PROPERTY(TrackData, Variant::FLOAT, sleeper_spacing);
        ClassDB::bind_method(D_METHOD("set_sleeper_kind", "value"), &TrackData::set_sleeper_kind);
        ClassDB::bind_method(D_METHOD("get_sleeper_kind"), &TrackData::get_sleeper_kind);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "sleeper_kind", PROPERTY_HINT_ENUM, "Concrete,Wood,Steel",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackData.SleeperKind"),
                "set_sleeper_kind", "get_sleeper_kind");
        BIND_PROPERTY(TrackData, Variant::STRING_NAME, rendering_backend);
        ClassDB::bind_method(D_METHOD("translated", "offset"), &TrackData::translated);
    }

    Ref<TrackData> TrackData::translated(const Vector3 &p_offset) const {
        Ref<TrackData> moved = duplicate();
        moved->curve = curve.is_valid() ? curve->translated(p_offset) : curve;
        moved->diverging_curve = diverging_curve.is_valid() ? diverging_curve->translated(p_offset) : diverging_curve;
        return moved;
    }
} // namespace godot
