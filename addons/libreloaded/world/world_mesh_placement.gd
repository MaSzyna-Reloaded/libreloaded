extends RefCounted
class_name WorldMeshPlacement

## A mesh a SceneryStreamingProvider places (SceneryStreamingProvider.CONTENT_MESHES) - a piece of a
## world's geometry (WorldLayer.meshes) supplied as its cell is (WorldSectionProvider, or a plugin's
## provider of its world's sectors). Drawn by SceneryMeshRenderingServer as long as its cell is
## supplied: the mesh goes with it.

## The snow laid over a surface's own material - a roof's orthophoto, a facade - which has no weather
## of its own: its next pass. Not in the editor: a tool saving the world's meshes (the scenery
## importer) would write it into the world's files
const SNOW_OVERLAY:ShaderMaterial = preload("res://addons/libreloaded/environment/snow_overlay.tres")
## A surface's normal map, which the snow lies by: a ShaderMaterial's parameter of this name (a
## facade's), a StandardMaterial3D's normal_texture; SNOW_OVERLAY's own of this name gets it
const NORMAL_TEXTURE:StringName = &"texture_normal"

var mesh:ArrayMesh
## Where the mesh is placed in the world: its section's origin, or its sector's corner
var transform:Transform3D = Transform3D.IDENTITY
## How far it is drawn from [m]; 0: the draw distance (WorldLayer.range_end)
var range_end:float = 0.0


func _init(placed_mesh:ArrayMesh = null, placed_transform:Transform3D = Transform3D.IDENTITY, placed_range_end:float = 0.0) -> void:
    mesh = placed_mesh
    transform = placed_transform
    range_end = placed_range_end


## The placement drawn in `scenario` - the consumer of CONTENT_MESHES (SceneryServer): a surface
## named after the game's material (ReloadedMaterialServer) gets that material, one with its own (an
## orthophoto, a facade) keeps it, the environment's snow over it in a game (SNOW_OVERLAY, by its
## normal map when it has one). Its RID,
## for SceneryMeshRenderingServer.mesh_free()
static func create_mesh(placement:WorldMeshPlacement, scenario:RID) -> RID:
    for surface:int in placement.mesh.get_surface_count():
        var material:Material = placement.mesh.surface_get_material(surface)
        if not material:
            placement.mesh.surface_set_material(surface,
                    ReloadedMaterialServer.material_get(placement.mesh.surface_get_name(surface)))
        elif not material.next_pass and not Engine.is_editor_hint():
            material.next_pass = _snow_overlay(material)
    return SceneryMeshRenderingServer.mesh_create(placement.mesh, placement.transform, placement.range_end, scenario)


## The snow over `material`: SNOW_OVERLAY, or its copy with the material's normal map - made once,
## the material keeping it as its next pass for every mesh drawn with it
static func _snow_overlay(material:Material) -> ShaderMaterial:
    var normal:Texture2D = null
    if material is ShaderMaterial:
        normal = (material as ShaderMaterial).get_shader_parameter(NORMAL_TEXTURE) as Texture2D
    elif material is BaseMaterial3D and (material as BaseMaterial3D).normal_enabled:
        normal = (material as BaseMaterial3D).normal_texture
    if not normal:
        return SNOW_OVERLAY
    var overlay:ShaderMaterial = SNOW_OVERLAY.duplicate() as ShaderMaterial
    overlay.set_shader_parameter(NORMAL_TEXTURE, normal)
    return overlay

