@tool
extends Track3D
class_name TrackNormal3D

func _init() -> void:
    type = TrackServer.TRACK_NORMAL

func _enter_tree():
    super._enter_tree()
    _create_track()
    _update_track_data()
    _dirty = true

func _exit_tree():
    _free_track()
    super._exit_tree()

func _update_track_curve() -> void:
    TrackServer.track_update_curves(_track_rid, curve, null)

func _update_track_data() -> void:
    if not _track_rid.is_valid():
        return

    self.global_position = _get_global_aabb().get_center()
    _update_track_curve()

    TrackServer.track_update(
        _track_rid,
        type,
        track_name,
        width,
    )
    TrackServer.track_update_properties(_track_rid, damage_flag, friction, joint_spacing)
    TrackServer.track_set_velocity(_track_rid, speed_limit)
    TrackServer.track_set_meta(_track_rid, meta)

func _process_dirty(_delta: float) -> void:
    super._process_dirty(_delta)
    _update_track_data()
    _update_track_rendering()

func _update_track_rendering():
    if not _track_rid or not _track_rid.is_valid() or not _track_render_rid or not _track_render_rid.is_valid() or not curve:
        return

    TrackRenderingServer.track_set_description(_track_render_rid, get_track_data())
    TrackRenderingServer.track_rebuild(_track_render_rid)
    TrackRenderingServer.track_set_scenario(_track_render_rid, get_world_3d().scenario)
    TrackRenderingServer.track_set_visible(_track_render_rid, visible)

func _create_track() -> void:
    _free_track()
    _track_rid = TrackServer.track_create()
    _track_render_rid = TrackRenderingServer.track_create(_track_rid)
    TrackRenderingServer.track_set_scenario(_track_render_rid, get_world_3d().scenario)

func _free_track() -> void:
    if _track_render_rid.is_valid():
        TrackRenderingServer.track_free(_track_render_rid)
    if _track_rid.is_valid():
        TrackServer.track_free(_track_rid)
    _track_rid = RID()
    _track_render_rid = RID()
