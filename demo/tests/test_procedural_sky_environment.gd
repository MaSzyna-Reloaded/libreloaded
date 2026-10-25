extends ReloadedGutTest

## ProceduralSkyEnvironment - the core's basic sky - draws the world's environment and takes the
## player's Graphics settings into it as any SkyEnvironment does, and again as they change
## (libmaszyna#330: demo_3d's settings panel changed nothing).

## Frames for the sky to take a change in (SkyEnvironment._process())
const APPLY_FRAMES:int = 2
## A scenery's fog, other than EnvironmentNode's defaults [0..1, m]
const FOG_DENSITY:float = 0.4
const FOG_DISTANCE:float = 1200.0
## Adjustments other than the neutral 1
const ADJUSTED_BRIGHTNESS:float = 1.2
const ADJUSTED_CONTRAST:float = 0.9
const ADJUSTED_SATURATION:float = 1.3
## The night's own brightness and AgX contrast, and the day's AgX contrast
const NIGHT_BRIGHTNESS:float = 0.7
const DAY_AGX_CONTRAST:float = 1.2
const NIGHT_AGX_CONTRAST:float = 0.8
## A sun brighter than the light level gives
const SUN_ENERGY:float = 2.0
## SDFGI's light other than Godot's defaults
const SDFGI_ENERGY:float = 2.5
const SDFGI_BOUNCE_FEEDBACK:float = 0.9
## Auto exposure other than Godot's defaults, the night's its own
const AUTO_EXPOSURE_SPEED:float = 2.0
const DAY_AUTO_EXPOSURE_SCALE:float = 0.8
const DAY_AUTO_EXPOSURE_MIN_SENSITIVITY:float = 50.0
const DAY_AUTO_EXPOSURE_MAX_SENSITIVITY:float = 1600.0
const NIGHT_AUTO_EXPOSURE_SCALE:float = 0.1
const NIGHT_AUTO_EXPOSURE_MIN_SENSITIVITY:float = 200.0
const NIGHT_AUTO_EXPOSURE_MAX_SENSITIVITY:float = 3200.0
## A summer noon and midnight at the EnvironmentNode's place [h]
const NOON:float = 12.0
const MIDNIGHT:float = 0.0

var _environment_node:EnvironmentNode
var _sky:ProceduralSkyEnvironment
var _ssao_before:bool


func before_each() -> void:
    _ssao_before = bool(UserSettings.get_setting("render", "ssao_enabled", true))
    _environment_node = EnvironmentNode.new()
    _environment_node.name = "EnvironmentNode"
    add_child(_environment_node)
    _sky = ProceduralSkyEnvironment.new()
    _sky.environment_node_path = NodePath("../EnvironmentNode")
    add_child(_sky)
    # as a scene wires them (demo_3d.tscn, the game's world.tscn)
    _environment_node.configuration_changed.connect(_sky.apply_configuration)
    await wait_idle_frames(APPLY_FRAMES)


func after_each() -> void:
    UserSettings.save_setting("render", "ssao_enabled", _ssao_before)
    _sky.free()
    _environment_node.free()


func test_the_world_is_drawn_with_the_procedural_sky() -> void:
    var environment:Environment = get_viewport().find_world_3d().environment
    assert_not_null(environment, "the sky's environment is the world's")
    assert_true(environment.sky.sky_material is ProceduralSkyMaterial)


func test_the_player_s_graphics_settings_reach_the_environment() -> void:
    var environment:Environment = get_viewport().find_world_3d().environment
    assert_eq(environment.ssao_enabled, _ssao_before, "as the settings have it")

    UserSettings.save_setting("render", "ssao_enabled", not _ssao_before)
    await wait_idle_frames(APPLY_FRAMES)

    assert_eq(environment.ssao_enabled, not _ssao_before, "and as they change")


func test_the_scenery_s_fog_reaches_the_environment() -> void:
    var environment:Environment = get_viewport().find_world_3d().environment

    _environment_node.fog_density = FOG_DENSITY
    _environment_node.fog_distance = FOG_DISTANCE
    await wait_idle_frames(APPLY_FRAMES)

    assert_almost_eq(environment.fog_density, FOG_DENSITY, 0.0001, "its density")
    assert_almost_eq(environment.fog_depth_end, FOG_DISTANCE, 0.001, "reached at its distance")
    assert_eq(environment.fog_depth_begin, 0.0, "growing from the camera")


