#include "UserSettings.hpp"
#include "plugins/ReloadedPlugin.hpp"
#include "plugins/ReloadedPluginServer.hpp"
#include "utils/ReloadedProjectSetting.hpp"

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
        singleton = this;
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

        // the plugins' settings, theirs applied as the game's are: before any server reads them
        ReloadedPluginServer *plugin_server = ReloadedPluginServer::get_instance();
        if (plugin_server != nullptr) {
            const TypedArray<ReloadedPlugin> plugins = plugin_server->plugin_get_list();
            for (int64_t index = 0; index < plugins.size(); index++) {
                const ReloadedPlugin *plugin = Object::cast_to<ReloadedPlugin>(plugins[index]);
                const Ref<UserSettingsProvider> provider =
                        plugin != nullptr ? plugin->get_user_settings_provider() : Ref<UserSettingsProvider>();
                if (provider.is_valid()) {
                    providers.push_back(provider);
                }
            }
        }

        load_config();
        _add_presets(_read_declarations_file().get("presets", Array()));
        for (const Ref<UserSettingsProvider> &provider: providers) {
            _add_presets(provider->get_presets());
        }
    }

    UserSettings::~UserSettings() {
        singleton = nullptr;
    }

    void UserSettings::project_settings_register() {
        // only the project settings the game's settings declare a player's are taken from the
        // player's settings.cfg
        ReloadedProjectSetting::add(DECLARATIONS_SETTING, String(), Variant::STRING, PROPERTY_HINT_FILE, "*.json");
    }

    void UserSettings::_bind_methods() {
        ClassDB::bind_method(D_METHOD("load_config"), &UserSettings::load_config);
        ClassDB::bind_method(D_METHOD("set_config_path", "path"), &UserSettings::set_config_path);
        ClassDB::bind_method(D_METHOD("get_config_path"), &UserSettings::get_config_path);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "config_path"), "set_config_path", "get_config_path");

        ClassDB::bind_method(D_METHOD("save_setting", "section", "key", "value"), &UserSettings::save_setting);
        ClassDB::bind_method(D_METHOD("set_setting", "section", "key", "value"), &UserSettings::set_setting);
        ClassDB::bind_method(D_METHOD("save_config"), &UserSettings::save_config);
        ClassDB::bind_method(D_METHOD("is_config_changed"), &UserSettings::is_config_changed);
        ClassDB::bind_method(D_METHOD("erase_setting", "section", "key"), &UserSettings::erase_setting);
        ClassDB::bind_method(D_METHOD("get_project_value", "name"), &UserSettings::get_project_value);

        ClassDB::bind_method(
                D_METHOD("get_setting", "section", "key", "default_value"), &UserSettings::get_setting,
                DEFVAL(Variant()));

        ClassDB::bind_method(D_METHOD("persist_setting", "section", "key", "value"), &UserSettings::persist_setting);
        ClassDB::bind_method(D_METHOD("get_data_dir"), &UserSettings::get_data_dir);
        ClassDB::bind_method(D_METHOD("get_declarations"), &UserSettings::get_declarations);
        ClassDB::bind_method(D_METHOD("provider_add", "provider"), &UserSettings::provider_add);
        ClassDB::bind_method(D_METHOD("preset_list"), &UserSettings::preset_list);
        ClassDB::bind_method(D_METHOD("preset_get", "name"), &UserSettings::preset_get);
        ClassDB::bind_method(D_METHOD("preset_create", "name", "title", "values"), &UserSettings::preset_create);
        ClassDB::bind_method(D_METHOD("preset_delete", "name"), &UserSettings::preset_delete);
        ClassDB::bind_method(D_METHOD("preset_set", "name"), &UserSettings::preset_set);
        ClassDB::bind_method(D_METHOD("preset_get_current"), &UserSettings::preset_get_current);

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

    void UserSettings::set_config_path(const String &p_path) {
        config_file_path = p_path;
        load_config();
    }

    String UserSettings::get_config_path() const {
        return config_file_path;
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
            _restore_project_setting(name);
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
         * ones the game's settings declare a player's (get_declarations()); a value the game no
         * longer declares stays in the file and is not applied (2026-10-17: the draw distance taken
         * out of the game's Settings still overrode the project's). The constructor runs before
         * every server's, so each server reads them at its own init. */
        _apply_project_values(get_declarations().get("sections", Array()));
        saved_text = config->encode_to_text();

        emit_signal(config_changed_signal);
        if (data_dir != previous_data_dir) {
            emit_signal(data_dir_changed_signal);
        }
    }

    void UserSettings::_apply_project_values(const Array &p_sections) {
        PackedStringArray declared;
        for (const Variant &section: p_sections) {
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
    }

    Dictionary UserSettings::_read_declarations_file() const {
        const String declarations = ProjectSettings::get_singleton()->get_setting(DECLARATIONS_SETTING);
        const Variant parsed =
                declarations.is_empty() ? Variant() : JSON::parse_string(FileAccess::get_file_as_string(declarations));
        return parsed.get_type() == Variant::DICTIONARY ? Dictionary(parsed) : Dictionary();
    }

    Dictionary UserSettings::get_declarations() const {
        Array sections = _read_declarations_file().get("sections", Array());
        for (const Ref<UserSettingsProvider> &provider: providers) {
            for (const Variant &added: provider->get_sections()) {
                const Dictionary added_section = added;
                bool merged = false;
                for (const Variant &existing: sections) {
                    Dictionary section = existing;
                    if (section.get("title", "") == added_section.get("title", "")) {
                        Array settings = section.get("settings", Array());
                        settings.append_array(added_section.get("settings", Array()));
                        section["settings"] = settings;
                        merged = true;
                        break;
                    }
                }
                if (!merged) {
                    sections.append(added_section.duplicate(true));
                }
            }
        }
        Dictionary result;
        result["sections"] = sections;
        return result;
    }

    void UserSettings::provider_add(const Ref<UserSettingsProvider> &p_provider) {
        ERR_FAIL_COND(p_provider.is_null());
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");
        providers.push_back(p_provider);
        _apply_project_values(p_provider->get_sections());
        _add_presets(p_provider->get_presets());
        emit_signal(config_changed_signal);
    }

    void UserSettings::_add_presets(const Array &p_presets) {
        for (const Variant &added: p_presets) {
            const Dictionary added_preset = added;
            const String name = added_preset.get("name", "");
            ERR_CONTINUE_MSG(name.is_empty(), "A preset of the user settings has no name.");
            Dictionary preset = presets.get(name, Dictionary());
            if (added_preset.has("title")) {
                preset["title"] = added_preset["title"];
            }
            Dictionary values = preset.get("values", Dictionary());
            const Dictionary added_values = added_preset.get("values", Dictionary());
            for (const Variant &section: added_values.keys()) {
                Dictionary section_values = values.get(section, Dictionary());
                section_values.merge(added_values[section], true);
                values[section] = section_values;
            }
            preset["values"] = values;
            presets[name] = preset;
        }
    }

    Dictionary UserSettings::_get_declared_settings() const {
        Dictionary declared;
        for (const Variant &section: Array(get_declarations().get("sections", Array()))) {
            for (const Variant &setting: Array(Dictionary(section).get("settings", Array()))) {
                const Dictionary entry = setting;
                const String section_name = entry.get("store", "") == Variant("project")
                                                    ? String(PROJECT_SETTINGS_SECTION)
                                                    : String(entry.get("section", ""));
                Dictionary keys = declared.get(section_name, Dictionary());
                keys[entry.get("key", "")] = entry;
                declared[section_name] = keys;
            }
        }
        return declared;
    }

    Variant UserSettings::_as_declared(const Variant &p_value, const Dictionary &p_setting) {
        const String type = p_setting.get("type", "");
        if (type == "bool") {
            return UtilityFunctions::type_convert(p_value, Variant::BOOL);
        }
        if (type == "int") {
            return UtilityFunctions::type_convert(p_value, Variant::INT);
        }
        if (type == "float") {
            return UtilityFunctions::type_convert(p_value, Variant::FLOAT);
        }
        return p_value;
    }

    PackedStringArray UserSettings::preset_list() const {
        PackedStringArray names;
        for (const Variant &name: presets.keys()) {
            names.append(name);
        }
        return names;
    }

    Dictionary UserSettings::preset_get(const String &p_name) const {
        ERR_FAIL_COND_V_MSG(!presets.has(p_name), Dictionary(), "No preset of the user settings " + p_name + ".");
        return Dictionary(presets[p_name]).duplicate(true);
    }

    void UserSettings::preset_create(const String &p_name, const String &p_title, const Dictionary &p_values) {
        Dictionary preset;
        preset["title"] = p_title;
        preset["values"] = p_values.duplicate(true);
        presets[p_name] = preset;
    }

    void UserSettings::preset_delete(const String &p_name) {
        ERR_FAIL_COND_MSG(!presets.has(p_name), "No preset of the user settings " + p_name + ".");
        presets.erase(p_name);
    }

    void UserSettings::preset_set(const String &p_name) {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");
        ERR_FAIL_COND_MSG(!presets.has(p_name), "No preset of the user settings " + p_name + ".");
        const Dictionary declared = _get_declared_settings();
        const Dictionary values = Dictionary(presets[p_name]).get("values", Dictionary());
        PackedStringArray changed_sections;
        PackedStringArray changed_keys;
        for (const Variant &section: values.keys()) {
            const Dictionary section_values = values[section];
            const Dictionary section_declared = declared.get(section, Dictionary());
            for (const Variant &key: section_values.keys()) {
                const Variant value = _as_declared(section_values[key], section_declared.get(key, Dictionary()));
                if (_write_setting(section, key, value)) {
                    changed_sections.append(section);
                    changed_keys.append(key);
                }
            }
        }
        for (int64_t index = 0; index < changed_keys.size(); index++) {
            emit_signal(setting_changed_signal, changed_sections[index], changed_keys[index]);
        }
        if (!changed_keys.is_empty()) {
            emit_signal(config_changed_signal);
        }
    }

    String UserSettings::preset_get_current() const {
        ERR_FAIL_COND_V_MSG(config.is_null(), String(), "UserSettings config is null.");
        ProjectSettings *project_settings = ProjectSettings::get_singleton();
        const Dictionary declared = _get_declared_settings();
        const auto has_now = [&](const String &p_section, const String &p_key, const Variant &p_value) {
            const Dictionary setting = Dictionary(declared.get(p_section, Dictionary())).get(p_key, Dictionary());
            Variant current = setting.get("default", Variant());
            if (p_section == PROJECT_SETTINGS_SECTION) {
                if (project_settings->has_setting(p_key)) {
                    current = project_settings->get_setting(p_key);
                }
            } else if (config->has_section_key(p_section, p_key)) {
                current = config->get_value(p_section, p_key);
            }
            if (current.get_type() == Variant::NIL) {
                return false;
            }
            current = _as_declared(current, setting);
            const Variant expected = _as_declared(p_value, setting);
            if (current.get_type() == Variant::FLOAT && expected.get_type() == Variant::FLOAT) {
                return UtilityFunctions::is_equal_approx(current, expected);
            }
            return current == expected;
        };
        for (const Variant &name: presets.keys()) {
            const Dictionary values = Dictionary(presets[name]).get("values", Dictionary());
            bool matches = true;
            for (const Variant &section: values.keys()) {
                const Dictionary section_values = values[section];
                for (const Variant &key: section_values.keys()) {
                    matches = matches && has_now(section, key, section_values[key]);
                }
            }
            if (matches) {
                return name;
            }
        }
        return String();
    }

    bool UserSettings::_write_setting(const String &p_section, const String &p_key, const Variant &p_value) {
        // an empty Variant is no default to ConfigFile: a key not saved yet is looked up only if it is there
        const Variant old_value =
                config->has_section_key(p_section, p_key) ? config->get_value(p_section, p_key) : Variant();
        if (old_value == p_value) {
            return false;
        }
        config->set_value(p_section, p_key, p_value);

        // what is created from now on takes it; what read it at its init keeps the old value
        if (p_section == PROJECT_SETTINGS_SECTION) {
            _set_project_setting(p_key, p_value);
        }
        return true;
    }

    void UserSettings::set_setting(const String &p_section, const String &p_key, const Variant &p_value) {
        ERR_FAIL_COND_MSG(config.is_null(), "UserSettings config is null.");
        if (!_write_setting(p_section, p_key, p_value)) {
            return;
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
            _restore_project_setting(p_key);
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

    void UserSettings::_restore_project_setting(const String &p_name) {
        ProjectSettings::get_singleton()->set_setting(p_name, get_project_value(p_name));
    }

    /* A setting the project had no value of as the player's was applied - one a library registers in
     * GDScript as the game starts, after this constructor (ReloadedProjectSettings) - has its initial
     * value, not none: set to none, the setting would be gone */
    Variant UserSettings::get_project_value(const String &p_name) const {
        ProjectSettings *project_settings = ProjectSettings::get_singleton();
        if (!project_values.has(p_name)) {
            return project_settings->get_setting(p_name);
        }
        const Variant value = project_values[p_name];
        return value.get_type() == Variant::NIL ? project_settings->property_get_revert(p_name) : value;
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
