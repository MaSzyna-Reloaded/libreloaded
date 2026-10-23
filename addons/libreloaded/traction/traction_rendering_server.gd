@tool
extends Node

## Draws the traction's spans, each by a RID of its own, while the camera is near
## (SceneryStreamingServer). It keeps the spans, their streaming, scenario, transform, visibility and
## description (what a span is, a TractionData), and forwards the drawing of each span to the backend
## its data names (TractionData.rendering_backend, RENDERING_BACKEND_SETTING's for none): the core's
## plain wires, or one a plugin registers (backend_register()).

## The backend that draws the spans whose data names none
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
## The backend drawing each span
var _traction_backends: Dictionary[RID, TractionRenderingBackend] = {}
var _next_traction_id: int = 0
var _stream_owner: int = -1
var _backends: Dictionary[StringName, TractionRenderingBackend] = {PLAIN_BACKEND: TractionRenderingBackend.new()}


## [param backend] draws the spans whose data names [param backend_name] - those there already are
## made again by it
func backend_register(backend_name: StringName, backend: TractionRenderingBackend) -> void:
    _backends[backend_name] = backend
    _choose_backends()


## The spans [param backend_name] drew are made again by the backend chosen for them now
func backend_unregister(backend_name: StringName) -> void:
    _backends.erase(backend_name)
    _choose_backends()


## The backend registered as [param backend_name]; null for none
func backend_get(backend_name: StringName) -> TractionRenderingBackend:
    return _backends.get(backend_name)


func backend_get_list() -> PackedStringArray:
    return PackedStringArray(_backends.keys())


## The state kept for the span of [param traction_rid], null for none
func traction_get_state(traction_rid: RID) -> TractionRenderingBackend.TractionState:
    return _tractions.get(traction_rid)


func create_traction() -> RID:
    _next_traction_id += 1
    var traction_rid: RID = rid_from_int64(_next_traction_id)
    var backend: TractionRenderingBackend = _backends[_get_backend_name(null)]
    _tractions[traction_rid] = backend.make_state()
    _traction_backends[traction_rid] = backend
    return traction_rid


func free_traction(traction_rid: RID) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    if state.stream_rid.is_valid():
        SceneryStreamingServer.stream_free(state.stream_rid)
    _traction_backends[traction_rid].free_state(state)
    _tractions.erase(traction_rid)
    _traction_backends.erase(traction_rid)


func set_traction_scenario(traction_rid: RID, scenario: RID) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.scenario = scenario
    _traction_backends[traction_rid].apply_scenario(state)


func set_traction_transform(traction_rid: RID, transform: Transform3D) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.transform = transform
    _traction_backends[traction_rid].apply_transform(state)


func set_traction_visible(traction_rid: RID, visible: bool) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.visible = visible
    _traction_backends[traction_rid].apply_visible(state)


## What the span is: where its wires hang, how they are strung and which backend draws it; the
## backend picks its look
func set_traction_description(traction_rid: RID, description: TractionData) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.description = description
    _choose_backend(traction_rid)
    _build_if_in_range(traction_rid)


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
func _build_if_in_range(traction_rid: RID) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions[traction_rid]
    if state.stream_rid.is_valid() and not state.streamed:
        return # out of range: built by _stream_build() when the camera comes into range
    _traction_backends[traction_rid].build(state)


## The name of the backend that draws a span of [param description]: the one its data names, else
## the one the project setting names, else the plain one
func _get_backend_name(description: TractionData) -> StringName:
    if description and description.rendering_backend in _backends:
        return description.rendering_backend
    var default_name: StringName = ProjectSettings.get_setting(RENDERING_BACKEND_SETTING, PLAIN_BACKEND)
    return default_name if default_name in _backends else PLAIN_BACKEND


## Every span whose backend is another one now is made again by it
func _choose_backends() -> void:
    for traction_rid: RID in _tractions:
        if _choose_backend(traction_rid):
            _build_if_in_range(traction_rid)


## The span is made again by the backend chosen for it, when that is another one than draws it;
## true when it was
func _choose_backend(traction_rid: RID) -> bool:
    var old: TractionRenderingBackend.TractionState = _tractions[traction_rid]
    var backend: TractionRenderingBackend = _backends[_get_backend_name(old.description)]
    if backend == _traction_backends[traction_rid]:
        return false
    _traction_backends[traction_rid].free_state(old)
    var state: TractionRenderingBackend.TractionState = backend.make_state()
    state.stream_rid = old.stream_rid
    state.streamed = old.streamed
    state.scenario = old.scenario
    state.transform = old.transform
    state.visible = old.visible
    state.description = old.description
    _tractions[traction_rid] = state
    _traction_backends[traction_rid] = backend
    backend.apply_scenario(state)
    backend.apply_transform(state)
    backend.apply_visible(state)
    return true


func _stream_build(traction_rid: RID, _preloaded: Variant) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.streamed = true
    _traction_backends[traction_rid].build(state)


func _stream_clear(traction_rid: RID) -> void:
    var state: TractionRenderingBackend.TractionState = _tractions.get(traction_rid)
    if not state:
        return
    state.streamed = false
    _traction_backends[traction_rid].clear(state)
