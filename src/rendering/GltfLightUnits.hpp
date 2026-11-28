#pragma once

#include <godot_cpp/classes/gltf_document_extension.hpp>
#include <godot_cpp/classes/gltf_state.hpp>

namespace godot {
    /* A glTF model's lights (KHR_lights_punctual) in Godot's units, as the model format writes them in
     * the physical ones: a point's or a spot's intensity [cd] over LIGHT_ENERGY_CANDELA_SETTING its
     * energy, a light of no range LIGHT_DEFAULT_RANGE_SETTING far; a directional light [lx] as it
     * is. Registered with GLTFDocument as the library starts, it works on every glTF read - imported
     * into a project and its packs, or read at run time. */
    class GltfLightUnits : public GLTFDocumentExtension {
            GDCLASS(GltfLightUnits, GLTFDocumentExtension)

        public:
            /// The candelas of a light of energy 1 - a headlight of 3000 cd as bright as a vehicle's lamps
            /// of other models - and how far a light of no range reaches [m]
            static constexpr const char *LIGHT_ENERGY_CANDELA_SETTING = "reloaded/models/light_energy_candela";
            static constexpr double DEFAULT_LIGHT_ENERGY_CANDELA = 3000.0;
            static constexpr const char *LIGHT_DEFAULT_RANGE_SETTING = "reloaded/models/light_default_range";
            static constexpr double DEFAULT_LIGHT_RANGE = 60.0;

        protected:
            static void _bind_methods() {}

        public:
            static void project_settings_register();

            Error _import_post_parse(const Ref<GLTFState> &p_state) override;
    };
} // namespace godot
