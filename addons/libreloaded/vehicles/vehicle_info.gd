@tool
extends RefCounted
class_name VehicleInfo

## A vehicle as the catalogue lists it (VehicleCatalogServer.vehicle_list()): whose it is, by what
## name its provider finds it, its title and its kind. Its liveries are the catalogue's
## (VehicleCatalogServer.livery_list()).

## What kind of vehicle it is
enum VehicleCategory {
    ELECTRIC_LOCOMOTIVE,
    DIESEL_LOCOMOTIVE,
    STEAM_LOCOMOTIVE,
    MULTIPLE_UNIT,
    RAILCAR,
    TRAM,
    DRAISINE,
    UTILITY_VEHICLE,
    PASSENGER_CAR,
    FREIGHT_CAR,
    ROAD_CAR,
    ROAD_BUS,
    ROAD_TRUCK,
    SPECIAL,
    OTHER,
}

## The kinds of vehicle that run on roads
const ROAD_CATEGORIES:Array[VehicleCategory] = [
    VehicleCategory.ROAD_CAR, VehicleCategory.ROAD_BUS, VehicleCategory.ROAD_TRUCK,
]

## The name of the provider the vehicle is the one of (VehicleCatalogServer.provider_register())
var vehicle_provider:StringName = &""
## What the provider finds the vehicle by (VehicleData.vehicle)
var vehicle:String = ""
## For the player to read; empty shows the vehicle's name
var title:String = ""
var category:VehicleCategory = VehicleCategory.OTHER
