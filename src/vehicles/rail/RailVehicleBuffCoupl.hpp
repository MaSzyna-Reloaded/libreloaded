#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"

namespace godot {
    class VehicleController;
    class RailVehicleBuffCoupl : public RailVehicleComponent {
            GDCLASS(RailVehicleBuffCoupl, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_BUFFERS;
            }

        private:
            static void _bind_methods();

        public:
            enum CouplerType {
                COUPLER_TYPE_AUTOMATIC,
                COUPLER_TYPE_SCREW,
                COUPLER_TYPE_CHAIN,
                COUPLER_TYPE_BARE,
                COUPLER_TYPE_ARTICULATED
            };
            enum BufferLocation { BUFFER_LOCATION_FRONT, BUFFER_LOCATION_BACK, BUFFER_LOCATION_BOTH };

        private:
            double buffer_stiffness_k = 1.0;
            double buffer_max_compression_tolerance = 0.1;
            double buffer_max_tension_tolerance = 1000.0;
            double coupler_stiffness_k = 1.0;
            double coupler_max_compression_tolerance = 0.1;
            double coupler_max_tension_tolerance = 1000.0;
            // what the coupler can join, and what it joins by itself
            BitField<RailVehicleController::CouplingFlags> allowed_flag = RailVehicleController::COUPLING_FLAG_NONE;
            BitField<RailVehicleController::CouplingFlags> automatic_flag = RailVehicleController::COUPLING_FLAG_NONE;
            // TCoupling::PowerFlag (MOVER.h:773): 24V and 110V pass unless the FIZ says otherwise
            BitField<RailVehicleController::CouplingFlags> power_flag =
                    RailVehicleController::COUPLING_FLAG_POWER_24V | RailVehicleController::COUPLING_FLAG_POWER_110V;
            // TCoupling::PowerCoupling (MOVER.h:774): the coupling power passes by
            BitField<RailVehicleController::CouplingFlags> power_coupling =
                    RailVehicleController::COUPLING_FLAG_PERMANENT;
            String control_type = "";
            CouplerType coupler_type = CouplerType::COUPLER_TYPE_AUTOMATIC;
            BufferLocation buffer_location = BufferLocation::BUFFER_LOCATION_FRONT;

        public:
            MAKE_GS(double, buffer_stiffness_k);
            MAKE_GS(double, buffer_max_compression_tolerance);
            MAKE_GS(double, buffer_max_tension_tolerance);
            MAKE_GS(double, coupler_stiffness_k);
            MAKE_GS(double, coupler_max_compression_tolerance);
            MAKE_GS(double, coupler_max_tension_tolerance);
            MAKE_GS_NR(BitField<RailVehicleController::CouplingFlags>, allowed_flag);
            MAKE_GS_NR(BitField<RailVehicleController::CouplingFlags>, automatic_flag);
            MAKE_GS_NR(BitField<RailVehicleController::CouplingFlags>, power_flag);
            MAKE_GS_NR(BitField<RailVehicleController::CouplingFlags>, power_coupling);
            MAKE_GS(String, control_type);
            MAKE_GS_NR(CouplerType, coupler_type);
            MAKE_GS_NR(BufferLocation, buffer_location);
            /* The coupler's strength at an end [N], as the simulation made it of the configuration
             * (FmaxC) */
            virtual double get_coupler_max_force(RailVehicleController::CouplerEnd p_end) const = 0;
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleBuffCoupl::CouplerType)
VARIANT_ENUM_CAST(RailVehicleBuffCoupl::BufferLocation)
