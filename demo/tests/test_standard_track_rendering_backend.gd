extends ReloadedGutTest

## The core's track backend (StandardTrackRenderingBackend) draws a track as its TrackData says: two
## rails, a bed of its kind and the formation under it, a road's carriageway and shoulders

const TRACK_LENGTH: float = 20.0
## The rails' heads' top: the rails stand on the track's curve (Track.cpp:555)
const RAIL_HEAD: float = 0.18
## A crushed stone bed's foot: the sleeper's height and the ballast's depth under it
const BALLAST_FOOT: float = -(0.2 + 0.3)
## Its half width there: the crest (half a sleeper and the shoulder) and the slope down to the foot
const BALLAST_FOOT_HALF_WIDTH: float = 1.3 + 0.4 + (0.2 + 0.3 - 0.03) * 1.5
## A backend's priority unless it gives its own (TrackRenderingBackend.DEFAULT_PRIORITY)
const DEFAULT_PRIORITY: float = 1.0

var _backend: StandardTrackRenderingBackend = null
var _track: RID = RID()
var _track_render: RID = RID()


func before_each() -> void:
    _backend = instance_from_id(TrackRenderingServer.backend_get(&"standard"))
    _track = build_track("standard", TRACK_LENGTH)
    _track_render = TrackRenderingServer.track_create(_track)


func after_each() -> void:
    TrackRenderingServer.track_free(_track_render)
    TrackServer.track_free(_track)


func _build(description: TrackData) -> ArrayMesh:
    TrackRenderingServer.track_set_description(_track_render, description)
    TrackRenderingServer.track_rebuild(_track_render)
    return _backend.track_get_mesh(_track_render)


func test_crushed_stone_track_has_rails_and_ballast() -> void:
    var mesh: ArrayMesh = _build(TrackData.new())
    assert_eq(mesh.get_surface_count(), 2, "rails, ballast; no formation where the track gives none")
    var bounds: AABB = mesh.get_aabb()
    assert_almost_eq(bounds.end.y, RAIL_HEAD, 0.001, "the rails' heads on the curve")
    assert_almost_eq(bounds.position.y, BALLAST_FOOT, 0.001, "the ballast's foot")
    assert_almost_eq(bounds.size.z, 2.0 * BALLAST_FOOT_HALF_WIDTH, 0.001, "the ballast's foot")


func test_formation_height_banks_the_track() -> void:
    var description: TrackData = TrackData.new()
    description.formation_height = 2.0
    var bounds: AABB = _build(description).get_aabb()
    assert_almost_eq(bounds.position.y, BALLAST_FOOT - 2.0, 0.001, "the bank's foot")
    assert_almost_eq(bounds.size.z,
            2.0 * (BALLAST_FOOT_HALF_WIDTH + description.formation_width + 2.0 * description.formation_slope), 0.001,
            "the slopes out by the height times the slope")


func test_negative_formation_height_cuts_the_track_in() -> void:
    var description: TrackData = TrackData.new()
    description.formation_height = -1.0
    var bounds: AABB = _build(description).get_aabb()
    assert_almost_eq(bounds.end.y, BALLAST_FOOT + 1.0, 0.001, "the cutting's slopes rise over the rails")


func test_embedded_track_has_rails_and_street_without_formation() -> void:
    var description: TrackData = TrackData.new()
    description.bed = TrackData.BED_EMBEDDED
    description.formation_height = 2.0
    var mesh: ArrayMesh = _build(description)
    assert_eq(mesh.get_surface_count(), 2, "rails, street")
    assert_almost_eq(mesh.get_aabb().position.y, 0.0, 0.001, "nothing under the rails' feet")


func test_road_has_carriageway_and_shoulders_where_it_has_them() -> void:
    TrackServer.track_update(_track, TrackServer.TRACK_ROAD, "standard", 6.0)
    var description: TrackData = TrackData.new()
    description.width = 6.0
    assert_eq(_build(description).get_surface_count(), 1, "the carriageway")
    description.shoulder = true
    var mesh: ArrayMesh = _build(description)
    assert_eq(mesh.get_surface_count(), 2, "the carriageway and its shoulders")
    assert_almost_eq(mesh.get_aabb().size.z, 6.0 + 2.0 * 0.75, 0.001, "the shoulders beside it")


func test_cleared_track_draws_nothing() -> void:
    _build(TrackData.new())
    _backend.track_clear(_track_render)
    assert_null(_backend.track_get_mesh(_track_render))


func test_track_data_is_drawn_by_the_default_backend() -> void:
    assert_not_null(_build(TrackData.new()), "a core TrackData names no backend: the default one draws it")


## The core's tracks are unfinished: a plugin's backend of the default priority draws them
func test_standard_backend_is_under_the_default_priority() -> void:
    assert_lt(_backend.get_priority(), DEFAULT_PRIORITY)


## By default ("auto") a track is drawn by the best backend registered, whatever its data names
func test_track_of_an_unregistered_backend_s_data_is_drawn_by_the_best_registered() -> void:
    var description: TrackData = TrackData.new()
    description.rendering_backend = &"unregistered"
    assert_not_null(_build(description), "the only backend registered draws it")


func test_sleepers_every_spacing_under_the_rails() -> void:
    _build(TrackData.new())
    var sleepers: Array = _backend.track_get_sleeper_transforms(_track_render)
    # 20 m at 0.6 m, half a spacing in from either end: 33, one chunk (37.5 m holds 62)
    assert_eq(sleepers.size(), 33)
    assert_eq(_backend.track_get_sleeper_multimeshes(_track_render).size(), 1)
    var first: Transform3D = sleepers[0]
    var second: Transform3D = sleepers[1]
    assert_almost_eq(first.origin.y, 0.0, 0.001, "the sleeper's top under the rails' feet, on the curve")
    assert_almost_eq(first.origin.x, 0.3, 0.01, "half a spacing from the start: none on the joint")
    assert_almost_eq(first.origin.distance_to(second.origin), 0.6, 0.01, "the spacing")
    assert_almost_eq(absf(first.basis.x.dot(Vector3(1.0, 0.0, 0.0))), 0.0, 0.001, "laid across the track")


func test_long_track_has_sleepers_in_chunks() -> void:
    var description: TrackData = TrackData.new()
    description.sleeper_spacing = 1.0
    TrackServer.track_update_curves(_track, _straight(100.0), null)
    _build(description)
    # 100 m at 1 m: 100 sleepers, 37 a chunk (DoS's 37.5 m chunks)
    assert_eq(_backend.track_get_sleeper_transforms(_track_render).size(), 100)
    assert_eq(_backend.track_get_sleeper_multimeshes(_track_render).size(), 3)


func test_slab_and_embedded_tracks_have_no_sleepers() -> void:
    for bed: TrackData.Bed in [TrackData.BED_SLAB, TrackData.BED_EMBEDDED]:
        var description: TrackData = TrackData.new()
        description.bed = bed
        _build(description)
        assert_eq(_backend.track_get_sleeper_multimeshes(_track_render).size(), 0, "bed %d" % bed)


func test_cleared_track_frees_its_sleepers() -> void:
    _build(TrackData.new())
    _backend.track_clear(_track_render)
    assert_eq(_backend.track_get_sleeper_multimeshes(_track_render).size(), 0)


func _straight(length: float) -> TrackCurve:
    var curve: TrackCurve = TrackCurve.new()
    curve.p2 = Vector3(length, 0.0, 0.0)
    return curve
