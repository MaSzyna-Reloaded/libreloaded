#include "vehicles/road/RoadVehicleController.hpp"
#include "vehicles/road/RoadVehicleServer.hpp"

#include <godot_cpp/core/math.hpp>

namespace godot {
    RoadVehicleController::RoadVehicleController() {
        set_category(CATEGORY_ROAD);
        set_implementation(RoadVehicleServer::IMPLEMENTATION_NAME);
    }

    void RoadVehicleController::_bind_methods() {
        BIND_PROPERTY(RoadVehicleController, Variant::FLOAT, following_gap);
        BIND_PROPERTY(RoadVehicleController, Variant::FLOAT, acceleration_limit);
        BIND_PROPERTY(RoadVehicleController, Variant::FLOAT, obstacle_range);
        BIND_PROPERTY(RoadVehicleController, Variant::FLOAT, wheel_diameter);
        BIND_PROPERTY(RoadVehicleController, Variant::FLOAT, transmission_ratio);
        ClassDB::bind_method(D_METHOD("wanted_velocity_set", "velocity"), &RoadVehicleController::wanted_velocity_set);
        ClassDB::bind_method(D_METHOD("side_offset_set", "offset"), &RoadVehicleController::side_offset_set);
        ClassDB::bind_method(D_METHOD("lights_set", "lights"), &RoadVehicleController::lights_set);
        ClassDB::bind_method(D_METHOD("get_lights"), &RoadVehicleController::get_lights);
        BIND_BITFIELD_FLAG(LIGHTS_NONE);
        BIND_BITFIELD_FLAG(LIGHTS_HEAD);
        BIND_BITFIELD_FLAG(LIGHTS_TAIL);
        ClassDB::bind_method(D_METHOD("horns_set", "horns"), &RoadVehicleController::horns_set);
        ClassDB::bind_method(D_METHOD("get_horns"), &RoadVehicleController::get_horns);
        ClassDB::bind_method(D_METHOD("engine_start"), &RoadVehicleController::engine_start);
        ClassDB::bind_method(D_METHOD("engine_stop"), &RoadVehicleController::engine_stop);
        ClassDB::bind_method(D_METHOD("is_engine_running"), &RoadVehicleController::is_engine_running);
        ClassDB::bind_method(D_METHOD("get_engine_revolutions"), &RoadVehicleController::get_engine_revolutions);
        ClassDB::bind_method(D_METHOD("get_throttle"), &RoadVehicleController::get_throttle);
        ClassDB::bind_method(D_METHOD("get_brake_force_ratio"), &RoadVehicleController::get_brake_force_ratio);
        BIND_BITFIELD_FLAG(HORNS_NONE);
        BIND_BITFIELD_FLAG(HORNS_LOW);
        BIND_BITFIELD_FLAG(HORNS_HIGH);
        ClassDB::bind_method(D_METHOD("get_wanted_velocity"), &RoadVehicleController::get_wanted_velocity);
        ClassDB::bind_method(D_METHOD("get_side_offset"), &RoadVehicleController::get_side_offset);
        ClassDB::bind_method(D_METHOD("get_wheels_angle_deg"), &RoadVehicleController::get_wheels_angle_deg);
    }

    /* No backend: the vehicle is ready once it has its configuration, and starts at the velocity
     * the scenery gave it */
    void RoadVehicleController::_initialize_simulation() {
        velocity = Math::abs(get_initial_velocity()) / KMH_PER_MPS;
        simulation_ready = true;
        apply_configuration();
        emit_signal(simulation_initialized_signal);
    }

    void RoadVehicleController::release() {
        VehicleController::release();
        simulation_ready = false;
    }

    void RoadVehicleController::_fill_config_dictionary(Dictionary &p_config) const {
        p_config["max_speed"] = get_max_velocity();
        p_config["length"] = get_dimensions_length();
        p_config["following_gap"] = following_gap;
        p_config["acceleration_limit"] = acceleration_limit;
        p_config["obstacle_range"] = obstacle_range;
        p_config["wheel_diameter"] = wheel_diameter;
        p_config["transmission_ratio"] = transmission_ratio;
    }

    void RoadVehicleController::_fill_state_dictionary(Dictionary &p_state) const {
        VehicleController::_fill_state_dictionary(p_state);
        p_state["wanted_velocity"] = wanted_velocity;
        p_state["side_offset"] = side_offset;
        p_state["lights"] = static_cast<int64_t>(lights);
        // what the horns' sounds follow, by the names a rail vehicle's horns have
        p_state["horn_low_active"] = horns.has_flag(HORNS_LOW);
        p_state["horn_high_active"] = horns.has_flag(HORNS_HIGH);
        p_state["engine_running"] = engine_running;
        p_state["engine_rpm"] = engine_running ? get_engine_revolutions() * Units::SECONDS_PER_MINUTE : 0.0;
        p_state["wheels_angle_deg"] = get_wheels_angle_deg();
    }