## The light shines from where SimulationServer puts the sun, as bright as its light level
func test_the_sun_shines_from_where_the_server_puts_it() -> void:
    # the sky's WorldEnvironment is its internal child, the sun that one's child
    var sun:DirectionalLight3D = _sky.get_child(0, true).get_node_or_null(
            NodePath(ProceduralSkyEnvironment.SUN_NAME)) as DirectionalLight3D
    assert_not_null(sun, "the sky has its sun")
    if not sun:
        return
    assert_almost_eq(sun.global_basis.z.dot(SimulationServer.sun_get_direction()), 1.0, 0.001,
            "it shines along its -Z, from the sun")
    assert_almost_eq(sun.light_energy, SimulationServer.light_level, 0.001)
    _sky.sun_energy = SUN_ENERGY
    await wait_idle_frames(APPLY_FRAMES)
    assert_almost_eq(sun.light_energy, SimulationServer.light_level * SUN_ENERGY, 0.001, "times its sun_energy")


## The noon's ambient light is the day's, the midnight's the night's
func test_the_ambient_light_is_the_day_s_or_the_night_s() -> void:
    var environment:Environment = get_viewport().find_world_3d().environment
    var time_of_day:float = SimulationServer.time_of_day
    _environment_node.current_time = NOON
    await wait_idle_frames(APPLY_FRAMES)
    assert_true(SimulationServer.sun_get_direction().y > 0.5, "the noon sun is high")
    assert_almost_eq(environment.ambient_light_energy, _sky.day_ambient_energy, 0.001)
    assert_almost_eq(environment.ambient_light_sky_contribution, _sky.day_ambient_sky_contribution, 0.001)
    assert_true(environment.ambient_light_color.is_equal_approx(_sky.day_ambient_color))

    _environment_node.current_time = MIDNIGHT
    await wait_idle_frames(APPLY_FRAMES)

    assert_almost_eq(environment.ambient_light_energy, _sky.night_ambient_energy, 0.001)
    assert_true(environment.ambient_light_color.is_equal_approx(_sky.night_ambient_color))
    _environment_node.current_time = time_of_day


## The noon's fog light is the day's, the midnight's the night's - not the day's grey under the night
## sky (libmaszyna#330)
func test_the_fog_light_is_the_day_s_or_the_night_s() -> void:
    var environment:Environment = get_viewport().find_world_3d().environment
    var time_of_day:float = SimulationServer.time_of_day
    _environment_node.current_time = NOON
    await wait_idle_frames(APPLY_FRAMES)
    assert_true(environment.fog_light_color.is_equal_approx(_sky.day_fog_color))

    _environment_node.current_time = MIDNIGHT
    await wait_idle_frames(APPLY_FRAMES)

    assert_true(environment.fog_light_color.is_equal_approx(_sky.night_fog_color))
    _environment_node.current_time = time_of_day


## SDFGI's energy and bounces set on the sky reach the environment
func test_the_sdfgi_light_reaches_the_environment() -> void:
    var environment:Environment = get_viewport().find_world_3d().environment

    _sky.sdfgi_energy = SDFGI_ENERGY
    _sky.sdfgi_bounce_feedback = SDFGI_BOUNCE_FEEDBACK
    await wait_idle_frames(APPLY_FRAMES)

    assert_almost_eq(environment.sdfgi_energy, SDFGI_ENERGY, 0.001)
    assert_almost_eq(environment.sdfgi_bounce_feedback, SDFGI_BOUNCE_FEEDBACK, 0.001)


