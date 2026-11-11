#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    /* A vehicle: which vehicle of the catalogue it is (VehicleCatalogServer), in which livery, who is
     * aboard and what it carries. Where it stands in a trainset is the trainset's
     * (TrainsetMemberData). A provider's subclass keeps what it knows of it besides. Its name, for
     * the player to read (none shows the vehicle_id), is the resource's name. */
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
            DriverType driver_type = DRIVER_NOBODY;
            /* What it carries and how much */
            String load_name;
            double load_amount = 0.0;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(StringName, vehicle_provider);
            MAKE_GS(String, vehicle);
            MAKE_GS(String, livery);
            MAKE_GS(String, vehicle_id);
            MAKE_GS_NR(DriverType, driver_type);
            MAKE_GS(String, load_name);
            MAKE_GS_NR(double, load_amount);

            /// It becomes the vehicle p_other is, in its livery and under its name - the id stays, what
            /// finds it
            void take_vehicle_of(const Ref<VehicleData> &p_other);
    };
} // namespace godot

VARIANT_ENUM_CAST(VehicleData::DriverType)
