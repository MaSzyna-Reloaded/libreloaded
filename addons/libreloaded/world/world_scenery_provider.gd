@tool
extends SceneryProvider

## The worlds of the game's own format (World): the core's autoload, registered with SceneryServer as
## its provider_name, it lists the worlds/*.json of the game's directory it takes (takes()) and makes
## the world of one. A world loads itself (World._instantiate_content()); what it registers of its
## scenery beyond the core's tracks, traction and power sources is a provider's: its models, the
## signal heads among them, and its trainsets - their drivers thinking with nothing, a world has no
## AI of its own yet. A plugin's provider of the worlds it divides otherwise extends this one.

const NAME:StringName = &"world"
## The label of its worlds on a list (SceneryProvider.title)
const TITLE:String = "Rails Reloaded"

## The name it is registered under
var provider_name:StringName = NAME


func _init() -> void:
    title = TITLE


func _ready() -> void:
    SceneryServer.register_provider(provider_name, self)


func _exit_tree() -> void:
    SceneryServer.unregister_provider(provider_name)


## The world `description` (World.read_description()) is this provider's: one not divided into a
## plugin's sectors
func takes(description:Dictionary) -> bool:
    return not description.has("sectors")


## Every world of worlds/ it takes, by its title - its file name when it names none
func list() -> Array[SceneryInfo]:
    var worlds:Array[SceneryInfo] = []
    var worlds_dir:String = UserSettings.get_data_dir().path_join(World.WORLDS_DIR)
    if not DirAccess.dir_exists_absolute(worlds_dir):
        return worlds
    for file:String in DirAccess.get_files_at(worlds_dir):
        if not file.get_extension().to_lower() == World.DESCRIPTION_EXTENSION:
            continue
        var description:Dictionary = World.read_description(file)
        if not description or not takes(description):
            continue
        var info:SceneryInfo = SceneryInfo.new()
        info.provider = provider_name
        info.id = file
        info.title = description.get("title", file.get_basename().capitalize())
        worlds.append(info)
    return worlds


## The world's description and the trainsets of its scenario (World.read_scenario())
func describe(info:SceneryInfo) -> SceneryInfo:
    var described:SceneryInfo = SceneryInfo.new()
    described.provider = info.provider
    described.id = info.id
    described.title = info.title
    described.description = World.read_description(info.id).get("description", "")
    var scenario:CompiledScenery = World.read_scenario(info.id)
    if scenario:
        described.trainsets.assign(scenario.trainsets)
    return described


## The world made of its data, its packs mounted (World.open())
func create_scenery(info:SceneryInfo) -> SceneryContentNode:
    return World.open(World.read_description(info.id))


## The world's models, through ModelRenderingServer by their names, and the signal heads of those of
## a signal_head_kind in the world's own signalling system, of the world's name - attached no
## implementation: a world has no signalling of its own yet
func instantiate_content(root:SceneryContentNode, compiled:CompiledScenery, _track_rids:Array[RID]) -> void:
    var world_3d:World3D = root.get_world_3d()
    var signalling_system:RID = SignallingServer.system_create()
    SignallingServer.system_set_name(signalling_system, root.get_scenery_name())
    root._signalling_system_rids.append(signalling_system)
    var built_count:int = 0
    for model_data:ModelData in compiled.models:
        var model_rid:RID = SceneryServer.build_model(model_data, model_data.model, world_3d)
        var signal_head:RID = SceneryServer.build_signal_head(model_rid, model_data)
        if signal_head.is_valid():
            SignallingServer.system_add_signal_head(signalling_system, signal_head)
        if model_rid.is_valid():
            root._model_rids.append(model_rid)
        built_count += 1
        await SceneryServer.report_progress_throttled(
            root, lerpf(SceneryServer.CONTENT_PROGRESS, SceneryServer.OBJECTS_PROGRESS, float(built_count) / compiled.models.size()),
            SceneryContentNode.LoadStage.INFRASTRUCTURE, TranslationServer.translate("Registering %s") % model_data.model
        )
        if SceneryServer.is_load_given_up(root):
            return


func instantiate_vehicles(root:SceneryContentNode, compiled:CompiledScenery) -> void:
    await SceneryServer.instantiate_trainsets(root, compiled.trainsets, &"")
