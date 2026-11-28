#include "rendering/GltfLightUnits.hpp"
#include "utils/ReloadedProjectSetting.hpp"

#include <godot_cpp/classes/gltf_light.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {
    namespace {
        /* The model format's lights of a luminous intensity (KHR_lights_punctual's types) */
        const char *const POINT_LIGHT = "point";
        const char *const SPOT_LIGHT = "spot";
    } // namespace

    void GltfLightUnits::project_settings_register() {
        ReloadedProjectSetting::add(LIGHT_CANDELA_TO_ENERGY_SETTING, DEFAULT_LIGHT_CANDELA_TO_ENERGY, Variant::FLOAT);
        ReloadedProjectSetting::add(LIGHT_DEFAULT_RANGE_SETTING, DEFAULT_LIGHT_RANGE, Variant::FLOAT);
    }

    /* The lights read, before the scene is made of them */
    Error GltfLightUnits::_import_post_parse(const Ref<GLTFState> &p_state) {
        const ProjectSettings *settings = ProjectSettings::get_singleton();
        const double candela_to_energy =
                settings->get_setting(LIGHT_CANDELA_TO_ENERGY_SETTING, DEFAULT_LIGHT_CANDELA_TO_ENERGY);
        const double default_range = settings->get_setting(LIGHT_DEFAULT_RANGE_SETTING, DEFAULT_LIGHT_RANGE);
        const TypedArray<GLTFLight> lights = p_state->get_lights();
        for (int index = 0; index < lights.size(); ++index) {
            const Ref<GLTFLight> light = lights[index];
            const String type = light->get_light_type();
            if (type != POINT_LIGHT && type != SPOT_LIGHT) {
                continue;
            }
            light->set_intensity(static_cast<float>(light->get_intensity() * candela_to_energy));
            if (!Math::is_finite(light->get_range())) {
                light->set_range(static_cast<float>(default_range));
            }
        }
        return OK;
    }
} // namespace godot
