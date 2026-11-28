#include "CabinLogic.hpp"

namespace godot {
    void CabinLogic::_bind_methods() {
        // `register` is a word of C++, so the method has another name here
        ClassDB::bind_method(D_METHOD("register", "vehicle_rid", "cabin"), &CabinLogic::register_cabin);
        ClassDB::bind_method(D_METHOD("unregister"), &CabinLogic::unregister);
        ClassDB::bind_method(D_METHOD("input", "event"), &CabinLogic::input);
        ClassDB::bind_method(D_METHOD("press", "control_id"), &CabinLogic::press);
        ClassDB::bind_method(D_METHOD("release", "control_id"), &CabinLogic::release);
        ClassDB::bind_method(D_METHOD("increase", "control_id"), &CabinLogic::increase);
        ClassDB::bind_method(D_METHOD("decrease", "control_id"), &CabinLogic::decrease);
        ClassDB::bind_method(D_METHOD("get_action", "control_id", "gesture"), &CabinLogic::get_action);

        GDVIRTUAL_BIND(_register, "vehicle_rid", "cabin");
        GDVIRTUAL_BIND(_unregister);
        GDVIRTUAL_BIND(_input, "event");
        GDVIRTUAL_BIND(_press, "control_id");
        GDVIRTUAL_BIND(_release, "control_id");
        GDVIRTUAL_BIND(_increase, "control_id");
        GDVIRTUAL_BIND(_decrease, "control_id");
        GDVIRTUAL_BIND(_get_action, "control_id", "gesture");

        BIND_ENUM_CONSTANT(PRESS);
        BIND_ENUM_CONSTANT(INCREASE);
        BIND_ENUM_CONSTANT(DECREASE);
    }

    void CabinLogic::register_cabin(const RID &p_vehicle_rid, const RID &p_cabin) {
        GDVIRTUAL_CALL(_register, p_vehicle_rid, p_cabin);
    }

    void CabinLogic::unregister() {
        GDVIRTUAL_CALL(_unregister);
    }

    void CabinLogic::input(const Ref<InputEvent> &p_event) {
        GDVIRTUAL_CALL(_input, p_event);
    }

    void CabinLogic::press(const StringName &p_control_id) {
        GDVIRTUAL_CALL(_press, p_control_id);
    }

    void CabinLogic::release(const StringName &p_control_id) {
        GDVIRTUAL_CALL(_release, p_control_id);
    }

    void CabinLogic::increase(const StringName &p_control_id) {
        GDVIRTUAL_CALL(_increase, p_control_id);
    }

    void CabinLogic::decrease(const StringName &p_control_id) {
        GDVIRTUAL_CALL(_decrease, p_control_id);
    }

    StringName CabinLogic::get_action(const StringName &p_control_id, const Gesture p_gesture) const {
        StringName result;
        GDVIRTUAL_CALL(_get_action, p_control_id, static_cast<int>(p_gesture), result);
        return result;
    }
} // namespace godot
