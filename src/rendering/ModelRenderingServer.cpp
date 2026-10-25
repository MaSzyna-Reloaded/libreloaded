#include "rendering/ModelRenderingBackend.hpp"
#include "rendering/ModelRenderingServer.hpp"
#include "scenery/SceneryStreamingProvider.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/templates/local_vector.hpp>

namespace godot {
    const char *ModelRenderingServer::instance_built_signal = "instance_built";
    const char *ModelRenderingServer::instance_freed_signal = "instance_freed";
    ModelRenderingServer *ModelRenderingServer::singleton = nullptr;

    void ModelRenderingServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("backend_register", "name", "backend_id"), &ModelRenderingServer::backend_register);
        ClassDB::bind_method(D_METHOD("backend_unregister", "name"), &ModelRenderingServer::backend_unregister);
        ClassDB::bind_method(D_METHOD("backend_get", "name"), &ModelRenderingServer::backend_get);
        ClassDB::bind_method(D_METHOD("backend_get_list"), &ModelRenderingServer::backend_get_list);
        ClassDB::bind_method(D_METHOD("model_load", "provider", "name"), &ModelRenderingServer::model_load);
        ClassDB::bind_method(
                D_METHOD("material_preload", "provider", "model", "data_path", "skins", "max_texture_size"),
                &ModelRenderingServer::material_preload);
        ClassDB::bind_method(
                D_METHOD("instance_create", "provider", "model", "instancer", "instance_kind"),
                &ModelRenderingServer::instance_create);
        ClassDB::bind_method(
                D_METHOD(
                        "instance_register", "provider", "name", "skins", "transform", "range_begin", "range_end",
                        "scenario"),
                &ModelRenderingServer::instance_register);
        ClassDB::bind_method(D_METHOD("instance_free", "instance"), &ModelRenderingServer::instance_free);
        ClassDB::bind_method(D_METHOD("instance_build", "instance"), &ModelRenderingServer::instance_build);
        ClassDB::bind_method(
                D_METHOD(
                        "instance_set_options", "instance", "data_path", "skins", "exclude_node_names", "force_alpha",
                        "force_alpha_part_paths", "max_texture_size"),
                &ModelRenderingServer::instance_set_options);
        ClassDB::bind_method(
                D_METHOD("instance_set_scenario", "instance", "scenario"),
                &ModelRenderingServer::instance_set_scenario);
        ClassDB::bind_method(
                D_METHOD("instance_set_transform", "instance", "transform"),
                &ModelRenderingServer::instance_set_transform);
        ClassDB::bind_method(
                D_METHOD("instance_set_visible", "instance", "visible"), &ModelRenderingServer::instance_set_visible);
        ClassDB::bind_method(
                D_METHOD("instance_attach_object_instance_id", "instance", "id"),
                &ModelRenderingServer::instance_attach_object_instance_id);
        ClassDB::bind_method(
                D_METHOD("instance_set_node_transform", "instance", "transform"),
                &ModelRenderingServer::instance_set_node_transform);
        ClassDB::bind_method(
                D_METHOD("instance_set_instancer", "instance", "instancer"),
                &ModelRenderingServer::instance_set_instancer);
        ClassDB::bind_method(D_METHOD("instance_get_aabb", "instance"), &ModelRenderingServer::instance_get_aabb);
        ClassDB::bind_method(
                D_METHOD("instance_get_transform", "instance"), &ModelRenderingServer::instance_get_transform);
        ClassDB::bind_method(
                D_METHOD("instance_has_part", "instance", "part"), &ModelRenderingServer::instance_has_part);
        ClassDB::bind_method(
                D_METHOD("instance_get_part_transform", "instance", "part"),
                &ModelRenderingServer::instance_get_part_transform);
        ClassDB::bind_method(
                D_METHOD("instance_set_part_poses", "instance", "poses"),
                &ModelRenderingServer::instance_set_part_poses);
        ClassDB::bind_method(
                D_METHOD("instance_set_part_visible", "instance", "part", "visible"),
                &ModelRenderingServer::instance_set_part_visible);
        ClassDB::bind_method(
                D_METHOD("instance_set_part_material_override", "instance", "part", "material"),
                &ModelRenderingServer::instance_set_part_material_override);
        ClassDB::bind_method(
                D_METHOD("instance_set_part_emission_energy", "instance", "part", "energy"),
                &ModelRenderingServer::instance_set_part_emission_energy);
        ClassDB::bind_method(
                D_METHOD("instance_set_emission_energy", "instance", "energy"),
                &ModelRenderingServer::instance_set_emission_energy);
        ClassDB::bind_method(
                D_METHOD("instance_has_light", "instance", "light"), &ModelRenderingServer::instance_has_light);
        ClassDB::bind_method(
                D_METHOD("instance_set_lights_state", "instance", "lights_state"),
                &ModelRenderingServer::instance_set_lights_state);
        ClassDB::bind_method(
                D_METHOD("instance_set_lights_dimmed", "instance", "lights_dimmed", "multiplier"),
                &ModelRenderingServer::instance_set_lights_dimmed);
        ClassDB::bind_method(
                D_METHOD("instance_set_lights_colors", "instance", "lights_colors"),
                &ModelRenderingServer::instance_set_lights_colors);
        ClassDB::bind_method(
                D_METHOD("instance_attach_signal_head", "instance", "signal_head"),
                &ModelRenderingServer::instance_attach_signal_head);
        ClassDB::bind_method(
                D_METHOD("signal_head_get_instance", "signal_head"), &ModelRenderingServer::signal_head_get_instance);
        ClassDB::bind_method(
                D_METHOD("instance_set_material_overlay", "instance", "material"),
                &ModelRenderingServer::instance_set_material_overlay);
        ClassDB::bind_method(
                D_METHOD("instance_intersect_segment", "instance", "from", "to"),
                &ModelRenderingServer::instance_intersect_segment);

        BIND_ENUM_CONSTANT(INSTANCER_OPTIMIZED);
        BIND_ENUM_CONSTANT(INSTANCER_NODES);
        BIND_ENUM_CONSTANT(INSTANCER_EDITABLE_NODES);
        BIND_ENUM_CONSTANT(INSTANCE_KIND_STATIC);
        BIND_ENUM_CONSTANT(INSTANCE_KIND_DYNAMIC);

        ADD_SIGNAL(MethodInfo(instance_freed_signal, PropertyInfo(Variant::RID, "instance")));
        ADD_SIGNAL(MethodInfo(instance_built_signal, PropertyInfo(Variant::RID, "instance")));
    }

    ModelRenderingServer *ModelRenderingServer::get_instance() {
        return singleton;
    }

    ModelRenderingServer::ModelRenderingServer() {
        singleton = this;
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL(streaming);
        streaming->content_set_consumer(
                SceneryStreamingProvider::CONTENT_MODELS, callable_mp(this, &ModelRenderingServer::_adopt_model),
                callable_mp(this, &ModelRenderingServer::instance_free));
    }

    ModelRenderingServer::~ModelRenderingServer() {
        singleton = nullptr;
    }

    /// A placement's model, its lights as the placement gives them
    RID ModelRenderingServer::_adopt_model(const Ref<SceneryModelPlacement> &p_placement, const RID &p_scenario) {
        ERR_FAIL_COND_V(p_placement.is_null(), RID());
        const RID instance = instance_register(
                p_placement->get_rendering_backend(), p_placement->get_model(), p_placement->get_skins(),
                p_placement->get_transform(), p_placement->get_range_min(), p_placement->get_range_max(), p_scenario);
        if (instance.is_valid() && !p_placement->get_lights_state().is_empty()) {
            instance_set_lights_state(instance, p_placement->get_lights_state());
        }
        if (instance.is_valid() && !p_placement->get_lights_colors().is_empty()) {
            instance_set_lights_colors(instance, p_placement->get_lights_colors());
        }
        return instance;
    }

    void ModelRenderingServer::backend_register(const StringName &p_name, const uint64_t p_backend_id) {
        ModelRenderingBackend *backend =
                Object::cast_to<ModelRenderingBackend>(ObjectDB::get_instance(ObjectID(p_backend_id)));
        ERR_FAIL_NULL(backend);
        ERR_FAIL_COND_MSG(backends.has(p_name), "A model rendering backend is registered already: " + p_name);
        backends[p_name] = backend;
        backend->connect(
                ModelRenderingBackend::instance_created_signal,
                callable_mp(this, &ModelRenderingServer::_on_instance_created).bind(p_name));
        backend->connect(
                ModelRenderingBackend::instance_built_signal,
                callable_mp(this, &ModelRenderingServer::_on_instance_built));
        backend->connect(
                ModelRenderingBackend::instance_freed_signal,
                callable_mp(this, &ModelRenderingServer::_on_instance_freed));
        if (StringName(ProjectSettings::get_singleton()->get_setting(BACKEND_SETTING, String())) == p_name) {
            default_backend = p_name;
        }
    }

    void ModelRenderingServer::backend_unregister(const StringName &p_name) {
        ModelRenderingBackend **found = backends.getptr(p_name);
        if (found == nullptr) {
            return;
        }
        ModelRenderingBackend *backend = *found;
        backend->disconnect(
                ModelRenderingBackend::instance_created_signal,
                callable_mp(this, &ModelRenderingServer::_on_instance_created).bind(p_name));
        backend->disconnect(
                ModelRenderingBackend::instance_built_signal,
                callable_mp(this, &ModelRenderingServer::_on_instance_built));
        backend->disconnect(
                ModelRenderingBackend::instance_freed_signal,
                callable_mp(this, &ModelRenderingServer::_on_instance_freed));
        LocalVector<RID> forgotten;
        for (const KeyValue<RID, ModelRenderingBackend *> &entry: instance_backends) {
            if (entry.value == backend) {
                forgotten.push_back(entry.key);
            }
        }
        for (const RID &instance: forgotten) {
            instance_backends.erase(instance);
        }
        backends.erase(p_name);
        if (default_backend == p_name) {
            default_backend = StringName();
        }
    }

    uint64_t ModelRenderingServer::backend_get(const StringName &p_name) const {
        ModelRenderingBackend *const *backend = backends.getptr(p_name);
        return backend != nullptr ? (*backend)->get_instance_id() : 0;
    }

    PackedStringArray ModelRenderingServer::backend_get_list() const {
        PackedStringArray result;
        for (const KeyValue<StringName, ModelRenderingBackend *> &entry: backends) {
            result.push_back(entry.key);
        }
        return result;
    }

    /// Read on the streaming worker threads as well: the backends are registered as the servers are
    /// made, before any model is loaded
    ModelRenderingBackend *ModelRenderingServer::_get_backend(const StringName &p_provider) const {
        const StringName &name = p_provider.is_empty() ? default_backend : p_provider;
        ModelRenderingBackend *const *found = backends.getptr(name);
        ERR_FAIL_NULL_V_MSG(found, nullptr, "No model rendering backend: \"" + name + "\"");
        return *found;
    }

    ModelRenderingBackend *ModelRenderingServer::_get_instance_backend(const RID &p_instance) const {
        ModelRenderingBackend *const *found = instance_backends.getptr(p_instance);
        return found != nullptr ? *found : nullptr;
    }

    void ModelRenderingServer::_on_instance_created(const RID &p_instance, const StringName &p_backend) {
        ModelRenderingBackend **found = backends.getptr(p_backend);
        ERR_FAIL_NULL(found);
        instance_backends[p_instance] = *found;
    }

    void ModelRenderingServer::_on_instance_built(const RID &p_instance) {
        emit_signal(instance_built_signal, p_instance);
    }

    void ModelRenderingServer::_on_instance_freed(const RID &p_instance) {
        instance_backends.erase(p_instance);
        emit_signal(instance_freed_signal, p_instance);
    }

    Ref<Resource> ModelRenderingServer::model_load(const StringName &p_provider, const String &p_name) {
        ModelRenderingBackend *backend = _get_backend(p_provider);
        ERR_FAIL_NULL_V(backend, Ref<Resource>());
        return backend->model_load(p_name);
    }

    Variant ModelRenderingServer::material_preload(
            const StringName &p_provider, const Ref<Resource> &p_model, const String &p_data_path,
            const PackedStringArray &p_skins, const int p_max_texture_size) {
        ModelRenderingBackend *backend = _get_backend(p_provider);
        ERR_FAIL_NULL_V(backend, Variant());
        return backend->material_preload(p_model, p_data_path, p_skins, p_max_texture_size);
    }

    RID ModelRenderingServer::instance_create(
            const StringName &p_provider, const Ref<Resource> &p_model, const Instancer p_instancer,
            const InstanceKind p_instance_kind) {
        ModelRenderingBackend *backend = _get_backend(p_provider);
        ERR_FAIL_NULL_V(backend, RID());
        return backend->instance_create(p_model, p_instancer, p_instance_kind);
    }

    RID ModelRenderingServer::instance_register(
            const StringName &p_provider, const String &p_name, const PackedStringArray &p_skins,
            const Transform3D &p_transform, const float p_range_begin, const float p_range_end, const RID &p_scenario) {
        ModelRenderingBackend *backend = _get_backend(p_provider);
        ERR_FAIL_NULL_V(backend, RID());
        return backend->instance_register(p_name, p_skins, p_transform, p_range_begin, p_range_end, p_scenario);
    }

    void ModelRenderingServer::instance_free(const RID &p_instance) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_free(p_instance);
    }

    void ModelRenderingServer::instance_build(const RID &p_instance) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_build(p_instance);
    }

    void ModelRenderingServer::instance_set_options(
            const RID &p_instance, const String &p_data_path, const PackedStringArray &p_skins,
            const Array &p_exclude_node_names, const bool p_force_alpha,
            const TypedArray<NodePath> &p_force_alpha_part_paths, const int p_max_texture_size) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_options(
                p_instance, p_data_path, p_skins, p_exclude_node_names, p_force_alpha, p_force_alpha_part_paths,
                p_max_texture_size);
    }

    void ModelRenderingServer::instance_set_scenario(const RID &p_instance, const RID &p_scenario) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_scenario(p_instance, p_scenario);
    }

    void ModelRenderingServer::instance_set_transform(const RID &p_instance, const Transform3D &p_transform) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_transform(p_instance, p_transform);
    }

    void ModelRenderingServer::instance_set_visible(const RID &p_instance, const bool p_visible) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_visible(p_instance, p_visible);
    }

    void ModelRenderingServer::instance_attach_object_instance_id(const RID &p_instance, const uint64_t p_id) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_attach_object_instance_id(p_instance, p_id);
    }

    void ModelRenderingServer::instance_set_node_transform(const RID &p_instance, const Transform3D &p_transform) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_node_transform(p_instance, p_transform);
    }

    void ModelRenderingServer::instance_set_instancer(const RID &p_instance, const Instancer p_instancer) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_instancer(p_instance, p_instancer);
    }

    AABB ModelRenderingServer::instance_get_aabb(const RID &p_instance) const {
        const ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL_V(backend, AABB());
        return backend->instance_get_aabb(p_instance);
    }

    Transform3D ModelRenderingServer::instance_get_transform(const RID &p_instance) const {
        const ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL_V(backend, Transform3D());
        return backend->instance_get_transform(p_instance);
    }

    bool ModelRenderingServer::instance_has_part(const RID &p_instance, const String &p_part) const {
        const ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL_V(backend, false);
        return backend->instance_has_part(p_instance, p_part);
    }

    Transform3D ModelRenderingServer::instance_get_part_transform(const RID &p_instance, const String &p_part) const {
        const ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL_V(backend, Transform3D());
        return backend->instance_get_part_transform(p_instance, p_part);
    }

    void ModelRenderingServer::instance_set_part_poses(const RID &p_instance, const Dictionary &p_poses) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_part_poses(p_instance, p_poses);
    }

    void
    ModelRenderingServer::instance_set_part_visible(const RID &p_instance, const String &p_part, const bool p_visible) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_part_visible(p_instance, p_part, p_visible);
    }

    void ModelRenderingServer::instance_set_part_material_override(
            const RID &p_instance, const String &p_part, const Ref<Material> &p_material) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_part_material_override(p_instance, p_part, p_material);
    }

    void ModelRenderingServer::instance_set_part_emission_energy(
            const RID &p_instance, const String &p_part, const float p_energy) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_part_emission_energy(p_instance, p_part, p_energy);
    }

    void ModelRenderingServer::instance_set_emission_energy(const RID &p_instance, const float p_energy) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_emission_energy(p_instance, p_energy);
    }

    bool ModelRenderingServer::instance_has_light(const RID &p_instance, const String &p_light) const {
        const ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL_V(backend, false);
        return backend->instance_has_light(p_instance, p_light);
    }

    void ModelRenderingServer::instance_set_lights_state(const RID &p_instance, const Dictionary &p_lights_state) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_lights_state(p_instance, p_lights_state);
    }

    void ModelRenderingServer::instance_set_lights_dimmed(
            const RID &p_instance, const Dictionary &p_lights_dimmed, const float p_multiplier) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_lights_dimmed(p_instance, p_lights_dimmed, p_multiplier);
    }

    void ModelRenderingServer::instance_set_lights_colors(const RID &p_instance, const Dictionary &p_lights_colors) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_lights_colors(p_instance, p_lights_colors);
    }

    void ModelRenderingServer::instance_attach_signal_head(const RID &p_instance, const RID &p_signal_head) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_attach_signal_head(p_instance, p_signal_head);
    }

    /// The backend that drew the head's instance knows it
    RID ModelRenderingServer::signal_head_get_instance(const RID &p_signal_head) const {
        for (const KeyValue<StringName, ModelRenderingBackend *> &item: backends) {
            const RID instance = item.value->signal_head_get_instance(p_signal_head);
            if (instance.is_valid()) {
                return instance;
            }
        }
        return RID();
    }

    void ModelRenderingServer::instance_set_material_overlay(const RID &p_instance, const Ref<Material> &p_material) {
        ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL(backend);
        backend->instance_set_material_overlay(p_instance, p_material);
    }

    Dictionary ModelRenderingServer::instance_intersect_segment(
            const RID &p_instance, const Vector3 &p_from, const Vector3 &p_to) const {
        const ModelRenderingBackend *backend = _get_instance_backend(p_instance);
        ERR_FAIL_NULL_V(backend, Dictionary());
        return backend->instance_intersect_segment(p_instance, p_from, p_to);
    }
} // namespace godot
