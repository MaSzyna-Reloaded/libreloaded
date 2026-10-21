@tool
extends EditorPlugin

# Custom nodes
const PLUGIN_NAME = "libreloaded"
const TRACTION: GDScript = preload("traction/traction_rendering_server.gd")

var environment_node_script = preload("res://addons/libreloaded/environment/environment_node.gd")
var environment_node_icon = preload("res://addons/libreloaded/environment/environment_node_icon.png")
var track_3d_script = preload("res://addons/libreloaded/tracks/track_3d.gd")
var track_normal_3d_script = preload("res://addons/libreloaded/tracks/track_normal_3d.gd")
var track_switch_3d_script = preload("res://addons/libreloaded/tracks/track_switch_3d.gd")

func _enable_plugin():
    add_autoload_singleton("Console", "res://addons/libreloaded/console/console.gd")
    add_autoload_singleton("VehicleCatalogServer", "res://addons/libreloaded/vehicles/vehicle_catalog_server.gd")
    add_autoload_singleton("TranscriptSystem", "res://addons/libreloaded/sound/transcript_system.gd")
    add_autoload_singleton("ScenerySoundServer", "res://addons/libreloaded/sound/scenery_sound_server.gd")
    add_autoload_singleton("CabinSystem", "res://addons/libreloaded/cabin/cabin_system.gd")
    add_autoload_singleton("WorldSceneryProvider", "res://addons/libreloaded/world/world_scenery_provider.gd")

    add_custom_type(
        "EnvironmentNode",
        "Node",
        environment_node_script,
        environment_node_icon,
    )

    add_custom_type(
        "Track3D",
        "VisualInstance3D",
        track_3d_script,
        null
    )

    add_custom_type(
        "TrackNormal3D",
        "VisualInstance3D",
        track_normal_3d_script,
        null
    )

    add_custom_type(
        "TrackSwitch3D",
        "VisualInstance3D",
        track_switch_3d_script,
        null
    )

    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scenery_toolbar", true)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/tracks", true)

func _disable_plugin():
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/scenery_toolbar", false)
    EditorInterface.set_plugin_enabled(PLUGIN_NAME + "/editor/tracks", false)

    remove_custom_type("EnvironmentNode")
    remove_custom_type("Track3D")
    remove_custom_type("TrackNormal3D")
    remove_custom_type("TrackSwitch3D")

    remove_autoload_singleton("WorldSceneryProvider")
    remove_autoload_singleton("ScenerySoundServer")
    remove_autoload_singleton("CabinSystem")
    remove_autoload_singleton("TranscriptSystem")
    remove_autoload_singleton("Console")
    remove_autoload_singleton("VehicleCatalogServer")

