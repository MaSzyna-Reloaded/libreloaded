@tool
@abstract
extends Node
class_name SkyEnvironment

## Draws the state of a EnvironmentNode - the WorldEnvironment, its sky, the sun and the cab
## light. A sky addon is a backend extending this; a plugin carries none of its own, the project
## that has the addon places its backend next to the environment node and connects the node's
## configuration_changed to apply_configuration() in the scene.

const GENERATED_WORLD_NAME: StringName = &"_WorldEnvironment"
## The ambient light is the night's below this height of the sun (its sine) and the day's from
## AMBIENT_DAY_FULL_ELEVATION up, as Skydome blends it (Skydome.gd:1185, day_full_elevation)
const AMBIENT_NIGHT_ALTITUDE: float = -0.2
const AMBIENT_DAY_FULL_ELEVATION: float = 6.0
## Glow as tuned in forest-test-scene materials/environment_filmic.tres - what the player's Graphics
## settings (render/glow_intensity, render/bloom_intensity) fall back to
const GLOW_INTENSITY_DEFAULT: float = 1.37
const BLOOM_INTENSITY_DEFAULT: float = 0.3
## The player's gamma (render/gamma): the picture's mid-tones as x^(1/gamma) - above 1 brighter,
## below darker, black and white staying put - through the environment's colour correction: baked
## into the colour correction look-up table (color_correction), or a curve of this many steps
## without one; at 1 the table as it is, or no correction at all
const GAMMA_DEFAULT: float = 1.0
const GAMMA_CURVE_STEPS: int = 256
## The player's film grain (render/film_grain_enabled, render/film_grain_intensity): drawn over the
## tonemapped 3D view by a full-screen pass on a canvas layer below the interface's (layer 0), off
## unless the player turns it on
const FILM_GRAIN_SHADER: Shader = preload("film_grain.gdshader")
const FILM_GRAIN_LAYER: int = -1
const FILM_GRAIN_ENABLED_DEFAULT: bool = false
const FILM_GRAIN_INTENSITY_DEFAULT: float = 0.12

const SHADOW_SCENERY_ENABLED_SETTING: StringName = &"reloaded/scenery/shadows/enabled"
const SHADOW_SCENERY_MODE_SETTING: StringName = &"reloaded/scenery/shadows/mode"
const SHADOW_SCENERY_BLUR_SETTING: StringName = &"reloaded/scenery/shadows/blur"
const SHADOW_SCENERY_OPACITY_SETTING: StringName = &"reloaded/scenery/shadows/opacity"
const SHADOW_SCENERY_BIAS_SETTING: StringName = &"reloaded/scenery/shadows/bias"
const SHADOW_SCENERY_NORMAL_BIAS_SETTING: StringName = &"reloaded/scenery/shadows/normal_bias"
const SHADOW_SCENERY_MAX_DISTANCE_SETTING: StringName = &"reloaded/scenery/shadows/max_distance"
const SHADOW_SCENERY_BLEND_SPLITS_SETTING: StringName = &"reloaded/scenery/shadows/blend_splits"
const SHADOW_SCENERY_SPLIT_SETTINGS: Array[StringName] = [
    &"reloaded/scenery/shadows/split_1",
    &"reloaded/scenery/shadows/split_2",
    &"reloaded/scenery/shadows/split_3",
]
## How sharp the shadows are - the size of their textures (get_shadow_atlas_size())
const SHADOW_TEXTURE_QUALITY_SETTING: StringName = &"reloaded/shadows/texture_quality"
enum ShadowTextureQuality { LOW, MEDIUM, HIGH, MAXIMUM }
const SHADOW_TEXTURE_QUALITY: ShadowTextureQuality = ShadowTextureQuality.MEDIUM
## The size of the shadow map of each shadowed directional light, by ShadowTextureQuality - MAXIMUM's
## takes 512 MB of video memory with 16-bit shadows
const SHADOW_TEXTURE_QUALITY_SIZES: PackedInt32Array = [2048, 4096, 8192, 16384]
## The largest atlas Godot takes (rendering/lights_and_shadows/directional_shadow/size): MAXIMUM's
## map does not fit twice, so beside a cab light the sun keeps half of it
const SHADOW_ATLAS_MAX_SIZE: int = 16384
## How the atlas of the omni and spot lights' shadows (a street lamp's) is shared out, quadrant by
## quadrant: the light nearest the camera gets a quadrant of its own - half the atlas across - and
## the rest four, sixteen and sixty-four maps a quadrant. Godot's own first quadrant holds four, so
## a lamp lighting the cab through its window drew its shadow from a quarter of that
const POSITIONAL_SHADOW_QUADRANTS: Array[Viewport.PositionalShadowAtlasQuadrantSubdiv] = [
    Viewport.SHADOW_ATLAS_QUADRANT_SUBDIV_1,
    Viewport.SHADOW_ATLAS_QUADRANT_SUBDIV_4,
    Viewport.SHADOW_ATLAS_QUADRANT_SUBDIV_16,
    Viewport.SHADOW_ATLAS_QUADRANT_SUBDIV_64,
]

