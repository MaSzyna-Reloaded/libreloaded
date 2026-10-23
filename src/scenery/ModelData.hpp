#pragma once
#include "macros.hpp"
#include "signalling/SignalHeadKind.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* One model in the game - a building, any 3D object: which model and whose (the backend that
     * finds it by its name), with which skins, where it stands and from how far it is seen; drawn
     * through ModelRenderingServer. A scenery holds them (CompiledScenery.models). Its name, the
     * placement's, empty for none - a signal head is registered under it - is the resource's name. */
    class ModelData : public Resource {
            GDCLASS(ModelData, Resource)

        private:
            /* The model's name; the backend of rendering_backend finds its file */
            String model;
            /* The name of the model renderer's backend the model is the one of
             * (ModelRenderingServer); empty for the default one */
            StringName rendering_backend;
            PackedStringArray skins;
            Vector3 position;
            /* Euler angles (radians), same order as Node3D.rotation */
            Vector3 rotation;
            double range_min = 0.0;
            /* Visible up to this distance, 0 - no limit */
            double range_max = 0.0;
            /* Set when the model is a signal head */
            Ref<SignalHeadKind> signal_head_kind;
            /* Its lights on or off, by name: {light: bool} (ModelRenderingServer.instance_set_lights_state());
             * a light not named is as the model has it */
            Dictionary lights_state;
            /* Its lights' colours, by name: {light: Color} (ModelRenderingServer.instance_set_lights_colors()) */
            Dictionary lights_colors;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(String, model);
            MAKE_GS(StringName, rendering_backend);
            MAKE_GS(PackedStringArray, skins);
            MAKE_GS(Vector3, position);
            MAKE_GS(Vector3, rotation);
            MAKE_GS_NR(double, range_min);
            MAKE_GS_NR(double, range_max);
            MAKE_GS_NR(Ref<SignalHeadKind>, signal_head_kind);
            MAKE_GS(Dictionary, lights_state);
            MAKE_GS(Dictionary, lights_colors);

            /// A copy of the model placed p_offset further
            Ref<ModelData> translated(const Vector3 &p_offset) const;
    };
} // namespace godot
