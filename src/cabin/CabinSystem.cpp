#include "CabinSystem.hpp"
#include "cabin/Cabin3D.hpp"
#include "logging/GameLog.hpp"
#include "logging/GameLogger.hpp"
#include "simulation/SimulationServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    /// The keys of p_map: what a loop calling out walks, as a callable may change the map
    template<typename T>
    static LocalVector<RID> keys_of(const HashMap<RID, T, RIDHasher, RIDComparator> &p_map) {
        LocalVector<RID> keys;
        keys.reserve(p_map.size());
        for (const KeyValue<RID, T> &entry: p_map) {
            keys.push_back(entry.key);
        }
        return keys;
    }

    const char *CabinSystem::control_changed_signal = "control_changed";
    const char *CabinSystem::vehicle_command_received_signal = "vehicle_command_received";
    const char *CabinSystem::vehicle_cabin_scene_changed_signal = "vehicle_cabin_scene_changed";
    const char *CabinSystem::vehicle_cabin_built_signal = "vehicle_cabin_built";
    const char *CabinSystem::cabin_light_level_changed_signal = "cabin_light_level_changed";
    const char *CabinSystem::cabin_instrument_light_changed_signal = "cabin_instrument_light_changed";
    const char *CabinSystem::cabin_dashboard_light_changed_signal = "cabin_dashboard_light_changed";
    const char *CabinSystem::cabin_timetable_light_changed_signal = "cabin_timetable_light_changed";

    void CabinSystem::_bind_methods() {
        ClassDB::bind_method(D_METHOD("vehicle_state", "vehicle_rid"), &CabinSystem::vehicle_state);
        ClassDB::bind_method(
                D_METHOD("vehicle_state_value", "vehicle_rid", "key", "default_value"),
                &CabinSystem::vehicle_state_value, DEFVAL(Variant()));
        ClassDB::bind_method(
                D_METHOD("state_computed_value_register", "vehicle_rid", "key", "callable"),
                &CabinSystem::state_computed_value_register);
        ClassDB::bind_method(
                D_METHOD("state_computed_value_unregister", "vehicle_rid", "key"),
                &CabinSystem::state_computed_value_unregister);
        ClassDB::bind_method(D_METHOD("vehicle_has_low_voltage", "vehicle_rid"), &CabinSystem::vehicle_has_low_voltage);
        ClassDB::bind_method(D_METHOD("vehicle_config", "vehicle_rid"), &CabinSystem::vehicle_config);
        ClassDB::bind_method(D_METHOD("vehicle_component", "vehicle_rid", "type"), &CabinSystem::vehicle_component);
        ClassDB::bind_method(
                D_METHOD("vehicle_attach_cab_logic", "vehicle_rid", "logic"), &CabinSystem::vehicle_attach_cab_logic);
        ClassDB::bind_method(D_METHOD("vehicle_get_cab_logic", "vehicle_rid"), &CabinSystem::vehicle_get_cab_logic);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_cabin_scene", "vehicle_rid", "scene"), &CabinSystem::vehicle_set_cabin_scene);
        ClassDB::bind_method(D_METHOD("vehicle_get_cabin_scene", "vehicle_rid"), &CabinSystem::vehicle_get_cabin_scene);
        ClassDB::bind_method(D_METHOD("cabin_show", "cabin_rid", "parent"), &CabinSystem::cabin_show);
        ClassDB::bind_method(D_METHOD("vehicle_hide_cabin", "vehicle_rid"), &CabinSystem::vehicle_hide_cabin);
        ClassDB::bind_method(D_METHOD("cabin_set_light_level", "cabin", "level"), &CabinSystem::cabin_set_light_level);
        ClassDB::bind_method(D_METHOD("cabin_get_light_level", "cabin"), &CabinSystem::cabin_get_light_level);
        ClassDB::bind_method(
                D_METHOD("cabin_set_instrument_light_enabled", "cabin", "enabled"),
                &CabinSystem::cabin_set_instrument_light_enabled);
        ClassDB::bind_method(
                D_METHOD("cabin_get_instrument_light_enabled", "cabin"),
                &CabinSystem::cabin_get_instrument_light_enabled);
        ClassDB::bind_method(
                D_METHOD("cabin_set_dashboard_light_enabled", "cabin", "enabled"),
                &CabinSystem::cabin_set_dashboard_light_enabled);
        ClassDB::bind_method(
                D_METHOD("cabin_get_dashboard_light_enabled", "cabin"),
                &CabinSystem::cabin_get_dashboard_light_enabled);
        ClassDB::bind_method(
                D_METHOD("cabin_set_timetable_light_enabled", "cabin", "enabled"),
                &CabinSystem::cabin_set_timetable_light_enabled);
        ClassDB::bind_method(
                D_METHOD("cabin_get_timetable_light_enabled", "cabin"),
                &CabinSystem::cabin_get_timetable_light_enabled);
        ClassDB::bind_method(D_METHOD("get_cabin_state", "cabin"), &CabinSystem::get_cabin_state);
        ClassDB::bind_method(
                D_METHOD("register_control", "cabin", "control_id", "handler"), &CabinSystem::register_control);
        ClassDB::bind_method(
                D_METHOD("unregister_control", "cabin", "control_id", "handler"), &CabinSystem::unregister_control);
        ClassDB::bind_method(D_METHOD("has_control", "cabin", "control_id"), &CabinSystem::has_control);
        ClassDB::bind_method(D_METHOD("get_controls", "cabin"), &CabinSystem::get_controls);
        ClassDB::bind_method(D_METHOD("register_process", "cabin", "callable"), &CabinSystem::register_process);
        ClassDB::bind_method(D_METHOD("unregister_process", "cabin", "callable"), &CabinSystem::unregister_process);
        ClassDB::bind_method(
                D_METHOD("register_display_process", "cabin", "callable"), &CabinSystem::register_display_process);
        ClassDB::bind_method(
                D_METHOD("unregister_display_process", "cabin", "callable"), &CabinSystem::unregister_display_process);
        ClassDB::bind_method(
                D_METHOD("act", "cabin", "control_id", "action", "value"), &CabinSystem::act, DEFVAL(Variant()));
        ClassDB::bind_method(D_METHOD("get_control", "cabin", "control_id"), &CabinSystem::get_control);
        ClassDB::bind_method(D_METHOD("get_state", "cabin"), &CabinSystem::get_state);
        ClassDB::bind_method(D_METHOD("get_actions"), &CabinSystem::get_actions);

        ADD_SIGNAL(MethodInfo(
                control_changed_signal, PropertyInfo(Variant::RID, "cabin"),
                PropertyInfo(Variant::STRING_NAME, "control_id"), PropertyInfo(Variant::NIL, "value")));
        ADD_SIGNAL(MethodInfo(
                vehicle_command_received_signal, PropertyInfo(Variant::RID, "vehicle_rid"),
                PropertyInfo(Variant::STRING, "command"), PropertyInfo(Variant::NIL, "p1"),
                PropertyInfo(Variant::NIL, "p2")));
        ADD_SIGNAL(MethodInfo(vehicle_cabin_scene_changed_signal, PropertyInfo(Variant::RID, "vehicle_rid")));
        ADD_SIGNAL(MethodInfo(
                vehicle_cabin_built_signal, PropertyInfo(Variant::RID, "vehicle_rid"),
                PropertyInfo(Variant::OBJECT, "cabin", PROPERTY_HINT_NODE_TYPE, "Cabin3D")));
        ADD_SIGNAL(MethodInfo(
                cabin_light_level_changed_signal, PropertyInfo(Variant::RID, "cabin"),
                PropertyInfo(Variant::FLOAT, "level")));
        ADD_SIGNAL(MethodInfo(
                cabin_instrument_light_changed_signal, PropertyInfo(Variant::RID, "cabin"),
                PropertyInfo(Variant::BOOL, "enabled")));
        ADD_SIGNAL(MethodInfo(
                cabin_dashboard_light_changed_signal, PropertyInfo(Variant::RID, "cabin"),
                PropertyInfo(Variant::BOOL, "enabled")));
        ADD_SIGNAL(MethodInfo(
                cabin_timetable_light_changed_signal, PropertyInfo(Variant::RID, "cabin"),
                PropertyInfo(Variant::BOOL, "enabled")));
    }

    /// No explicit disconnect: callable_mp reports this instance as the callable's object, so the
    /// engine drops the connections when the instance dies.
    CabinSystem::CabinSystem() {
        singleton = this;
        for (const char *action: {"increase", "decrease", "hold", "release", "toggle", "set"}) {
            actions.push_back(StringName(action));
        }
        SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        simulation->connect(
                SimulationServer::simulation_advanced_signal, callable_mp(this, &CabinSystem::_on_simulation_advanced));
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        vehicles->connect(
                VehicleServer::vehicle_command_received_signal,
                callable_mp(this, &CabinSystem::_on_vehicle_command_received));
        vehicles->connect(VehicleServer::vehicle_freed_signal, callable_mp(this, &CabinSystem::_on_vehicle_freed));
        RailVehicleServer *rail = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(rail);
        rail->connect(
                RailVehicleServer::vehicle_driver_cabin_changed_signal,
                callable_mp(this, &CabinSystem::_on_vehicle_driver_cabin_changed));
    }

    /* What the cabs hold of the scripts - handlers, processes, cab logics and the cabs' states -
     * is let go when the tree goes away at quit: the engine finishes the script
     * languages before it frees this singleton, and a script's callable freed after that is freed
     * twice. Followed once there is a tree, from the first thing taken. */
    void CabinSystem::_follow_tree_exit() {
        if (following_tree_exit) {
            return;
        }
        const SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree == nullptr || tree->get_root() == nullptr) {
            return;
        }
        following_tree_exit = true;
        tree->get_root()->connect("tree_exiting", callable_mp(this, &CabinSystem::_release_scripts));
    }

    void CabinSystem::_release_scripts() {
        for (const KeyValue<RID, Ref<CabinLogic>> &logic: cab_logics) {
            logic.value->unregister();
        }
        cab_logics.clear();
        states.clear();
        controls.clear();
        processes.clear();
        display_processes.clear();
        cabin_scenes.clear();
        cabins.clear();
        state_computed_values.clear();
    }

    CabinSystem::~CabinSystem() {
        singleton = nullptr;
    }

    /// A freed vehicle takes its cabins along - a handle is never reused for another one.
    void CabinSystem::_on_vehicle_freed(const RID &p_vehicle_rid) {
        if (const Ref<CabinLogic> *logic = cab_logics.getptr(p_vehicle_rid); logic != nullptr) {
            const Ref<CabinLogic> unregistered = *logic;
            cab_logics.erase(p_vehicle_rid);
            unregistered->unregister();
        }
        cabin_scenes.erase(p_vehicle_rid);
        cabins.erase(p_vehicle_rid);
        state_computed_values.erase(p_vehicle_rid);
        for (const RID &cabin: keys_of(states)) {
            if (states[cabin]->get_vehicle_rid() == p_vehicle_rid) {
                states.erase(cabin);
                controls.erase(cabin);
                processes.erase(cabin);
                display_processes.erase(cabin);
            }
        }
    }

    void CabinSystem::_on_vehicle_command_received(
            const RID &p_vehicle_rid, const String &p_command, const Variant &p_p1, const Variant &p_p2) {
        // every vehicle's commands come here - hundreds a second from a scenery's computer drivers -
        // and every control of a shown cab would be called for each
        const RailVehicleServer *rail = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(rail);
        const RID trainset = rail->vehicle_get_trainset(p_vehicle_rid);
        for (const KeyValue<RID, ObjectID> &shown: cabins) {
            if (shown.key == p_vehicle_rid ||
                (trainset.is_valid() && rail->vehicle_get_trainset(shown.key) == trainset)) {
                emit_signal(vehicle_command_received_signal, p_vehicle_rid, p_command, p_p1, p_p2);
                return;
            }
        }
    }

    /// The driver moved, sat down or got up: the controls are those of the driver's cabin now, none
    /// while nobody drives - a cab of every vehicle at work costs every frame
    void CabinSystem::_on_vehicle_driver_cabin_changed(const RID &p_vehicle_rid, const RID &p_cabin) {
        const Ref<CabinLogic> *found = cab_logics.getptr(p_vehicle_rid);
        if (found == nullptr || found->is_null()) {
            return;
        }
        const Ref<CabinLogic> logic = *found;
        logic->unregister();
        if (p_cabin.is_valid()) {
            logic->register_cabin(p_vehicle_rid, p_cabin);
        }
    }

    /// The cabs' own timing runs on the simulation's clock: a relay held for its delay at x10 closes
    /// in a tenth of the real time, and nothing runs while paused
    void CabinSystem::_on_simulation_advanced(const double p_seconds) {
        for (const RID &cabin: keys_of(processes)) {
            const Array *callables = processes.getptr(cabin);
            const Ref<CabinState> *state = states.getptr(cabin);
            if (callables == nullptr || state == nullptr) {
                continue;
            }
            const Array walked = *callables;
            const Ref<CabinState> held = *state;
            for (int64_t i = 0; i < walked.size(); i++) {
                Callable(walked[i]).call(held, p_seconds);
            }
        }
        for (const RID &cabin: keys_of(display_processes)) {
            const Array *callables = display_processes.getptr(cabin);
            const Ref<CabinState> *state = states.getptr(cabin);
            if (callables == nullptr || state == nullptr || !cabins.has((*state)->get_vehicle_rid())) {
                continue;
            }
            const Array walked = *callables;
            const Ref<CabinState> held = *state;
            for (int64_t i = 0; i < walked.size(); i++) {
                Callable(walked[i]).call(held, p_seconds);
            }
        }
    }

    /// The whole vehicle's state, by name. VehicleServer builds it once per step and keeps it until
    /// the step or a command moves it on, so the dozens of elements of a cab asking in one frame
    /// share one dump.
    Dictionary CabinSystem::vehicle_state(const RID &p_vehicle_rid) const {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, Dictionary());
        return p_vehicle_rid.is_valid() ? vehicles->vehicle_dump_state(p_vehicle_rid) : Dictionary();
    }

    /// One named value of the vehicle's state; a value the cab computes is answered by its callable
    Variant
    CabinSystem::vehicle_state_value(const RID &p_vehicle_rid, const String &p_key, const Variant &p_default_value) {
        if (const HashMap<String, Callable> *computed = state_computed_values.getptr(p_vehicle_rid);
            computed != nullptr) {
            if (const Callable *callable = computed->getptr(p_key); callable != nullptr && callable->is_valid()) {
                return callable->call();
            }
        }
        return vehicle_state(p_vehicle_rid).get(p_key, p_default_value);
    }

    void CabinSystem::state_computed_value_register(
            const RID &p_vehicle_rid, const String &p_key, const Callable &p_callable) {
        _follow_tree_exit();
        state_computed_values[p_vehicle_rid][p_key] = p_callable;
    }

    void CabinSystem::state_computed_value_unregister(const RID &p_vehicle_rid, const String &p_key) {
        if (HashMap<String, Callable> *computed = state_computed_values.getptr(p_vehicle_rid); computed != nullptr) {
            computed->erase(p_key);
        }
    }

    /// Whether the vehicle has its low voltage, without which every lamp of its cab is dark
    bool CabinSystem::vehicle_has_low_voltage(const RID &p_vehicle_rid) const {
        const Dictionary state = vehicle_state(p_vehicle_rid);
        return bool(state.get("power24_available", false)) || bool(state.get("power110_available", false));
    }

    Dictionary CabinSystem::vehicle_config(const RID &p_vehicle_rid) const {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, Dictionary());
        return p_vehicle_rid.is_valid() ? vehicles->vehicle_dump_config(p_vehicle_rid) : Dictionary();
    }

    Ref<VehicleComponent>
    CabinSystem::vehicle_component(const RID &p_vehicle_rid, const VehicleComponentType::Type p_type) const {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, Ref<VehicleComponent>());
        return p_vehicle_rid.is_valid() ? vehicles->vehicle_component_get(p_vehicle_rid, p_type)
                                        : Ref<VehicleComponent>();
    }

    /// One logic per vehicle, whoever drives it, registered for the cabin its driver sits in while
    /// somebody drives it; a null one detaches it
    void CabinSystem::vehicle_attach_cab_logic(const RID &p_vehicle_rid, const Ref<CabinLogic> &p_logic) {
        _follow_tree_exit();
        if (const Ref<CabinLogic> *attached = cab_logics.getptr(p_vehicle_rid); attached != nullptr) {
            const Ref<CabinLogic> detached = *attached;
            cab_logics.erase(p_vehicle_rid);
            detached->unregister();
        }
        if (p_logic.is_null()) {
            return;
        }
        cab_logics[p_vehicle_rid] = p_logic;
        const RailVehicleServer *rail = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(rail);
        const RID cabin = rail->vehicle_get_driver_cabin(p_vehicle_rid);
        if (cabin.is_valid()) {
            p_logic->register_cabin(p_vehicle_rid, cabin);
        }
    }

    Ref<CabinLogic> CabinSystem::vehicle_get_cab_logic(const RID &p_vehicle_rid) const {
        const Ref<CabinLogic> *logic = cab_logics.getptr(p_vehicle_rid);
        return logic != nullptr ? *logic : Ref<CabinLogic>();
    }

    /// The cab interior of a vehicle, a scene rooted in a Cabin3D - what cabin_show() builds. Only a
    /// view: the cab logic is the vehicle's (vehicle_attach_cab_logic()).
    void CabinSystem::vehicle_set_cabin_scene(const RID &p_vehicle_rid, const Ref<PackedScene> &p_scene) {
        cabin_scenes[p_vehicle_rid] = p_scene;
        emit_signal(vehicle_cabin_scene_changed_signal, p_vehicle_rid);
    }

    Ref<PackedScene> CabinSystem::vehicle_get_cabin_scene(const RID &p_vehicle_rid) const {
        const Ref<PackedScene> *scene = cabin_scenes.getptr(p_vehicle_rid);
        return scene != nullptr ? *scene : Ref<PackedScene>();
    }

    /// The interior of the cabin built under p_parent, which rides on its vehicle; it is built within
    /// add_child(), so it returns built. One interior a vehicle is shown: the one of another of its
    /// cabins is rebuilt for this one. Null for a vehicle without a cab.
    Cabin3D *CabinSystem::cabin_show(const RID &p_cabin_rid, Node *p_parent) {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, nullptr);
        const RID vehicle_rid = vehicles->cabin_get_vehicle(p_cabin_rid);
        // what the cab shows is worked out before its elements read it - they read it when they are
        // built and follow its changes from then on
        _run_vehicle_display_processes(vehicle_rid);
        if (Cabin3D *shown = _shown_cabin(vehicle_rid); shown != nullptr) {
            shown->set_cabin(p_cabin_rid);
            return shown;
        }
        const Ref<PackedScene> scene = vehicle_get_cabin_scene(vehicle_rid);
        if (scene.is_null()) {
            UtilityFunctions::push_warning("CabinSystem: the vehicle has no cab interior to show");
            return nullptr;
        }
        Node *instance = scene->instantiate();
        Cabin3D *cabin = Object::cast_to<Cabin3D>(instance);
        if (cabin == nullptr) {
            UtilityFunctions::push_error("CabinSystem: the root of a cabin scene must be a Cabin3D");
            if (instance != nullptr) {
                memdelete(instance);
            }
            return nullptr;
        }
        cabins[vehicle_rid] = cabin->get_instance_id();
        p_parent->add_child(cabin);
        // a cabin holds the handle of its cabin, its vehicle comes with it, and takes everything else
        // from here - told once it is in the tree, because building its interior puts nodes there
        cabin->set_cabin(p_cabin_rid);
        emit_signal(vehicle_cabin_built_signal, vehicle_rid, cabin);
        return cabin;
    }

    /// The cab interior freed; a camera put into it is to be taken out first
    void CabinSystem::vehicle_hide_cabin(const RID &p_vehicle_rid) {
        Cabin3D *cabin = _shown_cabin(p_vehicle_rid);
        cabins.erase(p_vehicle_rid);
        if (cabin == nullptr) {
            return;
        }
        cabin->get_parent()->remove_child(cabin);
        cabin->queue_free();
    }

    Cabin3D *CabinSystem::_shown_cabin(const RID &p_vehicle_rid) const {
        const ObjectID *id = cabins.getptr(p_vehicle_rid);
        return id != nullptr ? Object::cast_to<Cabin3D>(ObjectDB::get_instance(*id)) : nullptr;
    }

    /// The cab light of a cabin at p_level (0..1); the vehicle's drawing follows cabin_light_level_changed
    void CabinSystem::cabin_set_light_level(const RID &p_cabin, const double p_level) {
        const Ref<CabinState> state = get_cabin_state(p_cabin);
        if (state->get_light_level() == p_level) {
            return;
        }
        state->set_light_level(p_level);
        emit_signal(cabin_light_level_changed_signal, p_cabin, p_level);
    }

    double CabinSystem::cabin_get_light_level(const RID &p_cabin) {
        _refresh_display(p_cabin);
        const Ref<CabinState> *state = states.getptr(p_cabin);
        return state != nullptr ? (*state)->get_light_level() : 0.0;
    }

    void CabinSystem::cabin_set_instrument_light_enabled(const RID &p_cabin, const bool p_enabled) {
        const Ref<CabinState> state = get_cabin_state(p_cabin);
        if (state->get_instrument_light_enabled() == p_enabled) {
            return;
        }
        state->set_instrument_light_enabled(p_enabled);
        emit_signal(cabin_instrument_light_changed_signal, p_cabin, p_enabled);
    }

    bool CabinSystem::cabin_get_instrument_light_enabled(const RID &p_cabin) {
        _refresh_display(p_cabin);
        const Ref<CabinState> *state = states.getptr(p_cabin);
        return state != nullptr && (*state)->get_instrument_light_enabled();
    }

    void CabinSystem::cabin_set_dashboard_light_enabled(const RID &p_cabin, const bool p_enabled) {
        const Ref<CabinState> state = get_cabin_state(p_cabin);
        if (state->get_dashboard_light_enabled() == p_enabled) {
            return;
        }
        state->set_dashboard_light_enabled(p_enabled);
        emit_signal(cabin_dashboard_light_changed_signal, p_cabin, p_enabled);
    }

    bool CabinSystem::cabin_get_dashboard_light_enabled(const RID &p_cabin) {
        _refresh_display(p_cabin);
        const Ref<CabinState> *state = states.getptr(p_cabin);
        return state != nullptr && (*state)->get_dashboard_light_enabled();
    }

    void CabinSystem::cabin_set_timetable_light_enabled(const RID &p_cabin, const bool p_enabled) {
        const Ref<CabinState> state = get_cabin_state(p_cabin);
        if (state->get_timetable_light_enabled() == p_enabled) {
            return;
        }
        state->set_timetable_light_enabled(p_enabled);
        emit_signal(cabin_timetable_light_changed_signal, p_cabin, p_enabled);
    }

    bool CabinSystem::cabin_get_timetable_light_enabled(const RID &p_cabin) {
        _refresh_display(p_cabin);
        const Ref<CabinState> *state = states.getptr(p_cabin);
        return state != nullptr && (*state)->get_timetable_light_enabled();
    }

    Ref<CabinState> CabinSystem::get_cabin_state(const RID &p_cabin) {
        if (const Ref<CabinState> *state = states.getptr(p_cabin); state != nullptr) {
            return *state;
        }
        _follow_tree_exit();
        Ref<CabinState> state;
        state.instantiate();
        state->setup(p_cabin);
        states.insert(p_cabin, state);
        return state;
    }

    // FIXME(#184, #28): imperative, per-cabin registration mirroring VehicleController.register_command;
    // cabin behaviours should rather declare the control ids/actions they handle.
    void CabinSystem::register_control(const RID &p_cabin, const StringName &p_control_id, const Callable &p_handler) {
        _follow_tree_exit();
        controls[p_cabin][p_control_id] = p_handler;
    }

    void
    CabinSystem::unregister_control(const RID &p_cabin, const StringName &p_control_id, const Callable &p_handler) {
        HashMap<StringName, Callable> *cabin_controls = controls.getptr(p_cabin);
        if (cabin_controls == nullptr) {
            return;
        }
        if (const Callable *handler = cabin_controls->getptr(p_control_id);
            handler != nullptr && *handler == p_handler) {
            cabin_controls->erase(p_control_id);
        }
    }

    bool CabinSystem::has_control(const RID &p_cabin, const StringName &p_control_id) const {
        const HashMap<StringName, Callable> *cabin_controls = controls.getptr(p_cabin);
        return cabin_controls != nullptr && cabin_controls->has(p_control_id);
    }

    /// Control ids with a registered handler in the cabin, sorted
    Array CabinSystem::get_controls(const RID &p_cabin) const {
        Array result;
        if (const HashMap<StringName, Callable> *cabin_controls = controls.getptr(p_cabin); cabin_controls != nullptr) {
            for (const KeyValue<StringName, Callable> &entry: *cabin_controls) {
                result.push_back(entry.key);
            }
        }
        result.sort();
        return result;
    }

    /// p_callable(state: CabinState, delta: float) - called every frame while registered
    void CabinSystem::register_process(const RID &p_cabin, const Callable &p_callable) {
        _follow_tree_exit();
        get_cabin_state(p_cabin);
        const Array *registered = processes.getptr(p_cabin);
        Array replaced = registered != nullptr ? registered->duplicate() : Array();
        replaced.push_back(p_callable);
        processes[p_cabin] = replaced;
    }

    void CabinSystem::unregister_process(const RID &p_cabin, const Callable &p_callable) {
        const Array *registered = processes.getptr(p_cabin);
        if (registered == nullptr) {
            return;
        }
        Array replaced = registered->duplicate();
        replaced.erase(p_callable);
        processes[p_cabin] = replaced;
    }

    /// p_callable(state: CabinState, delta: float) for what only the cab shows - its lights: run with
    /// the simulation while the interior of the cabin's vehicle is shown, and for a cab not shown
    /// when it is about to be (cabin_show()) or something reads it (cabin_get_light_level(), ...)
    void CabinSystem::register_display_process(const RID &p_cabin, const Callable &p_callable) {
        _follow_tree_exit();
        get_cabin_state(p_cabin);
        const Array *registered = display_processes.getptr(p_cabin);
        Array replaced = registered != nullptr ? registered->duplicate() : Array();
        replaced.push_back(p_callable);
        display_processes[p_cabin] = replaced;
    }

    void CabinSystem::unregister_display_process(const RID &p_cabin, const Callable &p_callable) {
        const Array *registered = display_processes.getptr(p_cabin);
        if (registered == nullptr) {
            return;
        }
        Array replaced = registered->duplicate();
        replaced.erase(p_callable);
        display_processes[p_cabin] = replaced;
    }

    void CabinSystem::_refresh_display(const RID &p_cabin) {
        const Ref<CabinState> *state = states.getptr(p_cabin);
        if (state == nullptr || cabins.has((*state)->get_vehicle_rid())) {
            return;
        }
        const Ref<CabinState> held = *state;
        const Array *callables = display_processes.getptr(p_cabin);
        if (callables == nullptr) {
            return;
        }
        const Array walked = *callables;
        for (int64_t i = 0; i < walked.size(); i++) {
            Callable(walked[i]).call(held, 0.0);
        }
    }

    void CabinSystem::_run_vehicle_display_processes(const RID &p_vehicle_rid) {
        for (const RID &cabin: keys_of(display_processes)) {
            const Ref<CabinState> *state = states.getptr(cabin);
            const Array *callables = display_processes.getptr(cabin);
            if (state == nullptr || callables == nullptr || (*state)->get_vehicle_rid() != p_vehicle_rid) {
                continue;
            }
            const Ref<CabinState> held = *state;
            const Array walked = *callables;
            for (int64_t i = 0; i < walked.size(); i++) {
                Callable(walked[i]).call(held, 0.0);
            }
        }
    }

    /// Reports a manipulation of a cabin control; returns the handler's result, or null when no
    /// handler is registered for the control
    Variant CabinSystem::act(
            const RID &p_cabin, const StringName &p_control_id, const StringName &p_action, const Variant &p_value) {
        if (actions.find(p_action) < 0) {
            GameLog::get_instance()->get_logger("game")->error(
                    vformat("%s: Unknown cabin action: %s", _cabin_vehicle_id(p_cabin), p_action));
            return Variant();
        }
        const HashMap<StringName, Callable> *cabin_controls = controls.getptr(p_cabin);
        const Callable *handler = cabin_controls != nullptr ? cabin_controls->getptr(p_control_id) : nullptr;
        if (handler == nullptr || !handler->is_valid()) {
            GameLog::get_instance()->get_logger("game")->error(
                    vformat("%s: Unknown cabin control: %s", _cabin_vehicle_id(p_cabin), p_control_id));
            return Variant();
        }
        const Callable called = *handler;
        return called.call(get_cabin_state(p_cabin), p_action, p_value);
    }

    /// The id of a cabin's vehicle, made only when there is a message to write: act() is called on
    /// every decision of every computer driver
    String CabinSystem::_cabin_vehicle_id(const RID &p_cabin) {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, String());
        return vehicles->vehicle_get_id(vehicles->cabin_get_vehicle(p_cabin));
    }

    Variant CabinSystem::get_control(const RID &p_cabin, const StringName &p_control_id) {
        return get_cabin_state(p_cabin)->get_value(p_control_id);
    }

    PackedStringArray CabinSystem::get_actions() const {
        PackedStringArray result;
        for (const StringName &action: actions) {
            result.push_back(action);
        }
        return result;
    }

    Dictionary CabinSystem::get_state(const RID &p_cabin) {
        return get_cabin_state(p_cabin)->get_values().duplicate();
    }

} // namespace godot