# The default values of the shadow settings, which ReloadedProjectSettings registers - the only
# place they are written down
const SHADOW_ENABLED: bool = true
const SHADOW_OPACITY: float = 1.0
const SHADOW_BIAS: float = 0.1
const SHADOW_BLEND_SPLITS: bool = true
## The world's sun lights the cab too (no cab light of its own): its first cascade sharp and
## stable over the cab's own couple of metres of a 1 km reach
const SHADOW_SCENERY_MODE: DirectionalLight3D.ShadowMode = DirectionalLight3D.SHADOW_PARALLEL_4_SPLITS
const SHADOW_SCENERY_SPLITS: Array[float] = [0.002, 0.125, 0.5]
const SHADOW_SCENERY_MAX_DISTANCE: float = 1000.0
const SHADOW_SCENERY_BLUR: float = 0.4
## Godot's own DirectionalLight3D default. The lookup moves by normal_bias times the texel of a
## cascade - at 10 it moved 2.6 m in the last one and the shadow of a mast or a person was gone.
const SHADOW_SCENERY_NORMAL_BIAS: float = 1.0
## Shadow maps drawn with front faces culled (opengl33renderer.cpp:1758)
const REVERSE_CULL_FACE_SETTING: StringName = &"reloaded/lights/reverse_cull_face"
const REVERSE_CULL_FACE: bool = false
const DIRECTIONAL_SHADOW_16_BITS_SETTING: StringName = &"rendering/lights_and_shadows/directional_shadow/16_bits"
## How soft the directional lights' shadows are - Godot's own setting, which the renderer reads only
## as it starts: taken again with the lights' settings, so a change shows at once
const DIRECTIONAL_SOFT_SHADOW_QUALITY_SETTING: StringName = (
    &"rendering/lights_and_shadows/directional_shadow/soft_shadow_filter_quality")

## Skydome's volumetric fog volume is 8 m deep by day and 3 m by night. The volume is measured from
## the camera, so a light shaft can only be seen while its lamp is inside it - at 3 m, never. This
## stretches the volume; the density is divided by the same factor, which leaves the optical depth
## (extinction per metre times length) - and so the look of the fog - unchanged. At 24 that is 72 m
## by night and 192 m by day. Raising it further spreads the same froxel depth slices over more
## metres, which softens the fog near the camera, and shafts still stop where the light's shadow
## does (the fade distance of the model backend's light shadows) - the two have to be raised together.
const FOG_VOLUMETRIC_LENGTH_SCALE_SETTING: StringName = &"reloaded/weather/fog/volumetric_length_scale"
const FOG_VOLUMETRIC_LENGTH_SCALE_DEFAULT: float = 24.0
const VOLUMETRIC_FOG_ENERGY_SETTING: StringName = &"reloaded/weather/fog/volumetric_energy"
const VOLUMETRIC_FOG_ENERGY_DEFAULT: float = 1.0
## EnvironmentNode.fog_distance is scaled by these for the day and for the night fog of a
## sky backend that tells them apart; the night default keeps Skydome's own 200 m to 470 m ratio
const FOG_DAY_DISTANCE_FACTOR_SETTING: StringName = &"reloaded/weather/fog/day_distance_factor"
const FOG_NIGHT_DISTANCE_FACTOR_SETTING: StringName = &"reloaded/weather/fog/night_distance_factor"
## How the fog builds up with the distance z from the camera: opacity = fog_density *
## (z / fog_distance) ^ exponent (Environment.fog_depth_curve). 1 - evenly, above 1 - clear near the
## camera and thickening further away, below 1 - thick right away. It has nothing to do with how
## fog_density itself is scaled. The original's fog is 1 - exp(-(z / range)^2) (apply_fog.glsl:16);
## an exponent of 1.5 with the fog complete at 1.5 of that range starts as slowly as it does
## (19/54/100% against 22/63/90% at 0.5/1/1.5 of the range) - hence the two defaults.
## How the fog builds up from the camera to EnvironmentNode.fog_distance
## (Environment.fog_depth_curve), drawn by the editor as it works: x is the distance (0 at the
## camera, 1 at fog_distance), y the share of fog_density reached there - share = x ^ curve.
## 1 - evenly, above 1 - clear near the camera and thickening further away, below 1 - thick right
## away; FOG_CURVE_MIN keeps it from becoming a wall of fog in front of the camera. The original's
## fog is 1 - exp(-(z / range)^2) (apply_fog.glsl:16); a curve of 1.5 with the fog complete at 1.5
## of that range starts as slowly as it does (19/54/100% against 22/63/90% at 0.5/1/1.5 of the
## range) - hence the two defaults.
const FOG_CURVE_SETTING: StringName = &"reloaded/weather/fog/curve"
const FOG_CURVE_DEFAULT: float = 1.5
const FOG_CURVE_MIN: float = 0.5
## How much fog the sky takes, by how far the fog reaches: a fog of FOG_SKY_NEAR or closer hides the
## whole sky, one of FOG_SKY_FAR or further leaves the stars, the moon and the clouds alone, and
## between the two the share falls smoothly with the distance's logarithm - a 1 km fog takes about
## half. Rain is no thin layer - it fills the air all the way up - so the fog a downpour brings
## reaches the sky in full (the backends).
## No setting - the look is worked out here, not left to the player.
const FOG_SKY_NEAR: float = 300.0
const FOG_SKY_FAR: float = 3000.0
## How much the fog takes the colour of the sky's radiance instead of its own
## (Environment.fog_aerial_perspective). Off by default: at dusk that radiance is as dark as the
## scene itself, so a fully fogged object stays a dark silhouette against a brighter sky instead of
## fading out - the fog stops reading as fog.
const FOG_AERIAL_PERSPECTIVE_SETTING: StringName = &"reloaded/weather/fog/aerial_perspective"
const FOG_AERIAL_PERSPECTIVE_DEFAULT: float = 0.0
## A downpour brings a fog of its own, whatever fog is set: towards a full precipitation the fog
## distance shortens to this visibility and the fog density rises to this opacity (never the other
## way - a fog already closer or thicker stays as it is).
const RAIN_FOG_DISTANCE_SETTING: StringName = &"reloaded/weather/rain/fog_distance"
const RAIN_FOG_DISTANCE_DEFAULT: float = 800.0
const RAIN_FOG_DENSITY_SETTING: StringName = &"reloaded/weather/rain/fog_density"
const RAIN_FOG_DENSITY_DEFAULT: float = 0.6
## How fast the fog right in front of the camera (the volumetric layer) dies out once fog_distance
## grows past the distance the sky backend is tuned for: its density follows
## (reference / fog_distance) ^ falloff there. 1 - as below that distance; 2 - a fog reaching
## kilometres leaves the view in front of the camera as crisp as no fog at all.
const FOG_VOLUMETRIC_FAR_FALLOFF_SETTING: StringName = &"reloaded/weather/fog/volumetric_far_falloff"
const FOG_VOLUMETRIC_FAR_FALLOFF_DEFAULT: float = 2.0
## Floor under that falloff, as a share of the volumetric density at the reference distance. A
## scenery that declares a fog of kilometres (stary_jawor_noc asks for 2-4 km, which becomes a
## fog_distance of 2250-4500 m) drives the falloff to 0.01-0.04 and leaves the air by the camera
## with no haze at all - and a street lamp with nothing to scatter in casts no visible shaft. Real
## night air is never that clean, so the haze thins towards this share instead of towards nothing.
const FOG_VOLUMETRIC_MINIMUM_SETTING: StringName = &"reloaded/weather/fog/volumetric_minimum"
const FOG_VOLUMETRIC_MINIMUM_DEFAULT: float = 0.25
## fog_distance of a scenery that declares its fog, as a multiple of the original's fog range
const FOG_SCENERY_DISTANCE_FACTOR_SETTING: StringName = &"reloaded/weather/fog/scenery_distance_factor"
const FOG_SCENERY_DISTANCE_FACTOR_DEFAULT: float = 1.5
const FOG_DAY_DISTANCE_FACTOR_DEFAULT: float = 1.0
const FOG_NIGHT_DISTANCE_FACTOR_DEFAULT: float = 0.4255

