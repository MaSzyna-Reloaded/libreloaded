extends ReloadedGutTest

## The weather window's wind in metres a second: the slider sets the environment's wind to it, and
## shows the environment's

const WEATHER_CONTROLS:PackedScene = preload("res://addons/libreloaded/debug_hud/weather_controls.tscn")


func test_the_wind_in_metres_a_second() -> void:
    var environment:EnvironmentNode = EnvironmentNode.new()
    add_child_autofree(environment)
    var controls:Node = WEATHER_CONTROLS.instantiate()
    add_child_autofree(controls)
    controls.attach_environment(environment)
    var slider:HSlider = controls.find_child("WindSlider", true, false) as HSlider
    slider.value = 10.0
    assert_almost_eq(environment.get_wind_speed(), 10.0, 0.001)
    assert_eq((controls.find_child("WindValueLabel", true, false) as Label).text, "10.0 m/s")
    environment.wind_strength = 1.0
    controls._process_dirty()
    assert_almost_eq(slider.value, EnvironmentNode.WIND_SPEED_MAX, 0.05, "the environment's shown")
