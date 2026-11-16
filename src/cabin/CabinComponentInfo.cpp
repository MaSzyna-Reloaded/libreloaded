#include "CabinComponentInfo.hpp"

namespace godot {
    void CabinComponentInfo::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_name", "value"), &CabinComponentInfo::set_name);
        ClassDB::bind_method(D_METHOD("get_name"), &CabinComponentInfo::get_name);
        ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "name"), "set_name", "get_name");
        ClassDB::bind_method(D_METHOD("set_title", "value"), &CabinComponentInfo::set_title);
        ClassDB::bind_method(D_METHOD("get_title"), &CabinComponentInfo::get_title);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");
        ClassDB::bind_method(D_METHOD("set_widget_class", "value"), &CabinComponentInfo::set_widget_class);
        ClassDB::bind_method(D_METHOD("get_widget_class"), &CabinComponentInfo::get_widget_class);
        ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "widget_class"), "set_widget_class", "get_widget_class");
        ClassDB::bind_method(D_METHOD("set_commands", "value"), &CabinComponentInfo::set_commands);
        ClassDB::bind_method(D_METHOD("get_commands"), &CabinComponentInfo::get_commands);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "commands"), "set_commands", "get_commands");
    }

    void CabinComponentInfo::set_name(const StringName &p_value) {
        name = p_value;
    }

    StringName CabinComponentInfo::get_name() const {
        return name;
    }

    void CabinComponentInfo::set_title(const String &p_value) {
        title = p_value;
    }

    String CabinComponentInfo::get_title() const {
        return title;
    }

    void CabinComponentInfo::set_widget_class(const StringName &p_value) {
        widget_class = p_value;
    }

    StringName CabinComponentInfo::get_widget_class() const {
        return widget_class;
    }

    void CabinComponentInfo::set_commands(const PackedStringArray &p_value) {
        commands = p_value;
    }

    PackedStringArray CabinComponentInfo::get_commands() const {
        return commands;
    }
} // namespace godot