## The environment this sky draws
@export_node_path("EnvironmentNode") var environment_node_path: NodePath

@export_group("Tone Mapping")
@export var tonemap_mode: Environment.ToneMapper = Environment.TONE_MAPPER_AGX:
    set(value):
        tonemap_mode = value
        _dirty_visuals = true

@export_range(0.0, 16.0, 0.01) var tonemap_white: float = 6.0:
    set(value):
        tonemap_white = value
        _dirty_visuals = true

@export_range(0.0, 16.0, 0.01) var tonemap_agx_white: float = 6.19:
    set(value):
        tonemap_agx_white = value
        _dirty_visuals = true

## AgX's contrast by day and by night, blended by the sun's height as the ambient light is
## (_apply_day_and_night())
@export_range(0.0, 2.0, 0.01) var day_tonemap_agx_contrast: float = 1.55:
    set(value):
        day_tonemap_agx_contrast = value
        _dirty_day_and_night = true

@export_range(0.0, 2.0, 0.01) var night_tonemap_agx_contrast: float = 1.55:
    set(value):
        night_tonemap_agx_contrast = value
        _dirty_day_and_night = true

@export_group("Adjustments")
## Godot's adjustments of the picture (Environment.adjustment_*): the player's gamma (render/gamma)
## and the brightness, contrast and saturation of the day and of the night, blended by the sun's
## height as the ambient light is (_apply_day_and_night()), 1 leaving the picture as it is
@export var adjustment_enabled: bool = true:
    set(value):
        adjustment_enabled = value
        _dirty_visuals = true

## A look-up table every colour of the picture goes through (Environment.adjustment_color_correction):
## a Texture3D - a LUT, e.g. a strip of slices imported as Texture3D - or a Texture2D of one row,
## a curve for every channel; the player's gamma (render/gamma) is baked into it
@export var color_correction: Texture = null:
    set(value):
        color_correction = value
        _dirty_visuals = true

