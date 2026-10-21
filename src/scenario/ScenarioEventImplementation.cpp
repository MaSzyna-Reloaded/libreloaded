#include "ScenarioEventImplementation.hpp"

namespace godot {
    void ScenarioEventImplementation::_bind_methods() {
        GDVIRTUAL_BIND(_system_attached, "system");
        GDVIRTUAL_BIND(_system_detached, "system");
        GDVIRTUAL_BIND(_event_can_queue, "system", "event", "activator");
        GDVIRTUAL_BIND(_event_get_run_time, "system", "event", "activator", "now", "run_time");
        GDVIRTUAL_BIND(_event_ran, "system", "event", "activator");
        GDVIRTUAL_BIND(_scheduled, "system", "owner", "ticket");
    }

    void ScenarioEventImplementation::system_attached(const RID &p_system) {
        GDVIRTUAL_CALL(_system_attached, p_system);
    }

    void ScenarioEventImplementation::system_detached(const RID &p_system) {
        GDVIRTUAL_CALL(_system_detached, p_system);
    }

    bool ScenarioEventImplementation::event_can_queue(
            const RID &p_system, const RID &p_event, const RID &p_activator) const {
        bool can_queue = true;
        GDVIRTUAL_CALL(_event_can_queue, p_system, p_event, p_activator, can_queue);
        return can_queue;
    }

    double ScenarioEventImplementation::event_get_run_time(
            const RID &p_system, const RID &p_event, const RID &p_activator, const double p_now,
            const double p_run_time) const {
        double run_time = p_run_time;
        GDVIRTUAL_CALL(_event_get_run_time, p_system, p_event, p_activator, p_now, p_run_time, run_time);
        return run_time;
    }

    void ScenarioEventImplementation::event_ran(const RID &p_system, const RID &p_event, const RID &p_activator) {
        GDVIRTUAL_CALL(_event_ran, p_system, p_event, p_activator);
    }

    void ScenarioEventImplementation::scheduled(const RID &p_system, const RID &p_owner, const int64_t p_ticket) {
        GDVIRTUAL_CALL(_scheduled, p_system, p_owner, p_ticket);
    }
} // namespace godot
