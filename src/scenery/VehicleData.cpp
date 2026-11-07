#include "VehicleData.hpp"

namespace godot {
    void VehicleData::_bind_methods() {
        BIND_ENUM_CONSTANT(DRIVER_NOBODY);
        BIND_ENUM_CONSTANT(DRIVER_HEAD);
        BIND_ENUM_CONSTANT(DRIVER_REAR);

        BIND_PROPERTY(VehicleData, Variant::STRING_NAME, vehicle_provider);
        BIND_PROPERTY(VehicleData, Variant::STRING, vehicle);
        BIND_PROPERTY(VehicleData, Variant::STRING, livery);
        BIND_PROPERTY(VehicleData, Variant::STRING, vehicle_id);
        // the resource's own name (resource_name), bound as the property name too: Resource's own
        // accessors, bound as this class's (godot-cpp binds a method to the class it is of)
        ClassDB::bind_method(
                D_METHOD("set_name", "name"), static_cast<void (VehicleData::*)(const String &)>(&Resource::set_name));
        ClassDB::bind_method(D_METHOD("get_name"), static_cast<String (VehicleData::*)() const>(&Resource::get_name));
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_name",
                "get_name");
        ClassDB::bind_method(D_METHOD("set_driver_type", "value"), &VehicleData::set_driver_type);
        ClassDB::bind_method(D_METHOD("get_driver_type"), &VehicleData::get_driver_type);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "driver_type", PROPERTY_HINT_ENUM, "Nobody,Head,Rear",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "VehicleData.DriverType"),
                "set_driver_type", "get_driver_type");
        BIND_PROPERTY(VehicleData, Variant::STRING, load_name);
        BIND_PROPERTY(VehicleData, Variant::FLOAT, load_amount);
        ClassDB::bind_method(D_METHOD("take_vehicle_of", "other"), &VehicleData::take_vehicle_of);
    }

    void VehicleData::take_vehicle_of(const Ref<VehicleData> &p_other) {
        ERR_FAIL_COND(p_other.is_null());
        vehicle_provider = p_other->vehicle_provider;
        vehicle = p_other->vehicle;
        livery = p_other->livery;
    }
} // namespace godot
