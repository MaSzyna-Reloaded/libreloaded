#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /// The behaviour of a ScenarioEventServer event system: what the events of a scenario mean
    /// beyond their delay, condition and action - which may not be queued, when one runs, what
    /// follows its run - and its own entries in the server's queue (system_schedule()). The server
    /// is the dispatcher; the original's event system, its memory cells and launchers, is one
    /// implementation. This class is only the interface - implement it in C++ by overriding the
    /// virtual methods, or in GDScript by overriding their script counterparts.
    ///
    /// Every callback carries the system: a Resource is shared by default, so one implementation may
    /// serve several systems and keeps whatever state it needs per system RID. An implementation
    /// changes the events only through the ScenarioEventServer API.
    class ScenarioEventImplementation : public Resource {
            GDCLASS(ScenarioEventImplementation, Resource)
            friend class ScenarioEventServer;

        protected:
            static void _bind_methods();

            GDVIRTUAL1(_system_attached, RID)
            GDVIRTUAL1(_system_detached, RID)
            GDVIRTUAL3RC(bool, _event_can_queue, RID, RID, RID)
            GDVIRTUAL5RC(double, _event_get_run_time, RID, RID, RID, double, double)
            GDVIRTUAL3(_event_ran, RID, RID, RID)
            GDVIRTUAL3(_scheduled, RID, RID, int64_t)

            /// Called by ScenarioEventServer. A C++ implementation overrides these; the default
            /// forwards to the script.
            virtual void system_attached(const RID &p_system);
            /// ...and the implementation lets go of everything it kept for the system
            virtual void system_detached(const RID &p_system);
            /// Whether the system's event may be queued now by the activator; it may by default
            virtual bool event_can_queue(const RID &p_system, const RID &p_event, const RID &p_activator) const;
            /// The simulation time the system's event runs at, queued by the activator at p_now: by
            /// default p_run_time, now with the event's delays
            virtual double event_get_run_time(
                    const RID &p_system, const RID &p_event, const RID &p_activator, double p_now,
                    double p_run_time) const;
            /// The system's event ran, its action done
            virtual void event_ran(const RID &p_system, const RID &p_event, const RID &p_activator);
            /// The time the implementation scheduled p_owner for (ScenarioEventServer::system_schedule())
            /// came; p_ticket is what system_schedule() returned
            virtual void scheduled(const RID &p_system, const RID &p_owner, int64_t p_ticket);
    };
} // namespace godot
