@tool
extends Node
class_name VehicleCatalogProvider

## A source of rolling stock (VehicleCatalogServer.provider_register()): it lists its vehicles and
## their liveries, shows a vehicle from its side, says what a vehicle is and what it looks like in
## the core's terms, and builds a vehicle of what a VehicleData says. A plugin's provider is an
## autoload deriving from this one.

## The provider's vehicles of [param category]
func vehicle_list(_category:VehicleInfo.VehicleCategory) -> Array[VehicleInfo]:
    var none:Array[VehicleInfo] = []
    return none


## The provider's own liveries of [param vehicle], one of its vehicle_list()
func livery_list(_vehicle:VehicleInfo) -> Array[VehicleLivery]:
    var none:Array[VehicleLivery] = []
    return none


## [param vehicle] seen from its side, in its livery, as long as it is - what a list of vehicles shows;
## await it, as it may take frames to render; null for none
func vehicle_get_side_view(_vehicle:VehicleData) -> Texture2D:
    return null


## The width [px] of [param vehicle]'s side view between its couplers - where the next vehicle of a
## trainset starts; 0 for the side view's whole width
func vehicle_get_side_view_coupling_width(_vehicle:VehicleData) -> float:
    return 0.0


## What [param vehicle] is - its size, its top speed, its components: the core's VehicleController
## of it as the provider builds it, configured, not created (VehicleServer); null for none
func vehicle_get_description(_vehicle:VehicleData) -> VehicleController:
    return null


## What [param vehicle] looks like in its livery - its models and their skins: the core's
## RailVehicleAppearance of it; null for none
func vehicle_get_appearance(_vehicle:VehicleData) -> RailVehicleAppearance:
    return null


## A vehicle of [param vehicle], drawn in the world of the scene node [param scene_node_id]: its
## VehicleServer handle, given at once - the vehicle itself may be built later
func vehicle_create(_vehicle:VehicleData, _scene_node_id:int) -> RID:
    return RID()


## The vehicle goes, with everything the provider made for it
func vehicle_free(_vehicle:RID) -> void:
    pass


## What the vehicle was created from
func vehicle_get_data(_vehicle:RID) -> VehicleData:
    return null
