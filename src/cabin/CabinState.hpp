#pragma once
#include "vehicles/base/VehicleComponent.hpp"
#include "vehicles/base/VehicleComponentType.hpp"
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* State of one cabin - a VehicleServer cabin, of the vehicle it is attached to. Owned by
     * CabinSystem and handed to every registered cabin control handler, which may read and modify it.
     *
     * The vehicle is reached only by its VehicleServer handle (commands and state).
     *
     * A cab works three vehicles: the one it is in, the one its controls drive (an EMU's motor car)
     * and the one carrying its pantographs. A control names its own (Target). */
    class CabinState : public RefCounted {
            GDCLASS(CabinState, RefCounted)

        public:
            enum Target {
                /// The vehicle the cab is in
                OCCUPIED,
                /// The vehicle its controls drive
                CONTROLLED,
                /// The vehicle carrying its pantographs, the controlled one without it
                PANTOGRAPH_UNIT,
            };

            /// A light of the cab itself, as a cab element follows it
            enum Light {
                NONE,
                /// The cab light, at its level (CabinSystem.cabin_light_level_changed)
                CAB,
                /// The instrument light (CabinSystem.cabin_instrument_light_changed)
                INSTRUMENT,
                /// The dashboard light (CabinSystem.cabin_dashboard_light_changed)
                DASHBOARD,
                /// The timetable light (CabinSystem.cabin_timetable_light_changed)
                TIMETABLE,
            };

        private:
            RID vehicle_rid;
            RID cabin;
            /// control_id -> current value of the physical control (button pressed, switch position, ...)
            Dictionary values;
            /// Private state of the cabin logic behaviours (timers, state machines, ...)
            Dictionary data;
            /// The level (0..1) the cab light shines at. Written only by CabinSystem.cabin_set_light_level()
            double light_level = 0.0;
            /// Written only by CabinSystem.cabin_set_instrument_light_enabled()
            bool instrument_light_enabled = false;
            /// Written only by CabinSystem.cabin_set_dashboard_light_enabled()
            bool dashboard_light_enabled = false;
            /// Written only by CabinSystem.cabin_set_timetable_light_enabled()
            bool timetable_light_enabled = false;

        protected:
            static void _bind_methods();

        public:
            /// Called once, by CabinSystem when it makes the state of p_cabin
            void setup(const RID &p_cabin);

            void set_vehicle_rid(const RID &p_value);
            RID get_vehicle_rid() const;
            void set_cabin(const RID &p_value);
            RID get_cabin() const;
            void set_values(const Dictionary &p_value);
            Dictionary get_values() const;
            void set_data(const Dictionary &p_value);
            Dictionary get_data() const;
            void set_light_level(double p_value);
            double get_light_level() const;
            void set_instrument_light_enabled(bool p_value);
            bool get_instrument_light_enabled() const;
            void set_dashboard_light_enabled(bool p_value);
            bool get_dashboard_light_enabled() const;
            void set_timetable_light_enabled(bool p_value);
            bool get_timetable_light_enabled() const;

            Variant get_value(const StringName &p_control_id, const Variant &p_default = Variant()) const;
            /// Emits CabinSystem's control_changed when the value changes
            void set_value(const StringName &p_control_id, const Variant &p_value);
            /* Whether a push control ends up pressed by the manipulation: held, or toggled to the
             * given value - a toggle without one (e.g. from the console) flips its current position */
            bool is_pressed(const StringName &p_control_id, const StringName &p_action, const Variant &p_value) const;
            /// The vehicle's state, through the cab's own system
            Dictionary vehicle_state() const;
            /// The vehicle of a target of the cab in p_occupied
            static RID vehicle_of(const RID &p_occupied, Target p_target);
            /// One named value of a target's state - what a control driven by a state's name reads
            Variant vehicle_state_value(
                    const String &p_key, const Variant &p_default_value = Variant(), Target p_target = OCCUPIED) const;
            /* One component of a target - what a cab logic reads on every step: the dump is composed
             * anew for each read in a step */
            Ref<VehicleComponent>
            vehicle_component(VehicleComponentType::Type p_type, Target p_target = OCCUPIED) const;
            /// A command of a control, to its target
            Variant send_vehicle_command(
                    const StringName &p_command, const Variant &p_p1 = Variant(), const Variant &p_p2 = Variant(),
                    Target p_target = OCCUPIED) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(CabinState::Target);
VARIANT_ENUM_CAST(CabinState::Light);
