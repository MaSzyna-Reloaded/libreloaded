extends ReloadedGutTest

## SimulationServer.sun_get_direction() - where the sun stands at the environment's place, date and
## clock: east in the morning, south at noon of the northern hemisphere, west in the evening and
## below the horizon at night; +X east, -Z north (as the sky backends draw it).

## Katowice, a summer day, on the time zone's meridian so the clock is the solar time
const LATITUDE:float = 50.271
const TIMEZONE_OFFSET:int = 1
const LONGITUDE:float = 15.0
const MORNING:float = 8.0
const NOON:float = 12.0
const EVENING:float = 16.0
const MIDNIGHT:float = 0.0

var _latitude:float
var _longitude:float
var _timezone_offset:int
var _time_of_day:float
var _date:Vector3i


func before_each() -> void:
    _latitude = SimulationServer.latitude
    _longitude = SimulationServer.longitude
    _timezone_offset = SimulationServer.timezone_offset
    _time_of_day = SimulationServer.time_of_day
    _date = Vector3i(SimulationServer.date_get_year(), SimulationServer.date_get_month(), SimulationServer.date_get_day())
    SimulationServer.latitude = LATITUDE
    SimulationServer.longitude = LONGITUDE
    SimulationServer.timezone_offset = TIMEZONE_OFFSET
    SimulationServer.date_set(2026, 6, 21)


func after_each() -> void:
    SimulationServer.latitude = _latitude
    SimulationServer.longitude = _longitude
    SimulationServer.timezone_offset = _timezone_offset
    SimulationServer.time_of_day = _time_of_day
    SimulationServer.date_set(_date.x, _date.y, _date.z)


func _sun_at(hours:float) -> Vector3:
    SimulationServer.time_of_day = hours
    return SimulationServer.sun_get_direction()


func test_the_sun_rises_in_the_east_and_sets_in_the_west() -> void:
    assert_gt(_sun_at(MORNING).x, 0.0, "the morning sun is east")
    assert_lt(_sun_at(EVENING).x, 0.0, "the evening sun is west")


func test_the_noon_sun_stands_highest_in_the_south() -> void:
    var noon:Vector3 = _sun_at(NOON)
    assert_gt(noon.z, 0.0, "south")
    assert_almost_eq(noon.x, 0.0, 0.01, "neither east nor west")
    assert_gt(noon.y, _sun_at(MORNING).y, "higher than in the morning")


func test_the_sun_is_below_the_horizon_at_midnight() -> void:
    assert_lt(_sun_at(MIDNIGHT).y, 0.0)
