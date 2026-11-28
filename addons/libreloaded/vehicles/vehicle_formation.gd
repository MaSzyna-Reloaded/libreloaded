@tool
extends RefCounted
class_name VehicleFormation

## Vehicles that run together, each in its livery, coupled for good, as the catalogue lists them
## (VehicleCatalogServer.formation_list()). The catalogue builds none of it; whoever stands them on
## the track makes the trainset of them.

## Front to rear, each its vehicle (VehicleData.vehicle) and its livery (VehicleData.livery)
var vehicles:Array[VehicleData] = []
