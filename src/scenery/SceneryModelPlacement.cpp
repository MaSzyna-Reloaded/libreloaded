#include "scenery/SceneryModelPlacement.hpp"

namespace godot {
    void SceneryModelPlacement::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_rendering_backend", "value"), &SceneryModelPlacement::set_rendering_backend);
        ClassDB::bind_method(D_METHOD("get_rendering_backend"), &SceneryModelPlacement::get_rendering_backend);
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING_NAME, "rendering_backend"), "set_rendering_backend",
                "get_rendering_backend");
        ClassDB::bind_method(D_METHOD("set_model", "value"), &SceneryModelPlacement::set_model);
        ClassDB::bind_method(D_METHOD("get_model"), &SceneryModelPlacement::get_model);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "model"), "set_model", "get_model");
        ClassDB::bind_method(D_METHOD("set_skins", "value"), &SceneryModelPlacement::set_skins);
        ClassDB::bind_method(D_METHOD("get_skins"), &SceneryModelPlacement::get_skins);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "skins"), "set_skins", "get_skins");
        ClassDB::bind_method(D_METHOD("set_transform", "value"), &SceneryModelPlacement::set_transform);
        ClassDB::bind_method(D_METHOD("get_transform"), &SceneryModelPlacement::get_transform);
        ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "transform"), "set_transform", "get_transform");
        ClassDB::bind_method(D_METHOD("set_range_min", "value"), &SceneryModelPlacement::set_range_min);
        ClassDB::bind_method(D_METHOD("get_range_min"), &SceneryModelPlacement::get_range_min);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_min"), "set_range_min", "get_range_min");
        ClassDB::bind_method(D_METHOD("set_range_max", "value"), &SceneryModelPlacement::set_range_max);
        ClassDB::bind_method(D_METHOD("get_range_max"), &SceneryModelPlacement::get_range_max);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_max"), "set_range_max", "get_range_max");
        ClassDB::bind_method(D_METHOD("set_lights_state", "value"), &SceneryModelPlacement::set_lights_state);
        ClassDB::bind_method(D_METHOD("get_lights_state"), &SceneryModelPlacement::get_lights_state);
        ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "lights_state"), "set_lights_state", "get_lights_state");
        ClassDB::bind_method(D_METHOD("set_lights_colors", "value"), &SceneryModelPlacement::set_lights_colors);
        ClassDB::bind_method(D_METHOD("get_lights_colors"), &SceneryModelPlacement::get_lights_colors);
        ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "lights_colors"), "set_lights_colors", "get_lights_colors");
    }

    void SceneryModelPlacement::set_rendering_backend(const StringName &p_value) {
        rendering_backend = p_value;
    }

    StringName SceneryModelPlacement::get_rendering_backend() const {
        return rendering_backend;
    }

    void SceneryModelPlacement::set_model(const String &p_value) {
        model = p_value;
    }

    String SceneryModelPlacement::get_model() const {
        return model;
    }

    void SceneryModelPlacement::set_skins(const PackedStringArray &p_value) {
        skins = p_value;
    }

    PackedStringArray SceneryModelPlacement::get_skins() const {
        return skins;
    }

    void SceneryModelPlacement::set_transform(const Transform3D &p_value) {
        transform = p_value;
    }

    Transform3D SceneryModelPlacement::get_transform() const {
        return transform;
    }

    void SceneryModelPlacement::set_range_min(const float p_value) {
        range_min = p_value;
    }

    float SceneryModelPlacement::get_range_min() const {
        return range_min;
    }

    void SceneryModelPlacement::set_range_max(const float p_value) {
        range_max = p_value;
    }

    float SceneryModelPlacement::get_range_max() const {
        return range_max;
    }

    void SceneryModelPlacement::set_lights_state(const Dictionary &p_value) {
        lights_state = p_value;
    }

    Dictionary SceneryModelPlacement::get_lights_state() const {
        return lights_state;
    }

    void SceneryModelPlacement::set_lights_colors(const Dictionary &p_value) {
        lights_colors = p_value;
    }

    Dictionary SceneryModelPlacement::get_lights_colors() const {
        return lights_colors;
    }
} // namespace godot
