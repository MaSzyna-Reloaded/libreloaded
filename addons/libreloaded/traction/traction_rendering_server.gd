@tool
extends Node

## Draws the traction's spans, each by a RID of its own, while the camera is near
## (SceneryStreamingServer). It keeps the spans, their streaming, scenario, transform, visibility and
## description (what a span is, a TractionData), and forwards the drawing to the rendering backend
## chosen (set_rendering_backend()): the core's plain wires, or one a plugin registers
## (register_rendering_backend()).

## The tracks' rendering server's script: how far tracks and their traction are drawn
## The backend the spans are drawn by at the start, by its name
const RENDERING_BACKEND_SETTING: String = "reloaded/traction/backend"
## The core's own backend
const PLAIN_BACKEND: StringName = &"plain"

enum Wires {
    CONTACT,
    CONTACT_SUPPORT,
    TWO_CONTACT_ONE_SUPPORT,
    TWO_CONTACT_TWO_SUPPORT
}

var _tractions: Dictionary[RID, TractionRenderingBackend.TractionState] = {}
var _next_traction_id: int = 0
var _stream_owner: int = -1
var _backends: Dictionary[StringName, TractionRenderingBackend] = {PLAIN_BACKEND: TractionRenderingBackend.new()}
var _backend_name: StringName = PLAIN_BACKEND
var _backend: TractionRenderingBackend = _backends[PLAIN_BACKEND]


## A backend the spans can be drawn by, by its name; the one the project setting names is chosen as
## it comes
func register_rendering_backend(backend_name: StringName, backend: TractionRenderingBackend) -> void:
    _backends[backend_name] = backend
    if StringName(ProjectSettings.get_setting(RENDERING_BACKEND_SETTING, PLAIN_BACKEND)) == backend_name:
        set_rendering_backend(backend_name)


## Takes a registered backend away; its spans are drawn by the plain look
func unregister_rendering_backend(backend_name: StringName) -> void:
    if _backend_name == backend_name:
        set_rendering_backend(PLAIN_BACKEND)
    _backends.erase(backend_name)


## Every span is drawn by the backend named [param backend_name] from now on - each one of them made
## again by it
func set_rendering_backend(backend_name: StringName) -> void:
    var backend: TractionRenderingBackend = _backends.get(backend_name)
    if not backend or backend == _backend:
        return
    for traction_rid: RID in _tractions:
        var old: TractionRenderingBackend.TractionState = _tractions[traction_rid]
        _backend.free_state(old)
        var state: TractionRenderingBackend.TractionState = backend.make_state()
        state.stream_rid = old.stream_rid
        state.streamed = old.streamed
        state.scenario = old.scenario
        state.transform = old.transform
        state.visible = old.visible
        state.description = old.description
        _tractions[traction_rid] = state
    _backend = backend
    _backend_name = backend_name
    for traction_rid: RID in _tractions:
        var state: TractionRenderingBackend.TractionState = _tractions[traction_rid]
        _backend.apply_scenario(state)
        _backend.apply_transform(state)
        _backend.apply_visible(state)
        _build_if_in_range(state)


func get_rendering_backend() -> TractionRenderingBackend:
    return _backend


## The state kept for the span of [param traction_rid], null for none
func traction_get_state(traction_rid: RID) -> TractionRenderingBackend.TractionState:
    return _tractions.get(traction_rid)


func create_traction() -> RID:
    _next_traction_id += 1
    var traction_rid: RID = rid_from_int64(_next_traction_id)
    _tractions[traction_rid] = _backend.make_state()
    return traction_rid


func free_traction(traction_rid: RID) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    if state.stream_rid.is_valid():
        SceneryStreamingServer.stream_free(state.stream_rid)
    _backend.free_state(state)
    _tractions.erase(traction_rid)


func set_traction_scenario(traction_rid: RID, scenario: RID) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.scenario = scenario
    _backend.apply_scenario(state)


func set_traction_transform(traction_rid: RID, transform: Transform3D) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.transform = transform
    _backend.apply_transform(state)


func set_traction_visible(traction_rid: RID, visible: bool) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.visible = visible
    _backend.apply_visible(state)


## What the span is: where its wires hang and how they are strung; the backend picks its look
func set_traction_description(traction_rid: RID, description: TractionData) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.description = description
    _build_if_in_range(state)


## Registers the span for streaming instead of building its wires: a scenery has thousands of
## spans. The meshes are built when the camera comes within the tracks' distance of the span
## (SceneryStreamingServer) and dropped when it leaves.
func stream_traction(traction_rid: RID) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state or not state.description:
        return
    if _stream_owner < 0:
        _stream_owner = SceneryStreamingServer.owner_create("traction", Callable(), _stream_build, _stream_clear)
    # drawn as far as the tracks under it
    state.stream_rid = SceneryStreamingServer.stream_register(
        _stream_owner, traction_rid, state.description.contact_p1, TrackRenderingServer.tracks_distance
    )
    _stream_clear(traction_rid)


## A span built directly is built now, a streamed one only while in range
func _build_if_in_range(state: TractionRenderingBackend.TractionState) -> void:
    if state.stream_rid.is_valid() and not state.streamed:
        return # out of range: built by _stream_build() when the camera comes into range
    _backend.build(state)


func _stream_build(traction_rid: RID, _preloaded: Variant) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.streamed = true
    _backend.build(state)


func _stream_clear(traction_rid: RID) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.streamed = false
    _backend.clear(state)
