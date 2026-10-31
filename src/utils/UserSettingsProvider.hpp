#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

    /// Settings a game built on this one, or a plugin, adds to the game's own (UserSettings,
    /// DECLARATIONS_SETTING): its sections in the game's settings' format - each a "title" and its
    /// "settings" of a "title", a "store", a "key", ... - merged into the game's by their titles
    /// (UserSettings::get_declarations()), the settings screen showing them and the player's values of
    /// their "project" ones applied. A plugin gives one (ReloadedPlugin::get_user_settings_provider()),
    /// anything else adds one (UserSettings::provider_add()); _get_sections() in a script.
    class UserSettingsProvider : public RefCounted {
            GDCLASS(UserSettingsProvider, RefCounted)

        protected:
            static void _bind_methods();

            GDVIRTUAL0RC(Array, _get_sections)

        public:
            virtual Array get_sections() const;
    };

    /// The sections of a JSON file of the game's settings' format ({"sections": [...]})
    class UserSettingsFileProvider : public UserSettingsProvider {
            GDCLASS(UserSettingsFileProvider, UserSettingsProvider)

        private:
            String path;

        protected:
            static void _bind_methods();

        public:
            Array get_sections() const override;

            void set_path(const String &p_path);
            String get_path() const;
    };

} // namespace godot