@export_subgroup("Day")
@export_range(0.01, 8.0, 0.01) var day_adjustment_brightness: float = 1.0:
    set(value):
        day_adjustment_brightness = value
        _dirty_day_and_night = true

@export_range(0.01, 8.0, 0.01) var day_adjustment_contrast: float = 1.0:
    set(value):
        day_adjustment_contrast = value
        _dirty_day_and_night = true

@export_range(0.01, 8.0, 0.01) var day_adjustment_saturation: float = 1.0:
    set(value):
        day_adjustment_saturation = value
        _dirty_day_and_night = true

@export_subgroup("Night")
@export_range(0.01, 8.0, 0.01) var night_adjustment_brightness: float = 1.0:
    set(value):
        night_adjustment_brightness = value
        _dirty_day_and_night = true

@export_range(0.01, 8.0, 0.01) var night_adjustment_contrast: float = 1.0:
    set(value):
        night_adjustment_contrast = value
        _dirty_day_and_night = true

@export_range(0.01, 8.0, 0.01) var night_adjustment_saturation: float = 1.0:
    set(value):
        night_adjustment_saturation = value
        _dirty_day_and_night = true

@export_group("Sun and Moon")
## How bright the sun shines, times what the backend gives it (its light level, Skydome's day light)
@export_range(0.0, 16.0, 0.01) var sun_energy: float = 1.0:
    set(value):
        sun_energy = value
        _dirty_lights = true

## How bright the moon shines, times what the backend gives it (Skydome's night light); a backend
## with no moon (ProceduralSkyEnvironment) has nothing to take it
@export_range(0.0, 16.0, 0.01) var moon_energy: float = 1.0:
    set(value):
        moon_energy = value
        _dirty_lights = true

@export_group("Auto Exposure")
## The camera's exposure following the brightness of the picture (CameraAttributesPractical's
## auto_exposure_*): how fast it adapts and, by day and by night, how bright it keeps the picture
## and between which sensitivities [ISO], blended by the sun's height as the ambient light is
## (_apply_day_and_night()) - a night all in fog is not kept as bright as a day in it; Godot's
## defaults
@export var auto_exposure_enabled: bool = false:
    set(value):
        auto_exposure_enabled = value
        _dirty_visuals = true

@export_range(0.01, 64.0, 0.01) var auto_exposure_speed: float = 0.5:
    set(value):
        auto_exposure_speed = value
        _dirty_visuals = true

## The share of the picture's darkest pixels, in percent, the exposure is not measured by
## (RenderingServer.camera_attributes_set_auto_exposure_low_percent(),
## godot-patches/0003-local-auto-exposure-metering.patch): raised, a cab's window frame or a dark
## wagon near the camera does not brighten the picture; a plain Godot keeps it and leaves it out
@export_range(0.0, 89.0, 0.1, "suffix:%") var auto_exposure_low_percent: float = 10.0:
    set(value):
        auto_exposure_low_percent = value
        _dirty_visuals = true

@export_subgroup("Day")
@export_range(0.01, 16.0, 0.01) var day_auto_exposure_scale: float = 0.4:
    set(value):
        day_auto_exposure_scale = value
        _dirty_day_and_night = true

@export_range(0.0, 1600.0, 0.01) var day_auto_exposure_min_sensitivity: float = 0.0:
    set(value):
        day_auto_exposure_min_sensitivity = value
        _dirty_day_and_night = true

@export_range(30.0, 64000.0, 0.01) var day_auto_exposure_max_sensitivity: float = 800.0:
    set(value):
        day_auto_exposure_max_sensitivity = value
        _dirty_day_and_night = true

@export_subgroup("Night")
@export_range(0.01, 16.0, 0.01) var night_auto_exposure_scale: float = 0.4:
    set(value):
        night_auto_exposure_scale = value
        _dirty_day_and_night = true

@export_range(0.0, 1600.0, 0.01) var night_auto_exposure_min_sensitivity: float = 0.0:
    set(value):
        night_auto_exposure_min_sensitivity = value
        _dirty_day_and_night = true

@export_range(30.0, 64000.0, 0.01) var night_auto_exposure_max_sensitivity: float = 800.0:
    set(value):
        night_auto_exposure_max_sensitivity = value
        _dirty_day_and_night = true

@export_group("Global Illumination")
## SDFGI's indirect light (render/sdfgi_enabled): its energy (Environment.sdfgi_energy) and the share of
## it kept at every bounce from a surface (sdfgi_bounce_feedback) - above 0 light bounces on, into a
## cab through its windows; Godot's defaults
@export_range(0.0, 8.0, 0.01) var sdfgi_energy: float = 1.0:
    set(value):
        sdfgi_energy = value
        _dirty_visuals = true

@export_range(0.0, 1.99, 0.01) var sdfgi_bounce_feedback: float = 0.5:
    set(value):
        sdfgi_bounce_feedback = value
        _dirty_visuals = true