    void RoadVehicleController::_register_commands() {
        register_command("wanted_velocity_set", callable_mp(this, &RoadVehicleController::wanted_velocity_set));
        register_command("side_offset_set", callable_mp(this, &RoadVehicleController::side_offset_set));
        register_command("lights_set", callable_mp(this, &RoadVehicleController::lights_set));
        register_command("horns_set", callable_mp(this, &RoadVehicleController::horns_set));
        register_command("engine_start", callable_mp(this, &RoadVehicleController::engine_start));
        register_command("engine_stop", callable_mp(this, &RoadVehicleController::engine_stop));
    }

    void RoadVehicleController::_unregister_commands() {
        unregister_command("wanted_velocity_set");
        unregister_command("side_offset_set");
        unregister_command("lights_set");
        unregister_command("horns_set");
        unregister_command("engine_start");
        unregister_command("engine_stop");
    }

    void RoadVehicleController::wanted_velocity_set(const double p_velocity) {
        wanted_velocity = MAX(p_velocity, 0.0);
    }

    void RoadVehicleController::side_offset_set(const double p_offset) {
        side_offset = p_offset;
    }

    void RoadVehicleController::lights_set(const BitField<Lights> p_lights) {
        lights = p_lights;
    }

    BitField<RoadVehicleController::Lights> RoadVehicleController::get_lights() const {
        return lights;
    }

    void RoadVehicleController::horns_set(const BitField<Horns> p_horns) {
        horns = p_horns;
    }

    BitField<RoadVehicleController::Horns> RoadVehicleController::get_horns() const {
        return horns;
    }

    void RoadVehicleController::engine_start() {
        engine_running = true;
    }

    void RoadVehicleController::engine_stop() {
        engine_running = false;
    }

    bool RoadVehicleController::is_engine_running() const {
        return engine_running;
    }

    double RoadVehicleController::get_engine_revolutions() const {
        return wheel_diameter > 0.0 ? transmission_ratio * Math::abs(velocity) / (Math::PI * wheel_diameter) : 0.0;
    }

    double RoadVehicleController::get_throttle() const {
        return acceleration_limit > 0.0 ? CLAMP(acceleration / acceleration_limit, 0.0, 1.0) : 0.0;
    }

    double RoadVehicleController::get_brake_force_ratio() const {
        return acceleration_limit > 0.0 ? CLAMP(-acceleration / acceleration_limit, 0.0, 1.0) : 0.0;
    }

    double RoadVehicleController::get_wanted_velocity() const {
        return wanted_velocity;
    }

    double RoadVehicleController::get_side_offset() const {
        return side_offset;
    }

    /* A wheel turns by the distance over its radius */
    double RoadVehicleController::get_wheels_angle_deg() const {
        return wheel_diameter > 0.0 ? Math::wrapf(Math::rad_to_deg(2.0 * total_distance / wheel_diameter), 0.0, 360.0)
                                    : 0.0;
    }

    double RoadVehicleController::process_movement(const double p_delta, const double p_allowed_velocity) {
        const double change = acceleration_limit * p_delta;
        const double next = CLAMP(MAX(p_allowed_velocity, 0.0), velocity - change, velocity + change);
        const double distance = 0.5 * (velocity + next) * p_delta;
        acceleration = (next - velocity) / p_delta;
        velocity = next;
        total_distance += distance;
        return distance;
    }

    bool RoadVehicleController::is_simulation_ready() const {
        return simulation_ready;
    }

    double RoadVehicleController::get_velocity() const {
        return velocity;
    }

    double RoadVehicleController::get_speed() const {
        return velocity * KMH_PER_MPS;
    }

    double RoadVehicleController::get_acceleration() const {
        return acceleration;
    }

    double RoadVehicleController::get_mass_total() const {
        return get_mass();
    }

    double RoadVehicleController::get_total_distance() const {
        return total_distance;
    }

    /* It always drives forward - it turns about along a road of its own (RoadServer) */
    VehicleController::Direction RoadVehicleController::get_direction() const {
        return DIRECTION_FORWARD;
    }

    bool RoadVehicleController::is_physics_active() const {
        return simulation_ready;
    }
} // namespace godot
