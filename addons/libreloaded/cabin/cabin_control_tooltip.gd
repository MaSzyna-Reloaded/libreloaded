extends PanelContainer
class_name CabinControlTooltip

## The caption of the cab control (CabinHUDMouseSystem) or the scenery model (SceneryHUDMouseServer)
## under the cursor, drawn next to the cursor as the original's tooltip is (uilayer.cpp:526): what
## the control is, what it shows now and the keys that work it - a key a game draws as an icon
## (hint_icon) by its icon. A scenery model has no state.

## Where the panel sits relative to the cursor, so the pointer does not cover the text
const CURSOR_OFFSET:Vector2 = Vector2(16.0, 20.0)
## Height of a key's icon among the names of the keys [px] - a line of the names
const HINT_ICON_HEIGHT:int = 18

## The icon of a key's name under the caption, `(hint:String, height:int) -> Texture2D`: a game that
## draws its pad's buttons sets it, and a name it gives no icon (null) stays text. Unset, every name
## is text
static var hint_icon:Callable = Callable()

@onready var _caption:Label = %Caption
@onready var _hints:Label = %Hints
## The names of the keys with their icons, in place of _hints while one of them has an icon
@onready var _hint_icons:HBoxContainer = %HintIcons
@onready var _state:Label = %State
## Where the cursor was last seen; a drag captures the mouse, and a captured mouse reports the
## centre of the window instead
var _cursor_position:Vector2 = Vector2.ZERO
## The scenery's hover with no state line - kept to disconnect the same callable
var _on_pickable_hovered:Callable = _on_control_hovered.bind("")


func _ready() -> void:
    hide()
    CabinHUDMouseSystem.control_hovered.connect(_on_control_hovered)
    CabinHUDMouseSystem.control_unhovered.connect(hide)
    CabinHUDMouseSystem.control_state_changed.connect(_on_control_state_changed)
    SceneryHUDMouseServer.pickable_hovered.connect(_on_pickable_hovered)
    SceneryHUDMouseServer.pickable_unhovered.connect(hide)


func _exit_tree() -> void:
    CabinHUDMouseSystem.control_hovered.disconnect(_on_control_hovered)
    CabinHUDMouseSystem.control_unhovered.disconnect(hide)
    CabinHUDMouseSystem.control_state_changed.disconnect(_on_control_state_changed)
    SceneryHUDMouseServer.pickable_hovered.disconnect(_on_pickable_hovered)
    SceneryHUDMouseServer.pickable_unhovered.disconnect(hide)


func _input(event:InputEvent) -> void:
    # a drag (CabinHUDMouseSystem) captures the mouse, and its motion then carries the centre of the
    # window - the tooltip stays where the cursor was until the drag puts the cursor back
    if visible and event is InputEventMouseMotion and Input.mouse_mode == Input.MOUSE_MODE_VISIBLE:
        _cursor_position = event.position
        _place()


func _on_control_hovered(caption:String, hints:String, state:String) -> void:
    _caption.text = caption
    _hints.text = hints
    # gone before the panel is measured again (reset_size())
    for piece:Node in _hint_icons.get_children():
        piece.free()
    var names:PackedStringArray = []
    var icons:Array[Texture2D] = []
    if hint_icon.is_valid():
        names = hints.split(BaseCabinTool3D.HINTS_SEPARATOR, false)
        for hint:String in names:
            icons.append(hint_icon.call(hint, HINT_ICON_HEIGHT))
    var with_icons:bool = icons.any(func(icon:Texture2D) -> bool: return not icon == null)
    if with_icons:
        for index:int in range(names.size()):
            if index > 0:
                _hint_icons.add_child(_hint_text(BaseCabinTool3D.HINTS_SEPARATOR.strip_edges()))
            if icons[index]:
                var icon:TextureRect = TextureRect.new()
                icon.texture = icons[index]
                icon.mouse_filter = Control.MOUSE_FILTER_IGNORE
                icon.size_flags_vertical = Control.SIZE_SHRINK_CENTER
                _hint_icons.add_child(icon)
            else:
                _hint_icons.add_child(_hint_text(names[index]))
    _hint_icons.visible = with_icons
    _hints.visible = not hints == "" and not with_icons
    _state.text = state
    _state.visible = not state == ""
    reset_size()
    if Input.mouse_mode == Input.MOUSE_MODE_VISIBLE:
        _cursor_position = get_viewport().get_mouse_position()
    _place()
    show()


## A name among the icons, drawn as the names' line is
func _hint_text(text:String) -> Label:
    var label:Label = _hints.duplicate()
    label.text = text
    label.visible = true
    label.unique_name_in_owner = false
    label.size_flags_vertical = Control.SIZE_SHRINK_CENTER
    return label


func _on_control_state_changed(state:String) -> void:
    _state.text = state
    _state.visible = not state == ""
    reset_size()
    # the new width has to be kept inside the screen too
    _place()


## Next to the cursor, kept inside the screen
func _place() -> void:
    position = (_cursor_position + CURSOR_OFFSET).min(get_viewport_rect().size - size)
