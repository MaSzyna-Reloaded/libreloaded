@tool
extends Node
class_name SceneryProvider

## A source of sceneries (SceneryServer.register_provider()): it lists them, makes the node that
## loads one, reads one into a CompiledScenery, and registers with the servers what of it only it
## knows. A plugin's provider is an autoload deriving from this one.

## What a list of sceneries labels the provider's own with ("MaSzyna")
var title:String = ""


## The node of the scenery [param info] (one of list()), not in the tree and not loaded:
## SceneryContentNode.load() reads it once it is; null for a scenery the provider cannot make
func create_scenery(_info:SceneryInfo) -> SceneryContentNode:
    return null


## The sceneries the provider has, each by its id and title - read quickly, for a list of them
func list() -> Array[SceneryInfo]:
    var none:Array[SceneryInfo] = []
    return none


## The scenery [param info] (one of list()) with all the provider says of it: its description,
## image and trainsets
func describe(info:SceneryInfo) -> SceneryInfo:
    return info


## The scenery [param id] read, with [param parameters] - on workers, reporting the load's progress
## to [param root] (SceneryServer.report_progress(), run_on_worker()); null when it cannot be read or
## the load is given up
func compile(_root:SceneryContentNode, _id:String, _parameters:Dictionary) -> CompiledScenery:
    return null


## What of [param compiled] only the provider registers - after the core's tracks, traction and power
## sources ([param track_rids] in the order of compiled.tracks), before its lines and nodes
func instantiate_content(_root:SceneryContentNode, _compiled:CompiledScenery, _track_rids:Array[RID]) -> void:
    pass


## The trainsets of [param compiled] - after its nodes, last of the load
func instantiate_vehicles(_root:SceneryContentNode, _compiled:CompiledScenery) -> void:
    pass


## A load is cancelled (SceneryServer.scenery_cancel()): [param cancelled] true before its workers are
## joined, false after
func set_parsing_cancelled(_cancelled:bool) -> void:
    pass


## Starts the scenario of the scenery [param root] once it has loaded - what runs while it is loaded
## (its sounds, its scripts, its keys) - telling [param progressed] the share of the start done
## (0..1); await it. A game runs it, the scenery only loads it: one loaded in the editor runs nothing.
## Nothing of a provider whose sceneries have no scenario
func scenario_start(_root:SceneryContentNode, _progressed:Callable) -> void:
    pass


## The ScenarioScriptServer context the scenario of [param root] runs its scripts in; RID() for none
func scenario_get_script_context(_root:SceneryContentNode) -> RID:
    return RID()


## Stops the scenario of [param root] (scenario_start()) as it unloads
func scenario_stop(_root:SceneryContentNode) -> void:
    pass
