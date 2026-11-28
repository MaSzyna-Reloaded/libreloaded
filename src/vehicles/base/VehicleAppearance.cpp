#include "VehicleAppearance.hpp"

namespace godot {
    void VehicleAppearance::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_data_path", "value"), &VehicleAppearance::set_data_path);
        ClassDB::bind_method(D_METHOD("get_data_path"), &VehicleAppearance::get_data_path);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "data_path"), "set_data_path", "get_data_path");
        ClassDB::bind_method(D_METHOD("set_model_filename", "value"), &VehicleAppearance::set_model_filename);
        ClassDB::bind_method(D_METHOD("get_model_filename"), &VehicleAppearance::get_model_filename);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "model_filename"), "set_model_filename", "get_model_filename");
        ClassDB::bind_method(
                D_METHOD("set_low_poly_model_filename", "value"), &VehicleAppearance::set_low_poly_model_filename);
        ClassDB::bind_method(D_METHOD("get_low_poly_model_filename"), &VehicleAppearance::get_low_poly_model_filename);
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "low_poly_model_filename"), "set_low_poly_model_filename",
                "get_low_poly_model_filename");
        ClassDB::bind_method(
                D_METHOD("set_passengers_model_filename", "value"), &VehicleAppearance::set_passengers_model_filename);
        ClassDB::bind_method(
                D_METHOD("get_passengers_model_filename"), &VehicleAppearance::get_passengers_model_filename);
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "passengers_model_filename"), "set_passengers_model_filename",
                "get_passengers_model_filename");
        ClassDB::bind_method(
                D_METHOD("set_attachment_model_filenames", "value"),
                &VehicleAppearance::set_attachment_model_filenames);
        ClassDB::bind_method(
                D_METHOD("get_attachment_model_filenames"), &VehicleAppearance::get_attachment_model_filenames);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "attachment_model_filenames"),
                "set_attachment_model_filenames", "get_attachment_model_filenames");
        ClassDB::bind_method(D_METHOD("set_skins", "value"), &VehicleAppearance::set_skins);
        ClassDB::bind_method(D_METHOD("get_skins"), &VehicleAppearance::get_skins);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "skins"), "set_skins", "get_skins");
        ClassDB::bind_method(
                D_METHOD("set_replaceable_textures", "value"), &VehicleAppearance::set_replaceable_textures);
        ClassDB::bind_method(D_METHOD("get_replaceable_textures"), &VehicleAppearance::get_replaceable_textures);
        ADD_PROPERTY(
                PropertyInfo(Variant::DICTIONARY, "replaceable_textures"), "set_replaceable_textures",
                "get_replaceable_textures");
        ClassDB::bind_method(D_METHOD("set_hidden_parts", "value"), &VehicleAppearance::set_hidden_parts);
        ClassDB::bind_method(D_METHOD("get_hidden_parts"), &VehicleAppearance::get_hidden_parts);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "hidden_parts"), "set_hidden_parts", "get_hidden_parts");
        ClassDB::bind_method(D_METHOD("set_model_transform", "value"), &VehicleAppearance::set_model_transform);
        ClassDB::bind_method(D_METHOD("get_model_transform"), &VehicleAppearance::get_model_transform);
        ADD_PROPERTY(
                PropertyInfo(Variant::TRANSFORM3D, "model_transform"), "set_model_transform", "get_model_transform");
        ClassDB::bind_method(D_METHOD("set_rendering_backend", "value"), &VehicleAppearance::set_rendering_backend);
        ClassDB::bind_method(D_METHOD("get_rendering_backend"), &VehicleAppearance::get_rendering_backend);
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING_NAME, "rendering_backend"), "set_rendering_backend",
                "get_rendering_backend");
    }

    void VehicleAppearance::set_data_path(const String &p_value) {
        data_path = p_value;
    }

    String VehicleAppearance::get_data_path() const {
        return data_path;
    }

    void VehicleAppearance::set_model_filename(const String &p_value) {
        model_filename = p_value;
    }

    String VehicleAppearance::get_model_filename() const {
        return model_filename;
    }

    void VehicleAppearance::set_low_poly_model_filename(const String &p_value) {
        low_poly_model_filename = p_value;
    }

    String VehicleAppearance::get_low_poly_model_filename() const {
        return low_poly_model_filename;
    }

    void VehicleAppearance::set_passengers_model_filename(const String &p_value) {
        passengers_model_filename = p_value;
    }

    String VehicleAppearance::get_passengers_model_filename() const {
        return passengers_model_filename;
    }

    void VehicleAppearance::set_attachment_model_filenames(const PackedStringArray &p_value) {
        attachment_model_filenames = p_value;
    }

    PackedStringArray VehicleAppearance::get_attachment_model_filenames() const {
        return attachment_model_filenames;
    }

    void VehicleAppearance::set_skins(const PackedStringArray &p_value) {
        skins = p_value;
    }

    PackedStringArray VehicleAppearance::get_skins() const {
        return skins;
    }

    void VehicleAppearance::set_replaceable_textures(const Dictionary &p_value) {
        replaceable_textures = p_value;
    }

    Dictionary VehicleAppearance::get_replaceable_textures() const {
        return replaceable_textures;
    }

    void VehicleAppearance::set_hidden_parts(const PackedStringArray &p_value) {
        hidden_parts = p_value;
    }

    PackedStringArray VehicleAppearance::get_hidden_parts() const {
        return hidden_parts;
    }

    void VehicleAppearance::set_model_transform(const Transform3D &p_value) {
        model_transform = p_value;
    }

    Transform3D VehicleAppearance::get_model_transform() const {
        return model_transform;
    }

    void VehicleAppearance::set_rendering_backend(const StringName &p_value) {
        rendering_backend = p_value;
    }

    StringName VehicleAppearance::get_rendering_backend() const {
        return rendering_backend;
    }
} // namespace godot
