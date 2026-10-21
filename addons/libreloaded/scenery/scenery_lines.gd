@tool
extends RefCounted
class_name SceneryLines

## The scenery's "lines" nodes - the wires of power lines, mostly - drawn by
## SceneryMeshRenderingServer: a mesh a section of the 1 km grid, as the original keeps them in its
## sections (basic_region::insert(), scene.cpp:1399), merged as it merges them in their cells
## (SceneryLinesGeometry) and drawn by scenery_lines.gdshader.

const MATERIAL:Material = preload("scenery_lines_material.tres")


## The meshes of `lines` (CompiledScenery.lines) registered in `scenario`; their RIDs, for
## SceneryMeshRenderingServer.mesh_free()
static func create_meshes(lines:Array[LinesData], scenario:RID) -> Array[RID]:
    var geometries:Dictionary[Vector3, SceneryLinesGeometry] = {}
    for node:LinesData in lines:
        var centre:Vector3 = Vector3.ZERO
        for point:Vector3 in node.points:
            centre += point
        var origin:Vector3 = SceneryLinesGeometry.position_get_section_origin(centre / node.points.size())
        var geometry:SceneryLinesGeometry = geometries.get(origin)
        if not geometry:
            geometry = SceneryLinesGeometry.new()
            geometry.origin = origin
            geometries[origin] = geometry
        geometry.add_lines(node.points, node.color, node.width, node.range_min, node.range_max)
    var meshes:Array[RID] = []
    for geometry:SceneryLinesGeometry in geometries.values():
        meshes.append(create_mesh(geometry, scenario))
    return meshes


## The mesh of a section's lines registered in `scenario` - also how a region file's lines are drawn
## (SceneryStreamingProvider.CONTENT_LINES, SceneryServer)
static func create_mesh(geometry:SceneryLinesGeometry, scenario:RID) -> RID:
    var mesh:ArrayMesh = geometry.build_mesh()
    mesh.surface_set_material(0, MATERIAL)
    return SceneryMeshRenderingServer.mesh_create(
        mesh, Transform3D(Basis.IDENTITY, geometry.origin), geometry.get_range_end(), scenario
    )
