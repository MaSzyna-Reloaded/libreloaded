#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleWiperListItem.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>

namespace godot {
    class VehicleController;

    /* The vehicle's wipers: the positions of their switch, each with the wipers it sets going and
     * how they sweep, and where every wiper stands. The switch is this component's; how the wipers
     * move is the backend's. */
    class RailVehicleWipers : public RailVehicleComponent {
            GDCLASS(RailVehicleWipers, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_WIPERS;
            }

        private:
            static void _bind_methods();

            int switch_position = 0;
            double angle = 0.0;
            int default_position = 0;
            // Number of wipers of the vehicle model, set by the vehicle factory. 0: the highest wiper
            // the list switches on.
            int wiper_count = 0;
            TypedArray<RailVehicleWiperListItem> positions;
            void _set_switch_position(int p_position);

        protected:
            void _register_commands() override;
            void _unregister_commands() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            int get_switch_position() const;
            /* 0..1 sweeping out, 1..2 on the way back - wiper_pos of the original scene uniforms
             * (opengl33renderer.cpp:762-766) */
            virtual PackedFloat64Array get_sweep_positions() const = 0;
            void switch_increase();
            void switch_decrease();
            MAKE_GS(double, angle);
            /* The switch stands at the default (Mover.cpp:11942) whenever either is set - an applied
             * configuration leaves it where the driver moved it */
            int get_default_position() const;
            void set_default_position(int p_value);
            MAKE_GS(int, wiper_count);
            TypedArray<RailVehicleWiperListItem> get_positions() const;
            void set_positions(const TypedArray<RailVehicleWiperListItem> &p_value);
    };
} // namespace godot
