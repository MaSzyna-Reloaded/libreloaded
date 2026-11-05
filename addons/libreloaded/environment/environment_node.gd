@tool
extends Node
class_name EnvironmentNode

## Group the player sets cabin_view on when switching between the cabin and the exterior view
const GROUP: StringName = &"reloaded_environment"
## Render layer of the cab interior - with reloaded/cabin/improve_shadows_quality the cab is lit by a
## sun of its own that reaches only this layer (SkyEnvironment._create_cabin_light())
const CABIN_RENDER_LAYER: int = 1 << 18
## Wind speed the 0-1 wind_strength maps onto, m/s
const WIND_SPEED_MIN: float = 0.15
const WIND_SPEED_MAX: float = 3.0
## The overcast runs 0-1 for the cloud cover and on up to 2 for precipitation; a precipitation of
## OVERCAST_FULL_PRECIPITATION is an overcast of 2
const OVERCAST_MAX: float = 2.0
const OVERCAST_FULL_PRECIPITATION: float = 0.4
## The original's fog range with the fog switched off - the longest a scenery may declare
## (simulationstateserializer.cpp:216)
const FOG_RANGE_MAX: float = 25000.0
const WEATHER_PRESETS: Dictionary = {
    SimulationServer.WEATHER_CLEAR: {
        "precipitation": 0.0, "cloudiness": 0.1, "fog_density": 0.075, "wind_strength": 0.2,
    },
    SimulationServer.WEATHER_CLOUDY: {
        "precipitation": 0.0, "cloudiness": 0.7, "fog_density": 0.15, "wind_strength": 0.4,
    },
    SimulationServer.WEATHER_RAIN: {
        "precipitation": 0.8, "cloudiness": 0.9, "fog_density": 0.3, "wind_strength": 0.6,
    },
    SimulationServer.WEATHER_SNOW: {
        "precipitation": 0.0, "cloudiness": 0.9, "fog_density": 0.3, "wind_strength": 0.5,
    },
}

## The environment applied a change of its configuration - a preset, a scenery's own declarations,
## the cabin view or a property written from anywhere. Whoever shows this state (a
## SkyEnvironment) reacts to this instead of reading the node every frame; the running clock
## is deliberately not announced here.
signal configuration_changed

## The time, the date, the location, the cloud cover and the temperature are SimulationServer's -
## it runs the clock, rolls the date and works out the light level from the sun, and says so by its
## signals. This node only passes them through: the getters read the server, the setters write it,
## and what a backend needs of them (the model backend's lights, the shaders' light level, the
## materials' season) the node hands over when the server announces a change. Its defaults are the
## server's (SimulationServer DEFAULT_*), so a scene that keeps one stores nothing for it.

@export_category("Time")
@export var use_system_time: bool = false:
    set(value):
        SimulationServer.use_system_time = value
        _dirty_time = true
    get:
        return SimulationServer.use_system_time

@export_range(0.0, 23.9998) var current_time: float = 8.0:
    set(value):
        if not value == SimulationServer.time_of_day:
            SimulationServer.time_of_day = value
            _dirty_time = true
    get:
        return SimulationServer.time_of_day

@export_range(1, 31) var day: int = 3:
    set(value):
        if not value == SimulationServer.date_get_day():
            SimulationServer.date_set(SimulationServer.date_get_year(), SimulationServer.date_get_month(), value)
            _dirty_time = true
    get:
        return SimulationServer.date_get_day()

@export_range(1, 12) var month: int = 5:
    set(value):
        if not value == SimulationServer.date_get_month():
            SimulationServer.date_set(SimulationServer.date_get_year(), value, SimulationServer.date_get_day())
            _dirty_time = true
    get:
        return SimulationServer.date_get_month()

@export_range(0, 9999) var year: int = 2026:
    set(value):
        if not value == SimulationServer.date_get_year():
            SimulationServer.date_set(value, SimulationServer.date_get_month(), SimulationServer.date_get_day())
            _dirty_time = true
    get:
        return SimulationServer.date_get_year()

@export_range(-12, 14, 1) var timezone_offset: int = 1:
    set(value):
        SimulationServer.timezone_offset = value
        _dirty_time = true
    get:
        return SimulationServer.timezone_offset

## The fastest the simulation runs, as many times the wall clock
const MAX_SIMULATION_SPEED: float = 100.0

