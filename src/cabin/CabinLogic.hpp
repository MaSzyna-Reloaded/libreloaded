#pragma once
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
    /* The logic of a vehicle's cabs, attached to the vehicle in CabinSystem
     * (vehicle_attach_cab_logic()). It registers the control handlers of one cabin in CabinSystem;
     * CabinSystem registers it for the cabin the vehicle's driver sits in and moves it along when the
     * driver changes cabins.
     * It needs no 3D cab: whoever drives - the player by keys and mouse, the AI - reports
     * manipulations with CabinSystem.act(). The player's hand on a control - a key, a click on its
     * widget - is one of press(), release(), increase(), decrease(), which decide what the control
     * does with it as the cab has it. A plugin's cab logic derives from it and gives the virtuals. */
    class CabinLogic : public RefCounted {
            GDCLASS(CabinLogic, RefCounted)

        public:
            /// The driver's hand on a control, as a key moves it: press() and increase()/decrease()
            enum Gesture {
                PRESS,
                INCREASE,
                DECREASE,
            };

        protected:
            static void _bind_methods();

            GDVIRTUAL2(_register, RID, RID)
            GDVIRTUAL0(_unregister)
            GDVIRTUAL1(_input, Ref<InputEvent>)
            GDVIRTUAL1(_press, StringName)
            GDVIRTUAL1(_release, StringName)
            GDVIRTUAL1(_increase, StringName)
            GDVIRTUAL1(_decrease, StringName)
            // the gesture as its number: the enum gets its type info only below the class
            GDVIRTUAL2RC(StringName, _get_action, StringName, int)

        public:
            /// Registers the handlers of the vehicle's p_cabin (a VehicleServer cabin)
            void register_cabin(const RID &p_vehicle_rid, const RID &p_cabin);
            void unregister();
            /// The player's keys of the cab's controls
            void input(const Ref<InputEvent> &p_event);
            /// The driver's hand pushes the control: a button is pressed, a two-position switch flipped
            void press(const StringName &p_control_id);
            /// The driver's hand lets the control go: a push button springs back, a held lever stops
            void release(const StringName &p_control_id);
            /// The driver's hand moves the control one position (or, held, a knob) up
            void increase(const StringName &p_control_id);
            /// The driver's hand moves the control one position (or, held, a knob) down
            void decrease(const StringName &p_control_id);
            /// The input action whose key makes p_gesture on the control in this cab; empty when no key does
            StringName get_action(const StringName &p_control_id, Gesture p_gesture) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(CabinLogic::Gesture);
