extends ReloadedGutTest

## SceneryServer keeps the stations of the sceneries loaded (station_list()) - their CompiledScenery
## goes with the load - and lets them go with the scenery's content

var _scenery: SceneryContentNode


func before_each() -> void:
    _scenery = SceneryContentNode.new()
    _scenery.autoload = false
    add_child(_scenery)


func after_each() -> void:
    _scenery.queue_free()
    await wait_idle_frames(1)


func test_a_loaded_scenery_lists_its_stations_until_its_content_goes() -> void:
    var compiled: CompiledScenery = CompiledScenery.new()
    var station: StationData = StationData.new()
    station.station_name = "Trzebinia"
    compiled.stations.append(station)
    watch_signals(SceneryServer)

    await SceneryServer.instantiate_compiled(_scenery, compiled, SceneryLoadMeasurement.new(_scenery), SceneryProvider.new())

    assert_true(SceneryServer.station_list().has(station), "the scenery's station is listed")
    assert_signal_emitted(SceneryServer, "station_list_changed")
    await _scenery.clear()
    assert_false(SceneryServer.station_list().has(station), "it goes with the scenery's content")
