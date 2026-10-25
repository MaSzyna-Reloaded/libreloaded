#include "tracks/TrackServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/road/RoadVehicleServer.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *RoadVehicleServer::vehicle_placed_signal = "vehicle_placed";
    const char *RoadVehicleServer::vehicle_placement_changed_signal = "vehicle_placement_changed";

    /* The velocity [m/s] from which p_deceleration [m/s2] brakes to p_velocity within p_distance [m] */
    static double braked_velocity(const double p_velocity, const double p_distance, const double p_deceleration) {
        return Math::sqrt(MAX((p_velocity * p_velocity) + (2.0 * p_deceleration * p_distance), 0.0));
    }

    /* How far p_velocity [m/s] runs until p_deceleration [m/s2] stops it [m] */
    static double braking_distance(const double p_velocity, const double p_deceleration) {
        return p_deceleration > 0.0 ? p_velocity * p_velocity / (2.0 * p_deceleration) : 0.0;
    }

    /* Entered by one end, a road is travelled toward the other */
    static RoadServer::RoadEnd travelled_toward(const RoadServer::RoadEnd p_entered_by) {
        return p_entered_by == RoadServer::ROAD_END_START ? RoadServer::ROAD_END_END : RoadServer::ROAD_END_START;
    }

    /* A road laid along a track runs from the track's start to its end */
    static TrackServer::TravelDirection travel(const RoadServer::RoadEnd p_toward) {
        return p_toward == RoadServer::ROAD_END_END ? TrackServer::TRAVEL_TOWARD_END : TrackServer::TRAVEL_TOWARD_START;
    }

    /* How far along the road from p_toward's opposite end p_offset [m] is */
    static double along(const double p_offset, const double p_length, const RoadServer::RoadEnd p_toward) {
        return p_toward == RoadServer::ROAD_END_END ? p_offset : p_length - p_offset;
    }

    void RoadVehicleServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("vehicle_attach", "vehicle"), &RoadVehicleServer::vehicle_attach);
        ClassDB::bind_method(D_METHOD("vehicle_detach", "vehicle"), &RoadVehicleServer::vehicle_detach);
        ClassDB::bind_method(D_METHOD("vehicle_is_attached", "vehicle"), &RoadVehicleServer::vehicle_is_attached);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_road", "vehicle", "road", "offset", "facing"),
                &RoadVehicleServer::vehicle_set_road);
        ClassDB::bind_method(D_METHOD("vehicle_get_road", "vehicle"), &RoadVehicleServer::vehicle_get_road);
        ClassDB::bind_method(D_METHOD("vehicle_get_transform", "vehicle"), &RoadVehicleServer::vehicle_get_transform);
        ClassDB::bind_method(
                D_METHOD("vehicle_trace_route", "vehicle", "distance"), &RoadVehicleServer::vehicle_trace_route);

        ADD_SIGNAL(MethodInfo(vehicle_placed_signal, PropertyInfo(Variant::RID, "vehicle")));
        ADD_SIGNAL(MethodInfo(vehicle_placement_changed_signal, PropertyInfo(Variant::RID, "vehicle")));
    }

    RoadVehicleServer::RoadVehicleServer() {
        singleton = this;
        // the vehicle is VehicleServer's: freed there, it leaves the road; driven by another
        // controller, that one is held
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        vehicle_server->connect(
                VehicleServer::vehicle_freed_signal, callable_mp(this, &RoadVehicleServer::vehicle_detach));
        vehicle_server->connect(
                VehicleServer::vehicle_controller_changed_signal,
                callable_mp(this, &RoadVehicleServer::_on_vehicle_controller_changed));
    }

    RoadVehicleServer::~RoadVehicleServer() {
        singleton = nullptr;
    }

    void RoadVehicleServer::_on_vehicle_controller_changed(const RID &p_vehicle) {
        Placement *placement = vehicles.getptr(p_vehicle);
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        if (placement == nullptr || vehicle_server == nullptr) {
            return;
        }
        placement->controller = vehicle_server->vehicle_get_controller(p_vehicle);
    }

    void RoadVehicleServer::vehicle_attach(const RID &p_vehicle) {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        ERR_FAIL_COND_MSG(!vehicle_server->vehicle_exists(p_vehicle), "Not a vehicle of VehicleServer");
        if (vehicles.has(p_vehicle)) {
            return;
        }
        vehicles.insert(p_vehicle, Placement())->value.controller = vehicle_server->vehicle_get_controller(p_vehicle);
    }

    void RoadVehicleServer::vehicle_detach(const RID &p_vehicle) {
        const Placement *placement = vehicles.getptr(p_vehicle);
        if (placement == nullptr) {
            return;
        }
        const RID road = placement->road;
        const RID track = placement->track;
        vehicles.erase(p_vehicle);
        if (RoadServer *roads = RoadServer::get_instance(); roads != nullptr && road.is_valid()) {
            roads->road_report_vehicle_left(road, p_vehicle);
        }
        if (TrackServer *tracks = TrackServer::get_instance(); tracks != nullptr && track.is_valid()) {
            tracks->track_report_vehicle_left(track, p_vehicle);
        }
    }

    bool RoadVehicleServer::vehicle_is_attached(const RID &p_vehicle) const {
        return vehicles.has(p_vehicle);
    }

    void RoadVehicleServer::vehicle_set_road(
            const RID &p_vehicle, const RID &p_road, const double p_offset, const RoadServer::RoadEnd p_facing) {
        Placement *placement = vehicles.getptr(p_vehicle);
        const RoadServer *roads = RoadServer::get_instance();
        ERR_FAIL_NULL(placement);
        ERR_FAIL_NULL(roads);
        ERR_FAIL_COND_MSG(!roads->road_exists(p_road), "Not a road of RoadServer");
        placement->offset = CLAMP(p_offset, 0.0, roads->road_get_length(p_road));
        placement->route.clear();
        if (!_enter(p_vehicle, *placement, p_road, p_facing)) {
            return; // a listener of the reports freed it
        }
        placement = vehicles.getptr(p_vehicle);
        _place(*placement);
        emit_signal(vehicle_placed_signal, p_vehicle);
    }

    RID RoadVehicleServer::vehicle_get_road(const RID &p_vehicle) const {
        const Placement *placement = vehicles.getptr(p_vehicle);
        return placement != nullptr ? placement->road : RID();
    }

    Transform3D RoadVehicleServer::vehicle_get_transform(const RID &p_vehicle) const {
        const Placement *placement = vehicles.getptr(p_vehicle);
        return placement != nullptr ? placement->transform : Transform3D();
    }

    bool RoadVehicleServer::_route_link(
            Placement &p_placement, const uint32_t p_step, const RID &p_road, const RoadServer::RoadEnd p_toward,
            RoadServer::Link &p_r_link) {
        if (p_step < p_placement.route.size()) {
            p_r_link = p_placement.route[p_step];
            return true;
        }
        const RoadServer *roads = RoadServer::get_instance();
        const Vector<RoadServer::Link> &links = roads->road_get_link_list(p_road, p_toward);
        if (links.is_empty() || p_step >= MAX_ROUTE_ROADS || roads->road_get_length(p_road) < MIN_PASSABLE_LENGTH) {
            return false;
        }
        p_r_link = links[links.size() == 1 ? 0 : UtilityFunctions::randi_range(0, links.size() - 1)];
        p_placement.route.push_back(p_r_link);
        return true;
    }

    TypedArray<RoadRouteSegment> RoadVehicleServer::vehicle_trace_route(const RID &p_vehicle, const double p_distance) {
        TypedArray<RoadRouteSegment> route;
        Placement *placement = vehicles.getptr(p_vehicle);
        const RoadServer *roads = RoadServer::get_instance();
        if (placement == nullptr || roads == nullptr || !roads->road_exists(placement->road)) {
            return route;
        }
        RID road = placement->road;
        RoadServer::RoadEnd toward = placement->toward;
        double length = roads->road_get_length(road);
        // the road it stands on was entered behind it
        double entered = -along(placement->offset, length, toward);
        uint32_t step = 0;
        while (true) {
            Ref<RoadRouteSegment> segment;
            segment.instantiate();
            segment->set_road(road);
            segment->set_distance(entered);
            segment->set_length(length);
            segment->set_toward(toward);
            route.push_back(segment);
            const double left = entered + length;
            RoadServer::Link link;
            if (left >= p_distance || !_route_link(*placement, step++, road, toward, link) ||
                roads->road_get_length(link.road) < MIN_PASSABLE_LENGTH) {
                return route;
            }
            road = link.road;
            toward = travelled_toward(link.end);
            length = roads->road_get_length(road);
            entered = left;
        }
    }

    double RoadVehicleServer::_vehicles_allowed_velocity(
            const RID &p_vehicle, const Placement &p_placement, const RID &p_road, const RoadServer::RoadEnd p_toward,
            const double p_distance, const double p_step_distance, const double p_delta) const {
        const RoadServer *roads = RoadServer::get_instance();
        const RoadVehicleController *own = p_placement.controller.ptr();
        const double length = roads->road_get_length(p_road);
        // across the road, measured to the right of its own way from start to end
        const double own_side = own->get_side_offset() * (p_toward == RoadServer::ROAD_END_END ? 1.0 : -1.0);
        double allowed = Math::INF;
        for (const RID &listed: roads->road_get_vehicle_list(p_road)) {
            const Placement *other = rid_get_id(listed) == rid_get_id(p_vehicle) ? nullptr : vehicles.getptr(listed);
            if (other == nullptr || other->controller.is_null()) {
                continue;
            }
            const double ahead = p_distance + along(other->offset, length, p_toward);
            if (ahead <= 0.0) {
                continue;
            }
            const RoadVehicleController *vehicle = other->controller.ptr();
            const double other_side =
                    vehicle->get_side_offset() * (other->toward == RoadServer::ROAD_END_END ? 1.0 : -1.0);
            // beside it - it is passed, or it passes the other way
            if (2.0 * Math::abs(own_side - other_side) >
                own->get_dimensions_width() + vehicle->get_dimensions_width()) {
                continue;
            }
            const double gap = ahead - (0.5 * (own->get_dimensions_length() + vehicle->get_dimensions_length())) -
                               own->get_following_gap() - p_step_distance;
            // one ahead travelling its way is followed at its velocity; one coming the other way is
            // stopped for where it stops itself
            if (other->toward == p_toward) {
                allowed = MIN(allowed, braked_velocity(vehicle->get_velocity(), gap, own->get_acceleration_limit()));
                continue;
            }
            const double oncoming = vehicle->get_velocity();
            allowed =
                    MIN(allowed, braked_velocity(
                                         0.0,
                                         gap - braking_distance(oncoming, vehicle->get_acceleration_limit()) -
                                                 (oncoming * p_delta),
                                         own->get_acceleration_limit()));
        }
        return allowed;
    }

    double RoadVehicleServer::_allowed_velocity(const RID &p_vehicle, Placement &p_placement, const double p_delta) {
        const RoadServer *roads = RoadServer::get_instance();
        const RoadVehicleController *controller = p_placement.controller.ptr();
        const double deceleration = controller->get_acceleration_limit();
        // what is ahead is braked for from where this step takes it
        const double step_distance = controller->get_velocity() * p_delta;
        double allowed = controller->get_wanted_velocity() / RoadVehicleController::KMH_PER_MPS;
        if (controller->get_max_velocity() > 0.0) {
            allowed = MIN(allowed, controller->get_max_velocity() / RoadVehicleController::KMH_PER_MPS);
        }
        RID road = p_placement.road;
        RoadServer::RoadEnd toward = p_placement.toward;
        double length = roads->road_get_length(road);
        // where the road was entered, from the vehicle - behind it for the one it is on
        double entered = -along(p_placement.offset, length, toward);
        uint32_t step = 0;
        while (true) {
            if (const double limit = roads->road_get_velocity(road); limit >= 0.0) {
                allowed =
                        MIN(allowed, braked_velocity(
                                             limit / RoadVehicleController::KMH_PER_MPS,
                                             MAX(entered - step_distance, 0.0), deceleration));
            }
            allowed = MIN(
                    allowed,
                    _vehicles_allowed_velocity(p_vehicle, p_placement, road, toward, entered, step_distance, p_delta));
            const double left = entered + length;
            if (left >= controller->get_obstacle_range()) {
                return allowed;
            }
            RoadServer::Link link;
            if (!_route_link(p_placement, step++, road, toward, link) ||
                roads->road_get_length(link.road) < MIN_PASSABLE_LENGTH) {
                // a dead end - or a road that leads nowhere, never entered: it stops there
                return MIN(allowed, braked_velocity(0.0, MAX(left - step_distance, 0.0), deceleration));
            }
            road = link.road;
            toward = travelled_toward(link.end);
            length = roads->road_get_length(road);
            entered = left;
        }
    }

    bool RoadVehicleServer::_enter(
            const RID &p_vehicle, Placement &p_placement, const RID &p_road, const RoadServer::RoadEnd p_toward) {
        RoadServer *roads = RoadServer::get_instance();
        TrackServer *tracks = TrackServer::get_instance();
        const RID left_road = p_placement.road;
        const RID left_track = p_placement.track;
        const RID track = roads->road_get_track(p_road);
        p_placement.road = p_road;
        p_placement.toward = p_toward;
        p_placement.track = track;
        if (left_road.is_valid()) {
            roads->road_report_vehicle_left(left_road, p_vehicle);
        }
        if (tracks != nullptr && !(left_track == track)) {
            if (left_track.is_valid()) {
                tracks->track_report_vehicle_left(left_track, p_vehicle);
            }
            if (track.is_valid()) {
                tracks->track_report_vehicle_entered(track, p_vehicle, travel(p_toward));
            }
        }
        roads->road_report_vehicle_entered(p_road, p_vehicle, p_toward);
        return vehicles.has(p_vehicle);
    }

    bool RoadVehicleServer::_move(const RID &p_vehicle, Placement &p_placement, const double p_distance) {
        const RoadServer *roads = RoadServer::get_instance();
        double remaining = p_distance;
        while (true) {
            const double length = roads->road_get_length(p_placement.road);
            const double room = length - along(p_placement.offset, length, p_placement.toward);
            const double sign = p_placement.toward == RoadServer::ROAD_END_END ? 1.0 : -1.0;
            if (remaining <= room) {
                p_placement.offset += sign * remaining;
                return true;
            }
            RoadServer::Link link;
            if (!_route_link(p_placement, 0, p_placement.road, p_placement.toward, link) ||
                roads->road_get_length(link.road) < MIN_PASSABLE_LENGTH) {
                p_placement.offset += sign * room; // a dead end: it stands at it
                return true;
            }
            remaining -= room;
            p_placement.route.remove_at(0);
            p_placement.offset = link.end == RoadServer::ROAD_END_START ? 0.0 : roads->road_get_length(link.road);
            if (!_enter(p_vehicle, p_placement, link.road, travelled_toward(link.end))) {
                return false;
            }
        }
    }

    /* On the road's path - a road has no rails to stand on - at its place across the road, its
     * front (-Z) the way it travels */
    void RoadVehicleServer::_place(Placement &p_placement) const {
        Vector3 origin;
        Vector3 forward;
        if (!RoadServer::get_instance()->road_sample(p_placement.road, p_placement.offset, origin, forward) ||
            forward.is_zero_approx()) {
            return;
        }
        if (p_placement.toward == RoadServer::ROAD_END_START) {
            forward = -forward;
        }
        const Vector3 right = forward.cross(Vector3(0.0, 1.0, 0.0)).normalized();
        const Vector3 back = -forward;
        p_placement.transform = Transform3D(
                Basis(right, back.cross(right), back),
                origin + (right * static_cast<real_t>(p_placement.controller->get_side_offset())));
        p_placement.controller->set_world_transform(p_placement.transform);
    }

    void RoadVehicleServer::stepping_advance(
            const Vector<RID> &p_vehicles, const Vector<Ref<VehicleController>> & /*p_controllers*/,
            const double p_delta) {
        const RoadServer *roads = RoadServer::get_instance();
        ERR_FAIL_NULL(roads);
        Vector<RID> moved;
        for (const RID &vehicle: p_vehicles) {
            Placement *placement = vehicles.getptr(vehicle);
            if (placement == nullptr || placement->controller.is_null() || !roads->road_exists(placement->road)) {
                continue;
            }
            const Ref<RoadVehicleController> controller = placement->controller;
            const double distance =
                    controller->process_movement(p_delta, _allowed_velocity(vehicle, *placement, p_delta));
            if (distance > 0.0 && _move(vehicle, *placement, distance)) {
                _place(*vehicles.getptr(vehicle));
                moved.push_back(vehicle);
            }
            controller->process_components(p_delta);
        }
        for (const RID &vehicle: moved) {
            if (const Placement *placement = vehicles.getptr(vehicle); placement != nullptr) {
                placement->controller->emit_position_changed_if_needed();
                emit_signal(vehicle_placement_changed_signal, vehicle);
            }
        }
    }
} // namespace godot
