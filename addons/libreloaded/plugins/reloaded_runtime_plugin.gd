@tool
extends Node
class_name ReloadedRuntimePlugin

## A plugin's part of the game (ReloadedPlugin.runtime_script), as an EditorPlugin is its part of the
## editor: ReloadedPlugins puts it in the tree before the main scene, so its _enter_tree() makes and
## registers the providers it gives the core (VehicleCatalogServer.provider_register(), ...) before
## anything of the scene asks for them, and its _exit_tree() unregisters them.
