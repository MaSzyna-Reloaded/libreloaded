#pragma once
#include "rendering/ModelRenderingServer.hpp"
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* What ModelRenderingServer forwards the drawing of a model to: the renderer of one kind of models,
     * registered by its name (ModelRenderingServer::backend_register()). It loads a model of its own by
     * its file, and places, poses, lights and picks its instances by RID. Every instance it makes is
     * announced by instance_created - also one it makes for itself, outside the server - so that the
     * server forwards the instance's calls to it. */
    class ModelRenderingBackend : public Object {
            GDCLASS(ModelRenderingBackend, Object)

        public:
            using Instancer = ModelRenderingServer::Instancer;
            using InstanceKind = ModelRenderingServer::InstanceKind;

            static const char *instance_created_signal;
            static const char *instance_built_signal;
            static const char *instance_freed_signal;

        protected:
            static void _bind_methods();

        public:
            /// The model of the name, shared by every instance of it while it lives; the backend finds
            /// its file
            virtual Ref<Resource> model_load(const String &p_name) = 0;
            /// The files of the model's materials read ahead, on the calling thread; what it returns
            /// holds them until the build has made the materials
            virtual Variant material_preload(
                    const Ref<Resource> &p_model, const String &p_data_path, const PackedStringArray &p_skins,
                    int p_max_texture_size) = 0;

            /// An empty instance of the model; instance_created as it is made
            virtual RID
            instance_create(const Ref<Resource> &p_model, Instancer p_instancer, InstanceKind p_instance_kind) = 0;
            /// A placement of the model of the name, built only while the camera is within p_range_end of
            /// it (SceneryStreamingServer) and cleared when it leaves; instance_created as it is made
            virtual RID instance_register(
                    const String &p_name, const PackedStringArray &p_skins, const Transform3D &p_transform,
                    float p_range_begin, float p_range_end, const RID &p_scenario) = 0;
            /// instance_freed as it goes
            virtual void instance_free(const RID &p_instance) = 0;
            /// Builds (or builds again) what the instance draws; instance_built when done
            virtual void instance_build(const RID &p_instance) = 0;
            virtual void instance_set_options(
                    const RID &p_instance, const String &p_data_path, const PackedStringArray &p_skins,
                    const Array &p_exclude_node_names, bool p_force_alpha,
                    const TypedArray<NodePath> &p_force_alpha_part_paths, int p_max_texture_size) = 0;
            virtual void instance_set_scenario(const RID &p_instance, const RID &p_scenario) = 0;
            virtual void instance_set_transform(const RID &p_instance, const Transform3D &p_transform) = 0;
            virtual void instance_set_visible(const RID &p_instance, bool p_visible) = 0;
            /// The node a node tree is built under (INSTANCER_NODES), by its ObjectID
            virtual void instance_attach_object_instance_id(const RID &p_instance, uint64_t p_id) = 0;
            /// Where the model sits in the attached node
            virtual void instance_set_node_transform(const RID &p_instance, const Transform3D &p_transform) = 0;
            /// Builds the instance again with another instancer, keeping the RID and everything set on it
            virtual void instance_set_instancer(const RID &p_instance, Instancer p_instancer) = 0;
            /// The bounds of every mesh of the model, in the model's space
            virtual AABB instance_get_aabb(const RID &p_instance) const = 0;
            /// Where the instance stands in its scenario
            virtual Transform3D instance_get_transform(const RID &p_instance) const = 0;

            virtual bool instance_has_part(const RID &p_instance, const String &p_part) const = 0;
            /// The part's transform in the model, without any pose
            virtual Transform3D instance_get_part_transform(const RID &p_instance, const String &p_part) const = 0;
            /// Poses parts, by name, on top of their own transform: {part: Transform3D}, kept across
            /// rebuilds; an empty transform takes a part back to rest
            virtual void instance_set_part_poses(const RID &p_instance, const Dictionary &p_poses) = 0;
            virtual void instance_set_part_visible(const RID &p_instance, const String &p_part, bool p_visible) = 0;
            /// Draws the part with p_material instead of its own
            virtual void instance_set_part_material_override(
                    const RID &p_instance, const String &p_part, const Ref<Material> &p_material) = 0;
            /// The self-illumination of the part and of everything under it; below 0 the instance's own
            virtual void
            instance_set_part_emission_energy(const RID &p_instance, const String &p_part, float p_energy) = 0;
            /// The self-illumination of every emissive material of the instance
            virtual void instance_set_emission_energy(const RID &p_instance, float p_energy) = 0;

            /// The model has the light, by name
            virtual bool instance_has_light(const RID &p_instance, const String &p_light) const = 0;
            /// The model's lights on or off, by name: {light: bool}
            virtual void instance_set_lights_state(const RID &p_instance, const Dictionary &p_lights_state) = 0;
            /// The model's lights dimmed, by name: {light: bool}, by p_multiplier
            virtual void instance_set_lights_dimmed(
                    const RID &p_instance, const Dictionary &p_lights_dimmed, float p_multiplier) = 0;
            /// The model's lights' colours, by name: {light: Color}, over the colour each light carries
            virtual void instance_set_lights_colors(const RID &p_instance, const Dictionary &p_lights_colors) = 0;
            /// The instance draws the SignallingServer signal head's lights, and frees it with them
            virtual void instance_attach_signal_head(const RID &p_instance, const RID &p_signal_head) = 0;
            /// The instance of this backend's the signal head is attached to; RID() for none
            virtual RID signal_head_get_instance(const RID &p_signal_head) const = 0;

            /// Drawn over the instance's own materials (an outline), none for nothing
            virtual void instance_set_material_overlay(const RID &p_instance, const Ref<Material> &p_material) = 0;
            /// The nearest hit of the segment on the instance's meshes: {position, distance}, empty for none
            virtual Dictionary
            instance_intersect_segment(const RID &p_instance, const Vector3 &p_from, const Vector3 &p_to) const = 0;
    };
} // namespace godot
