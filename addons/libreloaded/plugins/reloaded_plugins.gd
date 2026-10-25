@tool
extends Node

## The plugins' parts of the game (ReloadedRuntimePlugin), as the editor holds its EditorPlugins: an
## autoload, which Godot puts in the tree with the main scene in one pass - every _enter_tree() before
## any _ready(), the autoloads first - so its children enter it before anything of the scene does.
## Made here, the plugins' providers are registered before the scene's vehicles ask for them.


func _init() -> void:
    for plugin:ReloadedPlugin in ReloadedPluginServer.plugin_get_list():
        if plugin.runtime_script:
            add_child((load(plugin.runtime_script) as Script).new() as ReloadedRuntimePlugin)
