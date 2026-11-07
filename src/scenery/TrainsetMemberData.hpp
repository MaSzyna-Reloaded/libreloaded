#pragma once
#include "macros.hpp"
#include "scenery/VehicleData.hpp"
#include "tracks/TrackServer.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    /* A vehicle of a trainset (TrainsetData) and how it stands in it: which way, how far behind the
     * vehicle before it and how it couples to the next one - a `dynamic` of a scenery's `trainset` */
    class TrainsetMemberData : public Resource {
            GDCLASS(TrainsetMemberData, Resource)

        private:
            /* Which vehicle it is */
            Ref<VehicleData> vehicle;
            /* Reversed in its trainset or not */
            TrackServer::Direction direction = TrackServer::DIRECTION_NORMAL;
            /* How far it stands behind the vehicle before it [m] */
            double gap = 0.0;
            /* How it couples to the next vehicle of the trainset */
            RailVehicleController::CouplingMode coupling_mode = RailVehicleController::COUPLING_MODE_NONE;
            /* In a trainset as the player arranged it (SceneryContentNode.trainset_override): its place
             * in the trainset as the scenery declares it, front first, by which it is paired with its
             * declared member; -1: a vehicle added to it */
            int trainset_index = -1;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS_NR(Ref<VehicleData>, vehicle);
            MAKE_GS_NR(TrackServer::Direction, direction);
            MAKE_GS_NR(double, gap);
            MAKE_GS_NR(RailVehicleController::CouplingMode, coupling_mode);
            MAKE_GS_NR(int, trainset_index);
    };
} // namespace godot
