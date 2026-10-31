extends ReloadedGutTest

## DebugHud: a window's menu entry carries the window and its key (HUDWindow.toggle_action), and
## window_replace() puts a game's own window in place of one of the DebugHud's - its entry, its key
## and the toggles its own, the environment handed to it

const DEBUG_HUD:PackedScene = preload("res://addons/libreloaded/debug_hud/debug_hud.tscn")


class GameWeatherWindow:
    extends HUDWindow
    var environment_node:EnvironmentNode = null

    func attach_environment(environment:EnvironmentNode) -> void:
        environment_node = environment


func test_a_games_window_in_the_weather_windows_place() -> void:
    var hud:DebugHud = DEBUG_HUD.instantiate() as DebugHud
    add_child_autofree(hud)
    var menu:PopupMenu = autofree(PopupMenu.new())
    hud.fill_menu(menu)
    var old:HUDWindow = hud.get_node("WeatherAndTime") as HUDWindow
    var index:int = old.get_index()
    var window:GameWeatherWindow = GameWeatherWindow.new()
    hud.window_replace(&"WeatherAndTime", window)
    assert_eq(hud.get_node("WeatherAndTime"), window)
    assert_eq(window.get_index(), index, "in the same place")
    # its menu entry: the one the weather window's key is on
    var environment:EnvironmentNode = autofree(EnvironmentNode.new())
    hud.environment = environment
    assert_eq(window.environment_node, environment)
    var found:bool = false
    for entry:int in menu.item_count:
        if menu.get_item_metadata(entry) == window:
            found = true
            assert_eq(window.toggle_action, &"toggle_weather_controls", "the weather window's key")
            assert_not_null(menu.get_item_shortcut(entry))
            window.visible = false
            hud.menu_index_pressed(entry)
            assert_true(window.visible, "its entry shows it")
    assert_true(found, "its menu entry")
    hud.windows_toggle()
    assert_true(window.visible, "shown with all the windows")


func test_a_windows_entry_its_own_whatever_comes_before() -> void:
    var hud:DebugHud = DEBUG_HUD.instantiate() as DebugHud
    add_child_autofree(hud)
    var menu:PopupMenu = autofree(PopupMenu.new())
    menu.add_item("Someone else's")
    hud.fill_menu(menu)
    var weather:HUDWindow = hud.get_node("WeatherAndTime") as HUDWindow
    for entry:int in menu.item_count:
        if menu.get_item_metadata(entry) == weather:
            weather.visible = false
            hud.menu_index_pressed(entry)
            assert_true(weather.visible)
    hud.menu_index_pressed(0)
