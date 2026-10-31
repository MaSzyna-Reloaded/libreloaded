#include "rendering/SceneModelRenderingBackend.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "signalling/SignallingServer.hpp"

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/geometry3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    SceneModelRenderingBackend::SceneModelRenderingBackend() {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL(streaming);
        // the scenes are loaded as a placement is built, on the main thread: nothing is read ahead
        stream_owner = streaming->owner_create(
                "scene models", Callable(), callable_mp(this, &SceneModelRenderingBackend::_stream_build),
                callable_mp(this, &SceneModelRenderingBackend::_stream_clear));
    }

    SceneModelRenderingBackend::~SceneModelRenderingBackend() {
        Vector<RID> remaining;
        for (const KeyValue<RID, InstanceData> &item: instances) {
            remaining.push_back(item.key);
        }
        for (const RID &rid: remaining) {
            instance_free(rid);
        }
    }

    Ref<Resource> SceneModelRenderingBackend::model_load(const String &p_name) {
        return ResourceLoader::get_singleton()->load(p_name, "PackedScene");
    }

    Variant SceneModelRenderingBackend::material_preload(
            const Ref<Resource> & /* p_model */, const String & /* p_data_path */,
            const PackedStringArray & /* p_skins */, const int /* p_max_texture_size */) {
        return Variant();
    }

    /// The model's nodes read off a scene made of it and freed at once: its meshes with the nodes
    /// above them, and every node's own transform by its name
    void SceneModelRenderingBackend::_read_model(InstanceData &p_instance) {
        p_instance.meshes.clear();
        p_instance.part_local_transforms.clear();
        p_instance.part_model_transforms.clear();
        if (p_instance.model.is_null()) {
            return;
        }
        Node *root = p_instance.model->instantiate();
        ERR_FAIL_NULL(root);
        for (int index = 0; index < root->get_child_count(); ++index) {
            _read_node(root->get_child(index), Vector<StringName>(), Vector<Transform3D>(), p_instance);
        }
        memdelete(root);
    }

    void SceneModelRenderingBackend::_read_node(
            const Node *p_node, const Vector<StringName> &p_names, const Vector<Transform3D> &p_transforms,
            InstanceData &p_instance) {
        const Node3D *spatial = Object::cast_to<Node3D>(p_node);
        Vector<StringName> names = p_names;
        Vector<Transform3D> transforms = p_transforms;
        if (spatial != nullptr) {
            names.push_back(p_node->get_name());
            transforms.push_back(spatial->get_transform());
            Transform3D model_transform;
            for (const Transform3D &transform: transforms) {
                model_transform = model_transform * transform;
            }
            p_instance.part_local_transforms[p_node->get_name()] = spatial->get_transform();
            p_instance.part_model_transforms[p_node->get_name()] = model_transform;
        }
        const MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(p_node);
        if (mesh_instance != nullptr && mesh_instance->get_mesh().is_valid()) {
            p_instance.meshes.push_back(MeshPart{mesh_instance->get_mesh(), names, transforms});
        }
        for (int index = 0; index < p_node->get_child_count(); ++index) {
            _read_node(p_node->get_child(index), names, transforms, p_instance);
        }
    }

    /// The mesh in the model's space, each node above it posed on top of its own transform
    Transform3D
    SceneModelRenderingBackend::_get_mesh_transform(const InstanceData &p_instance, const MeshPart &p_mesh) {
        Transform3D transform;
        for (int64_t index = 0; index < p_mesh.path_names.size(); ++index) {
            const Variant pose = p_instance.part_poses.get(p_mesh.path_names[index], Variant());
            transform = transform * p_mesh.path_transforms[index] *
                        (pose.get_type() == Variant::TRANSFORM3D ? Transform3D(pose) : Transform3D());
        }
        return transform;
    }

    bool SceneModelRenderingBackend::_is_mesh_visible(const InstanceData &p_instance, const MeshPart &p_mesh) const {
        for (const StringName &name: p_mesh.path_names) {
            const bool *visible = p_instance.part_visible.getptr(name);
            if (visible != nullptr && !*visible) {
                return false;
            }
        }
        return true;
    }

    /// What the mesh is drawn with instead of its own materials: the override of the deepest part
    /// above it that has one, else - while a self-illumination is set on a part above it or on the
    /// instance - its first emissive material lit that much; null for its own
    Ref<Material>
    SceneModelRenderingBackend::_get_mesh_material(const InstanceData &p_instance, const MeshPart &p_mesh) const {
        float energy = p_instance.emission_energy;
        for (int64_t index = p_mesh.path_names.size() - 1; index >= 0; --index) {
            const Ref<Material> *material = p_instance.part_materials.getptr(p_mesh.path_names[index]);
            if (material != nullptr && material->is_valid()) {
                return *material;
            }
        }
        for (const StringName &name: p_mesh.path_names) {
            const float *part_energy = p_instance.part_emission.getptr(name);
            if (part_energy != nullptr && *part_energy >= 0.0F) {
                energy = *part_energy;
            }
        }
        if (energy < 0.0F) {
            return Ref<Material>();
        }
        for (int surface = 0; surface < p_mesh.mesh->get_surface_count(); ++surface) {
            const Ref<BaseMaterial3D> own = p_mesh.mesh->surface_get_material(surface);
            if (own.is_valid() && own->get_feature(BaseMaterial3D::FEATURE_EMISSION)) {
                Ref<BaseMaterial3D> lit = own->duplicate();
                lit->set_emission_energy_multiplier(energy);
                return lit;
            }
        }
        return Ref<Material>();
    }

    Node3D *SceneModelRenderingBackend::_get_root(const InstanceData &p_instance) const {
        return Object::cast_to<Node3D>(ObjectDB::get_instance(p_instance.root_id));
    }

    /// What is set on the instance onto what it draws
    void SceneModelRenderingBackend::_apply(InstanceData &p_instance) const {
        if (!p_instance.built) {
            return;
        }
        RenderingServer *rs = RenderingServer::get_singleton();
        if (p_instance.instancer == ModelRenderingServer::INSTANCER_OPTIMIZED) {
            for (int index = 0; index < p_instance.meshes.size(); ++index) {
                const MeshPart &mesh = p_instance.meshes[index];
                const RID render_instance = p_instance.render_instances[index];
                rs->instance_set_scenario(render_instance, p_instance.scenario);
                rs->instance_set_transform(
                        render_instance, p_instance.transform * _get_mesh_transform(p_instance, mesh));
                rs->instance_set_visible(render_instance, p_instance.visible && _is_mesh_visible(p_instance, mesh));
                const Ref<Material> material = _get_mesh_material(p_instance, mesh);
                for (int surface = 0; surface < mesh.mesh->get_surface_count(); ++surface) {
                    rs->instance_set_surface_override_material(
                            render_instance, surface, material.is_valid() ? material->get_rid() : RID());
                }
                rs->instance_geometry_set_material_overlay(
                        render_instance, p_instance.overlay.is_valid() ? p_instance.overlay->get_rid() : RID());
                for (const Variant &name: p_instance.shader_parameters.keys()) {
                    rs->instance_geometry_set_shader_parameter(
                            render_instance, name, p_instance.shader_parameters[name]);
                }
            }
            return;
        }
        Node3D *root = _get_root(p_instance);
        if (root == nullptr) {
            return;
        }
        root->set_transform(p_instance.node_transform);
        root->set_visible(p_instance.visible);
        for (const KeyValue<StringName, Transform3D> &part: p_instance.part_local_transforms) {
            Node3D *node = Object::cast_to<Node3D>(root->find_child(part.key, true, false));
            if (node == nullptr) {
                continue;
            }
            const Variant pose = p_instance.part_poses.get(part.key, Variant());
            node->set_transform(
                    part.value * (pose.get_type() == Variant::TRANSFORM3D ? Transform3D(pose) : Transform3D()));
            const bool *visible = p_instance.part_visible.getptr(part.key);
            node->set_visible(visible == nullptr || *visible);
        }
        for (const MeshPart &mesh: p_instance.meshes) {
            MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(
                    root->find_child(mesh.path_names[mesh.path_names.size() - 1], true, false));
            if (mesh_instance == nullptr) {
                continue;
            }
            mesh_instance->set_material_override(_get_mesh_material(p_instance, mesh));
            mesh_instance->set_material_overlay(p_instance.overlay);
            for (const Variant &name: p_instance.shader_parameters.keys()) {
                mesh_instance->set_instance_shader_parameter(name, p_instance.shader_parameters[name]);
            }
        }
    }

    /// What the instance draws freed; what is set on it stays
    void SceneModelRenderingBackend::_clear(InstanceData &p_instance) {
        RenderingServer *rs = RenderingServer::get_singleton();
        for (const RID &render_instance: p_instance.render_instances) {
            rs->free_rid(render_instance);
        }
        p_instance.render_instances.clear();
        Node3D *root = _get_root(p_instance);
        if (root != nullptr) {
            root->queue_free();
        }
        p_instance.root_id = ObjectID();
        p_instance.built = false;
    }

    RID SceneModelRenderingBackend::instance_create(
            const Ref<Resource> &p_model, const Instancer p_instancer, const InstanceKind /* p_instance_kind */) {
        const Ref<PackedScene> model = p_model;
        ERR_FAIL_COND_V(model.is_null(), RID());
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        InstanceData &instance = instances[rid];
        instance.model = model;
        instance.instancer = p_instancer;
        _read_model(instance);
        emit_signal(instance_created_signal, rid);
        return rid;
    }

    /// Nothing is loaded until the camera comes within p_range_end of it (SceneryStreamingServer);
    /// a scene has no skins and is drawn whole, so p_skins and p_range_begin say nothing to it
    RID SceneModelRenderingBackend::instance_register(
            const String &p_name, const PackedStringArray & /* p_skins */, const Transform3D &p_transform,
            const float /* p_range_begin */, const float p_range_end, const RID &p_scenario) {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL_V(streaming, RID());
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        InstanceData &instance = instances[rid];
        instance.model_path = p_name;
        instance.transform = p_transform;
        instance.scenario = p_scenario;
        instance.stream_rid = streaming->stream_register(stream_owner, rid, p_transform.origin, p_range_end);
        emit_signal(instance_created_signal, rid);
        return rid;
    }

    void SceneModelRenderingBackend::_stream_build(const RID &p_instance, const Variant & /* p_preloaded */) {
        InstanceData *instance = instances.getptr(p_instance);
        if (instance == nullptr) {
            return;
        }
        if (instance->model.is_null()) {
            instance->model = model_load(instance->model_path);
            _read_model(*instance);
        }
        instance_build(p_instance);
    }

    /// Out of the camera's range the placement draws nothing and lets its scene go
    void SceneModelRenderingBackend::_stream_clear(const RID &p_instance) {
        InstanceData *instance = instances.getptr(p_instance);
        if (instance == nullptr) {
            return;
        }
        _clear(*instance);
        instance->model.unref();
        instance->meshes.clear();
    }

    void SceneModelRenderingBackend::instance_free(const RID &p_instance) {
        InstanceData *found = instances.getptr(p_instance);
        ERR_FAIL_NULL(found);
        // taken out first: freeing the stream or the signal head may re-enter the backend
        InstanceData data = *found;
        instances.erase(p_instance);
        _clear(data);
        if (data.stream_rid.is_valid()) {
            SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
            if (streaming != nullptr) {
                streaming->stream_free(data.stream_rid);
            }
        }
        if (data.signal_head.is_valid()) {
            SignallingServer *signalling = SignallingServer::get_instance();
            if (signalling != nullptr) {
                signalling->signal_head_free(data.signal_head);
            }
        }
        emit_signal(instance_freed_signal, p_instance);
    }

    /// Optimized: a RenderingServer instance a mesh; as nodes: a scene made of the model under the
    /// attached node
    void SceneModelRenderingBackend::instance_build(const RID &p_instance) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        _clear(*instance);
        if (instance->model.is_null()) {
            return;
        }
        if (instance->instancer == ModelRenderingServer::INSTANCER_OPTIMIZED) {
            RenderingServer *rs = RenderingServer::get_singleton();
            for (const MeshPart &mesh: instance->meshes) {
                const RID render_instance = rs->instance_create();
                rs->instance_set_base(render_instance, mesh.mesh->get_rid());
                instance->render_instances.push_back(render_instance);
            }
        } else {
            Node3D *attached = Object::cast_to<Node3D>(ObjectDB::get_instance(instance->attached_id));
            ERR_FAIL_NULL_MSG(attached, "A scene model built as nodes is built under its attached Node3D");
            Node3D *root = Object::cast_to<Node3D>(instance->model->instantiate());
            ERR_FAIL_NULL_MSG(root, "A scene model's root is a Node3D");
            attached->add_child(root);
            instance->root_id = root->get_instance_id();
        }
        instance->built = true;
        _apply(*instance);
        emit_signal(instance_built_signal, p_instance);
    }

    void SceneModelRenderingBackend::instance_set_options(
            const RID & /* p_instance */, const String & /* p_data_path */, const PackedStringArray & /* p_skins */,
            const Array & /* p_exclude_node_names */, const bool /* p_force_alpha */,
            const TypedArray<NodePath> & /* p_force_alpha_part_paths */, const int /* p_max_texture_size */) {
        // a scene is drawn whole, in its own materials
    }

    void SceneModelRenderingBackend::instance_set_scenario(const RID &p_instance, const RID &p_scenario) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->scenario = p_scenario;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_transform(const RID &p_instance, const Transform3D &p_transform) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->transform = p_transform;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_visible(const RID &p_instance, const bool p_visible) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->visible = p_visible;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_attach_object_instance_id(const RID &p_instance, const uint64_t p_id) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->attached_id = ObjectID(p_id);
    }

    void
    SceneModelRenderingBackend::instance_set_node_transform(const RID &p_instance, const Transform3D &p_transform) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->node_transform = p_transform;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_instancer(const RID &p_instance, const Instancer p_instancer) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->instancer = p_instancer;
        if (instance->built) {
            instance_build(p_instance);
        }
    }

    Transform3D SceneModelRenderingBackend::instance_get_transform(const RID &p_instance) const {
        const InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, Transform3D());
        return instance->transform;
    }

    AABB SceneModelRenderingBackend::instance_get_aabb(const RID &p_instance) const {
        const InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, AABB());
        AABB bounds;
        bool first = true;
        for (const MeshPart &mesh: instance->meshes) {
            const AABB mesh_bounds = _get_mesh_transform(*instance, mesh).xform(mesh.mesh->get_aabb());
            bounds = first ? mesh_bounds : bounds.merge(mesh_bounds);
            first = false;
        }
        return bounds;
    }

    bool SceneModelRenderingBackend::instance_has_part(const RID &p_instance, const String &p_part) const {
        const InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, false);
        return instance->part_model_transforms.has(p_part);
    }

    Transform3D
    SceneModelRenderingBackend::instance_get_part_transform(const RID &p_instance, const String &p_part) const {
        const InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, Transform3D());
        const Transform3D *transform = instance->part_model_transforms.getptr(p_part);
        return transform != nullptr ? *transform : Transform3D();
    }

    /// An empty transform takes a part back to its own
    void SceneModelRenderingBackend::instance_set_part_poses(const RID &p_instance, const Dictionary &p_poses) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        const Array parts = p_poses.keys();
        for (int index = 0; index < parts.size(); ++index) {
            const Transform3D pose = p_poses[parts[index]];
            if (pose == Transform3D()) {
                instance->part_poses.erase(StringName(parts[index]));
            } else {
                instance->part_poses[StringName(parts[index])] = pose;
            }
        }
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_part_visible(
            const RID &p_instance, const String &p_part, const bool p_visible) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->part_visible[p_part] = p_visible;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_part_material_override(
            const RID &p_instance, const String &p_part, const Ref<Material> &p_material) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->part_materials[p_part] = p_material;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_part_emission_energy(
            const RID &p_instance, const String &p_part, const float p_energy) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->part_emission[p_part] = p_energy;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_emission_energy(const RID &p_instance, const float p_energy) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->emission_energy = p_energy;
        _apply(*instance);
    }

    bool
    SceneModelRenderingBackend::instance_has_light(const RID & /* p_instance */, const String & /* p_light */) const {
        return false;
    }

    void SceneModelRenderingBackend::instance_set_lights_state(
            const RID & /* p_instance */, const Dictionary & /* p_lights_state */) {}

    void SceneModelRenderingBackend::instance_set_lights_dimmed(
            const RID & /* p_instance */, const Dictionary & /* p_lights_dimmed */, const float /* p_multiplier */) {}

    void SceneModelRenderingBackend::instance_set_lights_colors(
            const RID & /* p_instance */, const Dictionary & /* p_lights_colors */) {}

    /// A scene has no lights to draw the head with; the head goes with the instance
    void SceneModelRenderingBackend::instance_attach_signal_head(const RID &p_instance, const RID &p_signal_head) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->signal_head = p_signal_head;
    }

    RID SceneModelRenderingBackend::signal_head_get_instance(const RID &p_signal_head) const {
        for (const KeyValue<RID, InstanceData> &item: instances) {
            if (item.value.signal_head == p_signal_head) {
                return item.key;
            }
        }
        return RID();
    }

    void
    SceneModelRenderingBackend::instance_set_material_overlay(const RID &p_instance, const Ref<Material> &p_material) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->overlay = p_material;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_shader_parameter(
            const RID &p_instance, const StringName &p_name, const Variant &p_value) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        // a null is set too: it takes the uniform off what is drawn
        instance->shader_parameters[p_name] = p_value;
        _apply(*instance);
    }

    Ref<Material> SceneModelRenderingBackend::instance_get_material_overlay(const RID &p_instance) const {
        const InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, Ref<Material>());
        return instance->overlay;
    }

    /// Against the triangles of its visible meshes, posed
    Dictionary SceneModelRenderingBackend::instance_intersect_segment(
            const RID &p_instance, const Vector3 &p_from, const Vector3 &p_to) const {
        const InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, Dictionary());
        Geometry3D *geometry = Geometry3D::get_singleton();
        Dictionary nearest;
        real_t nearest_distance = Math::INF;
        for (const MeshPart &mesh: instance->meshes) {
            if (!_is_mesh_visible(*instance, mesh)) {
                continue;
            }
            const Transform3D transform = instance->transform * _get_mesh_transform(*instance, mesh);
            const PackedVector3Array faces = mesh.mesh->get_faces();
            for (int64_t index = 0; index + 2 < faces.size(); index += 3) {
                const Variant hit = geometry->segment_intersects_triangle(
                        p_from, p_to, transform.xform(faces[index]), transform.xform(faces[index + 1]),
                        transform.xform(faces[index + 2]));
                if (!(hit.get_type() == Variant::VECTOR3)) {
                    continue;
                }
                const Vector3 position = hit;
                const real_t distance = p_from.distance_to(position);
                if (distance < nearest_distance) {
                    nearest_distance = distance;
                    nearest["position"] = position;
                    nearest["distance"] = distance;
                }
            }
        }
        return nearest;
    }
} // namespace godot
