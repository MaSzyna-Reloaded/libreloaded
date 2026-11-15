#include "CabinState.hpp"
#include "cabin/CabinSystem.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

namespace godot {
    void CabinState::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_vehicle_rid", "value"), &CabinState::set_vehicle_rid);
        ClassDB::bind_method(D_METHOD("get_vehicle_rid"), &CabinState::get_vehicle_rid);
        ClassDB::bind_method(D_METHOD("set_cabin", "value"), &CabinState::set_cabin);
        ClassDB::bind_method(D_METHOD("get_cabin"), &CabinState::get_cabin);
        ClassDB::bind_method(D_METHOD("set_values", "value"), &CabinState::set_values);
        ClassDB::bind_method(D_METHOD("get_values"), &CabinState::get_values);
        ClassDB::bind_method(D_METHOD("set_data", "value"), &CabinState::set_data);
        ClassDB::bind_method(D_METHOD("get_data"), &CabinState::get_data);
        ClassDB::bind_method(D_METHOD("set_light_level", "value"), &CabinState::set_light_level);
        ClassDB::bind_method(D_METHOD("get_light_level"), &CabinState::get_light_level);
        ClassDB::bind_method(
                D_METHOD("set_instrument_light_enabled", "value"), &CabinState::set_instrument_light_enabled);
        ClassDB::bind_method(D_METHOD("get_instrument_light_enabled"), &CabinState::get_instrument_light_enabled);
        ClassDB::bind_method(
                D_METHOD("set_dashboard_light_enabled", "value"), &CabinState::set_dashboard_light_enabled);
        ClassDB::bind_method(D_METHOD("get_dashboard_light_enabled"), &CabinState::get_dashboard_light_enabled);
        ClassDB::bind_method(
                D_METHOD("set_timetable_light_enabled", "value"), &CabinState::set_timetable_light_enabled);
        ClassDB::bind_method(D_METHOD("get_timetable_light_enabled"), &CabinState::get_timetable_light_enabled);

