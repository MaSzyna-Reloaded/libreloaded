#pragma once
#include "roads/RoadServer.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* A road on a road vehicle's way ahead (RoadVehicleServer.vehicle_trace_route()): entered
     * `distance` metres ahead of the vehicle - negative for the one it is on - and travelled toward
     * its end `toward` */
    class RoadRouteSegment : public RefCounted {
            GDCLASS(RoadRouteSegment, RefCounted)

        private:
            RID road;
            double distance = 0.0;
            double length = 0.0;
            RoadServer::RoadEnd toward = RoadServer::ROAD_END_END;

        protected:
            static void _bind_methods();

        public:
            void set_road(const RID &p_road);
            RID get_road() const;
            void set_distance(double p_distance);
            double get_distance() const;
            void set_length(double p_length);
            double get_length() const;
            void set_toward(RoadServer::RoadEnd p_toward);
            RoadServer::RoadEnd get_toward() const;
    };
} // namespace godot
