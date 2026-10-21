@tool
extends RefCounted
class_name SceneryInfo

## A scenery as a list of them shows it (SceneryServer.scenery_list()): which provider has it
## (SceneryProvider.title labels it, create_scenery() loads it), by what id and its title; described
## (SceneryProvider.describe()) its description, image and trainsets too. A provider's subclass says
## more of it.

## Between the parts of a title, the scenery's name first and the scenario's after ("Bałtyk · SKM1")
const TITLE_PART_SEPARATOR:String = " · "

## The name it is registered with (SceneryServer.register_provider())
var provider:StringName = &""
## What the provider finds the scenery by (a .scn's file name, a world's name)
var id:String = ""
var title:String = ""
var description:String = ""
## Absolute path of the scenery's image, empty when it has none
var image_path:String = ""
## The trainsets it declares, as it declares them - their vehicles' ids, names and drivers
## (TrainsetData.get_arranged_vehicles() pairs a trainset arranged otherwise with them)
var trainsets:Array[TrainsetData] = []