@export_group("Ambient Light")
## The day's and the night's ambient light - its colour, energy and the share of it the sky gives
## (Environment.ambient_light_*), blended by the sun's height (_apply_day_and_night()); a backend that
## draws the ambient light itself (Skydome) takes them over; Skydome's defaults (Skydome.gd:171-253).
## SDFGI takes it in only once Godot's godot#55104 is fixed (godot#118329)
@export var day_ambient_color: Color = Color(0.91, 0.85, 0.69):
    set(value):
        day_ambient_color = value
        _dirty_lights = true
        _dirty_day_and_night = true

@export_range(0.0, 16.0, 0.01) var day_ambient_energy: float = 1.5:
    set(value):
        day_ambient_energy = value
        _dirty_lights = true
        _dirty_day_and_night = true

@export_range(0.0, 1.0, 0.01) var day_ambient_sky_contribution: float = 0.25:
    set(value):
        day_ambient_sky_contribution = value
        _dirty_lights = true
        _dirty_day_and_night = true

@export var night_ambient_color: Color = Color(0.02, 0.03, 0.06):
    set(value):
        night_ambient_color = value
        _dirty_lights = true
        _dirty_day_and_night = true

@export_range(0.0, 16.0, 0.01) var night_ambient_energy: float = 0.1:
    set(value):
        night_ambient_energy = value
        _dirty_lights = true
        _dirty_day_and_night = true

@export_range(0.0, 1.0, 0.01) var night_ambient_sky_contribution: float = 0.8:
    set(value):
        night_ambient_sky_contribution = value
        _dirty_lights = true
        _dirty_day_and_night = true

@export_group("Fog Light")
## The colour of the fog's light by day and by night (Environment.fog_light_color), blended by the
## sun's height as the ambient light is (_apply_day_and_night()); a backend that colours the fog
## itself (Skydome) takes them over. The day's the core's sky's own, the night's Skydome's
## (Skydome.gd:255).
@export var day_fog_color: Color = Color(0.3605356, 0.39691955, 0.44612736):
    set(value):
        day_fog_color = value
        _dirty_lights = true
        _dirty_day_and_night = true

@export var night_fog_color: Color = Color(0.04, 0.06, 0.12):
    set(value):
        night_fog_color = value
        _dirty_lights = true
        _dirty_day_and_night = true

var environment_node: EnvironmentNode
var _environment: Environment
var _camera_attributes: CameraAttributesPractical
## The full-screen pass of the film grain, shown while the player has it on
var _film_grain: ColorRect
var _dirty_visuals: bool = true
var _dirty_lights: bool = true
var _dirty_time: bool = true
## What the day and the night tell apart taken again: its values changed, or the sun moved
## (_apply_day_and_night())
var _dirty_day_and_night: bool = true
## The gamma and the look-up table the environment's colour correction was made for - it is made
## again only when either changes - and the correction itself: the table as it is at the default
## gamma, the gamma baked into it otherwise, none without a table at the default gamma
var _gamma: float = GAMMA_DEFAULT
var _color_correction_table: Texture = null
var _color_correction: Texture = null


func _ready() -> void:
    environment_node = get_node(environment_node_path) as EnvironmentNode
    var world_environment: WorldEnvironment = WorldEnvironment.new()
    world_environment.name = GENERATED_WORLD_NAME
    _environment = Environment.new()
    _environment.background_mode = Environment.BG_SKY
    _environment.sky = create_sky()
    _environment.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
    _environment.ambient_light_color = Color.WHITE
    _environment.reflected_light_source = Environment.REFLECTION_SOURCE_SKY
    _environment.fog_mode = Environment.FOG_MODE_DEPTH
    _environment.fog_light_energy = 0.7
    _environment.fog_sun_scatter = 0.07
    _environment.volumetric_fog_anisotropy = 0.0
    _environment.volumetric_fog_detail_spread = 1.0
    # Glow tuned as in forest-test-scene materials/environment_filmic.tres (its intensity and bloom
    # are the player's, _process()); the luminance cap keeps small specular highlights (e.g. rain
    # streaks) from blooming into large blobs.
    _environment.glow_normalized = true
    _environment.glow_strength = 0.8
    _environment.glow_hdr_threshold = 1.37
    _environment.glow_hdr_luminance_cap = 0.18
    world_environment.environment = _environment
    _camera_attributes = CameraAttributesPractical.new()
    world_environment.camera_attributes = _camera_attributes
    create_nodes(world_environment, _environment)
    add_child(world_environment, false, INTERNAL_MODE_BACK)
    var film_grain_layer: CanvasLayer = CanvasLayer.new()
    film_grain_layer.layer = FILM_GRAIN_LAYER
    _film_grain = ColorRect.new()
    _film_grain.set_anchors_preset(Control.PRESET_FULL_RECT)
    _film_grain.mouse_filter = Control.MOUSE_FILTER_IGNORE
    var film_grain_material: ShaderMaterial = ShaderMaterial.new()
    film_grain_material.shader = FILM_GRAIN_SHADER
    _film_grain.material = film_grain_material
    film_grain_layer.add_child(_film_grain)
    add_child(film_grain_layer, false, INTERNAL_MODE_BACK)


