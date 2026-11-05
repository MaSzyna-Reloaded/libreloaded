#include "UserSettingsProvider.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
    void UserSettingsProvider::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_sections"), &UserSettingsProvider::get_sections);
        GDVIRTUAL_BIND(_get_sections);
    }

    Array UserSettingsProvider::get_sections() const {
        Array sections;
        GDVIRTUAL_CALL(_get_sections, sections);
        return sections;
    }

    void UserSettingsFileProvider::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_path", "path"), &UserSettingsFileProvider::set_path);
        ClassDB::bind_method(D_METHOD("get_path"), &UserSettingsFileProvider::get_path);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "path", PROPERTY_HINT_FILE, "*.json"), "set_path", "get_path");
    }

    Array UserSettingsFileProvider::get_sections() const {
        if (path.is_empty()) {
            return {};
        }
        const Variant parsed = JSON::parse_string(FileAccess::get_file_as_string(path));
        return parsed.get_type() == Variant::DICTIONARY ? Array(Dictionary(parsed).get("sections", Array())) : Array();
    }

    void UserSettingsFileProvider::set_path(const String &p_path) {
        path = p_path;
    }

    String UserSettingsFileProvider::get_path() const {
        return path;
    }
} // namespace godot
