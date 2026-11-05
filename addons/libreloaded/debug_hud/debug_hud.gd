extends Control
class_name DebugHud

## The diagnostic windows of the player's vehicle and of the scenery - each a HUDWindow child of
## this node, closed at the start. Whoever has a menu for them (the game's top bar, a demo scene)
## fills its own PopupMenu with fill_menu() and connects the menu's index_pressed to
## menu_index_pressed() in its scene; entries it adds after them are its own. All the windows at
## once are shown and hidden by this node itself, on hud_toggle (F12) or windows_toggle().

## All the windows shown or hidden at once
signal windows_visibility_changed(shown: bool)

## The environment of the world being shown, for the weather and time window - a node of the scene
## or one set when a world is made; null while there is no world (the menu)
@export var environment: EnvironmentNode = null:
    set(value):
        environment = value
        if is_node_ready():
            _attach_environment()

## Menu order snapshot - HUDWindow.move_to_front() reorders the children.
var _windows: Array[HUDWindow] = []
## The menu fill_menu() filled: its entries' windows are their metadata
var _menu: PopupMenu = null
## The menu entry of the developer console (Console), after the windows
var _console_index: int = -1
## Whether the last windows_toggle() showed the windows
var _windows_shown: bool = false


func _ready() -> void:
    PlayerServer.player_vehicle_changed.connect(_on_player_vehicle_changed)
    for child: Node in get_children():
        var win: HUDWindow = child as HUDWindow
        win.visible = false
        _windows.append(win)
    _attach_environment()


func _exit_tree() -> void:
    PlayerServer.player_vehicle_changed.disconnect(_on_player_vehicle_changed)


## A game's own window in place of one of these (its own weather window), by its name: in its place
## among them, its menu entry and the toggles, with its toggle action unless it has its own; the one
## replaced freed. A window with attach_environment() is handed the environment, as the weather
## window is.
func window_replace(window_name: StringName, window: HUDWindow) -> void:
    var old: HUDWindow = get_node_or_null(NodePath(window_name)) as HUDWindow
    if not old:
        return
    var index: int = _windows.find(old)
    var child_index: int = old.get_index()
    remove_child(old)
    old.queue_free()
    window.name = window_name
    window.visible = old.visible
    if window.toggle_action.is_empty():
        window.toggle_action = old.toggle_action
    add_child(window)
    move_child(window, child_index)
    _windows[index] = window
    if _menu:
        for entry: int in _menu.item_count:
            if _menu.get_item_metadata(entry) == old:
                _menu.set_item_metadata(entry, window)
                _menu.set_item_text(entry, window.title)
                if not window.toggle_action.is_empty():
                    _menu.set_item_shortcut(entry, ActionShortcut.create(window.toggle_action))
    _attach_environment()


## The weather window's environment - this scene's own window's controls, or a game's window in its
## place (window_replace())
func _attach_environment() -> void:
    var weather: Node = get_node_or_null(^"WeatherAndTime")
    if weather and weather.has_method(&"attach_environment"):
        weather.attach_environment(environment)
    elif weather:
        %WeatherControls.attach_environment(environment)


## The windows and the developer console as entries of `menu`, with their keys: a window's entry
## carries its window (its metadata), so entries others add before or after are theirs
func fill_menu(menu: PopupMenu) -> void:
    _menu = menu
    # an entry a window, its window its metadata and its key its toggle action (the Tab of the mini
    # map as the original's map panel, driveruilayer.cpp:136)
    for win: HUDWindow in _windows:
        menu.add_item(win.title)
        var entry: int = menu.item_count - 1
        menu.set_item_metadata(entry, win)
        if not win.toggle_action.is_empty():
            menu.set_item_shortcut(entry, ActionShortcut.create(win.toggle_action))
    menu.add_separator()
    menu.add_item("Developer console")
    _console_index = menu.item_count - 1
    menu.set_item_shortcut(_console_index, ActionShortcut.create(&"console_toggle"))


## An entry of the menu filled by fill_menu() picked: its window shown or hidden, the console
## toggled; an entry past them is not this node's
func menu_index_pressed(index: int) -> void:
    if index == _console_index:
        Console.toggle_console()
        return
    if not _menu or index < 0 or index >= _menu.item_count:
        return
    var win: HUDWindow = _menu.get_item_metadata(index) as HUDWindow
    if not win or not _windows.has(win):
        return
    win.visible = not win.visible
    _propagate_vehicle(win, PlayerServer.player_get_vehicle())


func _unhandled_input(event: InputEvent) -> void:
    if event.is_action_pressed(&"hud_toggle", false, true):
        windows_toggle()
        get_viewport().set_input_as_handled()


## All the windows shown, or all hidden when the last toggle showed them
func windows_toggle() -> void:
    _windows_shown = not _windows_shown
    for win: HUDWindow in _windows:
        win.visible = _windows_shown
        _propagate_vehicle(win, PlayerServer.player_get_vehicle())
    windows_visibility_changed.emit(_windows_shown)


## The vehicle the player is driving, handed to every widget that shows something about it. The
## widgets used to be given a NodePath into the vehicle's own subtree and resolve it themselves,
## which reached across two scenes and could resolve before the vehicle had been built. They are
## given the vehicle itself now, and PlayerServer says when it changes.
## Each control and section picks the vehicle of its own target from it, as a cab control does.
func _on_player_vehicle_changed(vehicle: RID, _previous: RID) -> void:
    for win: HUDWindow in _windows:
        _propagate_vehicle(win, vehicle)


func _propagate_vehicle(node: Node, vehicle: RID) -> void:
    for child: Node in node.get_children():
        if "vehicle" in child:
            child.vehicle = vehicle
        _propagate_vehicle(child, vehicle)