        ADD_PROPERTY(PropertyInfo(Variant::RID, "vehicle_rid"), "set_vehicle_rid", "get_vehicle_rid");
        ADD_PROPERTY(PropertyInfo(Variant::RID, "cabin"), "set_cabin", "get_cabin");
        ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "values"), "set_values", "get_values");
        ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "data"), "set_data", "get_data");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "light_level"), "set_light_level", "get_light_level");
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "instrument_light_enabled"), "set_instrument_light_enabled",
                "get_instrument_light_enabled");
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "dashboard_light_enabled"), "set_dashboard_light_enabled",
                "get_dashboard_light_enabled");
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "timetable_light_enabled"), "set_timetable_light_enabled",
                "get_timetable_light_enabled");

        ClassDB::bind_method(D_METHOD("get_value", "control_id", "default"), &CabinState::get_value, DEFVAL(Variant()));
        ClassDB::bind_method(D_METHOD("set_value", "control_id", "value"), &CabinState::set_value);
        ClassDB::bind_method(D_METHOD("is_pressed", "control_id", "action", "value"), &CabinState::is_pressed);
        ClassDB::bind_method(D_METHOD("vehicle_state"), &CabinState::vehicle_state);
        ClassDB::bind_static_method(
                get_class_static(), D_METHOD("vehicle_of", "occupied", "target"), &CabinState::vehicle_of);
        ClassDB::bind_method(
                D_METHOD("vehicle_state_value", "key", "default_value", "target"), &CabinState::vehicle_state_value,
                DEFVAL(Variant()), DEFVAL(OCCUPIED));
        ClassDB::bind_method(
                D_METHOD("vehicle_component", "type", "target"), &CabinState::vehicle_component, DEFVAL(OCCUPIED));
        ClassDB::bind_method(
                D_METHOD("send_vehicle_command", "command", "p1", "p2", "target"), &CabinState::send_vehicle_command,
                DEFVAL(Variant()), DEFVAL(Variant()), DEFVAL(OCCUPIED));

        BIND_ENUM_CONSTANT(OCCUPIED);
        BIND_ENUM_CONSTANT(CONTROLLED);
        BIND_ENUM_CONSTANT(PANTOGRAPH_UNIT);
        BIND_ENUM_CONSTANT(NONE);
        BIND_ENUM_CONSTANT(CAB);
        BIND_ENUM_CONSTANT(INSTRUMENT);
        BIND_ENUM_CONSTANT(DASHBOARD);
        BIND_ENUM_CONSTANT(TIMETABLE);
    }

    void CabinState::setup(const RID &p_cabin) {
        cabin = p_cabin;
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        vehicle_rid = vehicles->cabin_get_vehicle(p_cabin);
    }

    void CabinState::set_vehicle_rid(const RID &p_value) {
        vehicle_rid = p_value;
    }

    RID CabinState::get_vehicle_rid() const {
        return vehicle_rid;
    }

    void CabinState::set_cabin(const RID &p_value) {
        cabin = p_value;
    }

    RID CabinState::get_cabin() const {
        return cabin;
    }

    void CabinState::set_values(const Dictionary &p_value) {
        values = p_value;
    }

    Dictionary CabinState::get_values() const {
        return values;
    }

    void CabinState::set_data(const Dictionary &p_value) {
        data = p_value;
    }

    Dictionary CabinState::get_data() const {
        return data;
    }

    void CabinState::set_light_level(const double p_value) {
        light_level = p_value;
    }

    double CabinState::get_light_level() const {
        return light_level;
    }

    void CabinState::set_instrument_light_enabled(const bool p_value) {
        instrument_light_enabled = p_value;
    }

    bool CabinState::get_instrument_light_enabled() const {
        return instrument_light_enabled;
    }

    void CabinState::set_dashboard_light_enabled(const bool p_value) {
        dashboard_light_enabled = p_value;
    }

    bool CabinState::get_dashboard_light_enabled() const {
        return dashboard_light_enabled;
    }

    void CabinState::set_timetable_light_enabled(const bool p_value) {
        timetable_light_enabled = p_value;
    }

    bool CabinState::get_timetable_light_enabled() const {
        return timetable_light_enabled;
    }

    Variant CabinState::get_value(const StringName &p_control_id, const Variant &p_default) const {
        return values.get(p_control_id, p_default);
    }

    void CabinState::set_value(const StringName &p_control_id, const Variant &p_value) {
        if (values.has(p_control_id) && values[p_control_id] == p_value) {
            return;
        }
        values[p_control_id] = p_value;
        CabinSystem *cabins = CabinSystem::get_instance();
        ERR_FAIL_NULL(cabins);
        cabins->emit_signal(CabinSystem::control_changed_signal, cabin, p_control_id, p_value);
    }

    bool
    CabinState::is_pressed(const StringName &p_control_id, const StringName &p_action, const Variant &p_value) const {
        if (p_action == StringName("toggle")) {
            return p_value.get_type() == Variant::NIL ? !bool(get_value(p_control_id, false)) : bool(p_value);
        }
        return p_action == StringName("hold");
    }

    Dictionary CabinState::vehicle_state() const {
        CabinSystem *cabins = CabinSystem::get_instance();
        ERR_FAIL_NULL_V(cabins, Dictionary());
        return cabins->vehicle_state(vehicle_rid);
    }

    RID CabinState::vehicle_of(const RID &p_occupied, const Target p_target) {
        const RailVehicleServer *rail = RailVehicleServer::get_instance();
        ERR_FAIL_NULL_V(rail, p_occupied);
        switch (p_target) {
            case CONTROLLED:
                return rail->vehicle_find_powered(p_occupied);
            case PANTOGRAPH_UNIT: {
                const RID carrier = rail->vehicle_find_pantograph_carrier(p_occupied);
                // without one the controlled vehicle stands in
                return carrier.is_valid() ? carrier : rail->vehicle_find_powered(p_occupied);
            }
            default:
                return p_occupied;
        }
    }

    Variant
    CabinState::vehicle_state_value(const String &p_key, const Variant &p_default_value, const Target p_target) const {
        CabinSystem *cabins = CabinSystem::get_instance();
        ERR_FAIL_NULL_V(cabins, p_default_value);
        return cabins->vehicle_state_value(vehicle_of(vehicle_rid, p_target), p_key, p_default_value);
    }

    Ref<VehicleComponent>
    CabinState::vehicle_component(const VehicleComponentType::Type p_type, const Target p_target) const {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, Ref<VehicleComponent>());
        const RID vehicle = vehicle_of(vehicle_rid, p_target);
        return vehicle.is_valid() ? vehicles->vehicle_component_get(vehicle, p_type) : Ref<VehicleComponent>();
    }

    Variant CabinState::send_vehicle_command(
            const StringName &p_command, const Variant &p_p1, const Variant &p_p2, const Target p_target) const {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, Variant());
        return vehicles->vehicle_send_command(vehicle_of(vehicle_rid, p_target), p_command, p_p1, p_p2);
    }
} // namespace godot
