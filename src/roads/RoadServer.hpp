#pragma once
#include "utils/RIDHash.hpp"

#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* The road network, by the roads' handles: a road is a path with a width and a speed limit, and
     * each of its two ends leads on to the roads its links name - one for a road going on, several
     * where roads part (a vehicle takes one of them), none at a dead end. The links are one-way: a
     * road's end leads on to another road's end that it enters by, and the way back is a link of
     * its own. Whoever reads a network from its data builds it (a plugin's scenery, from its
     * roads, crossroads and turning places) and frees it; the vehicles on it are
     * RoadVehicleServer's, reported here road by road. Nothing of it is drawn. */
    class RoadServer : public Object {
            GDCLASS(RoadServer, Object)

        public:
            /* A road's two ends: where its path starts and where it ends */
            enum RoadEnd {
                ROAD_END_START,
                ROAD_END_END,
            };

            static RoadServer *get_instance() {
                return singleton;
            }

            /* Where a road's end leads on: the road entered and the end it is entered by */
            struct Link {
                    RID road;
                    RoadEnd end = ROAD_END_START;
            };

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline RoadServer *singleton = nullptr;

            struct Road {
                    Ref<Curve3D> curve;
                    /* The curve's baked points and the distance along it to each [m] */
                    PackedVector3Array points;
                    PackedFloat64Array distances;
                    double width = 0.0;
                    /* Its own speed limit [km/h], negative for none - while it has no track */
                    double velocity = -1.0;
                    /* The TrackServer track it is laid along, if any */
                    RID track;
                    Vector<Link> links[2];
                    Vector<RID> vehicles;
            };

            RIDMap<Road> roads;
            /* The roads laid along each track, the first of them first */
            RIDMap<LocalVector<RID>> track_roads;

        protected:
            static void _bind_methods();

        public:
            /* A vehicle came onto the road, travelling toward its end p_toward (road, vehicle, toward:
             * RoadEnd), or left it (road, vehicle) - reported by RoadVehicleServer */
            static const char *road_vehicle_entered_signal;
            static const char *road_vehicle_left_signal;

            RoadServer();
            ~RoadServer() override;

            RID road_create();
            /* The road goes, with every link leading onto it */
            void road_free(const RID &p_road);
            void road_free_many(const TypedArray<RID> &p_roads);
            bool road_exists(const RID &p_road) const;
            /* The road's path, from its start to its end */
            void road_set_curve(const RID &p_road, const Ref<Curve3D> &p_curve);
            Ref<Curve3D> road_get_curve(const RID &p_road) const;
            double road_get_length(const RID &p_road) const;
            /* The point p_offset [m] along the road from its start, and its way there toward the
             * road's end - for C++ only, read by every road vehicle's step; false for a road
             * without a path */
            bool road_sample(const RID &p_road, double p_offset, Vector3 &p_r_position, Vector3 &p_r_forward) const;
            Vector3 road_get_position(const RID &p_road, double p_offset) const;
            void road_set_width(const RID &p_road, double p_width);
            double road_get_width(const RID &p_road) const;
            /* Its speed limit [km/h], negative for none. A road laid along a track has that track's
             * (TrackServer.track_get_velocity()), as a scenario changes it */
            void road_set_velocity(const RID &p_road, double p_velocity);
            double road_get_velocity(const RID &p_road) const;
            /* The track the road is laid along: a vehicle on the road is reported on the track too,
             * and the track's limit is the road's */
            void road_set_track(const RID &p_road, const RID &p_track);
            RID road_get_track(const RID &p_road) const;
            /* The first road laid along the track; none for a track no road is */
            RID track_get_road(const RID &p_track) const;

            /* Leaving p_road at p_end, a vehicle may go on onto p_next, entering it by p_next_end; a
             * way given again stays one */
            void road_link(const RID &p_road, RoadEnd p_end, const RID &p_next, RoadEnd p_next_end);
            /* Where the end leads on, as {road, end} dictionaries */
            TypedArray<Dictionary> road_get_links(const RID &p_road, RoadEnd p_end) const;
            /* The same for C++ only, without a copy; empty for a road that is gone */
            const Vector<Link> &road_get_link_list(const RID &p_road, RoadEnd p_end) const;

            void road_report_vehicle_entered(const RID &p_road, const RID &p_vehicle, RoadEnd p_toward);
            void road_report_vehicle_left(const RID &p_road, const RID &p_vehicle);
            TypedArray<RID> road_get_vehicles(const RID &p_road) const;
            /* The same for C++ only, without a copy; empty for a road that is gone */
            const Vector<RID> &road_get_vehicle_list(const RID &p_road) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(RoadServer::RoadEnd);
