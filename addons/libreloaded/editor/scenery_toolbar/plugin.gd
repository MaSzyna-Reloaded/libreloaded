@tool
extends EditorPlugin

## A scenery's inspector: "Load" and "Clear" side by side (scenery_load_inspector.gd)

const SceneryLoadInspector = preload("./scenery_load_inspector.gd")

var _load_inspector:SceneryLoadInspector = null


func _enter_tree() -> void:
    _load_inspector = SceneryLoadInspector.new()
    add_inspector_plugin(_load_inspector)


func _exit_tree() -> void:
    remove_inspector_plugin(_load_inspector)
    _load_inspector = null
