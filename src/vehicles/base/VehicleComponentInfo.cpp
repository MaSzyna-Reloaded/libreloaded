#include "VehicleComponentInfo.hpp"

namespace godot {
    void VehicleComponentInfo::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_name", "value"), &VehicleComponentInfo::set_name);
        ClassDB::bind_method(D_METHOD("get_name"), &VehicleComponentInfo::get_name);
        ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "name"), "set_name", "get_name");
        ClassDB::bind_method(D_METHOD("set_title", "value"), &VehicleComponentInfo::set_title);
        ClassDB::bind_method(D_METHOD("get_title"), &VehicleComponentInfo::get_title);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "title"), "set_title", "get_title");
        ClassDB::bind_method(D_METHOD("set_component_type", "value"), &VehicleComponentInfo::set_component_type);
        ClassDB::bind_method(D_METHOD("get_component_type"), &VehicleComponentInfo::get_component_type);
        ADD_PROPERTY(PropertyInfo(Variant::INT, "component_type"), "set_component_type", "get_component_type");
    }

    void VehicleComponentInfo::set_name(const StringName &p_value) {
        name = p_value;
    }

    StringName VehicleComponentInfo::get_name() const {
        return name;
    }

    void VehicleComponentInfo::set_title(const String &p_value) {
        title = p_value;
    }

    String VehicleComponentInfo::get_title() const {
        return title;
    }

    void VehicleComponentInfo::set_component_type(const int p_value) {
        component_type = p_value;
    }

    int VehicleComponentInfo::get_component_type() const {
        return component_type;
    }
} // namespace godot
