#pragma once

#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/variant.hpp>

namespace godot {

    class UserSettings : public Object {
            GDCLASS(UserSettings, Object)

        private:
            String config_file_path = "user://settings.cfg";
            Ref<ConfigFile> config;
            Dictionary defaults;
            /// The project's own value of each project setting the player's value replaced
            Dictionary project_values;
            /// The game's data directory in use, absolute - resolved where the configuration or the
            /// data_dir setting changes
            String data_dir;
            /// The configuration as the file has it - last loaded or saved - what a change is told from
            String saved_text;

            static constexpr const char *DATA_DIR_SECTION = "reloaded";
            static constexpr const char *DATA_DIR_KEY = "data_dir";
            /// Section of values set onto ProjectSettings, each under its full name
            /// ("reloaded/scenery/lights/distance")
            static constexpr const char *PROJECT_SETTINGS_SECTION = "project_settings";
            /// The project setting naming the game's settings (its JSON: "sections", each with
            /// "settings" of a "store" and a "key"): only those of the "project" store are a
            /// player's, applied from PROJECT_SETTINGS_SECTION; none without it
            static constexpr const char *DECLARATIONS_SETTING = "reloaded/user_settings/declarations";

            void _apply_defaults();
            void _set_project_setting(const String &p_name, const Variant &p_value);
            void _update_data_dir();

        protected:
            static void _bind_methods();

        public:
            static const char *config_changed_signal;
            static const char *setting_changed_signal;
            static const char *data_dir_changed_signal;

            /// How many positions a key press moves a stepped train brake handle - the player's, not
            /// the vehicle's (brakestep of the original's eu07.ini)
            static constexpr const char *CONTROLS_SECTION = "controls";
            static constexpr const char *BRAKE_STEP_KEY = "brakestep";
            static constexpr double DEFAULT_BRAKE_STEP = 1.0;

            UserSettings();

            static UserSettings *get_instance() {
                return dynamic_cast<UserSettings *>(Engine::get_singleton()->get_singleton("UserSettings"));
            }

            void load_config();

            /// Set in memory and in effect at once; saved by save_config(), dropped by load_config()
            void set_setting(const String &p_section, const String &p_key, const Variant &p_value);
            void save_config();
            /// Something set differs from what the file holds - set and not saved yet
            bool is_config_changed() const;
            /// The player's value goes, in memory like set_setting(): a key with a default takes the
            /// default, a project setting the project's own value
            void erase_setting(const String &p_section, const String &p_key);
            void save_setting(const String &p_section, const String &p_key, const Variant &p_value);
            Variant
            get_setting(const String &p_section, const String &p_key, const Variant &p_default_value = Variant()) const;

            /// Set in memory and saved at once, and only this key - the rest of the configuration may
            /// hold changes not saved yet
            void persist_setting(const String &p_section, const String &p_key, const Variant &p_value);

            /// The game's own data - its worlds and their assets - absolute: the data_dir the player
            /// chose, relative to the directory the game sits in (data_dir_changed)
            String get_data_dir() const;
    };

} // namespace godot
