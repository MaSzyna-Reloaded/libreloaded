#pragma once
#include "rendering/ModelRenderingBackend.hpp"
#include "utils/RIDHash.hpp"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {
    /* Godot's scenes as models (ModelRenderingServer's backend NAME): a model is a PackedScene by its
     * path (a .tscn, a .scn, an imported .glb), a part is a node of it by its name. An instance draws
     * the scene's meshes with RenderingServer instances (INSTANCER_OPTIMIZED) or as the scene's own
     * nodes under the attached node (INSTANCER_NODES); a streamed placement (instance_register()) is
     * built while the camera is in its range (SceneryStreamingServer). Its models have no lights, no
     * skins and no materials read ahead: has_light() is false for every one, a signal head attached
     * draws nothing and goes with the instance. */
    class SceneModelRenderingBackend : public ModelRenderingBackend {
            GDCLASS(SceneModelRenderingBackend, ModelRenderingBackend)

        public:
            /// The name it is registered with in ModelRenderingServer (ModelData.rendering_backend)
            static constexpr const char *NAME = "scene";

        private:
            /* One mesh of a model: the nodes from the scene's root down to it (their names and own
             * transforms), so a pose of any of them moves it */
            struct MeshPart {
                    Ref<Mesh> mesh;
                    Vector<StringName> path_names;
                    Vector<Transform3D> path_transforms;
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
                    HashMap<StringName, bool> part_visible;
                    HashMap<StringName, Ref<Material>> part_materials;
                    HashMap<StringName, float> part_emission;
                    float emission_energy = -1.0F;
                    Ref<Material> overlay;
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
                    /// What it draws, built optimized: one RenderingServer instance a mesh; built as
                    /// nodes: the scene's root under the attached node
                    Vector<RID> render_instances;
                    ObjectID root_id;
            };

            RIDMap<InstanceData> instances;
            int stream_owner = -1;

            static void _read_model(InstanceData &p_instance);
            static void _read_node(
                    const Node *p_node, const Vector<StringName> &p_names, const Vector<Transform3D> &p_transforms,
                    InstanceData &p_instance);
            static Transform3D _get_mesh_transform(const InstanceData &p_instance, const MeshPart &p_mesh);
            Node3D *_get_root(const InstanceData &p_instance) const;
            bool _is_mesh_visible(const InstanceData &p_instance, const MeshPart &p_mesh) const;
            Ref<Material> _get_mesh_material(const InstanceData &p_instance, const MeshPart &p_mesh) const;
            void _apply(InstanceData &p_instance) const;
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
            void instance_set_part_visible(const RID &p_instance, const String &p_part, bool p_visible) override;
            void instance_set_part_material_override(
                    const RID &p_instance, const String &p_part, const Ref<Material> &p_material) override;
            void
            instance_set_part_emission_energy(const RID &p_instance, const String &p_part, float p_energy) override;
            void instance_set_emission_energy(const RID &p_instance, float p_energy) override;
            bool instance_has_light(const RID &p_instance, const String &p_light) const override;
            void instance_set_lights_state(const RID &p_instance, const Dictionary &p_lights_state) override;
            void instance_set_lights_dimmed(
                    const RID &p_instance, const Dictionary &p_lights_dimmed, float p_multiplier) override;
            void instance_set_lights_colors(const RID &p_instance, const Dictionary &p_lights_colors) override;
            void instance_attach_signal_head(const RID &p_instance, const RID &p_signal_head) override;
            RID signal_head_get_instance(const RID &p_signal_head) const override;
            void instance_set_material_overlay(const RID &p_instance, const Ref<Material> &p_material) override;
            Ref<Material> instance_get_material_overlay(const RID &p_instance) const override;
            void instance_set_shader_parameter(
                    const RID &p_instance, const StringName &p_name, const Variant &p_value) override;
            Dictionary instance_intersect_segment(
                    const RID &p_instance, const Vector3 &p_from, const Vector3 &p_to) const override;
    };
} // namespace godot