func _enter_tree() -> void:
    UserSettings.config_changed.connect(_on_user_settings_changed)
    ProjectSettings.settings_changed.connect(_on_project_settings_changed)
    SimulationServer.simulation_paused.connect(pause_weather)
    SimulationServer.simulation_unpaused.connect(unpause_weather)
    SimulationServer.time_of_day_changed.connect(_on_sun_moved)
    SimulationServer.date_changed.connect(_on_sun_moved)


func _exit_tree() -> void:
    UserSettings.config_changed.disconnect(_on_user_settings_changed)
    ProjectSettings.settings_changed.disconnect(_on_project_settings_changed)
    SimulationServer.simulation_paused.disconnect(pause_weather)
    SimulationServer.simulation_unpaused.disconnect(unpause_weather)
    SimulationServer.time_of_day_changed.disconnect(_on_sun_moved)
    SimulationServer.date_changed.disconnect(_on_sun_moved)


func _process(delta: float) -> void:
    if _dirty_visuals:
        _dirty_visuals = false
        _apply_environment_configuration()
        apply_visual_configuration()
    if _dirty_lights:
        _dirty_lights = false
        apply_light_configuration()
        _apply_positional_shadow_settings()
    if _dirty_time:
        _dirty_time = false
        apply_time_configuration()
    if _dirty_day_and_night:
        _dirty_day_and_night = false
        _apply_day_and_night()
    process_time(delta)


## The environment node's configuration changed (EnvironmentNode.configuration_changed) -
## everything it shows is taken again
func apply_configuration() -> void:
    _dirty_visuals = true
    _dirty_lights = true
    _dirty_time = true


@abstract func create_sky() -> Sky


@abstract func create_nodes(
    world_environment: WorldEnvironment, environment: Environment
) -> void


## The backend's own share of the environment's weather and fog
@abstract func apply_visual_configuration() -> void


## Applies the scenery/shadows settings to the backend's directional lights.
@abstract func apply_light_configuration() -> void


## The environment's time and date set, not run - the sky jumps to them
@abstract func apply_time_configuration() -> void


## The running time of the environment, followed by the sky
func process_time(_delta: float) -> void:
    pass


## Holds the weather (rain, lightning) still while the runtime is paused (SimulationServer.simulation_pause()).
## A backend without weather of its own has nothing to hold.
func pause_weather() -> void:
    pass


func unpause_weather() -> void:
    pass


func _apply_environment_configuration() -> void:
    # fog_density is a multiplier; zero fades the fog out instead of switching it off.
    var fog_active: bool = environment_node.fog_enabled
    _environment.tonemap_mode = tonemap_mode
    _environment.tonemap_white = tonemap_white
    _environment.tonemap_agx_white = tonemap_agx_white
    _environment.ssr_enabled = bool(UserSettings.get_setting("render", "ssr_enabled", true))
    _environment.ssao_enabled = bool(UserSettings.get_setting("render", "ssao_enabled", true))
    _environment.ssil_enabled = bool(UserSettings.get_setting("render", "ssil_enabled", true))
    _environment.sdfgi_enabled = bool(UserSettings.get_setting("render", "sdfgi_enabled", true))
    _camera_attributes.auto_exposure_enabled = auto_exposure_enabled
    _camera_attributes.auto_exposure_speed = auto_exposure_speed
    # By name: the method is only in Godot built with godot-patches/0003-local-auto-exposure-
    # metering.patch, and a plain Godot's editor must still compile and run this script
    if RenderingServer.has_method(&"camera_attributes_set_auto_exposure_low_percent"):
        RenderingServer.call(&"camera_attributes_set_auto_exposure_low_percent",
                _camera_attributes.get_rid(), auto_exposure_low_percent)
    _environment.sdfgi_energy = sdfgi_energy
    _environment.sdfgi_bounce_feedback = sdfgi_bounce_feedback
    _environment.glow_enabled = bool(UserSettings.get_setting("render", "glow_enabled", true))
    _environment.glow_intensity = float(
        UserSettings.get_setting("render", "glow_intensity", GLOW_INTENSITY_DEFAULT))
    _environment.glow_bloom = float(
        UserSettings.get_setting("render", "bloom_intensity", BLOOM_INTENSITY_DEFAULT))
    _film_grain.visible = bool(
        UserSettings.get_setting("render", "film_grain_enabled", FILM_GRAIN_ENABLED_DEFAULT))
    (_film_grain.material as ShaderMaterial).set_shader_parameter(&"amount", float(
        UserSettings.get_setting("render", "film_grain_intensity", FILM_GRAIN_INTENSITY_DEFAULT)))
    _environment.adjustment_enabled = adjustment_enabled
    var gamma: float = float(UserSettings.get_setting("render", "gamma", GAMMA_DEFAULT))
    if not is_equal_approx(gamma, _gamma) or not color_correction == _color_correction_table:
        _gamma = gamma
        _color_correction_table = color_correction
        _color_correction = color_correction
        if not is_equal_approx(gamma, GAMMA_DEFAULT):
            # the table's images - a Texture3D's slices, a Texture2D's row, without a table one row
            # of the input along it in every channel - and x^(1/gamma) of every value they give
            var images: Array[Image] = []
            if color_correction is Texture3D:
                for slice: Image in (color_correction as Texture3D).get_data():
                    images.append(slice.duplicate() as Image)
            elif color_correction is Texture2D:
                images.append((color_correction as Texture2D).get_image().duplicate() as Image)
            else:
                var curve: Image = Image.create(GAMMA_CURVE_STEPS, 1, false, Image.FORMAT_RGBA8)
                for step: int in GAMMA_CURVE_STEPS:
                    var value: float = float(step) / float(GAMMA_CURVE_STEPS - 1)
                    curve.set_pixel(step, 0, Color(value, value, value))
                images.append(curve)
            for image: Image in images:
                if image.is_compressed():
                    image.decompress()
                image.convert(Image.FORMAT_RGBA8)
                for y: int in image.get_height():
                    for x: int in image.get_width():
                        var value: Color = image.get_pixel(x, y)
                        image.set_pixel(x, y, Color(pow(value.r, 1.0 / gamma), pow(value.g, 1.0 / gamma),
                                pow(value.b, 1.0 / gamma), value.a))
            if color_correction is Texture3D:
                # a renderer that keeps no texture data (--headless) gives no slices: the table as it is
                if images:
                    var table: ImageTexture3D = ImageTexture3D.new()
                    table.create(Image.FORMAT_RGBA8, images[0].get_width(), images[0].get_height(), images.size(),
                            false, images)
                    _color_correction = table
            else:
                _color_correction = ImageTexture.create_from_image(images[0])
    _environment.adjustment_color_correction = _color_correction
    _environment.fog_enabled = fog_active
    # the sky backends leave these two alone
    _environment.fog_aerial_perspective = float(ProjectSettings.get_setting(FOG_AERIAL_PERSPECTIVE_SETTING))
    _environment.fog_depth_curve = maxf(FOG_CURVE_MIN, float(
        ProjectSettings.get_setting(FOG_CURVE_SETTING)))
    _environment.volumetric_fog_enabled = (
        fog_active and bool(UserSettings.get_setting("render", "volumetric_fog_enabled", true))
    )


