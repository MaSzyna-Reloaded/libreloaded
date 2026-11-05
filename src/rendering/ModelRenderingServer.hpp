#pragma once
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "scenery/SceneryModelPlacement.hpp"
#include "utils/RIDHash.hpp"

namespace godot {
    class ModelRenderingBackend;

    /* What draws the models of the world: a model loaded from a file, its instances placed, posed,
     * lit and picked by RID. It draws nothing itself - each model is the one of a backend, by the
     * backend's name (backend_register(); ModelData.rendering_backend), and an instance's calls are
     * forwarded to the backend that made it; a model of no backend named is the one of the project's
     * default (BACKEND_SETTING). The core draws vehicles, road traffic and what the player picks
     * through it. A part is a named piece of a model (a bogie, a wheel); the lights are the model's
     * own, by name. */
    class ModelRenderingServer : public Object {
            GDCLASS(ModelRenderingServer, Object)

        public:
            /// How an instance is drawn
            enum Instancer {
                /// RenderingServer instances, no nodes
                INSTANCER_OPTIMIZED,
                /// A node tree under the attached node
                INSTANCER_NODES,
                /// A node tree the editor may change
                INSTANCER_EDITABLE_NODES,
            };

            /// What an instance is: a scenery's, standing still, or one that moves
            enum InstanceKind {
                INSTANCE_KIND_STATIC,
                INSTANCE_KIND_DYNAMIC,
            };

            /// The backend a model of no backend named is the one of, by its name
            static constexpr const char *BACKEND_SETTING = "reloaded/models/backend";

            static const char *instance_built_signal;
            static const char *instance_freed_signal;

        private:
            static ModelRenderingServer *singleton;

            /// The registered backends, by name; each one unregisters before it goes
            HashMap<StringName, ModelRenderingBackend *> backends;
            /// The backend BACKEND_SETTING names, once registered
            StringName default_backend;
            /// The backend each instance is drawn by, from its instance_created to its instance_freed
            RIDMap<ModelRenderingBackend *> instance_backends;

            ModelRenderingBackend *_get_backend(const StringName &p_provider) const;
            ModelRenderingBackend *_get_instance_backend(const RID &p_instance) const;
            void _on_instance_created(const RID &p_instance, const StringName &p_backend);
            void _on_instance_built(const RID &p_instance);
            void _on_instance_freed(const RID &p_instance);
            /// A model a scenery's streaming provider places (SceneryStreamingProvider.CONTENT_MODELS),
            /// registered with the backend it names
            RID _adopt_model(const Ref<SceneryModelPlacement> &p_placement, const RID &p_scenario);

        protected:
            static void _bind_methods();

        public:
            static ModelRenderingServer *get_instance();

            ModelRenderingServer();
            ~ModelRenderingServer() override;

            /// A ModelRenderingBackend, by its ObjectID, draws the models of p_name from now on
            void backend_register(const StringName &p_name, uint64_t p_backend_id);
            /// The backend's instances are forgotten with it
            void backend_unregister(const StringName &p_name);
            /// The backend registered as p_name, by its ObjectID; 0 for none
            uint64_t backend_get(const StringName &p_name) const;
            PackedStringArray backend_get_list() const;

            /// The model of the name of p_provider's (the default backend for none), shared by every
            /// instance of it while it lives
            Ref<Resource> model_load(const StringName &p_provider, const String &p_name);
            /// The files of p_provider's model's materials read ahead, on the calling thread; what it
            /// returns holds them until the build has made the materials
            Variant material_preload(
                    const StringName &p_provider, const Ref<Resource> &p_model, const String &p_data_path,
                    const PackedStringArray &p_skins, int p_max_texture_size);

