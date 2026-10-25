#pragma once
#include "roads/RoadRouteSegment.hpp"
#include "roads/RoadServer.hpp"
#include "utils/RIDHash.hpp"
#include "vehicles/base/VehicleImplementationServer.hpp"
#include "vehicles/road/RoadVehicleController.hpp"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* Where a road vehicle is and how it drives: which road of RoadServer's network it is on, how
     * far along it, which way it travels, the way it takes on - one of the roads at random where
     * they part, chosen as far ahead as it looks - and how fast what is ahead lets it go: the
     * velocity it wants, the roads' limits, the vehicle ahead in its lane, a dead end. The vehicle
     * itself - its handle, name, commands - is VehicleServer's; this server takes a vehicle created
     * there (vehicle_attach()) whose controller is a RoadVehicleController, and steps all of them in
     * one loop as their implementation (IMPLEMENTATION_NAME). No couplers, no cabs, no trainsets. A
     * vehicle on a road laid along a track is reported on that track as well (TrackServer). */
    class RoadVehicleServer : public VehicleImplementationServer {
            GDCLASS(RoadVehicleServer, VehicleImplementationServer)

        public:
            static constexpr const char *IMPLEMENTATION_NAME = "road";

            static RoadVehicleServer *get_instance() {
                return singleton;
            }

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline RoadVehicleServer *singleton = nullptr;

            /* A road shorter than this [m] is a point and leads nowhere: its way on would never take
             * a vehicle further, and a ring of such roads would be walked for ever */
            static constexpr double MIN_PASSABLE_LENGTH = 0.02;
            /* The most roads a vehicle looks ahead over, whatever their lengths */
            static constexpr uint32_t MAX_ROUTE_ROADS = 256;

            struct Placement {
                    /* Held so the step does not look it up per vehicle ahead */
                    Ref<RoadVehicleController> controller;
                    RID road;
                    /* From the road's start [m] */
                    double offset = 0.0;
                    /* The end it travels toward - its front faces that way */
                    RoadServer::RoadEnd toward = RoadServer::ROAD_END_END;
                    /* The way on it chose, the next road first */
                    Vector<RoadServer::Link> route;
                    /* The track it is reported on (RoadServer.road_get_track()) */
                    RID track;
                    Transform3D transform;
            };

            RIDMap<Placement> vehicles;

            /* The road the step-th link of the vehicle's way leads onto from p_road, travelled toward
             * p_toward: the one it chose, else one chosen now at random; false at a dead end */
            static bool _route_link(
                    Placement &p_placement, uint32_t p_step, const RID &p_road, RoadServer::RoadEnd p_toward,
                    RoadServer::Link &p_r_link);
            /* The velocity [m/s] the vehicle may have in a step of p_delta [s]: what it wants, its
             * own maximum, the limits of the roads ahead, the vehicle ahead in its lane and a dead
             * end, each braked to within its acceleration limit */
            double _allowed_velocity(const RID &p_vehicle, Placement &p_placement, double p_delta);
            /* What the vehicles on p_road let it go at, the road entered p_distance [m] ahead
             * (negative: the one it is on, entered that far behind it), travelled toward p_toward;
             * p_step_distance [m] is how far the step takes it */
            double _vehicles_allowed_velocity(
                    const RID &p_vehicle, const Placement &p_placement, const RID &p_road, RoadServer::RoadEnd p_toward,
                    double p_distance, double p_step_distance, double p_delta) const;
            /* Along its way by p_distance [m]; false when a listener of what it entered freed it */
            bool _move(const RID &p_vehicle, Placement &p_placement, double p_distance);
            /* It is on p_road now, travelling toward p_toward, and reported there; false when a
             * listener of the reports freed it */
            bool _enter(const RID &p_vehicle, Placement &p_placement, const RID &p_road, RoadServer::RoadEnd p_toward);
            void _place(Placement &p_placement) const;
            void _on_vehicle_controller_changed(const RID &p_vehicle);

        protected:
            static void _bind_methods();

        public:
            /* The vehicle stands on a road now - it was put there (vehicle_set_road()) */
            static const char *vehicle_placed_signal;
            /* The vehicle is somewhere else on its road - once a step it moved in; what draws it
             * follows */
            static const char *vehicle_placement_changed_signal;

            RoadVehicleServer();
            ~RoadVehicleServer() override;

            /* A VehicleServer vehicle becomes a road vehicle - called by whatever makes it one. A
             * vehicle freed by VehicleServer is detached on its own. */
            void vehicle_attach(const RID &p_vehicle);
            void vehicle_detach(const RID &p_vehicle);
            bool vehicle_is_attached(const RID &p_vehicle) const;
            /* The vehicle stands p_offset [m] from the road's start, kept within the road, its front
             * toward p_facing */
            void
            vehicle_set_road(const RID &p_vehicle, const RID &p_road, double p_offset, RoadServer::RoadEnd p_facing);
            RID vehicle_get_road(const RID &p_vehicle) const;
            Transform3D vehicle_get_transform(const RID &p_vehicle) const;
            /* The roads of the vehicle's way ahead as far as p_distance [m], the one it is on first -
             * the way it takes, chosen that far where it was not yet */
            TypedArray<RoadRouteSegment> vehicle_trace_route(const RID &p_vehicle, double p_distance);

            void stepping_advance(
                    const Vector<RID> &p_vehicles, const Vector<Ref<VehicleController>> &p_controllers,
                    double p_delta) override;
    };
} // namespace godot