## The auto exposure set on the sky reaches the world's camera attributes, the day's at noon and the
## night's at midnight - a night all in fog is not kept as bright as a day in it
func test_the_auto_exposure_is_the_day_s_or_the_night_s() -> void:
    var time_of_day:float = SimulationServer.time_of_day
    _sky.auto_exposure_enabled = true
    _sky.auto_exposure_speed = AUTO_EXPOSURE_SPEED
    _sky.day_auto_exposure_scale = DAY_AUTO_EXPOSURE_SCALE
    _sky.day_auto_exposure_min_sensitivity = DAY_AUTO_EXPOSURE_MIN_SENSITIVITY
    _sky.day_auto_exposure_max_sensitivity = DAY_AUTO_EXPOSURE_MAX_SENSITIVITY
    _sky.night_auto_exposure_scale = NIGHT_AUTO_EXPOSURE_SCALE
    _sky.night_auto_exposure_min_sensitivity = NIGHT_AUTO_EXPOSURE_MIN_SENSITIVITY
    _sky.night_auto_exposure_max_sensitivity = NIGHT_AUTO_EXPOSURE_MAX_SENSITIVITY
    _environment_node.current_time = NOON
    await wait_idle_frames(APPLY_FRAMES)

    var attributes:CameraAttributesPractical = get_viewport().find_world_3d().camera_attributes
    assert_not_null(attributes, "the sky's camera attributes are the world's")
    if not attributes:
        _environment_node.current_time = time_of_day
        return
    assert_true(attributes.auto_exposure_enabled)
    assert_almost_eq(attributes.auto_exposure_speed, AUTO_EXPOSURE_SPEED, 0.001)
    assert_almost_eq(attributes.auto_exposure_scale, DAY_AUTO_EXPOSURE_SCALE, 0.001)
    assert_almost_eq(attributes.auto_exposure_min_sensitivity, DAY_AUTO_EXPOSURE_MIN_SENSITIVITY, 0.001)
    assert_almost_eq(attributes.auto_exposure_max_sensitivity, DAY_AUTO_EXPOSURE_MAX_SENSITIVITY, 0.001)

    _environment_node.current_time = MIDNIGHT
    await wait_idle_frames(APPLY_FRAMES)

    assert_almost_eq(attributes.auto_exposure_scale, NIGHT_AUTO_EXPOSURE_SCALE, 0.001)
    assert_almost_eq(attributes.auto_exposure_min_sensitivity, NIGHT_AUTO_EXPOSURE_MIN_SENSITIVITY, 0.001)
    assert_almost_eq(attributes.auto_exposure_max_sensitivity, NIGHT_AUTO_EXPOSURE_MAX_SENSITIVITY, 0.001)
    _environment_node.current_time = time_of_day


## The cloud cover washes the sky out to grey
func test_an_overcast_sky_is_grey() -> void:
    var material:ProceduralSkyMaterial = get_viewport().find_world_3d().environment.sky.sky_material
    _environment_node.cloudiness = 0.0
    _environment_node.precipitation = 0.0
    await wait_idle_frames(APPLY_FRAMES)
    var clear:float = material.sky_top_color.s

    _environment_node.cloudiness = 1.0
    await wait_idle_frames(APPLY_FRAMES)

    assert_lt(material.sky_top_color.s, clear, "less blue")
    assert_almost_eq(material.sky_top_color.s, 0.0, 0.001, "grey under a full cover")


## The midnight's sky is the night's fog colour, not black - the sky and what the fog hides are of
## one tone (libmaszyna#330)
func test_the_night_sky_is_the_night_s_fog_colour() -> void:
    var material:ProceduralSkyMaterial = get_viewport().find_world_3d().environment.sky.sky_material
    var time_of_day:float = SimulationServer.time_of_day
    _environment_node.current_time = MIDNIGHT
    await wait_idle_frames(APPLY_FRAMES)

    assert_true(material.sky_top_color.is_equal_approx(_sky.night_fog_color))
    assert_true(material.sky_horizon_color.is_equal_approx(_sky.night_fog_color))
    assert_almost_eq(material.energy_multiplier, 1.0, 0.001, "not as dark as the night's light level")
    _environment_node.current_time = time_of_day


## The picture's adjustments and AgX contrast are the day's at noon and the night's at midnight
func test_the_adjustments_are_the_day_s_or_the_night_s() -> void:
    var environment:Environment = get_viewport().find_world_3d().environment
    var time_of_day:float = SimulationServer.time_of_day
    _sky.day_adjustment_brightness = ADJUSTED_BRIGHTNESS
    _sky.day_adjustment_contrast = ADJUSTED_CONTRAST
    _sky.day_adjustment_saturation = ADJUSTED_SATURATION
    _sky.day_tonemap_agx_contrast = DAY_AGX_CONTRAST
    _sky.night_adjustment_brightness = NIGHT_BRIGHTNESS
    _sky.night_tonemap_agx_contrast = NIGHT_AGX_CONTRAST
    _environment_node.current_time = NOON
    await wait_idle_frames(APPLY_FRAMES)

    assert_true(environment.adjustment_enabled)
    assert_almost_eq(environment.adjustment_brightness, ADJUSTED_BRIGHTNESS, 0.001)
    assert_almost_eq(environment.adjustment_contrast, ADJUSTED_CONTRAST, 0.001)
    assert_almost_eq(environment.adjustment_saturation, ADJUSTED_SATURATION, 0.001)
    assert_almost_eq(environment.tonemap_agx_contrast, DAY_AGX_CONTRAST, 0.001)

    _environment_node.current_time = MIDNIGHT
    await wait_idle_frames(APPLY_FRAMES)

    assert_almost_eq(environment.adjustment_brightness, NIGHT_BRIGHTNESS, 0.001)
    assert_almost_eq(environment.tonemap_agx_contrast, NIGHT_AGX_CONTRAST, 0.001)
    _environment_node.current_time = time_of_day
