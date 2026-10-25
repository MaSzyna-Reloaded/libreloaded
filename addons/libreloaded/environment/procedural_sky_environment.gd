@tool
extends SkyEnvironment
class_name ProceduralSkyEnvironment

## The core's basic sky, for a project with no sky addon of its own: Godot's procedural sky as the
## editor previews it (Node3DEditor's preview environment, node_3d_editor_plugin.cpp:10203-10212)
## and one sun, standing where SimulationServer puts it at the environment's place, date and clock
## (sun_get_direction()), as bright as its light level; by night the sky is the night's fog colour.
## The scenery's fog, the cloud cover and the player's Graphics settings reach it; it has no clouds,
## rain or wind to draw.

const SUN_NAME: StringName = &"Sun"
## The editor's preview sky and ground (node_3d_editor_plugin.cpp:10235-10236)
const SKY_TOP_COLOR: Color = Color(0.385, 0.454, 0.55)
const GROUND_BOTTOM_COLOR: Color = Color(0.2, 0.169, 0.133)
## ...and its horizon: halfway between the two, then halfway to the grey of its luminance scaled
## (node_3d_editor_plugin.cpp:10205-10207)
const HORIZON_MIX: float = 0.5
const HORIZON_LUMINANCE_SCALE: float = 3.333
## The most of the view a fog covers for its extinction to be finite - a fog_density of 1 hides all
## past fog_distance, which no extinction per metre does
const FOG_OPACITY_MAX: float = 0.999
## The sun's disc as wide as the real sun's seen from the Earth [deg]
const SUN_ANGULAR_DIAMETER: float = 0.53
## How far from the sun's centre its glow fades out [deg] - ProceduralSkyMaterial's own 30 blooms into
## a stain over half the sky under the environment's glow
const SUN_GLOW_ANGLE: float = 5.0

var _sun: DirectionalLight3D = null
var _sky_material: ProceduralSkyMaterial = null


func _enter_tree() -> void:
    super()
    SimulationServer.light_level_changed.connect(_on_light_level_changed)


func _exit_tree() -> void:
    super()
    SimulationServer.light_level_changed.disconnect(_on_light_level_changed)


func create_sky() -> Sky:
    _sky_material = ProceduralSkyMaterial.new()
    _sky_material.sun_angle_max = SUN_GLOW_ANGLE
    var sky: Sky = Sky.new()
    sky.sky_material = _sky_material
    return sky


func create_nodes(world_environment: WorldEnvironment, _sky_environment: Environment) -> void:
    _sun = DirectionalLight3D.new()
    _sun.name = SUN_NAME
    _sun.light_angular_distance = SUN_ANGULAR_DIAMETER
    world_environment.add_child(_sun)


## The scenery's fog (EnvironmentNode): its fog_density reached at its fog_distance, growing from
## the camera (the depth fog, SkyEnvironment), the sky taking the share of it its height gives, and
## the volumetric fog covering over its volume as much of the view as the depth fog does at the
## volume's end (1 - exp(-extinction * length) = depth fog there) - an even extinction is thick right
## at the camera where the depth fog is thin
func apply_visual_configuration() -> void:
    _environment.fog_density = environment_node.fog_density
    _environment.fog_depth_begin = 0.0
    _environment.fog_depth_end = environment_node.fog_distance
    var volume: float = _environment.volumetric_fog_length
    var depth_fog_at_volume_end: float = environment_node.fog_density * pow(
            smoothstep(0.0, environment_node.fog_distance, volume), _environment.fog_depth_curve)
    _environment.volumetric_fog_density = -log(1.0 - minf(depth_fog_at_volume_end, FOG_OPACITY_MAX)) / volume
    _environment.fog_sky_affect = get_fog_sky_share(environment_node.fog_distance) * environment_node.fog_density


## The sun as bright as SimulationServer's light level - dark at night, dimmed by clouds - times its
## sun_energy; the sky by day as the editor previews it, washed out to grey by the cloud cover and the
## rain, as bright as the light level, and by night the night's fog colour, blended by the sun's
## height (get_day_share()) - so the sky and what the fog hides are of one tone at night
func apply_light_configuration() -> void:
    _apply_sun_settings(_sun)
    _sun.light_energy = SimulationServer.light_level * sun_energy
    var overcast: float = clampf(maxf(environment_node.cloudiness, environment_node.precipitation), 0.0, 1.0)
    var horizon: Color = SKY_TOP_COLOR.lerp(GROUND_BOTTOM_COLOR, HORIZON_MIX)
    var horizon_luminance: float = horizon.get_luminance() * HORIZON_LUMINANCE_SCALE
    horizon = horizon.lerp(Color(horizon_luminance, horizon_luminance, horizon_luminance), HORIZON_MIX)
    var sky_grey: float = SKY_TOP_COLOR.get_luminance()
    var horizon_grey: float = horizon.get_luminance()
    var day: float = get_day_share()
    _sky_material.sky_top_color = night_fog_color.lerp(
            SKY_TOP_COLOR.lerp(Color(sky_grey, sky_grey, sky_grey), overcast), day)
    _sky_material.sky_horizon_color = night_fog_color.lerp(
            horizon.lerp(Color(horizon_grey, horizon_grey, horizon_grey), overcast), day)
    _sky_material.ground_bottom_color = night_fog_color.lerp(GROUND_BOTTOM_COLOR, day)
    _sky_material.ground_horizon_color = _sky_material.sky_horizon_color
    _sky_material.energy_multiplier = lerpf(1.0, SimulationServer.light_level, day)


## The sun where SimulationServer puts it: the light shines along its -Z, from the sun
func apply_time_configuration() -> void:
    _sun.basis = Basis(Quaternion(Vector3.BACK, SimulationServer.sun_get_direction()))


## The sun itself moves with the ambient light, and the sky's colours with its height
func _on_sun_moved() -> void:
    super()
    _dirty_time = true
    _dirty_lights = true


func _on_light_level_changed(_light_level: float) -> void:
    _dirty_lights = true
