@tool
extends EditorPlugin

# Custom nodes
const PLUGIN_NAME = "libreloaded"

var environment_node_script = preload("res://addons/libreloaded/environment/environment_node.gd")
var environment_node_icon = preload("res://addons/libreloaded/environment/environment_node_icon.png")
var track_3d_script = preload("res://addons/libreloaded/tracks/track_3d.gd")
var track_normal_3d_script = preload("res://addons/libreloaded/tracks/track_normal_3d.gd")
var track_switch_3d_script = preload("res://addons/libreloaded/tracks/track_switch_3d.gd")

func _enable_plugin():
    add_autoload_singleton("ReloadedPlugins", "res://addons/libreloaded/plugins/reloaded_plugins.gd")
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
    remove_autoload_singleton("ReloadedPlugins")

func _enter_tree():
    ReloadedProjectSettings.register()

