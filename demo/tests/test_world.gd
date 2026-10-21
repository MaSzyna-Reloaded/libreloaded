extends ReloadedGutTest

## A world of the game's own format (World): its data saved by its classes' names and no script, the
## node made of it, and what is drawn of it handed over a section a cell of the streaming grid
## (WorldSectionProvider)

const WORLD_DIR:String = "user://gut/world"
const SAVED:String = "user://gut/world/world.tres"
const SAVED_SECTION:String = "user://gut/world/section.tres"
const WORLD_NAME:String = "sections"
const ORIGIN:Vector2 = Vector2(437000.0, 700000.0)
const START_TIME:float = 6.25
## The cell there is a section of, and one there is none of
const SECTION_CELL:Vector2i = Vector2i(2, -1)
const EMPTY_CELL:Vector2i = Vector2i(3, -1)
## Where the section's mesh, decoration and wire stand: in its cell (X 2000..3000, Z -1000..0)
const MESH_CENTRE:Vector3 = Vector3(2500.0, 0.0, -500.0)
const MODEL_POSITION:Vector3 = Vector3(2200.0, 0.0, -300.0)
const WIRE_FROM:Vector3 = Vector3(2300.0, 8.0, -200.0)
const WIRE_TO:Vector3 = Vector3(2310.0, 8.0, -260.0)
const MESH_RANGE:float = 300.0
## The decoration's lights: its first on, red
const LIGHTS_STATE:Dictionary = {"00": true}
const LIGHTS_COLORS:Dictionary = {"00": Color.RED}
## The core's renderer of scenes (SceneModelRenderingBackend::NAME, not bound)
const MODEL_PROVIDER:StringName = &"scene"
## How far off a placed point may be [m]
const TOLERANCE:float = 0.001


func after_each() -> void:
    DirAccess.remove_absolute(SAVED)
    DirAccess.remove_absolute(SAVED_SECTION)
    DirAccess.remove_absolute(WORLD_DIR.path_join(World.section_file(SECTION_CELL)))


func test_saved_it_names_its_classes_and_no_script() -> void:
    DirAccess.make_dir_recursive_absolute(SAVED.get_base_dir())
    assert_eq(ResourceSaver.save(_data(), SAVED), OK)
    assert_eq(ResourceSaver.save(_section(), SAVED_SECTION), OK)

    var text:String = FileAccess.get_file_as_string(SAVED) + FileAccess.get_file_as_string(SAVED_SECTION)
    assert_false(text.contains(".gd"), "no script's path")
    assert_false(text.contains("type=\"Script\""), "no script at all")
    for type:String in ["WorldData", "WorldSection", "WorldLayer", "ModelData", "LinesData"]:
        assert_true(text.contains("type=\"%s\"" % type), "%s by its class name" % type)


func test_the_node_becomes_the_world_its_data_describes() -> void:
    var world:World = World.new()
    world.take_data(_data(), WORLD_DIR)
    assert_eq(world.get_scenery_name(), WORLD_NAME)
    assert_eq(world.origin, ORIGIN)
    assert_eq(world.world_dir, WORLD_DIR)
    assert_eq(world.section_cells, [SECTION_CELL] as Array[Vector2i])
    assert_eq(world.declared_start_time, START_TIME)
    world.free()


func test_a_world_declaring_no_clock_keeps_the_default() -> void:
    var data:WorldData = _data()
    data.declared_start_time = -1.0
    var world:World = World.new()
    world.take_data(data, WORLD_DIR)
    assert_eq(world.declared_start_time, SceneryContentNode.START_TIME_DEFAULT)
    world.free()


func test_provider_hands_over_what_of_its_section_stands_in_a_cell() -> void:
    DirAccess.make_dir_recursive_absolute(WORLD_DIR.path_join(World.SECTIONS_DIR))
    assert_eq(ResourceSaver.save(_section(), WORLD_DIR.path_join(World.section_file(SECTION_CELL))), OK)
    var provider:WorldSectionProvider = WorldSectionProvider.new(WORLD_DIR, [SECTION_CELL] as Array[Vector2i])

    assert_eq(provider.get_chunk_cells(), [SECTION_CELL] as Array[Vector2i])
    var meshes:Array = provider.chunk_load(SECTION_CELL, SceneryStreamingProvider.CONTENT_MESHES)
    assert_eq(meshes.size(), 1)
    var placement:WorldMeshPlacement = meshes[0]
    assert_almost_eq(placement.mesh.get_aabb().get_center(), MESH_CENTRE, Vector3.ONE * TOLERANCE)
    assert_eq(placement.transform.origin, Vector3.ZERO)
    assert_eq(placement.range_end, MESH_RANGE)
    var models:Array = provider.chunk_load(SECTION_CELL, SceneryStreamingProvider.CONTENT_MODELS)
    assert_eq(models.size(), 1)
    assert_almost_eq((models[0] as SceneryModelPlacement).transform.origin, MODEL_POSITION, Vector3.ONE * TOLERANCE)
    assert_eq((models[0] as SceneryModelPlacement).lights_state, LIGHTS_STATE, "its lights as the data gives them")
    assert_eq((models[0] as SceneryModelPlacement).lights_colors, LIGHTS_COLORS)
    var lines:Array = provider.chunk_load(SECTION_CELL, SceneryStreamingProvider.CONTENT_LINES)
    assert_eq(lines.size(), 1)
    assert_eq((lines[0] as SceneryLinesGeometry).get_node_count(), 1)
    assert_eq(provider.chunk_load(EMPTY_CELL, SceneryStreamingProvider.CONTENT_MESHES).size(), 0, "a cell of no section")


func _data() -> WorldData:
    var data:WorldData = WorldData.new()
    data.world_name = WORLD_NAME
    data.origin = ORIGIN
    data.declared_start_time = START_TIME
    data.section_cells = [SECTION_CELL] as Array[Vector2i]
    return data


## A section of SECTION_CELL: a layer of a mesh, one of a decoration and one of a wire
func _section() -> WorldSection:
    var section:WorldSection = WorldSection.new()
    section.cell = SECTION_CELL
    var geometry:WorldLayer = WorldLayer.new()
    geometry.range_end = MESH_RANGE
    var mesh:ArrayMesh = ArrayMesh.new()
    var arrays:Array = []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([MESH_CENTRE + Vector3(-1.0, 0.0, -1.0),
            MESH_CENTRE + Vector3(1.0, 0.0, -1.0), MESH_CENTRE + Vector3(0.0, 0.0, 1.0)])
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    geometry.meshes.append(mesh)
    var decorations:WorldLayer = WorldLayer.new()
    var model:ModelData = ModelData.new()
    model.model_provider = MODEL_PROVIDER
    model.model = "models/test/test"
    model.position = MODEL_POSITION
    model.lights_state = LIGHTS_STATE
    model.lights_colors = LIGHTS_COLORS
    decorations.models.append(model)
    var wires:WorldLayer = WorldLayer.new()
    var wire:LinesData = LinesData.new()
    wire.points = PackedVector3Array([WIRE_FROM, WIRE_TO])
    wires.lines.append(wire)
    section.layers.append_array([geometry, decorations, wires])
    return section
