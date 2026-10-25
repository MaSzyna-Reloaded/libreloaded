#include "TrainSoundListener3D.hpp"
#include "cabin/Cabin3D.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    const char *TrainSoundListener3D::context_changed_signal = "context_changed";

    void TrainSoundListener3D::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_listener_vehicle"), &TrainSoundListener3D::get_listener_vehicle);
        ClassDB::bind_method(D_METHOD("get_listener_cabin"), &TrainSoundListener3D::get_listener_cabin);
        ClassDB::bind_method(D_METHOD("get_listener_context"), &TrainSoundListener3D::get_listener_context);
        ClassDB::bind_method(D_METHOD("is_inside_vehicle", "vehicle"), &TrainSoundListener3D::is_inside_vehicle);
        BIND_CONSTANT(EXTERIOR_CONTEXT);
        ADD_SIGNAL(MethodInfo(context_changed_signal));
    }

    void TrainSoundListener3D::_notification(const int p_what) {
        if (Engine::get_singleton()->is_editor_hint()) {
            return;
        }
        switch (p_what) {
            case NOTIFICATION_READY: {
                make_current();
                _refresh_context(get_viewport()->get_camera_3d());
                set_process(true);
            } break;
            case NOTIFICATION_PROCESS: {
                // follows the current camera - the player's, in whichever of its views; none before
                // the player has started (PlayerServer.player_started)
                Camera3D *camera = get_viewport()->get_camera_3d();
                if (camera == nullptr) {
                    return;
                }
                set_global_transform(camera->get_global_transform());
                _refresh_context(camera);
            } break;
            default:
                break;
        }
    }

    void TrainSoundListener3D::_refresh_context(Camera3D *p_camera) {
        Cabin3D *cabin = nullptr;
        for (Node *node = p_camera; node != nullptr && cabin == nullptr; node = node->get_parent()) {
            cabin = Object::cast_to<Cabin3D>(node);
        }
        const RID vehicle = cabin != nullptr ? cabin->get_vehicle_rid() : RID();
        const ObjectID cabin_id = cabin != nullptr ? ObjectID(cabin->get_instance_id()) : ObjectID();
        const int context = cabin != nullptr ? cabin->get_sound_listener_context() : EXTERIOR_CONTEXT;
        if (listener_vehicle == vehicle && listener_cabin == cabin_id && listener_context == context) {
            return;
        }
        listener_vehicle = vehicle;
        listener_cabin = cabin_id;
        listener_context = context;
        emit_signal(context_changed_signal);
    }

    RID TrainSoundListener3D::get_listener_vehicle() const {
        return listener_vehicle;
    }

    Cabin3D *TrainSoundListener3D::get_listener_cabin() const {
        return Object::cast_to<Cabin3D>(ObjectDB::get_instance(listener_cabin));
    }

    int TrainSoundListener3D::get_listener_context() const {
        return listener_context;
    }

    bool TrainSoundListener3D::is_inside_vehicle(const RID &p_vehicle) const {
        return get_listener_cabin() != nullptr && listener_vehicle == p_vehicle;
    }
} // namespace godot
