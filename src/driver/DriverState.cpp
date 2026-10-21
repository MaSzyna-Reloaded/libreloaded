#include "DriverState.hpp"
#include "station/StationServer.hpp"

#include <godot_cpp/core/math.hpp>

namespace godot {
    void DriverState::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_task", "task"), &DriverState::set_task);
        ClassDB::bind_method(D_METHOD("get_task"), &DriverState::get_task);
        ClassDB::bind_method(
                D_METHOD("set_waiting_for_orders", "waiting_for_orders"), &DriverState::set_waiting_for_orders);
        ClassDB::bind_method(D_METHOD("get_waiting_for_orders"), &DriverState::get_waiting_for_orders);
        ClassDB::bind_method(D_METHOD("set_direction", "direction"), &DriverState::set_direction);
        ClassDB::bind_method(D_METHOD("get_direction"), &DriverState::get_direction);
        ClassDB::bind_method(D_METHOD("set_speed_limit", "speed_limit"), &DriverState::set_speed_limit);
        ClassDB::bind_method(D_METHOD("get_speed_limit"), &DriverState::get_speed_limit);
        ClassDB::bind_method(D_METHOD("set_next_speed_limit", "next_speed_limit"), &DriverState::set_next_speed_limit);
        ClassDB::bind_method(D_METHOD("get_next_speed_limit"), &DriverState::get_next_speed_limit);
        ClassDB::bind_method(
                D_METHOD("set_next_speed_limit_distance", "next_speed_limit_distance"),
                &DriverState::set_next_speed_limit_distance);
        ClassDB::bind_method(D_METHOD("get_next_speed_limit_distance"), &DriverState::get_next_speed_limit_distance);
        ClassDB::bind_method(D_METHOD("set_stop_reason", "stop_reason"), &DriverState::set_stop_reason);
        ClassDB::bind_method(D_METHOD("get_stop_reason"), &DriverState::get_stop_reason);
        ClassDB::bind_method(
                D_METHOD("set_stop_reason_details", "stop_reason_details"), &DriverState::set_stop_reason_details);
        ClassDB::bind_method(D_METHOD("get_stop_reason_details"), &DriverState::get_stop_reason_details);
        ClassDB::bind_method(D_METHOD("set_at_stop", "at_stop"), &DriverState::set_at_stop);
        ClassDB::bind_method(D_METHOD("get_at_stop"), &DriverState::get_at_stop);
        ClassDB::bind_method(D_METHOD("set_stop_time_left", "stop_time_left"), &DriverState::set_stop_time_left);
        ClassDB::bind_method(D_METHOD("get_stop_time_left"), &DriverState::get_stop_time_left);
        ClassDB::bind_method(D_METHOD("set_hints", "hints"), &DriverState::set_hints);
        ClassDB::bind_method(D_METHOD("get_hints"), &DriverState::get_hints);

        ADD_PROPERTY(PropertyInfo(Variant::STRING, "task"), "set_task", "get_task");
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "waiting_for_orders"), "set_waiting_for_orders", "get_waiting_for_orders");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "direction", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "VehicleController.Direction"),
                "set_direction", "get_direction");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "speed_limit"), "set_speed_limit", "get_speed_limit");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "next_speed_limit"), "set_next_speed_limit", "get_next_speed_limit");
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "next_speed_limit_distance"), "set_next_speed_limit_distance",
                "get_next_speed_limit_distance");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "stop_reason", PROPERTY_HINT_ENUM,
                        "None,Ahead,Not Ready,Waiting For Orders,Signal,Dispatch",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "DriverState.StopReason"),
                "set_stop_reason", "get_stop_reason");
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "stop_reason_details"), "set_stop_reason_details",
                "get_stop_reason_details");
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "at_stop"), "set_at_stop", "get_at_stop");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "stop_time_left"), "set_stop_time_left", "get_stop_time_left");
        ADD_PROPERTY(PropertyInfo(Variant::ARRAY, "hints"), "set_hints", "get_hints");

        BIND_ENUM_CONSTANT(STOP_REASON_NONE);
        BIND_ENUM_CONSTANT(STOP_REASON_AHEAD);
        BIND_ENUM_CONSTANT(STOP_REASON_NOT_READY);
        BIND_ENUM_CONSTANT(STOP_REASON_WAITING_FOR_ORDERS);
        BIND_ENUM_CONSTANT(STOP_REASON_SIGNAL);
        BIND_ENUM_CONSTANT(STOP_REASON_DISPATCH);
    }

    /// A driver that shows nothing: no limit, no change ahead
    DriverState::DriverState() :
        speed_limit(StationServer::NO_SPEED_LIMIT), next_speed_limit(StationServer::NO_SPEED_LIMIT),
        next_speed_limit_distance(Math::INF) {}

    void DriverState::set_task(const String &p_task) {
        task = p_task;
    }

    String DriverState::get_task() const {
        return task;
    }

    void DriverState::set_waiting_for_orders(const bool p_waiting_for_orders) {
        waiting_for_orders = p_waiting_for_orders;
    }

    bool DriverState::get_waiting_for_orders() const {
        return waiting_for_orders;
    }

    void DriverState::set_direction(const VehicleController::Direction p_direction) {
        direction = p_direction;
    }

    VehicleController::Direction DriverState::get_direction() const {
        return direction;
    }

    void DriverState::set_speed_limit(const double p_speed_limit) {
        speed_limit = p_speed_limit;
    }

    double DriverState::get_speed_limit() const {
        return speed_limit;
    }

    void DriverState::set_next_speed_limit(const double p_next_speed_limit) {
        next_speed_limit = p_next_speed_limit;
    }

    double DriverState::get_next_speed_limit() const {
        return next_speed_limit;
    }

    void DriverState::set_next_speed_limit_distance(const double p_next_speed_limit_distance) {
        next_speed_limit_distance = p_next_speed_limit_distance;
    }

    double DriverState::get_next_speed_limit_distance() const {
        return next_speed_limit_distance;
    }

    void DriverState::set_stop_reason(const StopReason p_stop_reason) {
        stop_reason = p_stop_reason;
    }

    DriverState::StopReason DriverState::get_stop_reason() const {
        return stop_reason;
    }

    void DriverState::set_stop_reason_details(const PackedStringArray &p_stop_reason_details) {
        stop_reason_details = p_stop_reason_details;
    }

    PackedStringArray DriverState::get_stop_reason_details() const {
        return stop_reason_details;
    }

    void DriverState::set_at_stop(const bool p_at_stop) {
        at_stop = p_at_stop;
    }

    bool DriverState::get_at_stop() const {
        return at_stop;
    }

    void DriverState::set_stop_time_left(const double p_stop_time_left) {
        stop_time_left = p_stop_time_left;
    }

    double DriverState::get_stop_time_left() const {
        return stop_time_left;
    }

    void DriverState::set_hints(const Array &p_hints) {
        hints = p_hints;
    }

    Array DriverState::get_hints() const {
        return hints;
    }
} // namespace godot
