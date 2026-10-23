#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    /// A model a SceneryStreamingProvider places (SceneryStreamingProvider.CONTENT_MODELS): drawn
    /// while its chunk is supplied, and named by nothing - no event or signal reaches it
    class SceneryModelPlacement : public Resource {
            GDCLASS(SceneryModelPlacement, Resource)

        private:
            StringName rendering_backend;
            String model;
            PackedStringArray skins;
            Transform3D transform;
            float range_min = 0.0F;
            float range_max = 0.0F;
            Dictionary lights_state;
            Dictionary lights_colors;

        protected:
            static void _bind_methods();

        public:
            /// The model renderer's backend the model is the one of (ModelData.rendering_backend); empty
            /// for the default one
            void set_rendering_backend(const StringName &p_value);
            StringName get_rendering_backend() const;
            /// The model's name, which its backend finds it by (ModelData.model)
            void set_model(const String &p_value);
            String get_model() const;
            /// The model's replaceable materials, by number
            void set_skins(const PackedStringArray &p_value);
            PackedStringArray get_skins() const;
            /// Where the model stands in the world
            void set_transform(const Transform3D &p_value);
            Transform3D get_transform() const;
            /// The distances it is drawn between [m]; a range_max of 0 or less is no limit
            void set_range_min(float p_value);
            float get_range_min() const;
            void set_range_max(float p_value);
            float get_range_max() const;
            /// Its lights on or off and their colours, by name (ModelData.lights_state, lights_colors)
            void set_lights_state(const Dictionary &p_value);
            Dictionary get_lights_state() const;
            void set_lights_colors(const Dictionary &p_value);
            Dictionary get_lights_colors() const;
    };
} // namespace godot