            /// An empty instance of p_provider's model
            RID instance_create(
                    const StringName &p_provider, const Ref<Resource> &p_model, Instancer p_instancer,
                    InstanceKind p_instance_kind);
            /// A placement of p_provider's model of the name, built only while the camera is within
            /// p_range_end of it (SceneryStreamingServer) and cleared when it leaves
            RID instance_register(
                    const StringName &p_provider, const String &p_name, const PackedStringArray &p_skins,
                    const Transform3D &p_transform, float p_range_begin, float p_range_end, const RID &p_scenario);
            void instance_free(const RID &p_instance);
            /// Builds (or builds again) what the instance draws; instance_built when done
            void instance_build(const RID &p_instance);
            void instance_set_options(
                    const RID &p_instance, const String &p_data_path, const PackedStringArray &p_skins,
                    const Array &p_exclude_node_names, bool p_force_alpha,
                    const TypedArray<NodePath> &p_force_alpha_part_paths, int p_max_texture_size);
            void instance_set_scenario(const RID &p_instance, const RID &p_scenario);
            void instance_set_transform(const RID &p_instance, const Transform3D &p_transform);
            void instance_set_visible(const RID &p_instance, bool p_visible);
            /// The node a node tree is built under (INSTANCER_NODES), by its ObjectID
            void instance_attach_object_instance_id(const RID &p_instance, uint64_t p_id);
            /// Where the model sits in the attached node
            void instance_set_node_transform(const RID &p_instance, const Transform3D &p_transform);
            /// Builds the instance again with another instancer, keeping the RID and everything set on it
            void instance_set_instancer(const RID &p_instance, Instancer p_instancer);
            /// The bounds of every mesh of the model, in the model's space
            AABB instance_get_aabb(const RID &p_instance) const;
            /// Where the instance stands in its scenario
            Transform3D instance_get_transform(const RID &p_instance) const;

            bool instance_has_part(const RID &p_instance, const String &p_part) const;
            /// The part's transform in the model, without any pose
            Transform3D instance_get_part_transform(const RID &p_instance, const String &p_part) const;
            /// Poses parts, by name, on top of their own transform: {part: Transform3D}, kept across
            /// rebuilds; an empty transform takes a part back to rest
            void instance_set_part_poses(const RID &p_instance, const Dictionary &p_poses);
            void instance_set_part_visible(const RID &p_instance, const String &p_part, bool p_visible);
            /// Draws the part with p_material instead of its own
            void instance_set_part_material_override(
                    const RID &p_instance, const String &p_part, const Ref<Material> &p_material);
            /// The self-illumination of the part and of everything under it; below 0 the instance's own
            void instance_set_part_emission_energy(const RID &p_instance, const String &p_part, float p_energy);
            /// The self-illumination of every emissive material of the instance
            void instance_set_emission_energy(const RID &p_instance, float p_energy);

            /// The model has the light, by name
            bool instance_has_light(const RID &p_instance, const String &p_light) const;
            /// The model's lights on or off, by name: {light: bool}
            void instance_set_lights_state(const RID &p_instance, const Dictionary &p_lights_state);
            /// The model's lights dimmed, by name: {light: bool}, by p_multiplier
            void
            instance_set_lights_dimmed(const RID &p_instance, const Dictionary &p_lights_dimmed, float p_multiplier);
            /// The model's lights' colours, by name: {light: Color}, over the colour each light carries
            void instance_set_lights_colors(const RID &p_instance, const Dictionary &p_lights_colors);
            /// The instance draws the SignallingServer signal head's lights, and frees it with them
            void instance_attach_signal_head(const RID &p_instance, const RID &p_signal_head);
            /// The instance the signal head is attached to (instance_attach_signal_head()); RID() for none
            RID signal_head_get_instance(const RID &p_signal_head) const;

            /// Drawn over the instance's own materials (an outline), none for nothing
            void instance_set_material_overlay(const RID &p_instance, const Ref<Material> &p_material);
            /// What is drawn over the instance's own materials, none for nothing
            Ref<Material> instance_get_material_overlay(const RID &p_instance) const;
            /// A per-instance uniform of the instance's shaders ("instance uniform"), on all its meshes -
            /// kept across a rebuild; null takes it off
            void instance_set_shader_parameter(const RID &p_instance, const StringName &p_name, const Variant &p_value);
            /// The nearest hit of the segment on the instance's meshes: {position, distance}, empty for none
            Dictionary
            instance_intersect_segment(const RID &p_instance, const Vector3 &p_from, const Vector3 &p_to) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(ModelRenderingServer::Instancer)
VARIANT_ENUM_CAST(ModelRenderingServer::InstanceKind)
