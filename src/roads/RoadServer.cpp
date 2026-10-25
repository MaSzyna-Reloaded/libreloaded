#include "roads/RoadServer.hpp"
#include "tracks/TrackServer.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *RoadServer::road_vehicle_entered_signal = "road_vehicle_entered";
    const char *RoadServer::road_vehicle_left_signal = "road_vehicle_left";

    void RoadServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("road_create"), &RoadServer::road_create);
        ClassDB::bind_method(D_METHOD("road_free", "road"), &RoadServer::road_free);
        ClassDB::bind_method(D_METHOD("road_free_many", "roads"), &RoadServer::road_free_many);
        ClassDB::bind_method(D_METHOD("road_exists", "road"), &RoadServer::road_exists);
        ClassDB::bind_method(D_METHOD("road_set_curve", "road", "curve"), &RoadServer::road_set_curve);
        ClassDB::bind_method(D_METHOD("road_get_curve", "road"), &RoadServer::road_get_curve);
        ClassDB::bind_method(D_METHOD("road_get_length", "road"), &RoadServer::road_get_length);
        ClassDB::bind_method(D_METHOD("road_get_position", "road", "offset"), &RoadServer::road_get_position);
        ClassDB::bind_method(D_METHOD("road_set_width", "road", "width"), &RoadServer::road_set_width);
        ClassDB::bind_method(D_METHOD("road_get_width", "road"), &RoadServer::road_get_width);
        ClassDB::bind_method(D_METHOD("road_set_velocity", "road", "velocity"), &RoadServer::road_set_velocity);
        ClassDB::bind_method(D_METHOD("road_get_velocity", "road"), &RoadServer::road_get_velocity);
        ClassDB::bind_method(D_METHOD("road_set_track", "road", "track"), &RoadServer::road_set_track);
        ClassDB::bind_method(D_METHOD("road_get_track", "road"), &RoadServer::road_get_track);
        ClassDB::bind_method(D_METHOD("track_get_road", "track"), &RoadServer::track_get_road);
        ClassDB::bind_method(D_METHOD("road_link", "road", "end", "next", "next_end"), &RoadServer::road_link);
        ClassDB::bind_method(D_METHOD("road_get_links", "road", "end"), &RoadServer::road_get_links);
        ClassDB::bind_method(
                D_METHOD("road_report_vehicle_entered", "road", "vehicle", "toward"),
                &RoadServer::road_report_vehicle_entered);
        ClassDB::bind_method(
                D_METHOD("road_report_vehicle_left", "road", "vehicle"), &RoadServer::road_report_vehicle_left);
        ClassDB::bind_method(D_METHOD("road_get_vehicles", "road"), &RoadServer::road_get_vehicles);

        BIND_ENUM_CONSTANT(ROAD_END_START);
        BIND_ENUM_CONSTANT(ROAD_END_END);

        ADD_SIGNAL(MethodInfo(
                road_vehicle_entered_signal, PropertyInfo(Variant::RID, "road"), PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(
                        Variant::INT, "toward", PROPERTY_HINT_ENUM, "", PROPERTY_USAGE_CLASS_IS_ENUM,
                        "RoadServer.RoadEnd")));
        ADD_SIGNAL(MethodInfo(
                road_vehicle_left_signal, PropertyInfo(Variant::RID, "road"), PropertyInfo(Variant::RID, "vehicle")));
    }

    RoadServer::RoadServer() {
        singleton = this;
    }

    RoadServer::~RoadServer() {
        singleton = nullptr;
    }

    RID RoadServer::road_create() {
        const RID road = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        roads.insert(road, Road());
        return road;
    }

    void RoadServer::road_free(const RID &p_road) {
        TypedArray<RID> freed;
        freed.push_back(p_road);
        road_free_many(freed);
    }

    /* All at once: the links of the roads that stay are looked over once, not once a road; a
     * vehicle still listed on one is RoadVehicleServer's to take off */
    void RoadServer::road_free_many(const TypedArray<RID> &p_roads) {
        RIDSet freed;
        for (int64_t index = 0; index < p_roads.size(); ++index) {
            const RID road = p_roads[index];
            if (!roads.has(road)) {
                continue;
            }
            road_set_track(road, RID());
            roads.erase(road);
            freed.insert(road);
        }
        for (KeyValue<RID, Road> &other: roads) {
            for (Vector<Link> &links: other.value.links) {
                for (int64_t index = static_cast<int64_t>(links.size()) - 1; index >= 0; --index) {
                    if (freed.has(links[index].road)) {
                        links.remove_at(index);
                    }
                }
            }
        }
    }

    bool RoadServer::road_exists(const RID &p_road) const {
        return roads.has(p_road);
    }

    void RoadServer::road_set_curve(const RID &p_road, const Ref<Curve3D> &p_curve) {
        Road *road = roads.getptr(p_road);
        ERR_FAIL_NULL(road);
        road->curve = p_curve;
        road->points = p_curve.is_valid() ? p_curve->get_baked_points() : PackedVector3Array();
        road->distances.resize(road->points.size());
        double distance = 0.0;
        for (int64_t index = 0; index < road->points.size(); ++index) {
            if (index > 0) {
                distance += road->points[index - 1].distance_to(road->points[index]);
            }
            road->distances.set(index, distance);
        }
    }

    Ref<Curve3D> RoadServer::road_get_curve(const RID &p_road) const {
        const Road *road = roads.getptr(p_road);
        return road != nullptr ? road->curve : Ref<Curve3D>();
    }

    double RoadServer::road_get_length(const RID &p_road) const {
        const Road *road = roads.getptr(p_road);
        return road != nullptr && !road->distances.is_empty() ? road->distances[road->distances.size() - 1] : 0.0;
    }

    /* Between the two baked points around the offset, found by halving */
    bool RoadServer::road_sample(
            const RID &p_road, const double p_offset, Vector3 &p_r_position, Vector3 &p_r_forward) const {
        const Road *road = roads.getptr(p_road);
        if (road == nullptr || road->points.size() < 2) {
            return false;
        }
        const double offset = CLAMP(p_offset, 0.0, road->distances[road->distances.size() - 1]);
        int64_t low = 0;
        int64_t high = road->points.size() - 1;
        while (high - low > 1) {
            const int64_t middle = (low + high) / 2;
            if (road->distances[middle] <= offset) {
                low = middle;
            } else {
                high = middle;
            }
        }
        const double span = road->distances[high] - road->distances[low];
        const double weight = span > 0.0 ? (offset - road->distances[low]) / span : 0.0;
        p_r_position = road->points[low].lerp(road->points[high], static_cast<real_t>(weight));
        p_r_forward = (road->points[high] - road->points[low]).normalized();
        return true;
    }

    Vector3 RoadServer::road_get_position(const RID &p_road, const double p_offset) const {
        Vector3 position;
        Vector3 forward;
        road_sample(p_road, p_offset, position, forward);
        return position;
    }

    void RoadServer::road_set_width(const RID &p_road, const double p_width) {
        Road *road = roads.getptr(p_road);
        ERR_FAIL_NULL(road);
        road->width = p_width;
    }

    double RoadServer::road_get_width(const RID &p_road) const {
        const Road *road = roads.getptr(p_road);
        return road != nullptr ? road->width : 0.0;
    }

    void RoadServer::road_set_velocity(const RID &p_road, const double p_velocity) {
        Road *road = roads.getptr(p_road);
        ERR_FAIL_NULL(road);
        road->velocity = p_velocity;
    }

    double RoadServer::road_get_velocity(const RID &p_road) const {
        const Road *road = roads.getptr(p_road);
        if (road == nullptr) {
            return -1.0;
        }
        const TrackServer *tracks = TrackServer::get_instance();
        return road->track.is_valid() && tracks != nullptr ? tracks->track_get_velocity(road->track) : road->velocity;
    }

    void RoadServer::road_set_track(const RID &p_road, const RID &p_track) {
        Road *road = roads.getptr(p_road);
        ERR_FAIL_NULL(road);
        if (LocalVector<RID> *laid = track_roads.getptr(road->track); laid != nullptr) {
            laid->erase(p_road);
            if (laid->is_empty()) {
                track_roads.erase(road->track);
            }
        }
        road->track = p_track;
        if (p_track.is_valid()) {
            track_roads[p_track].push_back(p_road);
        }
    }

    RID RoadServer::road_get_track(const RID &p_road) const {
        const Road *road = roads.getptr(p_road);
        return road != nullptr ? road->track : RID();
    }

    RID RoadServer::track_get_road(const RID &p_track) const {
        const LocalVector<RID> *laid = track_roads.getptr(p_track);
        return laid != nullptr ? (*laid)[0] : RID();
    }

    void RoadServer::road_link(const RID &p_road, const RoadEnd p_end, const RID &p_next, const RoadEnd p_next_end) {
        Road *road = roads.getptr(p_road);
        ERR_FAIL_NULL(road);
        ERR_FAIL_COND_MSG(!roads.has(p_next), "Not a road of RoadServer");
        // the same way on given again is the one way it was
        for (const Link &link: road->links[p_end]) {
            if (rid_get_id(link.road) == rid_get_id(p_next) && link.end == p_next_end) {
                return;
            }
        }
        road->links[p_end].push_back(Link{p_next, p_next_end});
    }

    TypedArray<Dictionary> RoadServer::road_get_links(const RID &p_road, const RoadEnd p_end) const {
        TypedArray<Dictionary> result;
        for (const Link &link: road_get_link_list(p_road, p_end)) {
            Dictionary entry;
            entry["road"] = link.road;
            entry["end"] = link.end;
            result.push_back(entry);
        }
        return result;
    }

    const Vector<RoadServer::Link> &RoadServer::road_get_link_list(const RID &p_road, const RoadEnd p_end) const {
        static const Vector<Link> no_links;
        const Road *road = roads.getptr(p_road);
        return road != nullptr ? road->links[p_end] : no_links;
    }

    void RoadServer::road_report_vehicle_entered(const RID &p_road, const RID &p_vehicle, const RoadEnd p_toward) {
        Road *road = roads.getptr(p_road);
        if (road == nullptr) {
            return;
        }
        if (!road->vehicles.has(p_vehicle)) {
            road->vehicles.push_back(p_vehicle);
        }
        emit_signal(road_vehicle_entered_signal, p_road, p_vehicle, p_toward);
    }

    void RoadServer::road_report_vehicle_left(const RID &p_road, const RID &p_vehicle) {
        Road *road = roads.getptr(p_road);
        if (road == nullptr) {
            return;
        }
        road->vehicles.erase(p_vehicle);
        emit_signal(road_vehicle_left_signal, p_road, p_vehicle);
    }

    TypedArray<RID> RoadServer::road_get_vehicles(const RID &p_road) const {
        TypedArray<RID> result;
        for (const RID &vehicle: road_get_vehicle_list(p_road)) {
            result.push_back(vehicle);
        }
        return result;
    }

    const Vector<RID> &RoadServer::road_get_vehicle_list(const RID &p_road) const {
        static const Vector<RID> no_vehicles;
        const Road *road = roads.getptr(p_road);
        return road != nullptr ? road->vehicles : no_vehicles;
    }
} // namespace godot
