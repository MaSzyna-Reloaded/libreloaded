extends GutTest

## VehicleCatalogServer: the providers' vehicles by kind, a vehicle built, freed and read by its own
## provider, and the liveries - the provider's and the player's, which the catalogue keeps

const PROVIDER:StringName = &"test_catalog"
const OTHER_PROVIDER:StringName = &"test_catalog_other"
const LIVERIES_DIRECTORY:String = "user://test_vehicle_catalog_liveries"
const VEHICLE:String = "test/wagon"
const PROVIDER_LIVERY:String = "red"
const PLAYER_LIVERY:String = "mine"
## The side view the test provider shows of its vehicle, and its width between the couplers [px]
const SIDE_VIEW:Texture2D = preload("res://icon.svg")
const COUPLING_WIDTH:float = 120.0


## Vehicles of one kind, liveries of its own, and the vehicles it made
class TestProvider extends VehicleCatalogProvider:
    var category:VehicleInfo.VehicleCategory = VehicleInfo.VehicleCategory.FREIGHT_CAR
    var vehicle_name:String = VEHICLE
    var made:Dictionary[RID, VehicleData] = {}
    var freed:Array[RID] = []

    func vehicle_list(p_category:VehicleInfo.VehicleCategory) -> Array[VehicleInfo]:
        var vehicles:Array[VehicleInfo] = []
        if p_category == category:
            var info:VehicleInfo = VehicleInfo.new()
            info.vehicle_provider = name
            info.vehicle = vehicle_name
            info.category = category
            vehicles.append(info)
        return vehicles

    func livery_list(_vehicle:VehicleInfo) -> Array[VehicleLivery]:
        var livery:VehicleLivery = VehicleLivery.new()
        livery.name = PROVIDER_LIVERY
        var liveries:Array[VehicleLivery] = [livery]
        return liveries

    func vehicle_create(vehicle:VehicleData, _scene_node_id:int) -> RID:
        var vehicle_rid:RID = rid_from_int64(made.size() + 1)
        made[vehicle_rid] = vehicle
        return vehicle_rid

    func vehicle_free(vehicle:RID) -> void:
        freed.append(vehicle)

    func vehicle_get_data(vehicle:RID) -> VehicleData:
        return made.get(vehicle)

    func vehicle_get_side_view(vehicle:VehicleData) -> Texture2D:
        return SIDE_VIEW if vehicle.vehicle == vehicle_name else null

    func vehicle_get_side_view_coupling_width(_vehicle:VehicleData) -> float:
        return COUPLING_WIDTH


var _provider:TestProvider
var _other:TestProvider
var _previous_liveries_directory:Variant
var _previous_provider:Variant


func before_each() -> void:
    _previous_liveries_directory = ProjectSettings.get_setting(VehicleCatalogServer.LIVERIES_DIRECTORY_SETTING)
    _previous_provider = ProjectSettings.get_setting(VehicleCatalogServer.PROVIDER_SETTING)
    ProjectSettings.set_setting(VehicleCatalogServer.LIVERIES_DIRECTORY_SETTING, LIVERIES_DIRECTORY)
    _provider = TestProvider.new()
    _provider.name = PROVIDER
    _other = TestProvider.new()
    _other.name = OTHER_PROVIDER
    _other.category = VehicleInfo.VehicleCategory.PASSENGER_CAR
    VehicleCatalogServer.provider_register(PROVIDER, _provider.get_instance_id())
    VehicleCatalogServer.provider_register(OTHER_PROVIDER, _other.get_instance_id())


func after_each() -> void:
    VehicleCatalogServer.provider_unregister(PROVIDER)
    VehicleCatalogServer.provider_unregister(OTHER_PROVIDER)
    _provider.free()
    _other.free()
    var directory:String = LIVERIES_DIRECTORY.path_join(PROVIDER).path_join(VEHICLE)
    for file:String in DirAccess.get_files_at(directory):
        DirAccess.remove_absolute(directory.path_join(file))
    ProjectSettings.set_setting(VehicleCatalogServer.LIVERIES_DIRECTORY_SETTING, _previous_liveries_directory)
    ProjectSettings.set_setting(VehicleCatalogServer.PROVIDER_SETTING, _previous_provider)


## The test provider's vehicle, among the freight cars of every provider registered
func _vehicle_info() -> VehicleInfo:
    return VehicleCatalogServer.vehicle_list(VehicleInfo.VehicleCategory.FREIGHT_CAR).filter(
            func(info:VehicleInfo) -> bool: return info.vehicle_provider == PROVIDER)[0]


func _player_livery(livery_name:String, base:String) -> VehicleLivery:
    var livery:VehicleLivery = VehicleLivery.new()
    livery.name = livery_name
    livery.base = base
    return livery


