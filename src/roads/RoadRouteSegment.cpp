#include "roads/RoadRouteSegment.hpp"

#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void RoadRouteSegment::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_road", "road"), &RoadRouteSegment::set_road);
        ClassDB::bind_method(D_METHOD("get_road"), &RoadRouteSegment::get_road);
        ClassDB::bind_method(D_METHOD("set_distance", "distance"), &RoadRouteSegment::set_distance);
        ClassDB::bind_method(D_METHOD("get_distance"), &RoadRouteSegment::get_distance);
        ClassDB::bind_method(D_METHOD("set_length", "length"), &RoadRouteSegment::set_length);
        ClassDB::bind_method(D_METHOD("get_length"), &RoadRouteSegment::get_length);
        ClassDB::bind_method(D_METHOD("set_toward", "toward"), &RoadRouteSegment::set_toward);
        ClassDB::bind_method(D_METHOD("get_toward"), &RoadRouteSegment::get_toward);
        ADD_PROPERTY(PropertyInfo(Variant::RID, "road"), "set_road", "get_road");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "distance"), "set_distance", "get_distance");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "length"), "set_length", "get_length");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "toward", PROPERTY_HINT_ENUM, "Start,End"), "set_toward", "get_toward");
    }

    void RoadRouteSegment::set_road(const RID &p_road) {
        road = p_road;
    }

    RID RoadRouteSegment::get_road() const {
        return road;
    }

    void RoadRouteSegment::set_distance(const double p_distance) {
        distance = p_distance;
    }

    double RoadRouteSegment::get_distance() const {
        return distance;
    }

    void RoadRouteSegment::set_length(const double p_length) {
        length = p_length;
    }

    double RoadRouteSegment::get_length() const {
        return length;
    }

    void RoadRouteSegment::set_toward(const RoadServer::RoadEnd p_toward) {
        toward = p_toward;
    }

    RoadServer::RoadEnd RoadRouteSegment::get_toward() const {
        return toward;
    }
} // namespace godot
