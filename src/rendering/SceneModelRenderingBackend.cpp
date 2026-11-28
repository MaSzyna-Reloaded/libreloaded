#include "rendering/SceneModelRenderingBackend.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "signalling/SignallingServer.hpp"

#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/collision_object3d.hpp>
#include <godot_cpp/classes/collision_shape3d.hpp>
#include <godot_cpp/classes/geometry3d.hpp>
#include <godot_cpp/classes/light3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/spot_light3d.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    SceneModelRenderingBackend::SceneModelRenderingBackend() {
        glare_mesh.instantiate();
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

    namespace {
        /* The metadata a glTF material keeps its extras in (GLTFDocument), and the key of them that
         * names its slot of replaceable textures - spelled as the models' format spells it */
        const char *const EXTRAS_META = "extras";
        /* The key of a node's extras naming its role, and the roles of a collision body's (the model
         * format's `<object>.collision.body` and `.interior`) */
        const char *const SEMANTIC_ID_KEY = "semantic_id";
        const char *const BODY_ROLE_SUFFIX = ".collision.body";
        const char *const INTERIOR_ROLE_SUFFIX = ".collision.interior";
        const char *const REPLACEABLE_TEXTURE_KEY = "replacable_texture";
        /* The key of a node's extras turning it by a channel, and the keys of what it gives (the
         * model format's procedural motion) */
        const char *const ROTATION_CONTROL_KEY = "rotation_control";
        const char *const CONTROL_SOURCE_KEY = "source";
        const char *const CONTROL_AXIS_KEY = "axis";
        const char *const CONTROL_SCALE_KEY = "scale";
        const char *const CONTROL_OFFSET_KEY = "offset";
        const char *const CONTROL_DEFAULT_KEY = "default";
        const char *const CONTROL_LIMITS_KEY = "limits";
        /* The keys of a light's extras dimming it by a channel, and of a lens naming its light */
        const char *const LIGHT_CONTROL_KEY = "light_control";
        const char *const LIGHT_ID_KEY = "light_id";
        /* The glare shader's instance uniforms: its lamp's colour, the cosine of its spot's edge and
         * its level (light_glare.gdshader) */
        const char *const GLARE_COLOR_PARAMETER = "light_color";
        const char *const GLARE_COS_OUTER_PARAMETER = "cos_outer";
        const char *const GLARE_LEVEL_PARAMETER = "glare_level";
        /* An axis written as its three components, limits as their two */
        constexpr int64_t AXIS_COMPONENTS = 3;
        constexpr int64_t LIMITS_BOUNDS = 2;
        /* The textures of a slot (ModelRenderingServer.instance_set_replaceable_textures()) */
        const char *const BASE_COLOR_TEXTURE = "base_color";
        const char *const ORM_TEXTURE = "orm";
        const char *const NORMAL_TEXTURE = "normal";
    } // namespace

    /// The model's nodes read off a scene made of it and freed at once: its meshes with the nodes
    /// above them, and every node's own transform by its path from the root; what was set of its
    /// parts before - a streamed placement's, read when built - by their paths from now on
    void SceneModelRenderingBackend::_read_model(InstanceData &p_instance) {
        p_instance.meshes.clear();
        p_instance.shapes.clear();
        p_instance.rotation_controls.clear();
        p_instance.light_controls.clear();
        p_instance.lens_controls.clear();
        p_instance.part_local_transforms.clear();
        p_instance.part_model_transforms.clear();
        p_instance.part_paths.clear();
        if (p_instance.model.is_null()) {
            return;
        }
        Node *root = p_instance.model->instantiate();
        ERR_FAIL_NULL(root);
        for (int index = 0; index < root->get_child_count(); ++index) {
            _read_node(root->get_child(index), Vector<StringName>(), Vector<Transform3D>(), p_instance);
        }
        // a lens takes the channel of the light it names, read by now (_read_node() keeps the name)
        Vector<LensControl> lenses;
        for (const LensControl &lens: p_instance.lens_controls) {
            for (const LightControl &light: p_instance.light_controls) {
                const Node *light_node = root->get_node_or_null(NodePath(String(light.part)));
                if (light_node == nullptr) {
                    continue;
                }
                const Dictionary extras = light_node->has_meta(EXTRAS_META)
                                                  ? Dictionary(light_node->get_meta(EXTRAS_META))
                                                  : Dictionary();
                // the light its lens names: by its node's name, its semantic_id, or the light_id both share
                const StringName semantic_id = String(extras.get(SEMANTIC_ID_KEY, String()));
                const StringName light_id = String(extras.get(LIGHT_ID_KEY, String()));
                if (lens.source == light_node->get_name() || lens.source == semantic_id || lens.source == light_id) {
                    lenses.push_back(LensControl{lens.part, light.source, light.default_value});
                }
            }
        }
        p_instance.lens_controls = lenses;
        memdelete(root);
        Dictionary poses;
        for (const Variant &part: p_instance.part_poses.keys()) {
            poses[_part_path(p_instance, part)] = p_instance.part_poses[part];
        }
        p_instance.part_poses = poses;
        HashMap<StringName, bool> visible;
        for (const KeyValue<StringName, bool> &part: p_instance.part_visible) {
            visible[_part_path(p_instance, part.key)] = part.value;
        }
        p_instance.part_visible = visible;
        HashMap<StringName, Ref<Material>> materials;
        for (const KeyValue<StringName, Ref<Material>> &part: p_instance.part_materials) {
            materials[_part_path(p_instance, part.key)] = part.value;
        }
        p_instance.part_materials = materials;
        HashMap<StringName, float> emission;
        for (const KeyValue<StringName, float> &part: p_instance.part_emission) {
            emission[_part_path(p_instance, part.key)] = part.value;
        }
        p_instance.part_emission = emission;
        _replace_materials(p_instance);
        _pose_channels(p_instance);
    }


    /// Each material of the model's meshes of a slot the replaceable textures have, copied with the
    /// slot's textures: the base colour; occlusion, roughness and metalness of one texture's red,
    /// green and blue; the normal map - none while there are none
    void SceneModelRenderingBackend::_replace_materials(InstanceData &p_instance) {
        p_instance.replaced_materials.clear();
        if (p_instance.replaceable_textures.is_empty()) {
            return;
        }
        for (const MeshPart &mesh: p_instance.meshes) {
            for (int surface = 0; surface < mesh.mesh->get_surface_count(); ++surface) {
                const Ref<BaseMaterial3D> own = mesh.mesh->surface_get_material(surface);
                if (own.is_null() || p_instance.replaced_materials.has(own->get_rid()) || !own->has_meta(EXTRAS_META)) {
                    continue;
                }
                const Dictionary extras = own->get_meta(EXTRAS_META);
                const Variant slot = extras.get(REPLACEABLE_TEXTURE_KEY, Variant());
                if (slot.get_type() == Variant::NIL || !p_instance.replaceable_textures.has(StringName(slot))) {
                    continue;
                }
                const Dictionary textures = p_instance.replaceable_textures[StringName(slot)];
                Ref<BaseMaterial3D> replaced = own->duplicate();
                const Ref<Texture2D> base_color = textures.get(BASE_COLOR_TEXTURE, Variant());
                if (base_color.is_valid()) {
                    replaced->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, base_color);
                }
                const Ref<Texture2D> orm = textures.get(ORM_TEXTURE, Variant());
                if (orm.is_valid()) {
                    replaced->set_feature(BaseMaterial3D::FEATURE_AMBIENT_OCCLUSION, true);
                    replaced->set_texture(BaseMaterial3D::TEXTURE_AMBIENT_OCCLUSION, orm);
                    replaced->set_ao_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_RED);
                    replaced->set_texture(BaseMaterial3D::TEXTURE_ROUGHNESS, orm);
                    replaced->set_roughness_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_GREEN);
                    replaced->set_texture(BaseMaterial3D::TEXTURE_METALLIC, orm);
                    replaced->set_metallic_texture_channel(BaseMaterial3D::TEXTURE_CHANNEL_BLUE);
                }
                const Ref<Texture2D> normal = textures.get(NORMAL_TEXTURE, Variant());
                if (normal.is_valid()) {
                    replaced->set_feature(BaseMaterial3D::FEATURE_NORMAL_MAPPING, true);
                    replaced->set_texture(BaseMaterial3D::TEXTURE_NORMAL, normal);
                }
                p_instance.replaced_materials[own->get_rid()] = replaced;
            }
        }
    }

    /// A part as the model's nodes are kept: its path from the root, of a name no other node has
    /// too; as it is given for any other (a path, a name of more nodes, one of no model read yet)
    StringName SceneModelRenderingBackend::_part_path(const InstanceData &p_instance, const StringName &p_part) {
        const StringName *path = p_instance.part_paths.getptr(p_part);
        return path != nullptr && !path->is_empty() ? *path : p_part;
    }

    void SceneModelRenderingBackend::_read_node(
            const Node *p_node, const Vector<StringName> &p_names, const Vector<Transform3D> &p_transforms,
            InstanceData &p_instance) {
        const Node3D *spatial = Object::cast_to<Node3D>(p_node);
        Vector<StringName> names = p_names;
        Vector<Transform3D> transforms = p_transforms;
        if (spatial != nullptr) {
            const StringName name = p_node->get_name();
            const StringName path =
                    p_names.is_empty() ? name : StringName(String(p_names[p_names.size() - 1]).path_join(name));
            names.push_back(path);
            transforms.push_back(spatial->get_transform());
            Transform3D model_transform;
            for (const Transform3D &transform: transforms) {
                model_transform = model_transform * transform;
            }
            p_instance.part_local_transforms[path] = spatial->get_transform();
            p_instance.part_model_transforms[path] = model_transform;
            // a name of more nodes names none of them
            p_instance.part_paths[name] = p_instance.part_paths.has(name) ? StringName() : path;
            // a node turned by a channel (the model format's rotation_control): of a source and an axis
            const Dictionary extras =
                    p_node->has_meta(EXTRAS_META) ? Dictionary(p_node->get_meta(EXTRAS_META)) : Dictionary();
            const Variant control_data = extras.get(ROTATION_CONTROL_KEY, Variant());
            if (control_data.get_type() == Variant::DICTIONARY) {
                const Dictionary control = control_data;
                const Array axis = control.get(CONTROL_AXIS_KEY, Array());
                const String source = control.get(CONTROL_SOURCE_KEY, String());
                if (!source.is_empty() && axis.size() == AXIS_COMPONENTS) {
                    RotationControl rotation;
                    rotation.part = path;
                    rotation.source = source;
                    rotation.axis = Vector3(axis[0], axis[1], axis[2]).normalized();
                    rotation.scale = control.get(CONTROL_SCALE_KEY, 1.0);
                    rotation.offset = control.get(CONTROL_OFFSET_KEY, 0.0);
                    rotation.default_value = control.get(CONTROL_DEFAULT_KEY, 0.0);
                    const Array limits = control.get(CONTROL_LIMITS_KEY, Array());
                    rotation.limited = limits.size() == LIMITS_BOUNDS;
                    if (rotation.limited) {
                        rotation.limit_min = limits[0];
                        rotation.limit_max = limits[1];
                    }
                    p_instance.rotation_controls.push_back(rotation);
                }
            }
            // a light dimmed by a channel, and a lens of a light (named here, its channel once all
            // the lights are read)
            const Light3D *light = Object::cast_to<Light3D>(p_node);
            const Variant light_data = extras.get(LIGHT_CONTROL_KEY, Variant());
            if (light != nullptr && light_data.get_type() == Variant::DICTIONARY) {
                const Dictionary control = light_data;
                const String source = control.get(CONTROL_SOURCE_KEY, String());
                if (!source.is_empty()) {
                    const SpotLight3D *spot = Object::cast_to<SpotLight3D>(p_node);
                    p_instance.light_controls.push_back(
                            LightControl{
                                    path, source, control.get(CONTROL_DEFAULT_KEY, 0.0),
                                    static_cast<float>(light->get_param(Light3D::PARAM_ENERGY)), names, transforms,
                                    light->get_color(),
                                    spot != nullptr
                                            ? static_cast<float>(Math::cos(
                                                      Math::deg_to_rad(spot->get_param(Light3D::PARAM_SPOT_ANGLE))))
                                            : -1.0F});
                }
            }
            const String light_id = extras.get(LIGHT_ID_KEY, String());
            if (!light_id.is_empty()) {
                p_instance.lens_controls.push_back(LensControl{path, light_id, 0.0});
            }
        }
        const MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(p_node);
        if (mesh_instance != nullptr && mesh_instance->get_mesh().is_valid()) {
            p_instance.meshes.push_back(
                    MeshPart{
                            mesh_instance->get_mesh(), names, transforms, mesh_instance->get_visibility_range_begin(),
                            mesh_instance->get_visibility_range_end(),
                            mesh_instance->get_visibility_range_begin_margin(),
                            mesh_instance->get_visibility_range_end_margin(),
                            static_cast<RenderingServer::VisibilityRangeFadeMode>(
                                    mesh_instance->get_visibility_range_fade_mode()),
                            mesh_instance->get_custom_aabb(),
                            mesh_instance->get_visibility_parent().is_empty()
                                    ? StringName()
                                    : StringName(String(names[names.size() - 1])
                                                         .path_join(String(mesh_instance->get_visibility_parent()))
                                                         .simplify_path())});
        }
        // a collision shape of a body of a role: on its role's layer (the model format's collisions)
        const CollisionShape3D *shape = Object::cast_to<CollisionShape3D>(p_node);
        const CollisionObject3D *body =
                shape != nullptr ? Object::cast_to<CollisionObject3D>(p_node->get_parent()) : nullptr;
        if (body != nullptr && shape->get_shape().is_valid() && body->has_meta(EXTRAS_META)) {
            const Dictionary extras = body->get_meta(EXTRAS_META);
            const String role = extras.get(SEMANTIC_ID_KEY, String());
            const char *setting = nullptr;
            if (role.ends_with(BODY_ROLE_SUFFIX)) {
                setting = ModelRenderingServer::BODY_LAYER_SETTING;
            } else if (role.ends_with(INTERIOR_ROLE_SUFFIX)) {
                setting = ModelRenderingServer::INTERIOR_LAYER_SETTING;
            }
            if (setting != nullptr) {
                const uint32_t layer = ProjectSettings::get_singleton()->get_setting(setting);
                p_instance.shapes.push_back(ShapePart{shape->get_shape(), names, transforms, layer});
            }
        }
        for (int index = 0; index < p_node->get_child_count(); ++index) {
            _read_node(p_node->get_child(index), names, transforms, p_instance);
        }
    }

    /// The nodes the channels turn posed of their values (instance_set_channels()), or of their
    /// defaults - a turn reduced to a full one - and the lights and the lenses they dim at their
    /// levels; whether a light's or a lens's level changed
    bool SceneModelRenderingBackend::_pose_channels(InstanceData &p_instance) {
        HashMap<StringName, float> light_levels;
        for (const LightControl &control: p_instance.light_controls) {
            light_levels[control.part] = static_cast<float>(
                    MAX(0.0, static_cast<double>(p_instance.channels.get(control.source, control.default_value))));
        }
        HashMap<StringName, float> lens_levels;
        for (const LensControl &control: p_instance.lens_controls) {
            lens_levels[control.part] = static_cast<float>(
                    MAX(0.0, static_cast<double>(p_instance.channels.get(control.source, control.default_value))));
        }
        bool lit_changed = light_levels.size() != p_instance.light_levels.size() ||
                           lens_levels.size() != p_instance.lens_levels.size();
        for (const KeyValue<StringName, float> &level: light_levels) {
            const float *was = p_instance.light_levels.getptr(level.key);
            lit_changed = lit_changed || was == nullptr || *was != level.value;
        }
        for (const KeyValue<StringName, float> &level: lens_levels) {
            const float *was = p_instance.lens_levels.getptr(level.key);
            lit_changed = lit_changed || was == nullptr || *was != level.value;
        }
        p_instance.light_levels = light_levels;
        p_instance.lens_levels = lens_levels;
        p_instance.channel_poses.clear();
        for (const RotationControl &control: p_instance.rotation_controls) {
            const double value = p_instance.channels.get(control.source, control.default_value);
            double angle = (value * control.scale) + control.offset;
            if (control.limited) {
                angle = Math::clamp(angle, control.limit_min, control.limit_max);
            }
            p_instance.channel_poses[control.part] =
                    Transform3D(Basis(control.axis, static_cast<real_t>(Math::fmod(angle, Math::TAU))));
        }
        return lit_changed;
    }

    /// A part's pose on top of its own transform: its channel's, the one writer of a part its data
    /// drives - a pose set for it too (instance_set_part_poses()) is left out - else the one set
    Transform3D SceneModelRenderingBackend::_get_part_pose(const InstanceData &p_instance, const StringName &p_part) {
        const Transform3D *channel_pose = p_instance.channel_poses.getptr(p_part);
        if (channel_pose != nullptr) {
            return *channel_pose;
        }
        const Variant pose = p_instance.part_poses.get(p_part, Variant());
        return pose.get_type() == Variant::TRANSFORM3D ? Transform3D(pose) : Transform3D();
    }

    /// A mesh or a shape in the model's space by the nodes down to it, each posed on top of its own
    /// transform
    Transform3D SceneModelRenderingBackend::_get_path_transform(
            const InstanceData &p_instance, const Vector<StringName> &p_names,
            const Vector<Transform3D> &p_transforms) {
        Transform3D transform;
        for (int64_t index = 0; index < p_names.size(); ++index) {
            transform = transform * p_transforms[index] * _get_part_pose(p_instance, p_names[index]);
        }
        return transform;
    }

    /// Whether no node down to a mesh or a shape is hidden (instance_set_part_visible())
    bool
    SceneModelRenderingBackend::_is_path_visible(const InstanceData &p_instance, const Vector<StringName> &p_names) {
        for (const StringName &name: p_names) {
            const bool *visible = p_instance.part_visible.getptr(name);
            if (visible != nullptr && !*visible) {
                return false;
            }
        }
        return true;
    }

    /// What a surface of the mesh is drawn with instead of its own material: the override of the
    /// deepest part above it that has one, else its material - its own, or the one of the replaceable
    /// textures of its slot - lit, while a self-illumination is set on a part above it or on the
    /// instance, that much when it is an emissive one; null for its own. A mesh of more materials
    /// keeps the others: only its emissive surfaces are lit.
    Ref<Material> SceneModelRenderingBackend::_get_surface_material(
            const InstanceData &p_instance, const MeshPart &p_mesh, const int p_surface) const {
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
        // a lens of a light dimmed by a channel: its own emission at the light's level, the one
        // writer of it (a self-illumination set for it too is left out)
        float lens_level = -1.0F;
        for (const StringName &name: p_mesh.path_names) {
            const float *level = p_instance.lens_levels.getptr(name);
            if (level != nullptr) {
                lens_level = *level;
            }
        }
        const Ref<Material> source = p_mesh.mesh->surface_get_material(p_surface);
        const Ref<Material> *replaced =
                source.is_valid() ? p_instance.replaced_materials.getptr(source->get_rid()) : nullptr;
        Ref<Material> material = replaced != nullptr ? *replaced : Ref<Material>();
        if (energy < 0.0F && lens_level < 0.0F) {
            return material;
        }
        const Ref<BaseMaterial3D> own = material.is_valid() ? material : source;
        if (own.is_null() || !own->get_feature(BaseMaterial3D::FEATURE_EMISSION)) {
            return material;
        }
        Ref<BaseMaterial3D> lit = own->duplicate();
        lit->set_emission_energy_multiplier(
                lens_level >= 0.0F ? own->get_emission_energy_multiplier() * lens_level : energy);
        return lit;
    }

    Node3D *SceneModelRenderingBackend::_get_root(const InstanceData &p_instance) const {
        return Object::cast_to<Node3D>(ObjectDB::get_instance(p_instance.root_id));
    }

    /// Where the instance's meshes, nodes and collision shapes stand, of their parts' poses
    void SceneModelRenderingBackend::_apply_poses(InstanceData &p_instance) const {
        if (!p_instance.built) {
            return;
        }
        PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
        for (int index = 0; index < p_instance.bodies.size(); ++index) {
            const ShapePart &shape = p_instance.shapes[index];
            ps->body_set_state(
                    p_instance.bodies[index], PhysicsServer3D::BODY_STATE_TRANSFORM,
                    p_instance.transform * _get_path_transform(p_instance, shape.path_names, shape.path_transforms));
        }
        if (p_instance.instancer == ModelRenderingServer::INSTANCER_OPTIMIZED) {
            RenderingServer *rs = RenderingServer::get_singleton();
            for (int index = 0; index < p_instance.meshes.size(); ++index) {
                const MeshPart &mesh = p_instance.meshes[index];
                rs->instance_set_transform(
                        p_instance.render_instances[index],
                        p_instance.transform * _get_path_transform(p_instance, mesh.path_names, mesh.path_transforms));
            }
            for (int index = 0; index < p_instance.glare_instances.size(); ++index) {
                const LightControl &control = p_instance.light_controls[index];
                rs->instance_set_transform(
                        p_instance.glare_instances[index],
                        p_instance.transform *
                                _get_path_transform(p_instance, control.path_names, control.path_transforms));
            }
            return;
        }
        Node3D *root = _get_root(p_instance);
        if (root == nullptr) {
            return;
        }
        for (const KeyValue<StringName, Transform3D> &part: p_instance.part_local_transforms) {
            Node3D *node = Object::cast_to<Node3D>(root->get_node_or_null(NodePath(String(part.key))));
            if (node != nullptr) {
                node->set_transform(part.value * _get_part_pose(p_instance, part.key));
            }
        }
    }

    /// What is set on the instance onto what it draws
    void SceneModelRenderingBackend::_apply(InstanceData &p_instance) const {
        if (!p_instance.built) {
            return;
        }
        _apply_poses(p_instance);
        // the collision shapes where the meshes of their parts are (_apply_poses()); none of a part hidden
        PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
        for (int index = 0; index < p_instance.bodies.size(); ++index) {
            ps->body_set_shape_disabled(
                    p_instance.bodies[index], 0,
                    !p_instance.visible || !_is_path_visible(p_instance, p_instance.shapes[index].path_names));
        }
        RenderingServer *rs = RenderingServer::get_singleton();
        if (p_instance.instancer == ModelRenderingServer::INSTANCER_OPTIMIZED) {
            p_instance.drawn_materials.clear();
            for (int index = 0; index < p_instance.meshes.size(); ++index) {
                const MeshPart &mesh = p_instance.meshes[index];
                const RID render_instance = p_instance.render_instances[index];
                rs->instance_set_scenario(render_instance, p_instance.scenario);
                rs->instance_set_visible(
                        render_instance, p_instance.visible && _is_path_visible(p_instance, mesh.path_names));
                rs->instance_set_layer_mask(render_instance, p_instance.layer_mask);
                for (int surface = 0; surface < mesh.mesh->get_surface_count(); ++surface) {
                    const Ref<Material> material = _get_surface_material(p_instance, mesh, surface);
                    if (material.is_valid()) {
                        p_instance.drawn_materials.push_back(material);
                    }
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
            // the glares of the lamps lit, at their levels
            for (int index = 0; index < p_instance.glare_instances.size(); ++index) {
                const LightControl &control = p_instance.light_controls[index];
                const float *level = p_instance.light_levels.getptr(control.part);
                const float lit = level != nullptr ? *level : 0.0F;
                const RID glare = p_instance.glare_instances[index];
                rs->instance_set_scenario(glare, p_instance.scenario);
                rs->instance_set_layer_mask(glare, p_instance.layer_mask);
                rs->instance_set_visible(
                        glare, p_instance.visible && lit > 0.0F && _is_path_visible(p_instance, control.path_names));
                rs->instance_geometry_set_shader_parameter(glare, GLARE_LEVEL_PARAMETER, lit);
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
            Node3D *node = Object::cast_to<Node3D>(root->get_node_or_null(NodePath(String(part.key))));
            if (node == nullptr) {
                continue;
            }
            const bool *visible = p_instance.part_visible.getptr(part.key);
            node->set_visible(visible == nullptr || *visible);
        }
        // the lights the channels dim, at their levels - off at 0; drawn optimized, a scene's
        // lights are not drawn at all
        for (const LightControl &control: p_instance.light_controls) {
            Light3D *light = Object::cast_to<Light3D>(root->get_node_or_null(NodePath(String(control.part))));
            const float *level = p_instance.light_levels.getptr(control.part);
            if (light == nullptr || level == nullptr) {
                continue;
            }
            light->set_param(Light3D::PARAM_ENERGY, control.energy * *level);
            light->set_visible(light->is_visible() && *level > 0.0F);
        }
        // the glares of the lamps lit, at their levels
        for (int index = 0; index < p_instance.glare_nodes.size(); ++index) {
            MeshInstance3D *glare =
                    Object::cast_to<MeshInstance3D>(ObjectDB::get_instance(p_instance.glare_nodes[index]));
            if (glare == nullptr) {
                continue;
            }
            const float *lit = p_instance.light_levels.getptr(p_instance.light_controls[index].part);
            const float level = lit != nullptr ? *lit : 0.0F;
            glare->set_visible(level > 0.0F);
            glare->set_instance_shader_parameter(GLARE_LEVEL_PARAMETER, level);
            glare->set_layer_mask(p_instance.layer_mask);
        }
        for (const MeshPart &mesh: p_instance.meshes) {
            MeshInstance3D *mesh_instance = Object::cast_to<MeshInstance3D>(
                    root->get_node_or_null(NodePath(String(mesh.path_names[mesh.path_names.size() - 1]))));
            if (mesh_instance == nullptr) {
                continue;
            }
            for (int surface = 0; surface < mesh.mesh->get_surface_count(); ++surface) {
                mesh_instance->set_surface_override_material(surface, _get_surface_material(p_instance, mesh, surface));
            }
            mesh_instance->set_material_overlay(p_instance.overlay);
            mesh_instance->set_layer_mask(p_instance.layer_mask);
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
        for (const RID &glare: p_instance.glare_instances) {
            rs->free_rid(glare);
        }
        p_instance.glare_instances.clear();
        // a glare built as a node goes with the scene's root, freed here too
        p_instance.glare_nodes.clear();
        p_instance.drawn_materials.clear();
        PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
        for (const RID &body: p_instance.bodies) {
            ps->free_rid(body);
        }
        p_instance.bodies.clear();
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
        const bool built = instance->built;
        _clear(*instance);
        instance->model.unref();
        instance->meshes.clear();
        if (built) {
            emit_signal(instance_cleared_signal, p_instance);
        }
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

    /// Optimized: a RenderingServer instance a mesh, drawn within its node's distances from the camera,
    /// in its node's box; as nodes: a scene made of the model under the attached node, but its
    /// collision objects. Either: a kinematic PhysicsServer3D body a collision shape, in the space set
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
                // drawn within its node's distances, as the nodes are: a level of detail of the model
                rs->instance_geometry_set_visibility_range(
                        render_instance, mesh.visibility_range_begin, mesh.visibility_range_end,
                        mesh.visibility_range_begin_margin, mesh.visibility_range_end_margin,
                        mesh.visibility_range_fade_mode);
                if (mesh.custom_aabb.has_surface()) {
                    rs->instance_set_custom_aabb(render_instance, mesh.custom_aabb);
                }
                instance->render_instances.push_back(render_instance);
            }
            // each mesh drawn while the one it depends on is hidden for being near, as its node is
            for (int index = 0; index < instance->meshes.size(); ++index) {
                const StringName &parent = instance->meshes[index].visibility_parent;
                for (int other = 0; !parent.is_empty() && other < instance->meshes.size(); ++other) {
                    const Vector<StringName> &names = instance->meshes[other].path_names;
                    if (names[names.size() - 1] == parent) {
                        rs->instance_set_visibility_parent(
                                instance->render_instances[index], instance->render_instances[other]);
                        break;
                    }
                }
            }
        } else {
            Node3D *attached = Object::cast_to<Node3D>(ObjectDB::get_instance(instance->attached_id));
            ERR_FAIL_NULL_MSG(attached, "A scene model built as nodes is built under its attached Node3D");
            Node3D *root = Object::cast_to<Node3D>(instance->model->instantiate());
            ERR_FAIL_NULL_MSG(root, "A scene model's root is a Node3D");
            // its collision shapes are the instance's bodies below, not the scene's own
            const TypedArray<Node> collision_objects = root->find_children("*", "CollisionObject3D", true, false);
            for (int64_t index = 0; index < collision_objects.size(); ++index) {
                Node *collision_object = Object::cast_to<Node>(collision_objects[index]);
                collision_object->get_parent()->remove_child(collision_object);
                memdelete(collision_object);
            }
            attached->add_child(root);
            instance->root_id = root->get_instance_id();
        }
        // a glare a lamp, drawn while it is lit (_apply()); none without its material
        if (glare_material.is_valid()) {
            RenderingServer *rs = RenderingServer::get_singleton();
            Node3D *root = _get_root(*instance);
            for (const LightControl &control: instance->light_controls) {
                if (instance->instancer == ModelRenderingServer::INSTANCER_OPTIMIZED) {
                    const RID glare = rs->instance_create();
                    rs->instance_set_base(glare, glare_mesh->get_rid());
                    rs->instance_geometry_set_material_override(glare, glare_material->get_rid());
                    rs->instance_geometry_set_cast_shadows_setting(glare, RenderingServer::SHADOW_CASTING_SETTING_OFF);
                    rs->instance_geometry_set_shader_parameter(glare, GLARE_COLOR_PARAMETER, control.color);
                    rs->instance_geometry_set_shader_parameter(glare, GLARE_COS_OUTER_PARAMETER, control.cos_outer);
                    instance->glare_instances.push_back(glare);
                    continue;
                }
                Node3D *light =
                        root != nullptr
                                ? Object::cast_to<Node3D>(root->get_node_or_null(NodePath(String(control.part))))
                                : nullptr;
                if (light == nullptr) {
                    // a place kept: the glares by the lights' own order
                    instance->glare_nodes.push_back(ObjectID());
                    continue;
                }
                MeshInstance3D *glare = memnew(MeshInstance3D);
                glare->set_mesh(glare_mesh);
                glare->set_material_override(glare_material);
                glare->set_cast_shadows_setting(GeometryInstance3D::SHADOW_CASTING_SETTING_OFF);
                glare->set_instance_shader_parameter(GLARE_COLOR_PARAMETER, control.color);
                glare->set_instance_shader_parameter(GLARE_COS_OUTER_PARAMETER, control.cos_outer);
                light->add_child(glare, false, Node::INTERNAL_MODE_BACK);
                instance->glare_nodes.push_back(ObjectID(glare->get_instance_id()));
            }
        }
        PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
        for (const ShapePart &shape: instance->shapes) {
            const RID body = ps->body_create();
            ps->body_set_mode(body, PhysicsServer3D::BODY_MODE_KINEMATIC);
            ps->body_add_shape(body, shape.shape->get_rid());
            ps->body_set_collision_layer(body, shape.layer);
            ps->body_set_collision_mask(body, 0);
            ps->body_set_space(body, instance->space);
            instance->bodies.push_back(body);
        }
        instance->built = true;
        _apply(*instance);
        emit_signal(instance_built_signal, p_instance);
    }

    void SceneModelRenderingBackend::instance_set_skins(
            const RID & /* p_instance */, const PackedStringArray & /* p_skins */) {
        // a scene is drawn whole, in its own materials
    }

    void
    SceneModelRenderingBackend::instance_set_replaceable_textures(const RID &p_instance, const Dictionary &p_textures) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->replaceable_textures = p_textures;
        _replace_materials(*instance);
        _apply(*instance);
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
            const AABB mesh_bounds =
                    _get_path_transform(*instance, mesh.path_names, mesh.path_transforms).xform(mesh.mesh->get_aabb());
            bounds = first ? mesh_bounds : bounds.merge(mesh_bounds);
            first = false;
        }
        return bounds;
    }

    bool SceneModelRenderingBackend::instance_has_part(const RID &p_instance, const String &p_part) const {
        const InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, false);
        return instance->part_model_transforms.has(_part_path(*instance, p_part));
    }

    Transform3D
    SceneModelRenderingBackend::instance_get_part_transform(const RID &p_instance, const String &p_part) const {
        const InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, Transform3D());
        const Transform3D *transform = instance->part_model_transforms.getptr(_part_path(*instance, p_part));
        return transform != nullptr ? *transform : Transform3D();
    }

    /// An empty transform takes a part back to its own
    void SceneModelRenderingBackend::instance_set_part_poses(const RID &p_instance, const Dictionary &p_poses) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        const Array parts = p_poses.keys();
        for (int index = 0; index < parts.size(); ++index) {
            const Transform3D pose = p_poses[parts[index]];
            const StringName part = _part_path(*instance, parts[index]);
            if (pose == Transform3D()) {
                instance->part_poses.erase(part);
            } else {
                instance->part_poses[part] = pose;
            }
        }
        _apply(*instance);
    }

    void SceneModelRenderingBackend::glare_set_material(const Ref<Material> &p_material) {
        glare_material = p_material;
    }

    void SceneModelRenderingBackend::instance_set_channels(const RID &p_instance, const Dictionary &p_channels) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->channels = p_channels.duplicate();
        // a light or a lens of another level redrawn whole - its materials - else only moved
        if (_pose_channels(*instance)) {
            _apply(*instance);
        } else {
            _apply_poses(*instance);
        }
    }

    void SceneModelRenderingBackend::instance_set_part_visible(
            const RID &p_instance, const String &p_part, const bool p_visible) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->part_visible[_part_path(*instance, p_part)] = p_visible;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_part_material_override(
            const RID &p_instance, const String &p_part, const Ref<Material> &p_material) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->part_materials[_part_path(*instance, p_part)] = p_material;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_part_emission_energy(
            const RID &p_instance, const String &p_part, const float p_energy) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->part_emission[_part_path(*instance, p_part)] = p_energy;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_emission_energy(const RID &p_instance, const float p_energy) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->emission_energy = p_energy;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_layer_mask(const RID &p_instance, const uint32_t p_mask) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->layer_mask = p_mask;
        _apply(*instance);
    }

    void SceneModelRenderingBackend::instance_set_space(const RID &p_instance, const RID &p_space) {
        InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->space = p_space;
        PhysicsServer3D *ps = PhysicsServer3D::get_singleton();
        for (const RID &body: instance->bodies) {
            ps->body_set_space(body, p_space);
        }
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

    /// Against the triangles of its visible meshes, posed
    Dictionary SceneModelRenderingBackend::instance_intersect_segment(
            const RID &p_instance, const Vector3 &p_from, const Vector3 &p_to) const {
        const InstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, Dictionary());
        Geometry3D *geometry = Geometry3D::get_singleton();
        Dictionary nearest;
        real_t nearest_distance = Math::INF;
        for (const MeshPart &mesh: instance->meshes) {
            if (!_is_path_visible(*instance, mesh.path_names)) {
                continue;
            }
            const Transform3D transform =
                    instance->transform * _get_path_transform(*instance, mesh.path_names, mesh.path_transforms);
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