func _enter_tree():
    add_custom_project_setting("reloaded/scenery/track_curve_bake_interval", 10.0, TYPE_FLOAT)
    # The original renders shadow maps with front faces culled (opengl33renderer.cpp:1758) against
    # self-shadowing acne; off by default like in Godot, because an open single-sided model then casts
    # no shadow and the acne only moves to the unlit side
    add_custom_project_setting("reloaded/lights/reverse_cull_face", false, TYPE_BOOL)
    # The cab gets a sun of its own with sharp shadows over its few metres, and the world's sun keeps
    # its far cascades in the cab view too; costs a second shadow map and a twice larger atlas
    add_custom_project_setting("reloaded/cabin/improve_shadows_quality", true, TYPE_BOOL)
    # ResourceLazyLoader: on, the models of a scenery, its terrain and a vehicle's models are loaded
    # when the camera comes near and let go when it leaves; off, they are all loaded with the
    # scenery and kept, and the streaming only builds and clears their instances. The
    # --enable-lazy-loading command-line switch turns it on (read at startup)
    add_custom_project_setting("reloaded/resources/lazy_loading", true, TYPE_BOOL)
    # MaszynaE3DRenderingServer streams registered scenery models in and out around the camera; this caps
    # every node's own range and stands in for the nodes that declare none (read at startup)
    add_custom_project_setting(
        "reloaded/scenery/draw_distance", 20000.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,50000.0,10.0,suffix:m"
    )
    # The game's settings (its JSON, UserSettings.DECLARATIONS_SETTING): only the project settings
    # it declares a player's are taken from the player's settings.cfg (read at startup)
    add_custom_project_setting("reloaded/user_settings/declarations", "", TYPE_STRING, PROPERTY_HINT_FILE, "*.json")
    # How far the tracks, the roads and the traction are drawn (TrackRenderingServer.tracks_distance),
    # a player's setting under the draw distance (read as the game starts)
    add_custom_project_setting(
        "reloaded/scenery/tracks/distance", TrackRenderingServer.TRACKS_DISTANCE, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,20000.0,10.0,suffix:m"
    )
    # The backend a track whose data names none is drawn by, by the name it is registered under
    # (TrackRenderingServer.BACKEND_SETTING): the core's StandardTrackRenderingBackend
    add_custom_project_setting("reloaded/tracks/backend", &"standard", TYPE_STRING_NAME)
    # The assets StandardTrackRenderingBackend draws the tracks with (a TrackRenderingLook)
    add_custom_project_setting(
        "reloaded/tracks/look", "res://addons/libreloaded/tracks/track_look.tres", TYPE_STRING,
        PROPERTY_HINT_FILE, "*.tres,*.res"
    )
    # ...and the traction (TractionRenderingServer.set_rendering_backend())
    add_custom_project_setting(TRACTION.RENDERING_BACKEND_SETTING, TRACTION.PLAIN_BACKEND, TYPE_STRING_NAME)
    # How long the simulation's running speed takes to reach one set, like a tape's motor
    # (SimulationServer.simulation_get_current_speed())
    add_custom_project_setting(
        "reloaded/simulation/speed_change_time", 0.4, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,5.0,0.05,suffix:s"
    )
    # How close and how far the zoom takes the external view (Shift+F4) from what it looks at
    add_custom_project_setting(
        "reloaded/camera/external_view_min_distance", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,20.0,0.1,suffix:m"
    )
    add_custom_project_setting(
        "reloaded/camera/external_view_max_distance", 200.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "10.0,2000.0,10.0,suffix:m"
    )
    # A vehicle to follow farther than this from the view is not flown to: the view jumps beside it
    # (the vehicle card's crosshair) and follows from there
    add_custom_project_setting(
        "reloaded/camera/follow_jump_distance", 10000.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,100000.0,100.0,suffix:m"
    )
    # Skydome's volumetric fog volume is 8 m deep by day and 3 m at night, and it is measured from
    # the camera - a light shaft is only visible while its lamp is inside it. This stretches the
    # volume while the density is divided by the same factor, which leaves the optical depth, and
    # so the look of the fog, alone. At 24 that is 72 m by night and 192 m by day.
    add_custom_project_setting(
        "reloaded/weather/fog/volumetric_length_scale", 24.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "1.0,64.0,0.5"
    )
    # Floor under fog_volumetric_far_falloff. A scenery asking for a fog of kilometres drives that
    # falloff to 0.01-0.04 and leaves no haze by the camera at all, so a street lamp has nothing to
    # scatter in and casts no visible shaft. Night air is never that clean.
    add_custom_project_setting(
        "reloaded/weather/fog/volumetric_minimum", 0.25, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    # Sun altitude between which the light level ramps from night to full day; a scenery light set
    # to come on automatically lights below a level of 0.325 (AnimModel.cpp:598), which lands about
    # 1.4 degrees below the horizon on this ramp. A winter noon sun peaks at 16-19 degrees at 50 N,
    # so the day end must stay well below that (FINDINGS.md).
    add_custom_project_setting(
        "reloaded/lights/night_altitude", -6.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "-18.0,0.0,0.5,suffix:°"
    )
    add_custom_project_setting(
        "reloaded/lights/day_altitude", 6.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,15.0,0.5,suffix:°"
    )
    # The distance a vehicle stops rendering from a node hierarchy at and switches to
    # RenderingServer instances; it switches back 25% closer. 350 m is where MaszynaE3DNodesBackend has
    # already faded its spotlights out completely (distance_fade_begin 150 + length 200), and the
    # OPTIMIZED backend's own lights are streamed by scenery_light_distance instead.
    add_custom_project_setting(
        "reloaded/vehicles/detail_distance", 350.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "50.0,10000.0,10.0,suffix:m"
    )
    # SmokeServer: with it off no emitter is built, a vehicle's or the scenery's
    add_custom_project_setting("reloaded/smoke/enabled", true, TYPE_BOOL)
    # The flipbook of the core's own smoke - the core ships none, the project using it fills this in
    add_custom_project_setting(
        "reloaded/smoke/modern/atlas", "", TYPE_STRING, PROPERTY_HINT_FILE, "*.png,*.webp,*.dds"
    )
    add_custom_project_setting("reloaded/smoke/modern/atlas_frames", Vector2i(4, 4), TYPE_VECTOR2I)
    # Every knob below comes in a pair, one for a vehicle's emitter and one for the scenery's.
    #
    # How far from the camera a scenery emitter is built; a chimney is seen from further away than
    # a street lamp, so this is not the scenery lights' distance.
    add_custom_project_setting(
        "reloaded/smoke/dynamic/distance", 1500.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,10000.0,10.0,suffix:m"
    )
    add_custom_project_setting(
        "reloaded/smoke/static/distance", 1500.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,10000.0,10.0,suffix:m"
    )
    # How many particles an emitter spawns a second, over what its source asks for. Each one is
    # made correspondingly fainter, so a denser plume comes out smoother rather than darker.
    add_custom_project_setting(
        "reloaded/smoke/dynamic/density", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.25,8.0,0.25"
    )
    add_custom_project_setting(
        "reloaded/smoke/static/density", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.25,8.0,0.25"
    )
    # How long a particle lives, over what its source makes of it. A slowly fading chimney keeps a
    # particle in the air for a minute - far more than a scenery prop needs, and it is also what
    # decides how many of them are in flight at once.
    add_custom_project_setting(
        "reloaded/smoke/dynamic/lifetime", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,4.0,0.05"
    )
    add_custom_project_setting(
        "reloaded/smoke/static/lifetime", 0.5, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,4.0,0.05"
    )
    # The particles one emitter holds at most. It has to leave room for the density above.
    add_custom_project_setting(
        "reloaded/smoke/dynamic/max_particles", 2000, TYPE_INT,
        PROPERTY_HINT_RANGE, "16,8000,1"
    )
    add_custom_project_setting(
        "reloaded/smoke/static/max_particles", 500, TYPE_INT,
        PROPERTY_HINT_RANGE, "16,8000,1"
    )
    add_custom_project_setting("reloaded/debug/physics_diagnostics", false, TYPE_BOOL)
    # off, no ResourceCache is read or written (the tests' hooks switch it off)
    add_custom_project_setting("reloaded/cache/enabled", true, TYPE_BOOL)
    add_custom_project_setting(
        "reloaded/weather/wind_turbulence", 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,4.0,0.01,or_greater"
    )
    add_custom_project_setting(SkyEnvironment.SHADOW_SCENERY_ENABLED_SETTING, true, TYPE_BOOL)
    add_custom_project_setting(SkyEnvironment.SHADOW_CABIN_ENABLED_SETTING, true, TYPE_BOOL)
    add_custom_project_setting(
        SkyEnvironment.SHADOW_SCENERY_MODE_SETTING, DirectionalLight3D.SHADOW_PARALLEL_4_SPLITS,
        TYPE_INT, PROPERTY_HINT_ENUM, "Orthogonal,PSSM 2 Splits,PSSM 4 Splits"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_CABIN_MODE_SETTING, DirectionalLight3D.SHADOW_PARALLEL_2_SPLITS,
        TYPE_INT, PROPERTY_HINT_ENUM, "Orthogonal,PSSM 2 Splits,PSSM 4 Splits"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_SCENERY_BLUR_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.01,or_greater"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_CABIN_BLUR_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.01,or_greater"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_SCENERY_OPACITY_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_CABIN_OPACITY_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_SCENERY_BIAS_SETTING, 0.1, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.001,or_greater"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_CABIN_BIAS_SETTING, 0.1, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.001,or_greater"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_SCENERY_NORMAL_BIAS_SETTING, 10.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,32.0,0.001,or_greater"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_CABIN_NORMAL_BIAS_SETTING, 5.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,32.0,0.001,or_greater"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_SCENERY_MAX_DISTANCE_SETTING, 100.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10000.0,1.0,suffix:m"
    )
    add_custom_project_setting(
        SkyEnvironment.SHADOW_CABIN_MAX_DISTANCE_SETTING, 150.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10000.0,1.0,suffix:m"
    )
    add_custom_project_setting(SkyEnvironment.SHADOW_SCENERY_BLEND_SPLITS_SETTING, true, TYPE_BOOL)
    add_custom_project_setting(SkyEnvironment.SHADOW_CABIN_BLEND_SPLITS_SETTING, true, TYPE_BOOL)
    for i: int in 3:
        add_custom_project_setting(
            SkyEnvironment.SHADOW_SCENERY_SPLIT_SETTINGS[i], SkyEnvironment.SHADOW_SCENERY_SPLITS[i],
            TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.001"
        )
        add_custom_project_setting(
            SkyEnvironment.SHADOW_CABIN_SPLIT_SETTINGS[i], SkyEnvironment.SHADOW_CABIN_SPLITS[i],
            TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.001"
        )
    add_custom_project_setting(
        SkyEnvironment.VOLUMETRIC_FOG_ENERGY_SETTING, 1.0, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,16.0,0.001,or_greater"
    )
    add_custom_project_setting(
        SkyEnvironment.FOG_CURVE_SETTING, SkyEnvironment.FOG_CURVE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_EXP_EASING, "positive_only"
    )
    add_custom_project_setting(
        SkyEnvironment.FOG_SKY_HEIGHT_SETTING, SkyEnvironment.FOG_SKY_HEIGHT_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "10.0,5000.0,10.0,or_greater,suffix:m"
    )
    add_custom_project_setting(
        SkyEnvironment.FOG_AERIAL_PERSPECTIVE_SETTING,
        SkyEnvironment.FOG_AERIAL_PERSPECTIVE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    add_custom_project_setting(
        SkyEnvironment.RAIN_FOG_DISTANCE_SETTING, SkyEnvironment.RAIN_FOG_DISTANCE_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "10.0,5000.0,10.0,or_greater,suffix:m"
    )
    add_custom_project_setting(
        SkyEnvironment.RAIN_FOG_DENSITY_SETTING, SkyEnvironment.RAIN_FOG_DENSITY_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    add_custom_project_setting(
        SkyEnvironment.FOG_VOLUMETRIC_FAR_FALLOFF_SETTING,
        SkyEnvironment.FOG_VOLUMETRIC_FAR_FALLOFF_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "1.0,4.0,0.05"
    )
    add_custom_project_setting(
        SkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_SETTING,
        SkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,4.0,0.01,or_greater"
    )
    add_custom_project_setting(
        SkyEnvironment.FOG_DAY_DISTANCE_FACTOR_SETTING,
        SkyEnvironment.FOG_DAY_DISTANCE_FACTOR_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.01,4.0,0.0001,or_greater"
    )
    add_custom_project_setting(
        SkyEnvironment.FOG_NIGHT_DISTANCE_FACTOR_SETTING,
        SkyEnvironment.FOG_NIGHT_DISTANCE_FACTOR_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.01,4.0,0.0001,or_greater"
    )
    # The wrapper's own .po files; ReloadedTranslationServer merges them with the game's catalogue into
    # the one translation TranslationServer uses, so they are not listed in Localization
    add_custom_project_setting(
        "reloaded/locale/translations", PackedStringArray(), TYPE_PACKED_STRING_ARRAY,
        PROPERTY_HINT_TYPE_STRING, "%d/%d:*.po" % [TYPE_STRING, PROPERTY_HINT_FILE]
    )

func add_custom_project_setting(name: String, default_value, type: int, hint: int = PROPERTY_HINT_NONE, hint_string: String = "") -> void:
    var setting_info: Dictionary = {
        "name": name,
        "type": type,
        "hint": hint,
        "hint_string": hint_string
    }

    # project.godot keeps only the value - the hint and the initial value are gone with every
    # editor restart, so a setting saved there still needs them registered again
    if not ProjectSettings.has_setting(name):
        ProjectSettings.set_setting(name, default_value)
    ProjectSettings.add_property_info(setting_info)
    ProjectSettings.set_initial_value(name, default_value)
