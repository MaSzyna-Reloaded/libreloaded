#include "UserSettings.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/error_macros.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *UserSettings::config_changed_signal = "config_changed";
    const char *UserSettings::setting_changed_signal = "setting_changed";
    const char *UserSettings::data_dir_changed_signal = "data_dir_changed";

    UserSettings::UserSettings() {
        config.instantiate();

        Dictionary e3d;
        e3d["auto_generate_normal"] = false;
        e3d["auto_generate_metallic"] = false;
        e3d["auto_generate_height"] = false;

        Dictionary reloaded;
        reloaded[DATA_DIR_KEY] = ".";

        Dictionary render;
        render["msaa_3d"] = RenderingServer::VIEWPORT_MSAA_DISABLED;
        render["anisotropic_filtering_level"] = RenderingServer::VIEWPORT_ANISOTROPY_DISABLED;
        render["use_taa"] = true;

        defaults["e3d"] = e3d;
        defaults[DATA_DIR_SECTION] = reloaded;
        defaults["render"] = render;

        load_config();
    }

    void UserSettings::_bind_methods() {
        ClassDB::bind_method(D_METHOD("load_config"), &UserSettings::load_config);

        ClassDB::bind_method(D_METHOD("save_setting", "section", "key", "value"), &UserSettings::save_setting);
        ClassDB::bind_method(D_METHOD("set_setting", "section", "key", "value"), &UserSettings::set_setting);
        ClassDB::bind_method(D_METHOD("save_config"), &UserSettings::save_config);
        ClassDB::bind_method(D_METHOD("is_config_changed"), &UserSettings::is_config_changed);
        ClassDB::bind_method(D_METHOD("erase_setting", "section", "key"), &UserSettings::erase_setting);

        ClassDB::bind_method(
                D_METHOD("get_setting", "section", "key", "default_value"), &UserSettings::get_setting,
                DEFVAL(Variant()));

        ClassDB::bind_method(D_METHOD("persist_setting", "section", "key", "value"), &UserSettings::persist_setting);
        ClassDB::bind_method(D_METHOD("get_data_dir"), &UserSettings::get_data_dir);

        ADD_SIGNAL(MethodInfo(config_changed_signal));
        ADD_SIGNAL(MethodInfo(
                setting_changed_signal, PropertyInfo(Variant::STRING, "section"),
                PropertyInfo(Variant::STRING, "key")));
        ADD_SIGNAL(MethodInfo(data_dir_changed_signal));
    }

    void UserSettings::_apply_defaults() {
        Array sections = defaults.keys();

        for (int i = 0; i < sections.size(); i++) {
            String section = sections[i];
            Dictionary section_defaults = defaults[section];
            Array keys = section_defaults.keys();

            for (int j = 0; j < keys.size(); j++) {
                String key = keys[j];

                if (!config->has_section_key(section, key)) {
                    config->set_value(section, key, section_defaults[key]);
                }
            }
        }
    }

    void UserSettings::load_config() {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");

        /* What was set and not saved is dropped: the configuration is the file again, over the
         * defaults, and every project setting the player changed is the project's own until the
         * file sets it again. */
        config->clear();
        _apply_defaults();
        ProjectSettings *project_settings = ProjectSettings::get_singleton();
        for (const Variant &name: project_values.keys()) {
            project_settings->set_setting(name, project_values[name]);
        }
        project_values.clear();

        Error err = config->load(config_file_path);

        if (err != OK) {
            Error save_err = config->save(config_file_path);
            ERR_FAIL_COND_MSG(save_err != OK, "Cannot save default user settings.");
        }
        // what was set and not saved ("Discard changes") may have been another data directory
        const String previous_data_dir = data_dir;
        _update_data_dir();

        /* The player's own values of the project's settings, keyed by their full name - only the
         * ones the game's settings declare a player's (DECLARATIONS_SETTING); a value the game no
         * longer declares stays in the file and is not applied (2026-10-17: the draw distance taken
         * out of the game's Settings still overrode the project's). The constructor runs before
         * every server's, so each server reads them at its own init. */
        PackedStringArray declared;
        const String declarations = project_settings->get_setting(DECLARATIONS_SETTING, "");
        const Variant parsed =
                declarations.is_empty() ? Variant() : JSON::parse_string(FileAccess::get_file_as_string(declarations));
        const Array sections =
                parsed.get_type() == Variant::DICTIONARY ? Array(Dictionary(parsed).get("sections", Array())) : Array();
        for (const Variant &section: sections) {
            for (const Variant &setting: Array(Dictionary(section).get("settings", Array()))) {
                const Dictionary entry = setting;
                if (entry.get("store", "") == Variant("project")) {
                    declared.append(entry.get("key", ""));
                }
            }
        }
        if (config->has_section(PROJECT_SETTINGS_SECTION)) {
            for (const String &key: config->get_section_keys(PROJECT_SETTINGS_SECTION)) {
                if (declared.has(key)) {
                    _set_project_setting(key, config->get_value(PROJECT_SETTINGS_SECTION, key));
                }
            }
        }
        saved_text = config->encode_to_text();

        emit_signal(config_changed_signal);
        if (data_dir != previous_data_dir) {
            emit_signal(data_dir_changed_signal);
        }
    }

    void UserSettings::set_setting(const String &p_section, const String &p_key, const Variant &p_value) {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");

        // an empty Variant is no default to ConfigFile: a key not saved yet is looked up only if it is there
        const Variant old_value =
                config->has_section_key(p_section, p_key) ? config->get_value(p_section, p_key) : Variant();
        if (old_value == p_value) {
            return;
        }
        config->set_value(p_section, p_key, p_value);

        // what is created from now on takes it; what read it at its init keeps the old value
        if (p_section == PROJECT_SETTINGS_SECTION) {
            _set_project_setting(p_key, p_value);
        }
        const bool is_data_dir = p_section == DATA_DIR_SECTION && p_key == DATA_DIR_KEY;
        if (is_data_dir) {
            _update_data_dir();
        }
        emit_signal(setting_changed_signal, p_section, p_key);
        emit_signal(config_changed_signal);

        if (is_data_dir) {
            emit_signal(data_dir_changed_signal);
        }
    }

    void UserSettings::erase_setting(const String &p_section, const String &p_key) {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");
        if (!config->has_section_key(p_section, p_key)) {
            return;
        }
        const Dictionary section_defaults = defaults.get(p_section, Dictionary());
        if (section_defaults.has(p_key)) {
            set_setting(p_section, p_key, section_defaults[p_key]);
            return;
        }
        config->erase_section_key(p_section, p_key);
        if (p_section == PROJECT_SETTINGS_SECTION && project_values.has(p_key)) {
            ProjectSettings::get_singleton()->set_setting(p_key, project_values[p_key]);
            project_values.erase(p_key);
        }
        emit_signal(setting_changed_signal, p_section, p_key);
        emit_signal(config_changed_signal);
        if (p_section == DATA_DIR_SECTION && p_key == DATA_DIR_KEY) {
            emit_signal(data_dir_changed_signal);
        }
    }

    void UserSettings::save_config() {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");

        Error err = config->save(config_file_path);
        ERR_FAIL_COND_MSG(err != OK, "Cannot save user settings.");
        saved_text = config->encode_to_text();
    }

    /* Told from the text of the whole configuration, so a value set and set back is no change */
    bool UserSettings::is_config_changed() const {
        ERR_FAIL_COND_V_MSG(config.is_null(), false, "UserSettings config is null.");
        return config->encode_to_text() != saved_text;
    }

    void UserSettings::save_setting(const String &p_section, const String &p_key, const Variant &p_value) {
        set_setting(p_section, p_key, p_value);
        save_config();
    }

    /* The editor keeps the project's values: one set there would be saved into project.godot. The
     * project's own value is kept the first time, so load_config() can put it back. */
    void UserSettings::_set_project_setting(const String &p_name, const Variant &p_value) {
        if (Engine::get_singleton()->is_editor_hint()) {
            return;
        }
        ProjectSettings *project_settings = ProjectSettings::get_singleton();
        if (!project_values.has(p_name)) {
            project_values[p_name] = project_settings->get_setting(p_name);
        }
        project_settings->set_setting(p_name, p_value);
    }

    Variant
    UserSettings::get_setting(const String &p_section, const String &p_key, const Variant &p_default_value) const {
        ERR_FAIL_COND_V_MSG(config.is_null(), p_default_value, "UserSettings config is null.");
        return config->get_value(p_section, p_key, p_default_value);
    }

    void UserSettings::persist_setting(const String &p_section, const String &p_key, const Variant &p_value) {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");
        config->set_value(p_section, p_key, p_value);
        Ref<ConfigFile> saved;
        saved.instantiate();
        saved->load(config_file_path);
        saved->set_value(p_section, p_key, p_value);
        ERR_FAIL_COND_MSG(saved->save(config_file_path) != OK, "Cannot save the user settings.");
    }

    /* Made absolute here, against the directory the game itself sits in, so a build dropped
     * anywhere still finds the data next to it. It must not be left relative: a path with no
     * prefix is read as res://, which in a shipped build is the packed data and holds no game
     * files (see FINDINGS.md, 2026-09-23). */
    void UserSettings::_update_data_dir() {
        OS *os = OS::get_singleton();
        ERR_FAIL_NULL(os);
        String dir = get_setting(DATA_DIR_SECTION, DATA_DIR_KEY, ".");
        if (dir.is_empty()) {
            dir = ".";
        }
        if (dir.is_relative_path()) {
            dir = os->get_executable_path().get_base_dir().path_join(dir);
        }
        data_dir = dir.simplify_path();
    }

    String UserSettings::get_data_dir() const {
        return data_dir;
    }
} // namespace godot
