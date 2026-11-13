#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {
    /// Registers a project setting with its default value and its hint, so the editor shows it and
    /// the running game knows its default: a setting at its default value is not saved in
    /// project.godot. Each setting is registered once, by its owner, from the constant it reads:
    /// one read in C++ as the library starts (initialize_libreloaded_module(), a plugin's
    /// register_classes()), before UserSettings applies the player's values; one read in GDScript
    /// by its library's list (ReloadedProjectSettings) in the editor and as the game starts
    class ReloadedProjectSetting : public Object {
            GDCLASS(ReloadedProjectSetting, Object)

        protected:
            static void _bind_methods();

        public:
            static void
            add(const String &p_name, const Variant &p_default_value, Variant::Type p_type,
                PropertyHint p_hint = PROPERTY_HINT_NONE, const String &p_hint_string = String());
    };
} // namespace godot