## How many times the wall clock the simulation runs - SimulationServer's; the node only sets it,
## from the scene and the inspector
@export_range(0.0, MAX_SIMULATION_SPEED) var simulation_speed: float = 1.0:
    set(value):
        SimulationServer.simulation_speed = clampf(value, 0.0, MAX_SIMULATION_SPEED)
    get:
        return SimulationServer.simulation_speed

@export_category("Location")
@export_range(-90.0, 90.0, 0.001, "suffix:°") var latitude: float = 50.271:
    set(value):
        if not value == SimulationServer.latitude:
            SimulationServer.latitude = value
            _dirty_time = true
    get:
        return SimulationServer.latitude

@export_range(-180.0, 180.0, 0.001, "suffix:°") var longitude: float = 19.04:
    set(value):
        if not value == SimulationServer.longitude:
            SimulationServer.longitude = value
            _dirty_time = true
    get:
        return SimulationServer.longitude

@export_category("Weather")
## Preset: changing it after the node is ready sets precipitation, cloudiness, fog density and
## wind strength (WEATHER_PRESETS); loading a scene keeps the saved values.
## SimulationServer's: the rails' grip follows it (simulation_get_weather_friction_factor())
@export var weather: SimulationServer.Weather = SimulationServer.WEATHER_CLEAR:
    set(value):
        if not value == weather:
            SimulationServer.weather = value
            _dirty_weather_preset = is_node_ready()
            _dirty_visuals = true
    get:
        return SimulationServer.weather

@export_range(0.0, 1.0, 0.01) var cloudiness: float = 0.5:
    set(value):
        SimulationServer.cloud_cover = value
        _dirty_visuals = true
    get:
        return SimulationServer.cloud_cover

## The scenery's rail friction multiplier (Global.fFriction) - a "friction" event of the scenario
## changes it as well
@export_range(0.0, 2.0, 0.01) var friction: float = 1.0:
    set(value):
        SimulationServer.friction = value
    get:
        return SimulationServer.friction

## Compass bearing the wind blows towards, in degrees. A plain angle rather than a vector: the
## weather backends and the particle emitters only ever need a horizontal direction.
@export_range(0.0, 360.0, 0.1, "suffix:°") var wind_direction: float = 135.0:
    set(value):
        wind_direction = wrapf(value, 0.0, 360.0)
        _dirty_visuals = true

@export_range(0.0, 10.0, 0.01) var wind_strength: float = 0.3:
    set(value):
        wind_strength = value
        _dirty_visuals = true

@export_range(0.0, 1.0, 0.01) var precipitation: float = 0.0:
    set(value):
        precipitation = value
        _dirty_visuals = true

## The ground starts under snow in winter, if the air is cold enough to keep it - the original's
## sceneries (SceneryContentNode), whose winter textures are snowed in
@export var winter_snow_cover: bool = false:
    set(value):
        winter_snow_cover = value
        _dirty_visuals = true

@export_range(-15.0, 45.0, 0.1, "suffix:°C") var temperature: float = 15.0:
    set(value):
        SimulationServer.air_temperature = value
        # what falls - snow, sleet or rain - follows it
        _dirty_visuals = true
    get:
        return SimulationServer.air_temperature

@export_group("Fog")
@export var fog_enabled: bool = true:
    set(value):
        fog_enabled = value
        _dirty_visuals = true

## How much of the view the fog covers at fog_distance: 0 - none, 1 - fully opaque. Whoever draws
## the sky may add its own share on top (day/night base fog, rain), so this is the scenery's part.
@export_range(0.0, 1.0, 0.001) var fog_density: float = 0.15:
    set(value):
        fog_density = value
        _dirty_visuals = true

## Distance the fog reaches fog_density at, growing linearly up to it. Whoever draws the sky may
## tell day from night (reloaded/weather/fog/day_distance_factor, night_distance_factor).
@export_range(10.0, 25000.0, 1.0, "suffix:m") var fog_distance: float = 470.0:
    set(value):
        fog_distance = value
        _dirty_visuals = true

var _dirty_time: bool = true
var _dirty_visuals: bool = true
var _dirty_weather_preset: bool = false
var _dirty_view: bool = false

## Cabin view (the player in a cab) - whoever draws the environment lets the cab light cast its
## shadows then (SkyEnvironment)
var cabin_view: bool = false:
    set(value):
        if not value == cabin_view:
            cabin_view = value
            _dirty_view = true


