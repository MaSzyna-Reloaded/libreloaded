#pragma once
#include "ScenarioEventAction.hpp"
#include "ScenarioEventCondition.hpp"
#include "ScenarioEventImplementation.hpp"
#include <functional>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <queue>
#include <vector>

namespace godot {
    /// RID based registry and queue of the scenario's events (the original's event_manager,
    /// Event.h:638) - the dispatcher of a scenario's events.
    ///
    /// An event is a delay, an optional condition and an action (ScenarioEventAction); the server
    /// knows no event types - what `updatevalues` or `lights` means is the business of an action.
    /// A queued event runs once its time comes: its condition is tested, then the action's run()
    /// or run_else() is called. The time is SimulationServer's simulation time, in seconds, which
    /// the physics and the drivers read too.
    ///
    /// An event may belong to an event system (system_create()), whose implementation
    /// (ScenarioEventImplementation) says what more its events are: which may not be queued, when
    /// one runs, what follows its run - and keeps entries of its own in the queue
    /// (system_schedule()). The original's event system is a plugin's implementation.
    class ScenarioEventServer : public Object {
            GDCLASS(ScenarioEventServer, Object)

        public:
            static const char *event_queued_signal;
            static const char *event_launched_signal;
            static const char *event_dequeued_signal;
            /// An event went (event_free()): whoever holds it forgets it
            static const char *event_freed_signal;

            static ScenarioEventServer *get_instance() {
                return singleton;
            }

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline ScenarioEventServer *singleton = nullptr;

            struct EventData {
                    StringName name;
                    double delay = 0.0;
                    double random_delay = 0.0;
                    Ref<ScenarioEventAction> action;
                    Ref<ScenarioEventCondition> condition;
                    /// The event system it belongs to, none for none
                    RID system;
                    /// The sequence of its entry in the queue, 0 while it is not queued
                    uint64_t queued_sequence = 0;
                    /// ...and the time the entry runs at
                    double run_time = 0.0;
            };

            struct SystemData {
                    Ref<ScenarioEventImplementation> implementation;
            };

            /// An event to run, or an entry of a system's implementation (system_schedule()), at a
            /// time; the sequence keeps entries of one time in the order they were queued
            struct QueueEntry {
                    double time = 0.0;
                    uint64_t sequence = 0;
                    RID owner;
                    RID activator;
                    /// The system an implementation's entry is of, none for an event
                    RID system;

                    bool operator>(const QueueEntry &p_other) const {
                        return time == p_other.time ? sequence > p_other.sequence : time > p_other.time;
                    }
            };

            HashMap<RID, EventData> events;
            HashMap<RID, SystemData> systems;
            HashMap<StringName, RID> events_by_name;
            std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> queue;
            uint64_t next_sequence = 1;
            /// While something is queued it holds the runtime's clock and runs as it advances
            bool processing = false;

            void _set_processing(bool p_processing);
            void _process_queue(double p_seconds);
            /// Puts the entry in the queue, returns its sequence
            uint64_t _schedule(const RID &p_owner, double p_time, const RID &p_activator, const RID &p_system);
            /// The implementation of the event's system, null for none
            Ref<ScenarioEventImplementation> _get_event_implementation(const EventData &p_event) const;

        protected:
            static void _bind_methods();

        public:
            ScenarioEventServer();
            ~ScenarioEventServer() override;

            /// An event system, of no implementation yet
            RID system_create();
            /// Its implementation is detached first; its events stay, of no system
            void system_free(const RID &p_system);
            /// The implementation of the system's events (system_detached() of the previous one,
            /// system_attached() of this one)
            void
            system_attach_implementation(const RID &p_system, const Ref<ScenarioEventImplementation> &p_implementation);
            Ref<ScenarioEventImplementation> system_get_implementation(const RID &p_system) const;
            /// The event belongs to the system from now on
            void system_add_event(const RID &p_system, const RID &p_event);
            /// The system's implementation is called back (ScenarioEventImplementation::scheduled())
            /// for p_owner at p_time of simulation time, in the queue's order; returns the entry's
            /// ticket, passed back with it
            int64_t system_schedule(const RID &p_system, const RID &p_owner, double p_time);

            RID event_create();
            void event_free(const RID &p_event);
            bool event_exists(const RID &p_event) const;
            /// The event system the event belongs to, none for none
            RID event_get_system(const RID &p_event) const;
            void event_set_name(const RID &p_event, const StringName &p_name);
            StringName event_get_name(const RID &p_event) const;
            RID event_get_rid_by_name(const StringName &p_name) const;
            void event_set_delay(const RID &p_event, double p_seconds);
            double event_get_delay(const RID &p_event) const;
            /// Up to this many seconds more, drawn each time the event is queued
            void event_set_random_delay(const RID &p_event, double p_seconds);
            double event_get_random_delay(const RID &p_event) const;
            void event_attach_action(const RID &p_event, const Ref<ScenarioEventAction> &p_action);
            Ref<ScenarioEventAction> event_get_action(const RID &p_event) const;
            void event_attach_condition(const RID &p_event, const Ref<ScenarioEventCondition> &p_condition);
            Ref<ScenarioEventCondition> event_get_condition(const RID &p_event) const;
            /// Queues the event to run after its delay plus p_extra_delay - as its system's
            /// implementation lets it, and when it says. An event is queued once: while it waits,
            /// another request is refused and its activator is dropped (event_manager::AddToQuery,
            /// Event.cpp:2380-2462).
            bool event_queue(const RID &p_event, const RID &p_activator = RID(), double p_extra_delay = 0.0);
            bool event_is_queued(const RID &p_event) const;
            /// The simulation time (SimulationServer.simulation_get_time()) a queued event runs at,
            /// negative when it is not queued
            double event_get_run_time(const RID &p_event) const;
            /// The queued events, in the order they run
            TypedArray<RID> queue_get_events() const;
    };
} // namespace godot
