@tool
extends EditorInspectorPlugin

## The inspector of a scenery: "Load" and "Clear" stand side by side where the "Load" tool button
## (SceneryContentNode.load_action) would stand alone

const SceneryLoadButtons = preload("./scenery_load_buttons.gd")
const LOAD_BUTTONS:PackedScene = preload("./scenery_load_buttons.tscn")
## The property the controls above stand in place of
const LOAD_ACTION:String = "load_action"


func _can_handle(object:Object) -> bool:
    return object is SceneryContentNode


func _parse_property(
        object:Object, _type:Variant.Type, name:String, _hint_type:PropertyHint, _hint_string:String,
        _usage_flags:int, _wide:bool) -> bool:
    if name == LOAD_ACTION:
        var buttons:SceneryLoadButtons = LOAD_BUTTONS.instantiate()
        buttons.set_scenery(object as SceneryContentNode)
        add_custom_control(buttons)
        return true
    return false
