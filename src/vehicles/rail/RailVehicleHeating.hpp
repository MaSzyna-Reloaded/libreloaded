#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"

namespace godot {
    class VehicleController;

    /* Train heating, as a vehicle offers it - the public contract, with no backend in it.
     *
     * The configuration is the wrapper's own and lives here, because the wrapper's properties and
     * enums are the authoring source of truth. The live values are pure virtual: whichever
     * simulation is underneath answers them, and a caller that took this component through
     * VehicleServer sees the same interface either way. */
    class RailVehicleHeating : public RailVehicleComponent {
            GDCLASS(RailVehicleHeating, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_HEATING;
            }

        private:
            static void _bind_methods();

        public:
            /* Live state */
            virtual bool get_active() const = 0;
            /* The heating switch's own state (HeatingAllow) - what trainheating_sw flips
             * (Train.cpp:6674) */
            virtual bool get_allowed() const = 0;
            virtual double get_power() const = 0;

            /* Original engine: OnCommand_heatingenable/disable (Train.cpp:5256-5296) ->
             * HeatingSwitch(State) - "trainheating_sw:"/ggTrainHeatingButton (Train.cpp:10116). */
            virtual void heating(bool p_enabled) = 0;

        protected:
            void _register_commands() override;
            void _unregister_commands() override;

        private:
            RailVehicleController::TrainPowerSource heating_source =
                    RailVehicleController::TrainPowerSource::POWER_SOURCE_GENERATOR;
            double heating_max_voltage = 0.0;

        public:
            MAKE_GS_NR(RailVehicleController::TrainPowerSource, heating_source);
            MAKE_GS(double, heating_max_voltage);
    };
} // namespace godot
