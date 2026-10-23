extends SceneryStreamingProvider
class_name WorldSectionProvider

## What is drawn of a world (World) - its geometry, its decorations' models, its power lines' wires -
## a section (WorldSection) a cell of SceneryStreamingServer's grid, read on the streaming worker as
## the camera comes within reach of the cell and let go once it has left: the cell's content goes
## with it. The simulation's part of the world is not here: the world loads it whole.

## SceneryStreamingServer::CHUNK_SIZE_M (not bound): a section a cell of its grid
const CELL_SIZE:float = 1000.0
## How far a cell's content reaches out of it [m]: a mesh stands in the cell of its centre and is a
## square of the world's geometry, triangles reaching out of the square by their centre
const OVERHANG:float = 256.0
## The sections read last, kept for the kinds of content asked for one after another
const KEPT_SECTIONS:int = 4

## The world's directory (World.world_dir) and the cells there are sections of
var _dir:String = ""
var _cells:Array[Vector2i] = []
## Cell -> its section, the oldest first; read on the streaming worker
var _kept:Dictionary[Vector2i, WorldSection] = {}
var _kept_mutex:Mutex = Mutex.new()


func _init(dir:String = "", cells:Array[Vector2i] = []) -> void:
    _dir = dir
    _cells = cells


func _get_chunk_cells() -> Array[Vector2i]:
    return _cells


func _chunk_get_overhang(_cell:Vector2i) -> float:
    return OVERHANG


func _get_content_kinds() -> PackedInt32Array:
    return PackedInt32Array([CONTENT_MESHES, CONTENT_MODELS, CONTENT_LINES])


func _chunk_load(cell:Vector2i, kind:int) -> Array:
    _kept_mutex.lock()
    var section:WorldSection = _kept.get(cell)
    _kept_mutex.unlock()
    if not section:
        var path:String = _dir.path_join(World.section_file(cell))
        section = ResourceLoader.load(path) as WorldSection if ResourceLoader.exists(path) else null
        if not section:
            return []
        _kept_mutex.lock()
        _kept[cell] = section
        if _kept.size() > KEPT_SECTIONS:
            _kept.erase(_kept.keys()[0])
        _kept_mutex.unlock()
    var items:Array = []
    var lines:Dictionary[Vector3, SceneryLinesGeometry] = {}
    for layer:WorldLayer in section.layers:
        append_layer_items(layer, Vector3.ZERO, cell, kind, items, lines)
    items.append_array(lines.values())
    return items


## What of `layer`, moved by `offset`, stands in the cell `cell` of the streaming grid, as the
## content `kind` of it, appended to `items`: a mesh by the centre of its bounds (WorldMeshPlacement),
## a model by its position (SceneryModelPlacement), lines by their centre, merged into `lines` - the
## cell's line sections by their origin (SceneryLinesGeometry), appended by the caller once every
## layer has been through. For a plugin's provider of its world's sectors too.
static func append_layer_items(layer:WorldLayer, offset:Vector3, cell:Vector2i, kind:int, items:Array,
        lines:Dictionary[Vector3, SceneryLinesGeometry]) -> void:
    var transform:Transform3D = Transform3D(Basis.IDENTITY, offset)
    match kind:
        CONTENT_MESHES:
            for mesh:ArrayMesh in layer.meshes:
                if cell_of(transform * mesh.get_aabb().get_center()) == cell:
                    items.append(WorldMeshPlacement.new(mesh, transform, layer.range_end))
        CONTENT_MODELS:
            for model:ModelData in layer.models:
                var position:Vector3 = model.position + offset
                if not cell_of(position) == cell:
                    continue
                var placement:SceneryModelPlacement = SceneryModelPlacement.new()
                placement.rendering_backend = model.rendering_backend
                placement.model = model.model
                placement.skins = model.skins
                placement.transform = Transform3D(Basis.from_euler(model.rotation), position)
                placement.range_min = model.range_min
                placement.range_max = model.range_max
                placement.lights_state = model.lights_state
                placement.lights_colors = model.lights_colors
                items.append(placement)
        CONTENT_LINES:
            # a node's lines go to the section of their centre, as SceneryLines.create_meshes() puts them
            for wires:LinesData in layer.lines:
                var moved:LinesData = wires.translated(offset)
                var centre:Vector3 = Vector3.ZERO
                for point:Vector3 in moved.points:
                    centre += point
                var origin:Vector3 = SceneryLinesGeometry.position_get_section_origin(centre / moved.points.size())
                if not cell_of(origin) == cell:
                    continue
                if not lines.has(origin):
                    lines[origin] = SceneryLinesGeometry.new()
                    lines[origin].origin = origin
                lines[origin].add_lines(moved.points, moved.color, moved.width, moved.range_min, moved.range_max)


## The cell of SceneryStreamingServer's grid a point of the world is in
static func cell_of(point:Vector3) -> Vector2i:
    return Vector2i(floori(point.x / CELL_SIZE), floori(point.z / CELL_SIZE))