## A new environment starts from SimulationServer's defaults - the values its scene stores are set
## after this, through the properties
func _init() -> void:
    if not Engine.is_editor_hint():
        SimulationServer.environment_reset()


func _ready() -> void:
    update()
    _process_dirty()


## What the backends take of SimulationServer's environment they are handed now, and again
## whenever the server announces a change
func _enter_tree() -> void:
    add_to_group(GROUP)
    # the time of day passes while the environment is there (SimulationServer's clock)
    if not Engine.is_editor_hint():
        SimulationServer.clock_hold()
    ProjectSettings.settings_changed.connect(_on_project_settings_changed)
    SimulationServer.light_level_changed.connect(_on_light_level_changed)
    _on_light_level_changed(SimulationServer.light_level)


func _exit_tree() -> void:
    if not Engine.is_editor_hint():
        SimulationServer.clock_release()
    ProjectSettings.settings_changed.disconnect(_on_project_settings_changed)
    SimulationServer.light_level_changed.disconnect(_on_light_level_changed)


func _process(_delta: float) -> void:
    _process_dirty()


func update() -> void:
    _dirty_time = true
    _dirty_visuals = true


## A date past the end of its month or year carries over (the 32nd of January is the 1st of
## February) - SimulationServer.date_set()
func set_date(next_year: int, next_month: int, next_day: int) -> void:
    SimulationServer.date_set(next_year, next_month, next_day)
    _dirty_time = true


## The light level of the free spotlights' glare (types/free_spotlight_glare.gdshader)
func _on_light_level_changed(light_level: float) -> void:
    RenderingServer.global_shader_parameter_set("reloaded_light_level", light_level)


## Unit vector the wind blows along - horizontal, from the compass bearing. The original keeps one
## wind for the whole simulation (simulationenvironment.cpp:255-268) and the smoke emitters drift
## with it.
func get_wind_direction() -> Vector3:
    var bearing: float = deg_to_rad(wind_direction)
    return Vector3(cos(bearing), 0.0, sin(bearing))


## Wind speed in metres per second, wind_strength mapped onto WIND_SPEED_MIN..WIND_SPEED_MAX
func get_wind_speed() -> float:
    return lerpf(WIND_SPEED_MIN, WIND_SPEED_MAX, wind_strength)


func _process_dirty() -> void:
    var applied: bool = false

    if _dirty_weather_preset:
        _dirty_weather_preset = false
        var preset: Dictionary = WEATHER_PRESETS[weather]
        precipitation = preset["precipitation"]
        cloudiness = preset["cloudiness"]
        fog_density = preset["fog_density"]
        wind_strength = preset["wind_strength"]
        applied = true

    if _dirty_visuals:
        _dirty_visuals = false
        # what falls and blows is SimulationServer's; whatever draws it follows its weather_changed
        SimulationServer.precipitation = precipitation
        SimulationServer.set_wind(get_wind_speed(), get_wind_direction())
        # rain_params of the "rain_windscreen" materials (opengl33renderer.cpp:752-754): the share of
        # active droplets and the time they take to return after a wiper pass
        RenderingServer.global_shader_parameter_set("reloaded_rain_intensity", precipitation)
        RenderingServer.global_shader_parameter_set("reloaded_wiper_regen_time", lerpf(15.0, 1.0, precipitation))
        # the free spotlights' points and glare (types/free_spotlight*.gdshader) follow the
        # original's Global.Overcast and m_fogrange (opengl33renderer.cpp:5009), which fog_distance
        # is a multiple of
        RenderingServer.global_shader_parameter_set(
            "reloaded_overcast", clampf(cloudiness + precipitation / OVERCAST_FULL_PRECIPITATION, 0.0, OVERCAST_MAX))
        RenderingServer.global_shader_parameter_set(
            "reloaded_fog_range",
            fog_distance / float(ProjectSettings.get_setting(
                SkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_SETTING,
                SkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_DEFAULT))
            if fog_enabled
            else FOG_RANGE_MAX)
        applied = true

    if _dirty_view:
        _dirty_view = false
        applied = true

    if _dirty_time:
        _dirty_time = false
        applied = true

    if applied:
        configuration_changed.emit()


## The fog follows its project settings while the scenery runs
func _on_project_settings_changed() -> void:
    _dirty_visuals = true
