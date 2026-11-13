#include "utils/ReloadedProjectSetting.hpp"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
    void ReloadedProjectSetting::_bind_methods() {
        ClassDB::bind_static_method(
                "ReloadedProjectSetting", D_METHOD("add", "name", "default_value", "type", "hint", "hint_string"),
                &ReloadedProjectSetting::add, DEFVAL(PROPERTY_HINT_NONE), DEFVAL(String()));
    }

    void ReloadedProjectSetting::add(
            const String &p_name, const Variant &p_default_value, const Variant::Type p_type, const PropertyHint p_hint,
            const String &p_hint_string) {
        ProjectSettings *project_settings = ProjectSettings::get_singleton();
        ERR_FAIL_NULL(project_settings);
        // project.godot keeps only the value - the hint and the initial value are registered again
        if (!project_settings->has_setting(p_name)) {
            project_settings->set_setting(p_name, p_default_value);
        }
        Dictionary info;
        info["name"] = p_name;
        info["type"] = p_type;
        info["hint"] = p_hint;
        info["hint_string"] = p_hint_string;
        project_settings->add_property_info(info);
        project_settings->set_initial_value(p_name, p_default_value);
    }
} // namespace godot
