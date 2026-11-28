extends GutTest

## A glTF model's lights (KHR_lights_punctual) in Godot's units however it is read (GltfLightUnits): a
## spot's intensity [cd] times reloaded/models/light_candela_to_energy its energy, a light of no range
## reloaded/models/light_default_range far, one of its own as it is

const CANDELA:float = 1500.0
const OWN_RANGE:float = 25.0
const CANDELA_TO_ENERGY_SETTING:String = "reloaded/models/light_candela_to_energy"
const DEFAULT_RANGE_SETTING:String = "reloaded/models/light_default_range"


## A model of two spots, of no range and of one of their own
func _model() -> PackedByteArray:
    var lights:Array = [
        {"type": "spot", "intensity": CANDELA, "spot": {"innerConeAngle": 0.1, "outerConeAngle": 0.3}},
        {"type": "spot", "intensity": CANDELA, "range": OWN_RANGE, "spot": {"innerConeAngle": 0.1, "outerConeAngle": 0.3}},
    ]
    var gltf:Dictionary = {
        "asset": {"version": "2.0"},
        "extensionsUsed": ["KHR_lights_punctual"],
        "extensions": {"KHR_lights_punctual": {"lights": lights}},
        "scene": 0,
        "scenes": [{"nodes": [0, 1]}],
        "nodes": [
            {"name": "Unranged", "extensions": {"KHR_lights_punctual": {"light": 0}}},
            {"name": "Ranged", "extensions": {"KHR_lights_punctual": {"light": 1}}},
        ],
    }
    return JSON.stringify(gltf).to_utf8_buffer()


func test_a_spots_candelas_are_its_energy_and_its_range_the_default_of_none() -> void:
    var document:GLTFDocument = GLTFDocument.new()
    var state:GLTFState = GLTFState.new()
    assert_eq(document.append_from_buffer(_model(), "", state), OK)
    var root:Node = document.generate_scene(state)

    var unranged:SpotLight3D = root.find_child("Unranged", true, false) as SpotLight3D
    var ranged:SpotLight3D = root.find_child("Ranged", true, false) as SpotLight3D
    var energy:float = CANDELA * float(ProjectSettings.get_setting(CANDELA_TO_ENERGY_SETTING))
    assert_almost_eq(unranged.light_energy, energy, 0.0001, "its candelas times the setting")
    assert_almost_eq(unranged.spot_range, float(ProjectSettings.get_setting(DEFAULT_RANGE_SETTING)), 0.001,
            "the default range of none")
    assert_almost_eq(ranged.spot_range, OWN_RANGE, 0.001, "its own range kept")
    root.free()
