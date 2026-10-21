#include "CompiledScenery.hpp"

namespace godot {
    void CompiledScenery::_bind_methods() {
        BIND_PROPERTY_W_HINT(CompiledScenery, Variant::OBJECT, nodes, PROPERTY_HINT_RESOURCE_TYPE, "PackedScene");
        BIND_PROPERTY_W_HINT_RES_ARRAY(CompiledScenery, Variant::ARRAY, tracks, PROPERTY_HINT_TYPE_STRING, "TrackData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                CompiledScenery, Variant::ARRAY, traction, PROPERTY_HINT_TYPE_STRING, "TractionData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                CompiledScenery, Variant::ARRAY, power_sources, PROPERTY_HINT_TYPE_STRING, "PowerSourceData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(CompiledScenery, Variant::ARRAY, models, PROPERTY_HINT_TYPE_STRING, "ModelData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(CompiledScenery, Variant::ARRAY, sounds, PROPERTY_HINT_TYPE_STRING, "SoundData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(CompiledScenery, Variant::ARRAY, lines, PROPERTY_HINT_TYPE_STRING, "LinesData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                CompiledScenery, Variant::ARRAY, trainsets, PROPERTY_HINT_TYPE_STRING, "TrainsetData");
        ClassDB::bind_method(D_METHOD("set_scripts", "value"), &CompiledScenery::set_scripts);
        ClassDB::bind_method(D_METHOD("get_scripts"), &CompiledScenery::get_scripts);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::ARRAY, "scripts", PROPERTY_HINT_ARRAY_TYPE, "String", PROPERTY_USAGE_DEFAULT,
                        "TypedArray<String>"),
                "set_scripts", "get_scripts");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                CompiledScenery, Variant::ARRAY, stations, PROPERTY_HINT_TYPE_STRING, "StationData");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                CompiledScenery, Variant::ARRAY, signal_boxes, PROPERTY_HINT_TYPE_STRING, "SignalBoxData");
    }
} // namespace godot
