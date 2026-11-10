#pragma once
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleComponentType.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"
#include "vehicles/rail/RailVehiclePantograph.hpp"

#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* The pantographs of a vehicle (TAnimPant, DynObj.h:106 of the original): as many as its data
     * gives, each built as its configuration says (RailVehiclePantograph) and raised as far as the
     * vehicle's state is. RailVehicleServer steps them under the wire it finds over each one
     * (pantograph_step()); whatever draws the vehicle poses their arms from the raise getters. The
     * raise outlives a configuration changed: rebuilt, a raised pantograph keeps its slider's height
     * where the new arms allow it (_apply_configuration()). */
    class RailVehiclePantographs : public RailVehicleComponent {
            GDCLASS(RailVehiclePantographs, RailVehicleComponent);

        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_PANTOGRAPHS;
            }

            /* The share of the gap to the wire closed in one step up and down, how fast a lowered
             * arm falls [rad/s]; the gap within which it stays, and the gap under which the slider
             * touches the wire [m] (LIBMASZYNA_SPLIT_SPECS.md, 1) */
            static constexpr double RISE_SHARE = 0.55;
            static constexpr double PRESS_SHARE = 0.4;
            static constexpr double FALL_RATE = 0.15;
            static constexpr double SETTLED_GAP = 0.001;
            static constexpr double CONTACT_GAP = 0.01;
            /* Halvings of the lower arm's range that find the angle of a slider's height
             * (pantograph_fit()) - far below a millimetre */
            static constexpr int FIT_STEPS = 40;

        protected:
            static void _bind_methods();
            void _apply_configuration() override;
            void _fill_state_dictionary(Dictionary &p_state) const override;

        private:
            /* A pantograph's raise - fAngleL, fAngleU and PantWys of the original; how it is built
             * is its configuration's (pantographs). All zero is a pantograph not raised. */
            struct Pantograph {
                    double lower_angle = 0.0;
                    double upper_angle = 0.0;
                    double height = 0.0;
                    /* the slider reaches the wire (PantDiff < 0.01) */
                    bool reaches_wire = false;
            };

            TypedArray<RailVehiclePantograph> pantographs;
            LocalVector<Pantograph> states;

            /* The configuration of a pantograph, null for an index out of range */
            Ref<RailVehiclePantograph> _get_configuration(int p_index) const;
            /* The upper arm's angle for the lower one's, false where the arms cannot stand so */
            static bool
            _upper_angle(const Ref<RailVehiclePantograph> &p_config, double p_lower_angle, double &p_r_upper_angle);
            static double
            _height(const Ref<RailVehiclePantograph> &p_config, double p_lower_angle, double p_upper_angle);

        public:
            TypedArray<RailVehiclePantograph> get_pantographs() const;
            void set_pantographs(const TypedArray<RailVehiclePantograph> &p_pantographs);

            int get_pantograph_count() const;
            RailVehicleEnginePowerSource::PantographSelector get_pantograph_control(int p_index) const;
            bool get_pantograph_has_geometry(int p_index) const;
            /* Where it stands in the vehicle's own space - zero for one without geometry */
            Vector3 get_pantograph_position(int p_index) const;
            /* The slider's height over the pantograph's position [m] */
            double get_pantograph_height(int p_index) const;
            /* Always true for one without geometry */
            bool get_pantograph_reaches_wire(int p_index) const;
            /* How far the lower and the upper arm are raised over lowered [rad] - what is drawn */
            double get_pantograph_lower_raise(int p_index) const;
            double get_pantograph_upper_raise(int p_index) const;

            /* One step of the arms, p_gap [m] below the wire: towards it at p_speed_factor while
             * raised, down while lowered (LIBMASZYNA_SPLIT_SPECS.md, 1). A pantograph without
             * geometry is not moved. */
            void pantograph_step(int p_index, double p_gap, bool p_active, double p_speed_factor, double p_delta);
            /* The slider's height kept as far as the arms allow it: no lower than at rest, no higher
             * than they reach nor than p_max_height (the wire over it) */
            void pantograph_fit(int p_index, double p_max_height);
    };
} // namespace godot
