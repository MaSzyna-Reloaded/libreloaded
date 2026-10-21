@tool
extends Resource
class_name VehicleLivery

## A painting of one vehicle of the catalogue (VehicleCatalogServer.livery_list()): its provider's,
## or a player's, made on one of the provider's and kept by the catalogue (livery_add())

## What finds it among the vehicle's liveries
@export var name:String = ""
## For the player to read; empty shows the name
@export var title:String = ""
## The provider's livery a player's one is made on; empty for a provider's own
@export var base:String = ""
## What the livery paints on the vehicle's livery surfaces, by the surface's name; nothing draws them
## yet
@export var textures:Dictionary[StringName, Texture2D] = {}
## What the provider keeps of a livery of its own; the core reads none of it
@export var meta:Dictionary = {}
