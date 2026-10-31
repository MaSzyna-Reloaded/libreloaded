extends ReloadedGutTest

## SimulationServer.weather_friction_factor - the rails' grip a weather simulation works out of its
## weather, in place of the one the weather's kind gives (simulation_get_weather_friction_factor())

## A rain's onset on a dirty rail
const SIMULATED_FACTOR:float = 0.65

var _weather:SimulationServer.Weather
var _factor:float


func before_each() -> void:
    _weather = SimulationServer.weather
    _factor = SimulationServer.weather_friction_factor


func after_each() -> void:
    SimulationServer.weather = _weather
    SimulationServer.weather_friction_factor = _factor


func test_none_set_the_weathers_own() -> void:
    SimulationServer.weather_friction_factor = -1.0
    SimulationServer.weather = SimulationServer.WEATHER_RAIN
    assert_almost_eq(SimulationServer.simulation_get_weather_friction_factor(), 0.85, 0.0001)
    SimulationServer.weather = SimulationServer.WEATHER_SNOW
    assert_almost_eq(SimulationServer.simulation_get_weather_friction_factor(), 0.75, 0.0001)
    SimulationServer.weather = SimulationServer.WEATHER_CLEAR
    assert_almost_eq(SimulationServer.simulation_get_weather_friction_factor(), 1.0, 0.0001)


func test_a_simulations_in_place_of_the_weathers() -> void:
    SimulationServer.weather = SimulationServer.WEATHER_RAIN
    SimulationServer.weather_friction_factor = SIMULATED_FACTOR
    assert_almost_eq(SimulationServer.simulation_get_weather_friction_factor(), SIMULATED_FACTOR, 0.0001)


func test_setting_it_announces_the_weather() -> void:
    SimulationServer.weather_friction_factor = -1.0
    watch_signals(SimulationServer)
    SimulationServer.weather_friction_factor = SIMULATED_FACTOR
    assert_signal_emitted(SimulationServer, "weather_changed")


func test_an_environment_starts_with_none() -> void:
    SimulationServer.weather_friction_factor = SIMULATED_FACTOR
    var environment:EnvironmentNode = EnvironmentNode.new()
    assert_lt(SimulationServer.weather_friction_factor, 0.0)
    environment.free()
