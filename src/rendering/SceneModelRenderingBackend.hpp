#pragma once
#include "rendering/ModelRenderingBackend.hpp"
#include "utils/RIDHash.hpp"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/shape3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {
    /* Godot's scenes as models (ModelRenderingServer's backend NAME): a model is a PackedScene by its
     * path (a .tscn, a .scn, an imported .glb), a part is a node of it by its path from the scene's
     * root ("Bogie_A/Axle_A1"), or by its name alone where no other node has it. An instance draws
     * the scene's meshes with RenderingServer instances (INSTANCER_OPTIMIZED) or as the scene's own
     * nodes under the attached node (INSTANCER_NODES); a streamed placement (instance_register()) is
     * built while the camera is in its range (SceneryStreamingServer). Its models have no lights, no
     * skins and no materials read ahead: has_light() is false for every one, a signal head attached
     * draws nothing and goes with the instance. A material whose metadata "extras" - a glTF
     * material's - names a slot of replaceable textures ("replacable_texture") takes the textures
     * of its slot (instance_set_replaceable_textures()). */
    class SceneModelRenderingBackend : public ModelRenderingBackend {
            GDCLASS(SceneModelRenderingBackend, ModelRenderingBackend)

        public:
            /// The name it is registered with in ModelRenderingServer (ModelData.rendering_backend)
            static constexpr const char *NAME = "scene";
            /// The layers an instance is drawn on until it is given others: Godot's, the first
            static constexpr uint32_t DEFAULT_LAYER_MASK = 1;

        private:
            /* One mesh of a model: the nodes from the scene's root down to it (their paths from the
             * root and their own transforms), so a pose of any of them moves it; the distances from
             * the camera it is drawn within and its box, as its node has them (a level of detail of
             * the model) */
            struct MeshPart {
                    Ref<Mesh> mesh;
                    Vector<StringName> path_names;
                    Vector<Transform3D> path_transforms;
                    float visibility_range_begin = 0.0F;
                    float visibility_range_end = 0.0F;
                    float visibility_range_begin_margin = 0.0F;
                    float visibility_range_end_margin = 0.0F;
                    RenderingServer::VisibilityRangeFadeMode visibility_range_fade_mode =
                            RenderingServer::VISIBILITY_RANGE_FADE_DISABLED;
                    /* The box its node gives it in place of its mesh's own, none of no size: where its
                     * distance from the camera is measured from, its levels' one point */
                    AABB custom_aabb;
            };

            /* One collision shape of a model: the nodes from the scene's root down to its shape's node,
             * as a mesh's (MeshPart), so that it is posed as the meshes of its part are; the physics
             * layer of its body's role (ModelRenderingServer.BODY_LAYER_SETTING, .INTERIOR_...) */
            struct ShapePart {
                    Ref<Shape3D> shape;
                    Vector<StringName> path_names;
                    Vector<Transform3D> path_transforms;
                    uint32_t layer = 0;
            };

            /* A node turned by a channel's value, as its glTF extras' rotation_control says: about
             * its own axis by the value times the scale plus the offset [rad], within the limits
             * where it has them; the default for a channel of no value */
            struct RotationControl {
                    StringName part;
                    StringName source;
                    Vector3 axis;
                    double scale = 1.0;
                    double offset = 0.0;
                    double default_value = 0.0;
                    bool limited = false;
                    double limit_min = 0.0;
                    double limit_max = 0.0;
            };

            struct InstanceData {
                    Ref<PackedScene> model;
                    Instancer instancer = ModelRenderingServer::INSTANCER_OPTIMIZED;
                    RID scenario;
                    Transform3D transform;
                    Transform3D node_transform;
                    bool visible = true;
                    ObjectID attached_id;
                    Dictionary part_poses;
                    /* instance_set_channels(), the nodes of its model they turn (_read_model()), and
                     * their poses of them (_pose_channels()), in place of part_poses' */
                    Dictionary channels;
                    Vector<RotationControl> rotation_controls;
                    HashMap<StringName, Transform3D> channel_poses;
                    HashMap<StringName, bool> part_visible;
                    HashMap<StringName, Ref<Material>> part_materials;
                    HashMap<StringName, float> part_emission;
                    float emission_energy = -1.0F;
                    uint32_t layer_mask = DEFAULT_LAYER_MASK;
                    Ref<Material> overlay;
                    /* instance_set_replaceable_textures(), and the model's materials of their slots
                     * with them, by the material each is of (_replace_materials()) */
                    Dictionary replaceable_textures;
                    RIDMap<Ref<Material>> replaced_materials;
                    /* The materials its optimized render instances are drawn with, kept as long as
                     * they are: a render instance holds only a material's RID (_apply()) */
                    Vector<Ref<Material>> drawn_materials;
                    /* Per-instance uniforms of its shaders (instance_set_shader_parameter()) */
                    Dictionary shader_parameters;
                    RID signal_head;
                    /// A streamed placement: its stream, and the model's path it loads when built
                    RID stream_rid;
                    String model_path;
                    bool built = false;
                    /// What the model is: its meshes and its parts' own transforms, in their parent's
                    /// space and in the model's (_read_model())
                    Vector<MeshPart> meshes;
                    HashMap<StringName, Transform3D> part_local_transforms;
                    HashMap<StringName, Transform3D> part_model_transforms;
                    /// A part named by its name alone: the path of the one node of that name; an
                    /// empty path for a name more nodes have
                    HashMap<StringName, StringName> part_paths;
                    /// What it draws, built optimized: one RenderingServer instance a mesh; built as
                    /// nodes: the scene's root under the attached node
                    Vector<RID> render_instances;
                    /// Its model's collision shapes (_read_model()), a kinematic PhysicsServer3D body
                    /// each once built, in the space set (instance_set_space())
                    Vector<ShapePart> shapes;
                    Vector<RID> bodies;
                    RID space;
                    ObjectID root_id;
            };

            RIDMap<InstanceData> instances;
            int stream_owner = -1;

            static void _read_model(InstanceData &p_instance);
            static void _replace_materials(InstanceData &p_instance);
            static StringName _part_path(const InstanceData &p_instance, const StringName &p_part);
            static void _read_node(
                    const Node *p_node, const Vector<StringName> &p_names, const Vector<Transform3D> &p_transforms,
                    InstanceData &p_instance);
            static void _pose_channels(InstanceData &p_instance);
            static Transform3D _get_part_pose(const InstanceData &p_instance, const StringName &p_part);
            static Transform3D _get_path_transform(
                    const InstanceData &p_instance, const Vector<StringName> &p_names,
                    const Vector<Transform3D> &p_transforms);
            Node3D *_get_root(const InstanceData &p_instance) const;
            static bool _is_path_visible(const InstanceData &p_instance, const Vector<StringName> &p_names);
            Ref<Material>
            _get_surface_material(const InstanceData &p_instance, const MeshPart &p_mesh, int p_surface) const;
            void _apply(InstanceData &p_instance) const;
            void _apply_poses(InstanceData &p_instance) const;
            void _clear(InstanceData &p_instance);
            void _stream_build(const RID &p_instance, const Variant &p_preloaded);
            void _stream_clear(const RID &p_instance);

        protected:
            static void _bind_methods() {}

        public:
            SceneModelRenderingBackend();
            ~SceneModelRenderingBackend() override;

            Ref<Resource> model_load(const String &p_name) override;
            Variant material_preload(
                    const Ref<Resource> &p_model, const String &p_data_path, const PackedStringArray &p_skins,
                    int p_max_texture_size) override;
            RID
            instance_create(const Ref<Resource> &p_model, Instancer p_instancer, InstanceKind p_instance_kind) override;
            RID instance_register(
                    const String &p_name, const PackedStringArray &p_skins, const Transform3D &p_transform,
                    float p_range_begin, float p_range_end, const RID &p_scenario) override;
            void instance_free(const RID &p_instance) override;
            void instance_build(const RID &p_instance) override;
            void instance_set_options(
                    const RID &p_instance, const String &p_data_path, const PackedStringArray &p_skins,
                    const Array &p_exclude_node_names, bool p_force_alpha,
                    const TypedArray<NodePath> &p_force_alpha_part_paths, int p_max_texture_size) override;
            void instance_set_scenario(const RID &p_instance, const RID &p_scenario) override;
            void instance_set_transform(const RID &p_instance, const Transform3D &p_transform) override;
            void instance_set_visible(const RID &p_instance, bool p_visible) override;
            void instance_attach_object_instance_id(const RID &p_instance, uint64_t p_id) override;
            void instance_set_node_transform(const RID &p_instance, const Transform3D &p_transform) override;
            void instance_set_instancer(const RID &p_instance, Instancer p_instancer) override;
            AABB instance_get_aabb(const RID &p_instance) const override;
            Transform3D instance_get_transform(const RID &p_instance) const override;
            bool instance_has_part(const RID &p_instance, const String &p_part) const override;
            Transform3D instance_get_part_transform(const RID &p_instance, const String &p_part) const override;
            void instance_set_part_poses(const RID &p_instance, const Dictionary &p_poses) override;
            void instance_set_channels(const RID &p_instance, const Dictionary &p_channels) override;
            void instance_set_part_visible(const RID &p_instance, const String &p_part, bool p_visible) override;
            void instance_set_part_material_override(
                    const RID &p_instance, const String &p_part, const Ref<Material> &p_material) override;
            void
            instance_set_part_emission_energy(const RID &p_instance, const String &p_part, float p_energy) override;
            void instance_set_emission_energy(const RID &p_instance, float p_energy) override;
            void instance_set_layer_mask(const RID &p_instance, uint32_t p_mask) override;
            void instance_set_space(const RID &p_instance, const RID &p_space) override;
            bool instance_has_light(const RID &p_instance, const String &p_light) const override;
            void instance_set_lights_state(const RID &p_instance, const Dictionary &p_lights_state) override;
            void instance_set_lights_dimmed(
                    const RID &p_instance, const Dictionary &p_lights_dimmed, float p_multiplier) override;
            void instance_set_lights_colors(const RID &p_instance, const Dictionary &p_lights_colors) override;
            void instance_attach_signal_head(const RID &p_instance, const RID &p_signal_head) override;
            RID signal_head_get_instance(const RID &p_signal_head) const override;
            void instance_set_material_overlay(const RID &p_instance, const Ref<Material> &p_material) override;
            void instance_set_shader_parameter(
                    const RID &p_instance, const StringName &p_name, const Variant &p_value) override;
            void instance_set_skins(const RID &p_instance, const PackedStringArray &p_skins) override;
            void instance_set_replaceable_textures(const RID &p_instance, const Dictionary &p_textures) override;
            Dictionary instance_intersect_segment(
                    const RID &p_instance, const Vector3 &p_from, const Vector3 &p_to) const override;
    };
} // namespace godot
