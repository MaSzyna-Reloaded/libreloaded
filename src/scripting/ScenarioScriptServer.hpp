#pragma once
#include "ScenarioScriptCabinImplementation.hpp"
#include "ScriptRuntime.hpp"
#include "utils/RIDHash.hpp"
#include <functional>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/scene_tree_timer.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// Runs a scenario's scripts (the original's `lua <file>`, simulationstateserializer.cpp:346).
    ///
    /// A context is one scenario's script environment: the code loaded into it, and everything
    /// its scripts made - the events they created, their timers and their subscriptions to what
    /// the servers report. context_free() takes all of it along. The scripts reach the simulation
    /// only through the servers' own APIs; no server knows the scripts exist.
    ///
    /// A script's function never runs inside the server that reported something: a timer, an
    /// event and every subscription run as a ScenarioEventServer event, so they follow the
    /// simulation's clock and stop while it is paused, as the scenery's own events do.
    ///
    /// The code a context runs is grouped in units - a file, or what an editor applied under a
    /// name. Applying a unit again first frees whatever its previous run made.
    class ScenarioScriptServer : public Object {
            GDCLASS(ScenarioScriptServer, Object)
            friend class ScenarioScriptAction;

        public:
            /// Whether a timer runs once or keeps running
            enum Timer {
                TIMER_ONCE,
                TIMER_REPEATING,
            };

            /// What a server reports that a script may subscribe to, each about one target; a plugin
            /// registers its own (signal_register())
            enum ScriptSignal : int {
                /// A vehicle received a command (target: the vehicle)
                SIGNAL_VEHICLE_COMMAND_RECEIVED,
                /// A control of a vehicle's cab changed (target: the vehicle)
                SIGNAL_CABIN_CONTROL_CHANGED,
                /// A vehicle came onto the track, travelling towards its start or end, or neither
                /// when it was put there (target: the track)
                SIGNAL_VEHICLE_ENTERED_TRACK,
                /// A vehicle left the track
                SIGNAL_VEHICLE_LEFT_TRACK,
                /// A vehicle on the track started off, or turned the other way
                SIGNAL_VEHICLE_STARTED_ON_TRACK,
                /// A vehicle stopped on the track
                SIGNAL_VEHICLE_STOPPED_ON_TRACK,
                /// The first axle came onto the isolated section (target: the section)
                SIGNAL_ISOLATED_OCCUPIED,
                /// The last axle left the isolated section
                SIGNAL_ISOLATED_FREED,
                /// The switch moved to another track (target: the switch)
                SIGNAL_SWITCH_CHANGED,
                /// The signal head shows another aspect (target: the head)
                SIGNAL_SIGNAL_HEAD_ASPECT_CHANGED,
                /// The route went to another state (target: the route)
                SIGNAL_ROUTE_STATE_CHANGED,
                /// The event ran (target: the event)
                SIGNAL_EVENT_LAUNCHED,
                /// The first of the signals the plugins register
                SIGNAL_FIRST_REGISTERED,
            };

            /// Which of its functions an event runs: its condition passed, or it failed
            enum Branch {
                BRANCH_RUN,
                BRANCH_ELSE,
            };

            /// Which way a vehicle travels along a track, as a script names it
            /// (TrackServer::TravelDirection; nil for TRAVEL_NONE)
            static constexpr const char *TRAVEL_NAMES[] = {"", "start", "end"};
            /// The tracks of a switch, as a script names them (TrackServer::SwitchTrack)
            static constexpr const char *SWITCH_TRACK_NAMES[] = {"common", "diverging"};
            /// The states of a route, as a script names them (SignallingServer::RouteState)
            static constexpr const char *ROUTE_STATE_NAMES[] = {"idle", "setting", "closed", "locked", "releasing"};
            /// No function of the script
            static constexpr int64_t NO_FUNCTION = -1;
            /// What a context says when the build has no runtime (LIBMASZYNA_SPLIT_SPECS.md, 6)
            static constexpr const char *NO_RUNTIME_MESSAGE =
                    "This build has no Lua runtime: scenario scripts do not run.";

            /// A script failed (context: RID, message: String) - it is logged as well
            static const char *script_error_signal;
            /// The context's code ran again (context_reload()) - its scripts may declare other
            /// variables now (context: RID)
            static const char *context_reloaded_signal;

            static ScenarioScriptServer *get_instance() {
                return singleton;
            }
            ~ScenarioScriptServer() override;

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline ScenarioScriptServer *singleton = nullptr;

            enum HookKind {
                /// An event a script created
                HOOK_EVENT,
                HOOK_TIMER_ONCE,
                HOOK_TIMER_REPEATING,
                /// A timer of the clock on the wall, not of the simulation
                HOOK_CLOCK_TIMER_ONCE,
                HOOK_CLOCK_TIMER_REPEATING,
                HOOK_SUBSCRIPTION,
            };

            /// Something a script made that runs one of its functions
            struct Hook {
                    RID context;
                    StringName unit;
                    HookKind kind = HOOK_EVENT;
                    ScriptSignal signal = SIGNAL_FIRST_REGISTERED;
                    RID target;
                    /// The ScenarioEventServer event that runs the function; none for a clock timer
                    RID event;
                    /// A clock timer's: what runs the function, and how long it waits [s]
                    Ref<SceneTreeTimer> clock_timer;
                    double clock_seconds = 0.0;
                    int64_t function = NO_FUNCTION;
                    int64_t else_function = NO_FUNCTION;
                    /// What the servers reported for a subscription since its event last ran
                    Vector<Vector<ScriptArgument>> pending;
            };

            /// Code a context ran, to run again as it was (context_reload()): a file of its base
            /// directory, or a piece of code under its unit's name
            struct Unit {
                    StringName name;
                    /// Empty for a piece of code
                    String file;
                    String source;
                    Vector<ScriptArgument> arguments;
            };

            struct Context {
                    /// Null in a build without a runtime
                    ScriptRuntime *runtime = nullptr;
                    String base_dir;
                    String data_dir;
                    /// In the order they first ran
                    Vector<Unit> units;
                    /// What its owner set the scripts' variables to (context_set_variables())
                    Dictionary variables;
                    /// The variables its scripts declared, with their defaults (script_variable_export())
                    Dictionary exports;
                    Ref<ScenarioScriptCabinImplementation> cabin;
                    Vector<RID> hooks;
                    /// The DrawingServer drawings, fonts and images its scripts made, by the unit
                    /// that made each
                    RIDMap<StringName> drawing_resources;
                    /// The unit whose code is running, which owns what it makes
                    StringName active_unit;
            };

            RIDMap<Context> contexts;
            RIDMap<Hook> hooks;
            /// The subscriptions of each signal, by target
            HashMap<int, RIDMap<Vector<RID>>> subscriptions;

            /// The names of the handle kinds and of the script signals the plugins registered, in
            /// the order of their values from FIRST_REGISTERED and SIGNAL_FIRST_REGISTERED
            static Vector<String> &registered_handle_kinds();
            static Vector<String> &registered_signals();

            RID _create_hook(const RID &p_context, HookKind p_kind, int64_t p_function);
            void _free_hook(const RID &p_hook);
            /// A clock timer's wait, from now
            void _start_clock_timer(const RID &p_hook);
            void _on_clock_timer_timeout(const RID &p_hook);
            /// Frees the drawing resources of the context's unit, all of them for an empty unit
            void _free_drawing_resources(Context &p_context, const StringName &p_unit);
            /// Called by ScenarioScriptAction when the hook's event runs
            void _run_hook(const RID &p_hook, const RID &p_event, const RID &p_activator, Branch p_branch);
            /// The unit is run again as it is now, where it ran first
            void _keep_unit(Context &p_context, const Unit &p_unit);
            /// Runs the unit's file or code in the context
            bool _run_unit(const RID &p_context, const Unit &p_unit);
            /// Runs code of the context as the unit, reports its error; false when it failed
            bool
            _enter(const RID &p_context, const StringName &p_unit,
                   const std::function<String(ScriptRuntime *)> &p_code);
            void _report(const RID &p_context, const String &p_error);

            void _on_vehicle_command_received(
                    const RID &p_vehicle, const String &p_command, const Variant &p_p1, const Variant &p_p2);
            void _on_track_vehicle_entered(const RID &p_track, const RID &p_vehicle, int p_direction);
            void _on_track_vehicle_left(const RID &p_track, const RID &p_vehicle);
            void _on_track_vehicle_started(const RID &p_track, const RID &p_vehicle, int p_direction);
            void _on_track_vehicle_stopped(const RID &p_track, const RID &p_vehicle);
            void _on_isolated_occupied(const RID &p_isolated, const RID &p_vehicle);
            void _on_isolated_freed(const RID &p_isolated, const RID &p_vehicle);
            void _on_switch_active_track_changed(const RID &p_switch, int p_active_track);
            void _on_signal_head_aspect_changed(const RID &p_signal_head, const StringName &p_aspect);
            void _on_route_state_changed(const RID &p_route, int p_state);
            void _on_event_launched(const RID &p_event, const RID &p_activator);
            void _on_cabin_control_changed(
                    const RID &p_cabin, const StringName &p_control_id, const Variant &p_value, const RID &p_context);

        protected:
            static void _bind_methods();

        public:
            ScenarioScriptServer();

            /// A handle kind of a plugin's own things, by its name (a script's type name), from its
            /// ReloadedPlugin::register_classes() - before any context
            static ScriptHandleKind handle_kind_register(const String &p_name);
            /// The name of a handle kind a plugin registered
            static String handle_kind_get_name(ScriptHandleKind p_kind);
            static int handle_kind_get_count();
            /// A script signal of a plugin's own things, by its name, from its
            /// ReloadedPlugin::register_classes(); the plugin delivers it (script_deliver())
            static ScriptSignal signal_register(const String &p_name);

            /// A context whose scripts load their files from p_base_dir
            RID context_create(const String &p_base_dir);
            void context_free(const RID &p_context);
            TypedArray<RID> context_get_rids() const;
            /// Runs a file of the base directory as the unit named after it; false when it failed
            bool context_run_file(const RID &p_context, const String &p_path);
            /// ...handing the file's chunk the arguments (`local drawing = ...`)
            bool
            context_run_file(const RID &p_context, const String &p_path, const Vector<ScriptArgument> &p_arguments);
            /// The directory the scripts read their data files from - fonts, images; the base
            /// directory until it is set
            void context_set_data_dir(const RID &p_context, const String &p_data_dir);
            /// Compiles the code of the unit without running it; returns the error, empty when there
            /// is none
            String context_check_source(const RID &p_context, const StringName &p_unit, const String &p_source);
            /// Frees what the unit made, then runs the code as the unit; false when it failed
            bool context_apply_source(const RID &p_context, const StringName &p_unit, const String &p_source);
            /// ...handing the code's chunk the arguments
            bool context_apply_source(
                    const RID &p_context, const StringName &p_unit, const String &p_source,
                    const Vector<ScriptArgument> &p_arguments);
            /// What the context's scripts get for the variables they declare (reloaded.export_var()) -
            /// set by its owner before the scripts run (LuaNode's inspector)
            void context_set_variables(const RID &p_context, const Dictionary &p_variables);
            /// The variables the context's scripts declared, by their names, with their defaults
            Dictionary context_get_exports(const RID &p_context) const;
            /// Every file the context's code was loaded from, the ones it required too
            PackedStringArray context_get_files(const RID &p_context) const;
            /// Runs the context's code again, as it was first run and in that order: what its scripts
            /// made goes, and the runtime is made anew, so a changed file - a module it required too -
            /// is read again
            void context_reload(const RID &p_context);
            void context_attach_cabin_implementation(
                    const RID &p_context, const Ref<ScenarioScriptCabinImplementation> &p_cabin);
            Ref<ScenarioScriptCabinImplementation> context_get_cabin_implementation(const RID &p_context) const;

            // What the runtimes call for the scripts; what they make belongs to the running unit

            /// An event that runs the functions; the script owns it
            RID script_event_create(const RID &p_context, int64_t p_function, int64_t p_else_function);
            /// Frees an event the script created; false when it did not create it
            bool script_event_free(const RID &p_context, const RID &p_event);
            /// Runs the function after p_seconds of simulation time, once or on and on
            RID script_timer_create(const RID &p_context, double p_seconds, int64_t p_function, Timer p_timer);
            /// Runs the function after p_seconds of the clock on the wall, once or on and on - while
            /// the simulation is paused, and in the editor too
            RID script_clock_timer_create(const RID &p_context, double p_seconds, int64_t p_function, Timer p_timer);
            RID script_subscribe(const RID &p_context, ScriptSignal p_signal, const RID &p_target, int64_t p_function);
            /// Hands the arguments to the target's subscriptions of the signal and queues their
            /// events; only to the subscriptions of p_context when it is valid
            void script_deliver(
                    ScriptSignal p_signal, const RID &p_target, const Vector<ScriptArgument> &p_arguments,
                    const RID &p_activator, const RID &p_context = RID());
            /// A variable the script declares, with its default: what the context's owner set it to,
            /// the default when it set none or set a value of another type
            Variant script_variable_export(const RID &p_context, const String &p_name, const Variant &p_default);
            /// Stops a timer or a subscription; false when it is not the script's
            bool script_cancel(const RID &p_context, const RID &p_subscription);
            /// A DrawingServer drawing, font or image the running unit made: freed with it
            void script_drawing_resource_keep(const RID &p_context, const RID &p_resource);
            /// Frees one the script made; false when it did not make it
            bool script_drawing_resource_free(const RID &p_context, const RID &p_resource);
    };
} // namespace godot
