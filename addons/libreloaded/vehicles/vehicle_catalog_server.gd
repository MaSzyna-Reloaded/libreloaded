@tool
extends Node

## The rolling stock the game has: the vehicles of each provider (provider_register()) by their kind,
## their liveries - the provider's and the player's own, which it keeps - and a vehicle built of what
## a VehicleData says, by the provider the vehicle is the one of (VehicleData.vehicle_provider).

## The provider a vehicle of no provider named is the one of, by its name
const PROVIDER_SETTING:String = "reloaded/vehicles/provider"
## Where the player's liveries are kept, a directory of each provider's vehicle under it
const LIVERIES_DIRECTORY_SETTING:String = "reloaded/vehicles/liveries_directory"
const LIVERIES_DIRECTORY:String = "user://liveries"
const LIVERY_EXTENSION:String = "tres"

## The liveries of [param vehicle] changed: a player's one added or removed
signal livery_list_changed(vehicle:VehicleInfo)

## The registered providers' ObjectIDs, by name
var _providers:Dictionary[StringName, int] = {}
## The provider each vehicle made through the catalogue is the one of, until it is freed
var _vehicle_providers:Dictionary[RID, StringName] = {}


func _ready() -> void:
    VehicleServer.vehicle_freed.connect(_on_vehicle_freed)


func _exit_tree() -> void:
    VehicleServer.vehicle_freed.disconnect(_on_vehicle_freed)


## A VehicleCatalogProvider, by its ObjectID, supplies the vehicles of [param provider_name]
func provider_register(provider_name:StringName, provider_id:int) -> void:
    _providers[provider_name] = provider_id


func provider_unregister(provider_name:StringName) -> void:
    _providers.erase(provider_name)


## The VehicleCatalogProvider registered as [param provider_name], by its ObjectID; 0 for none
func provider_get(provider_name:StringName) -> int:
    return _providers.get(provider_name, 0)


func provider_get_list() -> PackedStringArray:
    return PackedStringArray(_providers.keys())


## The vehicles of [param category] of every provider
func vehicle_list(category:VehicleInfo.VehicleCategory) -> Array[VehicleInfo]:
    var vehicles:Array[VehicleInfo] = []
    for provider_name:StringName in _providers:
        vehicles.append_array(_get_provider(provider_name).vehicle_list(category))
    return vehicles


## A vehicle of [param vehicle], drawn in the world of the scene node [param scene_node_id], built by
## its provider: its VehicleServer handle, given at once; none when the provider is not there
func vehicle_create(vehicle:VehicleData, scene_node_id:int) -> RID:
    var provider_name:StringName = _get_provider_name(vehicle)
    var provider:VehicleCatalogProvider = _get_provider(provider_name)
    if not provider:
        push_error("No vehicle provider \"%s\" for %s" % [provider_name, vehicle.vehicle])
        return RID()
    var vehicle_rid:RID = provider.vehicle_create(vehicle, scene_node_id)
    if vehicle_rid.is_valid():
        _vehicle_providers[vehicle_rid] = provider_name
    return vehicle_rid


## The vehicle goes, with everything its provider made for it
func vehicle_free(vehicle_rid:RID) -> void:
    var provider:VehicleCatalogProvider = _get_provider(_vehicle_providers.get(vehicle_rid, &""))
    if provider:
        provider.vehicle_free(vehicle_rid)


## What the vehicle was created from; null for one the catalogue did not make
func vehicle_get_data(vehicle_rid:RID) -> VehicleData:
    var provider:VehicleCatalogProvider = _get_provider(_vehicle_providers.get(vehicle_rid, &""))
    return provider.vehicle_get_data(vehicle_rid) if provider else null


## [param vehicle] seen from its side by its provider, in its livery (VehicleCatalogProvider.
## vehicle_get_side_view()); await it; null when the provider is not there or has none
func vehicle_get_side_view(vehicle:VehicleData) -> Texture2D:
    var provider:VehicleCatalogProvider = _get_provider(_get_provider_name(vehicle))
    return await provider.vehicle_get_side_view(vehicle) if provider else null


## The width [px] of [param vehicle]'s side view between its couplers, 0 for its whole width
func vehicle_get_side_view_coupling_width(vehicle:VehicleData) -> float:
    var provider:VehicleCatalogProvider = _get_provider(_get_provider_name(vehicle))
    return provider.vehicle_get_side_view_coupling_width(vehicle) if provider else 0.0