## The share of the depth fog the sky takes for a fog reaching [param fog_distance] (FOG_SKY_NEAR,
## FOG_SKY_FAR) - for a backend to scale by the fog's density (Environment.fog_sky_affect fogs the
## sky whatever it is)
func get_fog_sky_share(fog_distance: float) -> float:
    return smoothstep(0.0, 1.0,
        log(FOG_SKY_FAR / maxf(fog_distance, 0.001)) / log(FOG_SKY_FAR / FOG_SKY_NEAR))


## The day's or the night's ambient light, fog light, AgX contrast, adjustments and auto exposure by the
## height of the sun SimulationServer puts at the environment's place and clock - the ambient energy rising
## with the square root of the day's share (Skydome.gd:1185, 1211-1214); Skydome sets the ambient colour
## and energy and the fog's colour too, from the same values
func _apply_day_and_night() -> void:
    var day: float = get_day_share()
    _environment.tonemap_agx_contrast = lerpf(night_tonemap_agx_contrast, day_tonemap_agx_contrast, day)
    _environment.adjustment_brightness = lerpf(night_adjustment_brightness, day_adjustment_brightness, day)
    _environment.adjustment_contrast = lerpf(night_adjustment_contrast, day_adjustment_contrast, day)
    _environment.adjustment_saturation = lerpf(night_adjustment_saturation, day_adjustment_saturation, day)
    _environment.ambient_light_color = night_ambient_color.lerp(day_ambient_color, day)
    _environment.fog_light_color = night_fog_color.lerp(day_fog_color, day)
    _environment.ambient_light_energy = lerpf(night_ambient_energy, day_ambient_energy, sqrt(day))
    _environment.ambient_light_sky_contribution = lerpf(
            night_ambient_sky_contribution, day_ambient_sky_contribution, day)
    _camera_attributes.auto_exposure_scale = lerpf(night_auto_exposure_scale, day_auto_exposure_scale, day)
    _camera_attributes.auto_exposure_min_sensitivity = lerpf(
            night_auto_exposure_min_sensitivity, day_auto_exposure_min_sensitivity, day)
    _camera_attributes.auto_exposure_max_sensitivity = lerpf(
            night_auto_exposure_max_sensitivity, day_auto_exposure_max_sensitivity, day)


## How much of the day there is in what the day and the night tell apart, by the height of the sun
## SimulationServer puts at the environment's place and clock: 0 below AMBIENT_NIGHT_ALTITUDE, 1 from
## AMBIENT_DAY_FULL_ELEVATION up (Skydome.gd:1185)
func get_day_share() -> float:
    return smoothstep(AMBIENT_NIGHT_ALTITUDE, sin(deg_to_rad(AMBIENT_DAY_FULL_ELEVATION)),
            SimulationServer.sun_get_direction().y)


## The sun moved with the clock or the date: what the day and the night tell apart follows its height
func _on_sun_moved() -> void:
    _dirty_day_and_night = true


func _on_user_settings_changed() -> void:
    _dirty_visuals = true


## The fog, the shadows and the lights follow their project settings while the scenery runs
func _on_project_settings_changed() -> void:
    _dirty_visuals = true
    _dirty_lights = true


