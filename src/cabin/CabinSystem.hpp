#pragma once
#include "cabin/CabinComponentInfo.hpp"
#include "cabin/CabinImplementation.hpp"
#include "cabin/CabinLogic.hpp"
#include "cabin/CabinState.hpp"
#include "utils/RIDHash.hpp"
#include "vehicles/base/VehicleComponent.hpp"
#include "vehicles/base/VehicleComponentType.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    class Cabin3D;

    /* The cabin layer, kept apart from vehicle commands (VehicleServer.vehicle_send_command), which
     * execute on the vehicle immediately.
     *
     * Holds a CabinState per cabin and a registry of cabin control handlers. A cabin is its
     * VehicleServer handle (RailVehicleServer knows which of the vehicle's cabs it is), a vehicle its
     * VehicleServer handle - never its scenery name, which two vehicles may share and one may lack.
     * Cabin controls only report manipulations through act(); the handlers are registered by the
     * CabinLogic attached to the vehicle (a plugin's) - for the cabin its driver sits in - and
     * translate them into vehicle commands. CabinSystem itself has no cabin logic and forwards
     * nothing by default. */
    class CabinSystem : public Object {
            GDCLASS(CabinSystem, Object)

        public:
            static const char *control_changed_signal;
            static const char *vehicle_command_received_signal;
            static const char *vehicle_cabin_scene_changed_signal;
            static const char *vehicle_cabin_built_signal;
            static const char *cabin_light_level_changed_signal;
            static const char *cabin_instrument_light_changed_signal;
            static const char *cabin_dashboard_light_changed_signal;
            static const char *cabin_timetable_light_changed_signal;

            static CabinSystem *get_instance() {
                return singleton;
            }

            CabinSystem();
            ~CabinSystem() override;

        private:
            /// The one instance: set by the constructor, cleared by the destructor
            static inline CabinSystem *singleton = nullptr;

            /// Manipulations a control can report (press/release/repeat/set events)
            LocalVector<StringName> actions;

            /// By cabin: its state, its control handlers by id, the callables run with the simulation
            HashMap<RID, Ref<CabinState>, RIDHasher, RIDComparator> states;
            HashMap<RID, HashMap<StringName, Callable>, RIDHasher, RIDComparator> controls;
            /* The lists are replaced, never changed in place, so a step walks the one it started with
             * even when a callable registers or unregisters on the way */
            HashMap<RID, Array, RIDHasher, RIDComparator> processes;
            /// By cabin: the callables that only work out what the cab shows (register_display_process())
            HashMap<RID, Array, RIDHasher, RIDComparator> display_processes;
            HashMap<RID, Ref<CabinLogic>, RIDHasher, RIDComparator> cab_logics;
            /// The cab interior each vehicle's crew sits in, and the one shown, by instance id
            HashMap<RID, Ref<PackedScene>, RIDHasher, RIDComparator> cabin_scenes;
            HashMap<RID, ObjectID, RIDHasher, RIDComparator> cabins;
            /// Values of a vehicle's state the cab computes rather than reads, by name
            HashMap<RID, HashMap<String, Callable>, RIDHasher, RIDComparator> state_computed_values;
            /// What builds the cabs, by the name a cab gives (Cabin3D::implementation)
            HashMap<StringName, Ref<CabinImplementation>> implementations;
            /// Whether what is held of the scripts is let go when the tree goes away (_follow_tree_exit())
            bool following_tree_exit = false;

            void _on_vehicle_freed(const RID &p_vehicle_rid);
            void _on_vehicle_command_received(
                    const RID &p_vehicle_rid, const String &p_command, const Variant &p_p1, const Variant &p_p2);
            void _on_vehicle_driver_cabin_changed(const RID &p_vehicle_rid, const RID &p_cabin);
            void _on_simulation_advanced(double p_seconds);
            void _follow_tree_exit();
            void _release_scripts();
            /// The cab interior of the vehicle while it is shown (cabin_show()), else null
            Cabin3D *_shown_cabin(const RID &p_vehicle_rid) const;
            /// A cab whose interior is not shown works out what it shows now, when it is asked for it
            void _refresh_display(const RID &p_cabin);
            void _run_vehicle_display_processes(const RID &p_vehicle_rid);
            /// The id of a cabin's vehicle, for a message
            static String _cabin_vehicle_id(const RID &p_cabin);

        protected:
            static void _bind_methods();

        public:
            Dictionary vehicle_state(const RID &p_vehicle_rid) const;
            Variant vehicle_state_value(
                    const RID &p_vehicle_rid, const String &p_key, const Variant &p_default_value = Variant());
            void
            state_computed_value_register(const RID &p_vehicle_rid, const String &p_key, const Callable &p_callable);
            void state_computed_value_unregister(const RID &p_vehicle_rid, const String &p_key);
            bool vehicle_has_low_voltage(const RID &p_vehicle_rid) const;
            Dictionary vehicle_config(const RID &p_vehicle_rid) const;
            Ref<VehicleComponent> vehicle_component(const RID &p_vehicle_rid, VehicleComponentType::Type p_type) const;
            void vehicle_attach_cab_logic(const RID &p_vehicle_rid, const Ref<CabinLogic> &p_logic);
            Ref<CabinLogic> vehicle_get_cab_logic(const RID &p_vehicle_rid) const;
            void vehicle_set_cabin_scene(const RID &p_vehicle_rid, const Ref<PackedScene> &p_scene);
            Ref<PackedScene> vehicle_get_cabin_scene(const RID &p_vehicle_rid) const;
            Cabin3D *cabin_show(const RID &p_cabin_rid, Node *p_parent);
            void vehicle_hide_cabin(const RID &p_vehicle_rid);

            void cabin_set_light_level(const RID &p_cabin, double p_level);
            double cabin_get_light_level(const RID &p_cabin);
            void cabin_set_instrument_light_enabled(const RID &p_cabin, bool p_enabled);
            bool cabin_get_instrument_light_enabled(const RID &p_cabin);
            void cabin_set_dashboard_light_enabled(const RID &p_cabin, bool p_enabled);
            bool cabin_get_dashboard_light_enabled(const RID &p_cabin);
            void cabin_set_timetable_light_enabled(const RID &p_cabin, bool p_enabled);
            bool cabin_get_timetable_light_enabled(const RID &p_cabin);

            Ref<CabinState> get_cabin_state(const RID &p_cabin);
            /// p_handler(state: CabinState, action: StringName, value: Variant) -> Variant
            void register_control(const RID &p_cabin, const StringName &p_control_id, const Callable &p_handler);
            void unregister_control(const RID &p_cabin, const StringName &p_control_id, const Callable &p_handler);
            bool has_control(const RID &p_cabin, const StringName &p_control_id) const;
            Array get_controls(const RID &p_cabin) const;
            void register_process(const RID &p_cabin, const Callable &p_callable);
            void unregister_process(const RID &p_cabin, const Callable &p_callable);
            void register_display_process(const RID &p_cabin, const Callable &p_callable);
            void unregister_display_process(const RID &p_cabin, const Callable &p_callable);
            Variant
            act(const RID &p_cabin, const StringName &p_control_id, const StringName &p_action,
                const Variant &p_value = Variant());
            Variant get_control(const RID &p_cabin, const StringName &p_control_id);
            /// Manipulations a control can report (act())
            PackedStringArray get_actions() const;
            Dictionary get_state(const RID &p_cabin);

            /// p_implementation builds the cabs that name p_name; a name registered again is built by the new one
            void implementation_register(const StringName &p_name, const Ref<CabinImplementation> &p_implementation);
            void implementation_unregister(const StringName &p_name);
            /* The controls the implementation registered as p_implementation can make
             * (CabinImplementation::component_list()); none for no implementation */
            TypedArray<CabinComponentInfo> component_list(const StringName &p_implementation) const;
            /* A control the implementation registered as p_implementation makes by p_name, with its
             * defaults (CabinImplementation::component_create()); null for no such one */
            Node3D *component_create(const StringName &p_implementation, const StringName &p_name) const;
    };
} // namespace godot
