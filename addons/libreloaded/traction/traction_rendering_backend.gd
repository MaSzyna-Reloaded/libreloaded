@tool
extends RefCounted
class_name TractionRenderingBackend

## What TractionRenderingServer forwards the drawing of its spans to (the backend their data names,
## TractionData.rendering_backend). This one is the core's plain look: the contact
## wire straight between its points and, when the span has one, the support wire straight between
## its own, of one dark metal. A plugin's backend derives from it and keeps what it draws in a
## TractionState of its own (make_state()); the wires are ribbons of segments (_create_segment_mesh()).

const PLAIN_WIRE_MATERIAL: Material = preload("plain_wire_material.tres")
const MIN_SEGMENT_WIDTH: float = 0.01
## A wire's width over its thickness in mm
const WIRE_THICKNESS_SCALE: float = 0.0015
const EPSILON: float = 0.0001


## What TractionRenderingServer keeps of a span; a backend keeps more in a subclass
class TractionState:
    ## Valid when the span is streamed (SceneryStreamingServer); hand-placed nodes are not
    var stream_rid: RID = RID()
    var streamed: bool = false
    var scenario: RID = RID()
    var transform: Transform3D = Transform3D.IDENTITY
    var visible: bool = true
    ## What the span is (TractionRenderingServer.set_traction_description())
    var description: TractionData


## The plain look's own: one instance of one mesh
class PlainTractionState extends TractionState:
    var instance: RID = RID()
    var mesh: ArrayMesh


## A straight piece of wire, its ribbon turned across [member width_axis_hint]
class SegmentData:
    var start: Vector3 = Vector3.ZERO
    var finish: Vector3 = Vector3.ZERO
    var width_axis_hint: Vector3 = Vector3.ZERO

    func _init(p_start: Vector3, p_finish: Vector3, p_width_axis_hint: Vector3) -> void:
        start = p_start
        finish = p_finish
        width_axis_hint = p_width_axis_hint


## The state TractionRenderingServer keeps for a span drawn by this backend
func make_state() -> TractionState:
    return PlainTractionState.new()


## What the span drew goes as it is freed
func free_state(state: TractionState) -> void:
    var plain: PlainTractionState = state as PlainTractionState
    if plain.instance.is_valid():
        RenderingServer.free_rid(plain.instance)
    plain.mesh = null


func apply_scenario(state: TractionState) -> void:
    var plain: PlainTractionState = state as PlainTractionState
    if plain.instance.is_valid():
        RenderingServer.instance_set_scenario(plain.instance, state.scenario)


func apply_transform(state: TractionState) -> void:
    var plain: PlainTractionState = state as PlainTractionState
    if plain.instance.is_valid():
        RenderingServer.instance_set_transform(plain.instance, state.transform)


func apply_visible(state: TractionState) -> void:
    var plain: PlainTractionState = state as PlainTractionState
    if plain.instance.is_valid():
        RenderingServer.instance_set_visible(plain.instance, state.visible)


## The plain look: the contact wire and the support wire, straight
func build(state: TractionState) -> void:
    var plain: PlainTractionState = state as PlainTractionState
    var span: TractionData = state.description
    clear(state)
    if not span or span.contact_p1.distance_to(span.contact_p2) <= EPSILON:
        return
    var lateral_axis: Vector3 = (span.contact_p2 - span.contact_p1).cross(Vector3.UP).normalized()
    var segments: Array[SegmentData] = [SegmentData.new(span.contact_p1, span.contact_p2, lateral_axis)]
    if not span.wires == TractionRenderingServer.Wires.CONTACT and span.support_p1.distance_to(span.support_p2) > EPSILON:
        segments.append(SegmentData.new(span.support_p1, span.support_p2, lateral_axis))
    plain.mesh = _create_segment_mesh(segments, span.wire_thickness)
    plain.mesh.surface_set_material(0, PLAIN_WIRE_MATERIAL)
    if not plain.instance.is_valid():
        plain.instance = RenderingServer.instance_create()
    RenderingServer.instance_set_base(plain.instance, plain.mesh.get_rid())
    RenderingServer.instance_set_scenario(plain.instance, state.scenario)
    RenderingServer.instance_set_transform(plain.instance, state.transform)
    RenderingServer.instance_set_visible(plain.instance, state.visible)


## What the span drew goes as it leaves the camera's range
func clear(state: TractionState) -> void:
    var plain: PlainTractionState = state as PlainTractionState
    if plain.instance.is_valid():
        RenderingServer.instance_set_base(plain.instance, RID())
    plain.mesh = null