## A world light takes the scenery shadow settings
func _apply_sun_settings(world_light: DirectionalLight3D) -> void:
    RenderingServer.directional_shadow_atlas_set_size(
        get_shadow_atlas_size(), bool(ProjectSettings.get_setting(DIRECTIONAL_SHADOW_16_BITS_SETTING))
    )
    RenderingServer.directional_soft_shadow_filter_set_quality(
        int(ProjectSettings.get_setting(DIRECTIONAL_SOFT_SHADOW_QUALITY_SETTING)) as RenderingServer.ShadowQuality
    )
    _apply_directional_light_settings(world_light)


## The omni and spot lights' shadows are as sharp as the sun's: their atlas is the shadow texture
## quality's map, shared out by POSITIONAL_SHADOW_QUADRANTS. The viewport the world is drawn in
## holds it
func _apply_positional_shadow_settings() -> void:
    var viewport: Viewport = get_viewport()
    if viewport == null:
        return
    viewport.positional_shadow_atlas_size = get_positional_shadow_atlas_size()
    for quadrant: int in POSITIONAL_SHADOW_QUADRANTS.size():
        viewport.set_positional_shadow_atlas_quadrant_subdiv(quadrant, POSITIONAL_SHADOW_QUADRANTS[quadrant])


## The size of the omni and spot lights' shadow atlas: the shadow texture quality's map
func get_positional_shadow_atlas_size() -> int:
    return SHADOW_TEXTURE_QUALITY_SIZES[int(ProjectSettings.get_setting(SHADOW_TEXTURE_QUALITY_SETTING))]


## The size the directional shadow atlas is set to: the shadow map of the shadow texture quality,
## up to SHADOW_ATLAS_MAX_SIZE
func get_shadow_atlas_size() -> int:
    var quality: int = int(ProjectSettings.get_setting(SHADOW_TEXTURE_QUALITY_SETTING))
    return mini(SHADOW_TEXTURE_QUALITY_SIZES[quality], SHADOW_ATLAS_MAX_SIZE)


## Pins every cascade of world_light's shadow to a world-space point that travels together with
## whatever it is meant to shadow up close (RenderingServer.
## instance_light_set_shadow_cascade_anchor(), godot-patches/0010-local-directional-shadow-
## cascade-anchor.patch): a point moving together with it then never crosses a texel-grid line from
## travel alone, so its shadow does not pop while it does - a curve or a pitch still can, the same
## way a free-look camera's own rotation does. [param enabled] false returns to the plain camera
## frustum centroid every cascade reads by default. By name, as the other patch-only calls here
## are, so a plain Godot's editor still compiles and runs this script with nothing to anchor to.
func set_shadow_cascade_anchor(world_light: DirectionalLight3D, origin: Vector3, enabled: bool) -> void:
    if RenderingServer.has_method(&"instance_light_set_shadow_cascade_anchor"):
        RenderingServer.call(&"instance_light_set_shadow_cascade_anchor",
            world_light.get_instance(), origin, enabled)


func _apply_directional_light_settings(light: DirectionalLight3D) -> void:
    light.shadow_reverse_cull_face = bool(ProjectSettings.get_setting(REVERSE_CULL_FACE_SETTING))
    light.shadow_enabled = bool(ProjectSettings.get_setting(SHADOW_SCENERY_ENABLED_SETTING))
    light.shadow_opacity = float(ProjectSettings.get_setting(SHADOW_SCENERY_OPACITY_SETTING))
    light.shadow_bias = float(ProjectSettings.get_setting(SHADOW_SCENERY_BIAS_SETTING))
    light.shadow_blur = float(ProjectSettings.get_setting(SHADOW_SCENERY_BLUR_SETTING))
    # Normal bias pushes the shadow lookup along the surface normal by normal_bias texels of the
    # split - thinner casters than that lose their shadow, so it stays small outside.
    light.shadow_normal_bias = float(ProjectSettings.get_setting(SHADOW_SCENERY_NORMAL_BIAS_SETTING))
    light.directional_shadow_mode = int(
        ProjectSettings.get_setting(SHADOW_SCENERY_MODE_SETTING)
    ) as DirectionalLight3D.ShadowMode
    light.directional_shadow_max_distance = float(ProjectSettings.get_setting(SHADOW_SCENERY_MAX_DISTANCE_SETTING))
    light.directional_shadow_split_1 = float(ProjectSettings.get_setting(SHADOW_SCENERY_SPLIT_SETTINGS[0]))
    light.directional_shadow_split_2 = float(ProjectSettings.get_setting(SHADOW_SCENERY_SPLIT_SETTINGS[1]))
    light.directional_shadow_split_3 = float(ProjectSettings.get_setting(SHADOW_SCENERY_SPLIT_SETTINGS[2]))
    light.directional_shadow_blend_splits = bool(ProjectSettings.get_setting(SHADOW_SCENERY_BLEND_SPLITS_SETTING))
    light.light_volumetric_fog_energy = float(ProjectSettings.get_setting(VOLUMETRIC_FOG_ENERGY_SETTING))
