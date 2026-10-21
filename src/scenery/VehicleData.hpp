#pragma once
#include "macros.hpp"
#include "tracks/TrackServer.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    /* A vehicle of a trainset (TrainsetData): which vehicle of the catalogue it is
     * (VehicleCatalogServer), in which livery, what it is in the trainset and how it stands. A
     * provider's subclass keeps what it knows of it besides. Its name, for the player to read (none
     * shows the vehicle_id), is the resource's name. */
    class VehicleData : public Resource {
            GDCLASS(VehicleData, Resource)

        public:
            /// Who is aboard: nobody - the vehicle is not driven at all - or a driver in its front or
            /// rear cab
            enum DriverType {
                DRIVER_NOBODY,
                DRIVER_HEAD,
                DRIVER_REAR,
            };

        private:
            /* The name of the provider the vehicle is the one of (VehicleCatalogServer.provider_register());
             * empty for the project's default */
            StringName vehicle_provider;
            /* What the provider finds the vehicle by (VehicleInfo.vehicle) */
            String vehicle;
            /* The name of its livery, one of VehicleCatalogServer.livery_list(); empty for the
             * provider's default */
            String livery;
            /* The vehicle's id, what finds it; may be empty or repeated */
            String vehicle_id;
            /* Reversed in its trainset or not */
            TrackServer::Direction direction = TrackServer::DIRECTION_NORMAL;
            /* How far it stands behind the vehicle before it [m] */
            double gap = 0.0;
            /* Its coupling with the next vehicle of the trainset, a mask of
             * RailVehicleController.CouplingFlags */
            int coupling = 0;
            /* The velocity it starts with [km/h] - its trainset's, along the track; a reversed
             * vehicle takes it negated */
            double velocity = 0.0;
            DriverType driver_type = DRIVER_NOBODY;
            /* What it carries and how much */
            String load_name;
            double load_amount = 0.0;
            /* In a trainset as the player arranged it (SceneryContentNode.trainset_override): its place
             * in the trainset as the scenery declares it, front first, by which it is paired with its
             * declared vehicle; -1: a vehicle added to it */
            int trainset_index = -1;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(StringName, vehicle_provider);
            MAKE_GS(String, vehicle);
            MAKE_GS(String, livery);
            MAKE_GS(String, vehicle_id);
            MAKE_GS_NR(TrackServer::Direction, direction);
            MAKE_GS_NR(double, gap);
            MAKE_GS_NR(int, coupling);
            MAKE_GS_NR(double, velocity);
            MAKE_GS_NR(DriverType, driver_type);
            MAKE_GS(String, load_name);
            MAKE_GS_NR(double, load_amount);
            MAKE_GS_NR(int, trainset_index);

            /// The entry becomes the vehicle p_other is, in its livery, turned the way p_other is
            void take_vehicle_of(const Ref<VehicleData> &p_other);
    };
} // namespace godot

VARIANT_ENUM_CAST(VehicleData::DriverType)
