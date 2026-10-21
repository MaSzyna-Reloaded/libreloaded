#include "plugins/ReloadedPlugin.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void ReloadedPlugin::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_name"), &ReloadedPlugin::get_name);
        ClassDB::bind_method(D_METHOD("get_description"), &ReloadedPlugin::get_description);
        ClassDB::bind_method(D_METHOD("get_logo"), &ReloadedPlugin::get_logo);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "name"), "", "get_name");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "description"), "", "get_description");
        ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "logo", PROPERTY_HINT_RESOURCE_TYPE, "Texture2D"), "", "get_logo");
    }

    String ReloadedPlugin::get_name() const {
        return name;
    }

    String ReloadedPlugin::get_description() const {
        return description;
    }

    Ref<Texture2D> ReloadedPlugin::get_logo() const {
        return logo;
    }
} // namespace godot
