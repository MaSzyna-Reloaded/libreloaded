#include "rendering/ModelRenderingServer.hpp"
#include "rendering/ReloadedModel3D.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    const char *ReloadedModel3D::model_instance_created_signal = "model_instance_created";

    void ReloadedModel3D::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_model_instance"), &ReloadedModel3D::get_model_instance);
        ClassDB::bind_method(D_METHOD("set_model_instance", "instance"), &ReloadedModel3D::set_model_instance);
        ClassDB::bind_method(D_METHOD("set_lights_state", "lights_state"), &ReloadedModel3D::set_lights_state);
        ClassDB::bind_method(D_METHOD("get_lights_state"), &ReloadedModel3D::get_lights_state);
        ClassDB::bind_method(D_METHOD("set_lights_dimmed", "lights_dimmed"), &ReloadedModel3D::set_lights_dimmed);
        ClassDB::bind_method(D_METHOD("get_lights_dimmed"), &ReloadedModel3D::get_lights_dimmed);
        ClassDB::bind_method(
                D_METHOD("set_lights_dimmed_multiplier", "multiplier"), &ReloadedModel3D::set_lights_dimmed_multiplier);
        ClassDB::bind_method(D_METHOD("get_lights_dimmed_multiplier"), &ReloadedModel3D::get_lights_dimmed_multiplier);
        ClassDB::bind_method(D_METHOD("attach_signal_head", "signal_head"), &ReloadedModel3D::attach_signal_head);
        ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "lights_state"), "set_lights_state", "get_lights_state");
        ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "lights_dimmed"), "set_lights_dimmed", "get_lights_dimmed");
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "lights_dimmed_multiplier"), "set_lights_dimmed_multiplier",
                "get_lights_dimmed_multiplier");
        ADD_SIGNAL(MethodInfo(model_instance_created_signal, PropertyInfo(Variant::RID, "instance")));
        GDVIRTUAL_BIND(_normalize_lights_state, "lights_state");
    }

    RID ReloadedModel3D::get_model_instance() const {
        return model_instance;
    }

    void ReloadedModel3D::set_model_instance(const RID &p_instance) {
        model_instance = p_instance;
    }

    void ReloadedModel3D::set_lights_state(const Dictionary &p_lights_state) {
        Dictionary normalized = p_lights_state;
        GDVIRTUAL_CALL(_normalize_lights_state, p_lights_state, normalized);
        lights_state = normalized;
        if (model_instance.is_valid()) {
            ModelRenderingServer *models = ModelRenderingServer::get_instance();
            ERR_FAIL_NULL(models);
            models->instance_set_lights_state(model_instance, lights_state);
        }
    }

    Dictionary ReloadedModel3D::get_lights_state() const {
        return lights_state;
    }

    void ReloadedModel3D::set_lights_dimmed(const Dictionary &p_lights_dimmed) {
        lights_dimmed = p_lights_dimmed;
        if (model_instance.is_valid()) {
            ModelRenderingServer *models = ModelRenderingServer::get_instance();
            ERR_FAIL_NULL(models);
            models->instance_set_lights_dimmed(model_instance, lights_dimmed, lights_dimmed_multiplier);
        }
    }

    Dictionary ReloadedModel3D::get_lights_dimmed() const {
        return lights_dimmed;
    }

    void ReloadedModel3D::set_lights_dimmed_multiplier(const float p_multiplier) {
        lights_dimmed_multiplier = p_multiplier;
        if (model_instance.is_valid()) {
            ModelRenderingServer *models = ModelRenderingServer::get_instance();
            ERR_FAIL_NULL(models);
            models->instance_set_lights_dimmed(model_instance, lights_dimmed, lights_dimmed_multiplier);
        }
    }

    float ReloadedModel3D::get_lights_dimmed_multiplier() const {
        return lights_dimmed_multiplier;
    }

    void ReloadedModel3D::attach_signal_head(const RID &p_signal_head) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        ERR_FAIL_NULL(models);
        models->instance_attach_signal_head(model_instance, p_signal_head);
    }
} // namespace godot