## The liveries of [param vehicle]: its provider's, then the player's
func livery_list(vehicle:VehicleInfo) -> Array[VehicleLivery]:
    var liveries:Array[VehicleLivery] = []
    var provider:VehicleCatalogProvider = _get_provider(vehicle.vehicle_provider)
    if provider:
        liveries.append_array(provider.livery_list(vehicle))
    var directory:String = _get_liveries_directory(vehicle)
    if not DirAccess.dir_exists_absolute(directory):
        return liveries
    var files:PackedStringArray = DirAccess.get_files_at(directory)
    files.sort()
    for file:String in files:
        if not file.get_extension() == LIVERY_EXTENSION:
            continue
        var livery:VehicleLivery = ResourceLoader.load(
            directory.path_join(file), "", ResourceLoader.CACHE_MODE_IGNORE
        ) as VehicleLivery
        if livery:
            liveries.append(livery)
    return liveries


## The livery [param livery_name] of [param vehicle]; null when it has none of that name
func livery_get(vehicle:VehicleInfo, livery_name:String) -> VehicleLivery:
    for livery:VehicleLivery in livery_list(vehicle):
        if livery.name == livery_name:
            return livery
    return null


## Keeps the player's [param livery] of [param vehicle], made on one of the provider's liveries (its
## base); one of the same name is replaced. A provider's livery is not the player's to replace.
func livery_add(vehicle:VehicleInfo, livery:VehicleLivery) -> Error:
    if not livery.name.is_valid_filename():
        push_error("A livery's name must make a file name: \"%s\"" % livery.name)
        return ERR_INVALID_PARAMETER
    var provider_liveries:Array[String] = _get_provider_livery_names(vehicle)
    if livery.name in provider_liveries:
        push_error("\"%s\" is a livery of %s's provider" % [livery.name, vehicle.vehicle])
        return ERR_ALREADY_EXISTS
    if not livery.base in provider_liveries:
        push_error("A player's livery is made on one of the provider's: \"%s\"" % livery.base)
        return ERR_INVALID_PARAMETER
    var directory:String = _get_liveries_directory(vehicle)
    var error:Error = DirAccess.make_dir_recursive_absolute(directory)
    if not error == OK:
        return error
    error = ResourceSaver.save(livery, directory.path_join("%s.%s" % [livery.name, LIVERY_EXTENSION]))
    if error == OK:
        livery_list_changed.emit(vehicle)
    return error


## Removes the player's livery [param livery_name] of [param vehicle]; a provider's livery stays
func livery_remove(vehicle:VehicleInfo, livery_name:String) -> Error:
    var path:String = _get_liveries_directory(vehicle).path_join("%s.%s" % [livery_name, LIVERY_EXTENSION])
    if not livery_name.is_valid_filename() or not FileAccess.file_exists(path):
        return ERR_DOES_NOT_EXIST
    var error:Error = DirAccess.remove_absolute(path)
    if error == OK:
        livery_list_changed.emit(vehicle)
    return error


## What [param vehicle] is, as its provider describes it (VehicleCatalogProvider.
## vehicle_get_description()); null when the provider is not there or has none
func vehicle_get_description(vehicle:VehicleData) -> VehicleController:
    var provider:VehicleCatalogProvider = _get_provider(_get_provider_name(vehicle))
    return provider.vehicle_get_description(vehicle) if provider else null


## What [param vehicle] looks like in its livery, as its provider draws it (VehicleCatalogProvider.
## vehicle_get_appearance()); null when the provider is not there or has none
func vehicle_get_appearance(vehicle:VehicleData) -> RailVehicleAppearance:
    var provider:VehicleCatalogProvider = _get_provider(_get_provider_name(vehicle))
    return provider.vehicle_get_appearance(vehicle) if provider else null


## The provider registered as [param provider_name]; null for none
func _get_provider(provider_name:StringName) -> VehicleCatalogProvider:
    return instance_from_id(_providers.get(provider_name, 0)) as VehicleCatalogProvider


func _get_provider_name(vehicle:VehicleData) -> StringName:
    if vehicle.vehicle_provider:
        return vehicle.vehicle_provider
    return StringName(ProjectSettings.get_setting(PROVIDER_SETTING, ""))


func _get_provider_livery_names(vehicle:VehicleInfo) -> Array[String]:
    var names:Array[String] = []
    var provider:VehicleCatalogProvider = _get_provider(vehicle.vehicle_provider)
    if provider:
        for livery:VehicleLivery in provider.livery_list(vehicle):
            names.append(livery.name)
    return names


## The directory of [param vehicle]'s player liveries: its provider's, then its name's own
func _get_liveries_directory(vehicle:VehicleInfo) -> String:
    var root:String = ProjectSettings.get_setting(LIVERIES_DIRECTORY_SETTING, LIVERIES_DIRECTORY)
    return root.path_join(String(vehicle.vehicle_provider)).path_join(vehicle.vehicle)


func _on_vehicle_freed(vehicle_rid:RID) -> void:
    _vehicle_providers.erase(vehicle_rid)