func _create_segment_mesh(segments: Array[SegmentData], wire_thickness: float) -> ArrayMesh:
    var mesh: ArrayMesh = ArrayMesh.new()
    if not segments:
        return mesh

    var width: float = max(wire_thickness * WIRE_THICKNESS_SCALE, MIN_SEGMENT_WIDTH)
    var surface_tool: SurfaceTool = SurfaceTool.new()
    surface_tool.begin(Mesh.PRIMITIVE_TRIANGLES)

    for segment: SegmentData in segments:
        _append_ribbon_segment(surface_tool, segment, width)

    var committed_mesh: ArrayMesh = surface_tool.commit()
    return committed_mesh


func _append_ribbon_segment(surface_tool: SurfaceTool, segment: SegmentData, width: float) -> void:
    var segment_vector: Vector3 = segment.finish - segment.start
    var segment_length: float = segment_vector.length()
    if segment_length <= EPSILON:
        return

    var segment_direction: Vector3 = segment_vector / segment_length
    var width_axis: Vector3 = _get_ribbon_width_axis(segment_direction, segment.width_axis_hint)
    var depth_axis: Vector3 = width_axis.cross(segment_direction).normalized()
    var half_width: Vector3 = width_axis * (width * 0.5)
    var half_depth: Vector3 = depth_axis * (width * 0.5)

    var start_left_bottom: Vector3 = segment.start - half_width - half_depth
    var start_right_bottom: Vector3 = segment.start + half_width - half_depth
    var start_right_top: Vector3 = segment.start + half_width + half_depth
    var start_left_top: Vector3 = segment.start - half_width + half_depth
    var end_left_bottom: Vector3 = segment.finish - half_width - half_depth
    var end_right_bottom: Vector3 = segment.finish + half_width - half_depth
    var end_right_top: Vector3 = segment.finish + half_width + half_depth
    var end_left_top: Vector3 = segment.finish - half_width + half_depth

    _append_quad(
        surface_tool,
        start_left_top,
        end_left_top,
        end_right_top,
        start_right_top,
        depth_axis,
        segment_length
    )
    _append_quad(
        surface_tool,
        start_right_bottom,
        end_right_bottom,
        end_left_bottom,
        start_left_bottom,
        -depth_axis,
        segment_length
    )
    _append_quad(
        surface_tool,
        start_right_top,
        end_right_top,
        end_right_bottom,
        start_right_bottom,
        width_axis,
        segment_length
    )
    _append_quad(
        surface_tool,
        start_left_bottom,
        end_left_bottom,
        end_left_top,
        start_left_top,
        -width_axis,
        segment_length
    )


func _append_triangle(
    surface_tool: SurfaceTool,
    first: Vector3,
    second: Vector3,
    third: Vector3,
    normal: Vector3,
    u_start: float,
    u_end: float
) -> void:
    surface_tool.set_normal(normal)
    surface_tool.set_uv(Vector2(u_start, 0.0))
    surface_tool.add_vertex(first)
    surface_tool.set_normal(normal)
    surface_tool.set_uv(Vector2(u_start, 1.0))
    surface_tool.add_vertex(second)
    surface_tool.set_normal(normal)
    surface_tool.set_uv(Vector2(u_end, 0.0))
    surface_tool.add_vertex(third)


func _append_quad(
    surface_tool: SurfaceTool,
    first: Vector3,
    second: Vector3,
    third: Vector3,
    fourth: Vector3,
    normal: Vector3,
    u_end: float
) -> void:
    _append_triangle(surface_tool, first, second, third, normal, 0.0, u_end)
    _append_triangle(surface_tool, first, third, fourth, normal, 0.0, u_end)


func _get_ribbon_width_axis(segment_direction: Vector3, axis_hint: Vector3) -> Vector3:
    var axis: Vector3 = segment_direction.cross(Vector3.UP)
    if axis.length_squared() <= EPSILON:
        axis = axis_hint - segment_direction * segment_direction.dot(axis_hint)
    if axis.length_squared() <= EPSILON:
        axis = segment_direction.cross(Vector3.RIGHT)
    if axis.length_squared() <= EPSILON:
        axis = segment_direction.cross(Vector3.FORWARD)
    if axis.length_squared() <= EPSILON:
        return Vector3.RIGHT
    return axis.normalized()
