#include "rendering/ModelRenderingBackend.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    const char *ModelRenderingBackend::instance_created_signal = "instance_created";
    const char *ModelRenderingBackend::instance_built_signal = "instance_built";
    const char *ModelRenderingBackend::instance_freed_signal = "instance_freed";

    void ModelRenderingBackend::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_priority"), &ModelRenderingBackend::get_priority);
        ClassDB::bind_method(D_METHOD("model_load", "name"), &ModelRenderingBackend::model_load);
        ClassDB::bind_method(
                D_METHOD("material_preload", "model", "data_path", "skins", "max_texture_size"),
                &ModelRenderingBackend::material_preload);
        ClassDB::bind_method(
                D_METHOD("instance_create", "model", "instancer", "instance_kind"),
                &ModelRenderingBackend::instance_create);
        ClassDB::bind_method(
                D_METHOD("instance_register", "name", "skins", "transform", "range_begin", "range_end", "scenario"),
                &ModelRenderingBackend::instance_register);
        ClassDB::bind_method(D_METHOD("instance_free", "instance"), &ModelRenderingBackend::instance_free);
        ClassDB::bind_method(D_METHOD("instance_build", "instance"), &ModelRenderingBackend::instance_build);
        ClassDB::bind_method(
                D_METHOD(
                        "instance_set_options", "instance", "data_path", "skins", "exclude_node_names", "force_alpha",
                        "force_alpha_part_paths", "max_texture_size"),
                &ModelRenderingBackend::instance_set_options);
        ClassDB::bind_method(
                D_METHOD("instance_set_scenario", "instance", "scenario"),
                &ModelRenderingBackend::instance_set_scenario);
        ClassDB::bind_method(
                D_METHOD("instance_set_transform", "instance", "transform"),
                &ModelRenderingBackend::instance_set_transform);
        ClassDB::bind_method(
                D_METHOD("instance_set_visible", "instance", "visible"), &ModelRenderingBackend::instance_set_visible);
        ClassDB::bind_method(
                D_METHOD("instance_attach_object_instance_id", "instance", "id"),
                &ModelRenderingBackend::instance_attach_object_instance_id);
        ClassDB::bind_method(
                D_METHOD("instance_set_node_transform", "instance", "transform"),
                &ModelRenderingBackend::instance_set_node_transform);
        ClassDB::bind_method(
                D_METHOD("instance_set_instancer", "instance", "instancer"),
                &ModelRenderingBackend::instance_set_instancer);
        ClassDB::bind_method(D_METHOD("instance_get_aabb", "instance"), &ModelRenderingBackend::instance_get_aabb);
        ClassDB::bind_method(
                D_METHOD("instance_get_transform", "instance"), &ModelRenderingBackend::instance_get_transform);
        ClassDB::bind_method(
                D_METHOD("instance_has_part", "instance", "part"), &ModelRenderingBackend::instance_has_part);
        ClassDB::bind_method(
                D_METHOD("instance_get_part_transform", "instance", "part"),
                &ModelRenderingBackend::instance_get_part_transform);
        ClassDB::bind_method(
                D_METHOD("instance_set_part_poses", "instance", "poses"),
                &ModelRenderingBackend::instance_set_part_poses);
        ClassDB::bind_method(
                D_METHOD("instance_set_part_visible", "instance", "part", "visible"),
                &ModelRenderingBackend::instance_set_part_visible);
        ClassDB::bind_method(
                D_METHOD("instance_set_part_material_override", "instance", "part", "material"),
                &ModelRenderingBackend::instance_set_part_material_override);
        ClassDB::bind_method(
                D_METHOD("instance_set_part_emission_energy", "instance", "part", "energy"),
                &ModelRenderingBackend::instance_set_part_emission_energy);
        ClassDB::bind_method(
                D_METHOD("instance_set_emission_energy", "instance", "energy"),
                &ModelRenderingBackend::instance_set_emission_energy);
        ClassDB::bind_method(
                D_METHOD("instance_has_light", "instance", "light"), &ModelRenderingBackend::instance_has_light);
        ClassDB::bind_method(
                D_METHOD("instance_set_lights_state", "instance", "lights_state"),
                &ModelRenderingBackend::instance_set_lights_state);
        ClassDB::bind_method(
                D_METHOD("instance_set_lights_dimmed", "instance", "lights_dimmed", "multiplier"),
                &ModelRenderingBackend::instance_set_lights_dimmed);
        ClassDB::bind_method(
                D_METHOD("instance_attach_signal_head", "instance", "signal_head"),
                &ModelRenderingBackend::instance_attach_signal_head);
        ClassDB::bind_method(
                D_METHOD("signal_head_get_instance", "signal_head"), &ModelRenderingBackend::signal_head_get_instance);
        ClassDB::bind_method(
                D_METHOD("instance_set_material_overlay", "instance", "material"),
                &ModelRenderingBackend::instance_set_material_overlay);
        ClassDB::bind_method(
                D_METHOD("instance_intersect_segment", "instance", "from", "to"),
                &ModelRenderingBackend::instance_intersect_segment);

        ADD_SIGNAL(MethodInfo(instance_created_signal, PropertyInfo(Variant::RID, "instance")));
        ADD_SIGNAL(MethodInfo(instance_freed_signal, PropertyInfo(Variant::RID, "instance")));
        ADD_SIGNAL(MethodInfo(instance_built_signal, PropertyInfo(Variant::RID, "instance")));
    }

    double ModelRenderingBackend::get_priority() const {
        return DEFAULT_PRIORITY;
    }
} // namespace godot
