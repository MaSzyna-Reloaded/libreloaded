#include "ScenarioEventServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "utils/Names.hpp"
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *ScenarioEventServer::event_queued_signal = "event_queued";
    const char *ScenarioEventServer::event_launched_signal = "event_launched";
    const char *ScenarioEventServer::event_dequeued_signal = "event_dequeued";
    const char *ScenarioEventServer::event_freed_signal = "event_freed";

    void ScenarioEventServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("system_create"), &ScenarioEventServer::system_create);
        ClassDB::bind_method(D_METHOD("system_free", "system"), &ScenarioEventServer::system_free);
        ClassDB::bind_method(
                D_METHOD("system_attach_implementation", "system", "implementation"),
                &ScenarioEventServer::system_attach_implementation);
        ClassDB::bind_method(
                D_METHOD("system_get_implementation", "system"), &ScenarioEventServer::system_get_implementation);
        ClassDB::bind_method(D_METHOD("system_add_event", "system", "event"), &ScenarioEventServer::system_add_event);
        ClassDB::bind_method(
                D_METHOD("system_schedule", "system", "owner", "time"), &ScenarioEventServer::system_schedule);
        ClassDB::bind_method(D_METHOD("event_create"), &ScenarioEventServer::event_create);
        ClassDB::bind_method(D_METHOD("event_free", "event"), &ScenarioEventServer::event_free);
        ClassDB::bind_method(D_METHOD("event_exists", "event"), &ScenarioEventServer::event_exists);
        ClassDB::bind_method(D_METHOD("event_get_system", "event"), &ScenarioEventServer::event_get_system);
        ClassDB::bind_method(D_METHOD("event_set_name", "event", "name"), &ScenarioEventServer::event_set_name);
        ClassDB::bind_method(D_METHOD("event_get_name", "event"), &ScenarioEventServer::event_get_name);
        ClassDB::bind_method(D_METHOD("event_get_rid_by_name", "name"), &ScenarioEventServer::event_get_rid_by_name);
        ClassDB::bind_method(D_METHOD("event_set_delay", "event", "seconds"), &ScenarioEventServer::event_set_delay);
        ClassDB::bind_method(D_METHOD("event_get_delay", "event"), &ScenarioEventServer::event_get_delay);
        ClassDB::bind_method(
                D_METHOD("event_set_random_delay", "event", "seconds"), &ScenarioEventServer::event_set_random_delay);
        ClassDB::bind_method(D_METHOD("event_get_random_delay", "event"), &ScenarioEventServer::event_get_random_delay);
        ClassDB::bind_method(
                D_METHOD("event_attach_action", "event", "action"), &ScenarioEventServer::event_attach_action);
        ClassDB::bind_method(D_METHOD("event_get_action", "event"), &ScenarioEventServer::event_get_action);
        ClassDB::bind_method(
                D_METHOD("event_attach_condition", "event", "condition"), &ScenarioEventServer::event_attach_condition);
        ClassDB::bind_method(D_METHOD("event_get_condition", "event"), &ScenarioEventServer::event_get_condition);
        ClassDB::bind_method(
                D_METHOD("event_queue", "event", "activator", "extra_delay"), &ScenarioEventServer::event_queue,
                DEFVAL(RID()), DEFVAL(0.0));
        ClassDB::bind_method(D_METHOD("event_is_queued", "event"), &ScenarioEventServer::event_is_queued);
        ClassDB::bind_method(D_METHOD("event_get_run_time", "event"), &ScenarioEventServer::event_get_run_time);
        ClassDB::bind_method(D_METHOD("queue_get_events"), &ScenarioEventServer::queue_get_events);


        ADD_SIGNAL(MethodInfo(
                event_queued_signal, PropertyInfo(Variant::RID, "event"), PropertyInfo(Variant::RID, "activator")));
        ADD_SIGNAL(MethodInfo(
                event_launched_signal, PropertyInfo(Variant::RID, "event"), PropertyInfo(Variant::RID, "activator")));
        ADD_SIGNAL(MethodInfo(event_dequeued_signal, PropertyInfo(Variant::RID, "event")));
        ADD_SIGNAL(MethodInfo(event_freed_signal, PropertyInfo(Variant::RID, "event")));
    }

    ScenarioEventServer::ScenarioEventServer() {
        singleton = this;
    }

    ScenarioEventServer::~ScenarioEventServer() {
        _set_processing(false);
        singleton = nullptr;
    }

    /// Queued, it holds the runtime's clock, so the time the queue waits for passes
    void ScenarioEventServer::_set_processing(const bool p_processing) {
        if (processing == p_processing) {
            return;
        }
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        processing = p_processing;
        if (p_processing) {
            runtime->clock_subscribe(callable_mp(this, &ScenarioEventServer::_process_queue));
            return;
        }
        runtime->clock_unsubscribe(callable_mp(this, &ScenarioEventServer::_process_queue));
    }

    /// event_manager::CheckQuery() (Event.cpp:2465-2490): the entries whose time has come run in
    /// time order. Only those queued before the pass started are taken - what an action queues,
    /// even for now, runs on the next frame, as in the original - so an event that queues itself
    /// again cannot keep the pass going.
    void ScenarioEventServer::_process_queue(double /* p_seconds */) {
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        const double time = runtime->simulation_get_time();
        const uint64_t pass_end = next_sequence;
        while (!queue.empty() && queue.top().time <= time && queue.top().sequence < pass_end) {
            const QueueEntry entry = queue.top();
            queue.pop();
            if (entry.system.is_valid()) {
                const SystemData *system = systems.getptr(entry.system);
                // taken before it runs: the implementation may create or free systems
                const Ref<ScenarioEventImplementation> implementation =
                        system != nullptr ? system->implementation : Ref<ScenarioEventImplementation>();
                if (implementation.is_valid()) {
                    implementation->scheduled(entry.system, entry.owner, static_cast<int64_t>(entry.sequence));
                }
                continue;
            }
            EventData *event = events.getptr(entry.owner);
            if (event == nullptr || !(event->queued_sequence == entry.sequence)) {
                continue;
            }
            event->queued_sequence = 0;
            // taken before anything runs: an action may create or free events, which rehashes
            // the table
            const Ref<ScenarioEventAction> action = event->action;
            const Ref<ScenarioEventCondition> condition = event->condition;
            const RID event_system = event->system;
            const Ref<ScenarioEventImplementation> implementation = _get_event_implementation(*event);
            emit_signal(event_launched_signal, entry.owner, entry.activator);
            const bool passed = condition.is_null() || condition->test(entry.owner, entry.activator);
            if (action.is_null()) {
                continue;
            }
            if (passed) {
                action->run(entry.owner, entry.activator);
            } else {
                action->run_else(entry.owner, entry.activator);
            }
            if (implementation.is_valid()) {
                implementation->event_ran(event_system, entry.owner, entry.activator);
            }
        }
        _set_processing(!queue.empty());
    }

    uint64_t ScenarioEventServer::_schedule(
            const RID &p_owner, const double p_time, const RID &p_activator, const RID &p_system) {
        QueueEntry entry;
        entry.time = p_time;
        entry.sequence = next_sequence++;
        entry.owner = p_owner;
        entry.activator = p_activator;
        entry.system = p_system;
        queue.push(entry);
        _set_processing(!queue.empty());
        return entry.sequence;
    }

    Ref<ScenarioEventImplementation> ScenarioEventServer::_get_event_implementation(const EventData &p_event) const {
        const SystemData *system = systems.getptr(p_event.system);
        return system != nullptr ? system->implementation : Ref<ScenarioEventImplementation>();
    }

    // --- system ---

    RID ScenarioEventServer::system_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        systems.insert(rid, SystemData());
        return rid;
    }

    /// A scheduled entry of its implementation is left in the queue and skipped when its time comes
    void ScenarioEventServer::system_free(const RID &p_system) {
        ERR_FAIL_COND(!systems.has(p_system));
        system_attach_implementation(p_system, Ref<ScenarioEventImplementation>());
        systems.erase(p_system);
        for (KeyValue<RID, EventData> &event: events) {
            if (event.value.system == p_system) {
                event.value.system = RID();
            }
        }
    }

    void ScenarioEventServer::system_attach_implementation(
            const RID &p_system, const Ref<ScenarioEventImplementation> &p_implementation) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        const Ref<ScenarioEventImplementation> previous = system->implementation;
        system->implementation = p_implementation;
        if (previous.is_valid()) {
            previous->system_detached(p_system);
        }
        if (p_implementation.is_valid()) {
            p_implementation->system_attached(p_system);
        }
    }

    Ref<ScenarioEventImplementation> ScenarioEventServer::system_get_implementation(const RID &p_system) const {
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, Ref<ScenarioEventImplementation>());
        return system->implementation;
    }

    void ScenarioEventServer::system_add_event(const RID &p_system, const RID &p_event) {
        ERR_FAIL_COND(!systems.has(p_system));
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->system = p_system;
    }

    int64_t ScenarioEventServer::system_schedule(const RID &p_system, const RID &p_owner, const double p_time) {
        ERR_FAIL_COND_V(!systems.has(p_system), 0);
        return static_cast<int64_t>(_schedule(p_owner, p_time, RID(), p_system));
    }

    // --- event ---

    RID ScenarioEventServer::event_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        events.insert(rid, EventData());
        return rid;
    }

    /// A queued entry of the event is left in the queue and skipped when its time comes; that the
    /// event will never launch is announced by event_dequeued, once it is gone
    void ScenarioEventServer::event_free(const RID &p_event) {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        const bool queued = event->queued_sequence > 0;
        names_rename(events_by_name, event->name, StringName(), p_event);
        events.erase(p_event);
        if (queued) {
            emit_signal(event_dequeued_signal, p_event);
        }
        emit_signal(event_freed_signal, p_event);
    }

    bool ScenarioEventServer::event_exists(const RID &p_event) const {
        return events.has(p_event);
    }

    RID ScenarioEventServer::event_get_system(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, RID());
        return event->system;
    }

    void ScenarioEventServer::event_set_name(const RID &p_event, const StringName &p_name) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        const StringName previous = event->name;
        event->name = p_name;
        names_rename(events_by_name, previous, p_name, p_event);
    }

    StringName ScenarioEventServer::event_get_name(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, StringName());
        return event->name;
    }

    RID ScenarioEventServer::event_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = events_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    void ScenarioEventServer::event_set_delay(const RID &p_event, const double p_seconds) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->delay = p_seconds;
    }

    double ScenarioEventServer::event_get_delay(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, 0.0);
        return event->delay;
    }

    void ScenarioEventServer::event_set_random_delay(const RID &p_event, const double p_seconds) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->random_delay = p_seconds;
    }

    double ScenarioEventServer::event_get_random_delay(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, 0.0);
        return event->random_delay;
    }

    void ScenarioEventServer::event_attach_action(const RID &p_event, const Ref<ScenarioEventAction> &p_action) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->action = p_action;
    }

    Ref<ScenarioEventAction> ScenarioEventServer::event_get_action(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, Ref<ScenarioEventAction>());
        return event->action;
    }

    void
    ScenarioEventServer::event_attach_condition(const RID &p_event, const Ref<ScenarioEventCondition> &p_condition) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL(event);
        event->condition = p_condition;
    }

    Ref<ScenarioEventCondition> ScenarioEventServer::event_get_condition(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, Ref<ScenarioEventCondition>());
        return event->condition;
    }

    bool ScenarioEventServer::event_is_queued(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, false);
        return event->queued_sequence > 0;
    }

    bool ScenarioEventServer::event_queue(const RID &p_event, const RID &p_activator, const double p_extra_delay) {
        EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, false);
        if (event->queued_sequence > 0) {
            return false;
        }
        const Ref<ScenarioEventImplementation> implementation = _get_event_implementation(*event);
        if (implementation.is_valid() && !implementation->event_can_queue(event->system, p_event, p_activator)) {
            return false;
        }
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL_V(runtime, false);
        const double now = runtime->simulation_get_time();
        double run_time = now + event->delay + p_extra_delay + (event->random_delay * UtilityFunctions::randf());
        if (implementation.is_valid()) {
            run_time = implementation->event_get_run_time(event->system, p_event, p_activator, now, run_time);
        }
        event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, false);
        event->queued_sequence = _schedule(p_event, run_time, p_activator, RID());
        event->run_time = run_time;
        emit_signal(event_queued_signal, p_event, p_activator);
        return true;
    }

    double ScenarioEventServer::event_get_run_time(const RID &p_event) const {
        const EventData *event = events.getptr(p_event);
        ERR_FAIL_NULL_V(event, -1.0);
        return event->queued_sequence > 0 ? event->run_time : -1.0;
    }

    TypedArray<RID> ScenarioEventServer::queue_get_events() const {
        TypedArray<RID> result;
        // a copy popped in order; an entry of an event queued again since, or of an implementation, is
        // not the event's (as _process_queue() skips it)
        std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>> pending = queue;
        while (!pending.empty()) {
            const QueueEntry entry = pending.top();
            pending.pop();
            const EventData *event = events.getptr(entry.owner);
            if (event != nullptr && event->queued_sequence == entry.sequence) {
                result.push_back(entry.owner);
            }
        }
        return result;
    }
} // namespace godot
