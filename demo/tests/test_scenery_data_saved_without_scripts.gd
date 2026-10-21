extends ReloadedGutTest

## The scenery's data saved names no script (CODE_STYLE.md, "Saved data names no script"): its
## classes are the core's C++ ones, written by their names, and read back as they were

const SAVED:String = "user://gut/scenery_data.tres"
const TRACK_NAME:String = "t1"
const MODEL_NAME:String = "semaphore"
const VEHICLE_ID:String = "ep07-424"


func after_each() -> void:
    DirAccess.remove_absolute(SAVED)


func _scenery() -> CompiledScenery:
    var compiled:CompiledScenery = CompiledScenery.new()
    var track:TrackData = TrackData.new()
    track.name = TRACK_NAME
    track.surface = TrackData.SURFACE_GRAVEL
    track.curve = TrackCurve.new()
    track.curve.p2 = Vector3(0.0, 0.0, 100.0)
    compiled.tracks.append(track)
    var model:ModelData = ModelData.new()
    model.name = MODEL_NAME
    model.signal_head_kind = SignalHeadKind.new()
    compiled.models.append(model)
    var trainset:TrainsetData = TrainsetData.new()
    var vehicle:VehicleData = VehicleData.new()
    vehicle.vehicle_id = VEHICLE_ID
    vehicle.driver_type = VehicleData.DRIVER_HEAD
    trainset.vehicles.append(vehicle)
    compiled.trainsets.append(trainset)
    compiled.traction.append(TractionData.new())
    compiled.power_sources.append(PowerSourceData.new())
    compiled.lines.append(LinesData.new())
    compiled.sounds.append(SoundData.new())
    compiled.stations.append(StationData.new())
    compiled.signal_boxes.append(SignalBoxData.new())
    return compiled


func test_saved_it_names_its_classes_and_no_script() -> void:
    DirAccess.make_dir_recursive_absolute(SAVED.get_base_dir())
    assert_eq(ResourceSaver.save(_scenery(), SAVED), OK)

    var text:String = FileAccess.get_file_as_string(SAVED)
    assert_false(text.contains(".gd"), "no script's path")
    assert_false(text.contains("type=\"Script\""), "no script at all")
    for type:String in ["CompiledScenery", "TrackData", "TrackCurve", "ModelData", "SignalHeadKind", "TrainsetData",
            "VehicleData", "TractionData", "PowerSourceData", "LinesData", "SoundData", "StationData", "SignalBoxData"]:
        assert_true(text.contains("type=\"%s\"" % type), "%s by its class name" % type)


func test_read_back_it_is_what_was_saved() -> void:
    DirAccess.make_dir_recursive_absolute(SAVED.get_base_dir())
    ResourceSaver.save(_scenery(), SAVED)

    var read:CompiledScenery = ResourceLoader.load(SAVED, "", ResourceLoader.CACHE_MODE_IGNORE) as CompiledScenery
    assert_not_null(read)
    var track:TrackData = read.tracks[0]
    assert_eq(track.name, TRACK_NAME)
    assert_eq(track.surface, TrackData.SURFACE_GRAVEL)
    assert_eq(track.curve.p2, Vector3(0.0, 0.0, 100.0))
    assert_eq((read.models[0] as ModelData).name, MODEL_NAME)
    assert_not_null((read.models[0] as ModelData).signal_head_kind)
    var trainset:TrainsetData = read.trainsets[0]
    assert_eq(trainset.get_driver_vehicle().vehicle_id, VEHICLE_ID, "the driver's vehicle")
    assert_true(trainset.is_occupied())
