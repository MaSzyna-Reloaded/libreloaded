#pragma once
#include <godot_cpp/classes/visual_instance3d.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* A model in the scene, drawn by the ModelRenderingServer instance it stands for. A plugin's
     * node derives from it, makes the instance and hands it over (set_model_instance()), announcing
     * each new one (model_instance_created); the lights set here are that instance's. */
    class ReloadedModel3D : public VisualInstance3D {
            GDCLASS(ReloadedModel3D, VisualInstance3D)

        public:
            static const char *model_instance_created_signal;

        private:
            RID model_instance;
            Dictionary lights_state;
            Dictionary lights_dimmed;
            float lights_dimmed_multiplier = 1.0;

        protected:
            static void _bind_methods();

            /// The deriving node's say on a lights state set (the model's own lights in it, say)
            GDVIRTUAL1RC(Dictionary, _normalize_lights_state, Dictionary)

        public:
            /// The ModelRenderingServer instance the node stands for, none until there is one
            RID get_model_instance() const;
            /// From the deriving node, whenever it has another instance; it sets the lights on it and
            /// announces it itself
            void set_model_instance(const RID &p_instance);

            /// The model's lights on or off, by name: {light: bool}
            void set_lights_state(const Dictionary &p_lights_state);
            Dictionary get_lights_state() const;
            /// The model's lights dimmed, by name: {light: bool}, at lights_dimmed_multiplier
            void set_lights_dimmed(const Dictionary &p_lights_dimmed);
            Dictionary get_lights_dimmed() const;
            void set_lights_dimmed_multiplier(float p_multiplier);
            float get_lights_dimmed_multiplier() const;

            /// The model draws the SignallingServer signal head's lights, and frees it with them
            void attach_signal_head(const RID &p_signal_head);
    };
} // namespace godot
