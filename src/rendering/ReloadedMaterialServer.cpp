#include "ReloadedMaterialServer.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace godot {
    ReloadedMaterialServer *ReloadedMaterialServer::singleton = nullptr;

    ReloadedMaterialServer *ReloadedMaterialServer::get_instance() {
        return singleton;
    }

    ReloadedMaterialServer::ReloadedMaterialServer() {
        singleton = this;
    }

    ReloadedMaterialServer::~ReloadedMaterialServer() {
        singleton = nullptr;
    }

    void ReloadedMaterialServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("material_set_provider", "material", "texture_size", "texture_length"),
                &ReloadedMaterialServer::material_set_provider);
        ClassDB::bind_method(
                D_METHOD("material_get", "name", "options"), &ReloadedMaterialServer::material_get,
                DEFVAL(Dictionary()));
        ClassDB::bind_method(
                D_METHOD("material_get_texture_size", "name"), &ReloadedMaterialServer::material_get_texture_size);
        ClassDB::bind_method(
                D_METHOD("material_get_texture_length", "name"), &ReloadedMaterialServer::material_get_texture_length);
    }

    void ReloadedMaterialServer::material_set_provider(
            const Callable &p_material, const Callable &p_texture_size, const Callable &p_texture_length) {
        material_provider = p_material;
        texture_size_provider = p_texture_size;
        texture_length_provider = p_texture_length;
    }

    Ref<Material> ReloadedMaterialServer::material_get(const String &p_name, const Dictionary &p_options) const {
        return material_provider.is_valid() ? Ref<Material>(material_provider.call(p_name, p_options))
                                            : Ref<Material>();
    }

    Vector2i ReloadedMaterialServer::material_get_texture_size(const String &p_name) const {
        return texture_size_provider.is_valid() ? Vector2i(texture_size_provider.call(p_name)) : Vector2i();
    }

    double ReloadedMaterialServer::material_get_texture_length(const String &p_name) const {
        return texture_length_provider.is_valid() ? static_cast<double>(texture_length_provider.call(p_name)) : -1.0;
    }
} // namespace godot