func test_lists_the_vehicles_of_a_kind_of_every_provider() -> void:
    var freight:Array[VehicleInfo] = VehicleCatalogServer.vehicle_list(VehicleInfo.VehicleCategory.FREIGHT_CAR)
    var passenger:Array[VehicleInfo] = VehicleCatalogServer.vehicle_list(VehicleInfo.VehicleCategory.PASSENGER_CAR)

    assert_eq(freight.filter(func(info:VehicleInfo) -> bool: return info.vehicle_provider == PROVIDER).size(), 1)
    assert_eq(freight.filter(func(info:VehicleInfo) -> bool: return info.vehicle_provider == OTHER_PROVIDER).size(), 0)
    assert_eq(passenger.filter(func(info:VehicleInfo) -> bool: return info.vehicle_provider == OTHER_PROVIDER).size(), 1)


func test_a_vehicle_is_built_freed_and_read_by_its_own_provider() -> void:
    var data:VehicleData = VehicleData.new()
    data.vehicle_provider = OTHER_PROVIDER
    data.vehicle = VEHICLE

    var vehicle:RID = VehicleCatalogServer.vehicle_create(data, 0)

    assert_eq(_other.made.get(vehicle), data)
    assert_false(_provider.made.has(vehicle))
    assert_eq(VehicleCatalogServer.vehicle_get_data(vehicle), data)
    VehicleCatalogServer.vehicle_free(vehicle)
    assert_eq(_other.freed, [vehicle] as Array[RID])


func test_a_vehicle_of_no_provider_is_the_default_ones() -> void:
    ProjectSettings.set_setting(VehicleCatalogServer.PROVIDER_SETTING, String(PROVIDER))
    var data:VehicleData = VehicleData.new()

    var vehicle:RID = VehicleCatalogServer.vehicle_create(data, 0)

    assert_eq(_provider.made.get(vehicle), data)


func test_a_players_livery_is_kept_after_the_providers_and_removed() -> void:
    var vehicle:VehicleInfo = _vehicle_info()
    watch_signals(VehicleCatalogServer)

    assert_eq(VehicleCatalogServer.livery_add(vehicle, _player_livery(PLAYER_LIVERY, PROVIDER_LIVERY)), OK)
    var names:Array[String] = []
    for livery:VehicleLivery in VehicleCatalogServer.livery_list(vehicle):
        names.append(livery.name)
    assert_eq(names, [PROVIDER_LIVERY, PLAYER_LIVERY] as Array[String])
    assert_eq(VehicleCatalogServer.livery_get(vehicle, PLAYER_LIVERY).base, PROVIDER_LIVERY)
    assert_signal_emit_count(VehicleCatalogServer, "livery_list_changed", 1)

    assert_eq(VehicleCatalogServer.livery_remove(vehicle, PLAYER_LIVERY), OK)
    assert_null(VehicleCatalogServer.livery_get(vehicle, PLAYER_LIVERY))
    assert_signal_emit_count(VehicleCatalogServer, "livery_list_changed", 2)


func test_a_providers_livery_is_not_the_players_to_replace_or_remove() -> void:
    var vehicle:VehicleInfo = _vehicle_info()

    assert_eq(VehicleCatalogServer.livery_add(vehicle, _player_livery(PROVIDER_LIVERY, PROVIDER_LIVERY)), ERR_ALREADY_EXISTS)
    assert_push_error("is a livery of")
    assert_eq(VehicleCatalogServer.livery_remove(vehicle, PROVIDER_LIVERY), ERR_DOES_NOT_EXIST)
    assert_not_null(VehicleCatalogServer.livery_get(vehicle, PROVIDER_LIVERY))


func test_a_players_livery_is_made_on_one_of_the_providers() -> void:
    var vehicle:VehicleInfo = _vehicle_info()

    assert_eq(VehicleCatalogServer.livery_add(vehicle, _player_livery(PLAYER_LIVERY, "")), ERR_INVALID_PARAMETER)
    assert_push_error("is made on one of the provider's")
    assert_null(VehicleCatalogServer.livery_get(vehicle, PLAYER_LIVERY))


func test_a_vehicles_side_view_is_its_providers() -> void:
    var data:VehicleData = VehicleData.new()
    data.vehicle_provider = PROVIDER
    data.vehicle = VEHICLE

    assert_eq(await VehicleCatalogServer.vehicle_get_side_view(data), SIDE_VIEW)
    assert_eq(VehicleCatalogServer.vehicle_get_side_view_coupling_width(data), COUPLING_WIDTH)
    data.vehicle_provider = &"no_such_provider"
    assert_null(await VehicleCatalogServer.vehicle_get_side_view(data), "none of a provider not there")
