#include "RailMiniMap.hpp"

#include "tracks/TrackCurve.hpp"
#include "tracks/TrackServer.hpp"
#include "vehicles/base/VehiclePersonRole.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/font.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    namespace {
        // how many straight pieces a track's curve is drawn with
        const int CURVE_SEGMENTS = 16;
        // the factor a zoom button changes the zoom by
        const double ZOOM_STEP = 1.2;
        // the tracks are asked for this share of the view's longer side around it, so the view
        // moves that far before they are asked for again
        const real_t QUERY_MARGIN = 0.5;
        // the vehicles move by themselves: they are drawn again this often [s]
        const double VEHICLES_REDRAW_INTERVAL = 0.1;
        // track names from this zoom on [px/m] - further out they cover each other and the tracks
        const double TRACK_LABEL_MIN_ZOOM = 2.0;
        // switch handles from this zoom on [px/m]
        const double SWITCH_HANDLE_MIN_ZOOM = 0.5;
        const real_t SWITCH_HANDLE_RADIUS = 7.5;
        const int SWITCH_HANDLE_ARC_POINTS = 32;
        constexpr Color SWITCH_HANDLE_COMMON_COLOR = Color(1.0, 1.0, 1.0);
        constexpr Color SWITCH_HANDLE_DIVERGING_COLOR = Color(0.5, 0.5, 0.5);
        constexpr Color SWITCH_HANDLE_BORDER_COLOR = Color(0.0, 0.0, 0.0);
        const real_t VEHICLE_MARKER_SIZE = 10.0;
    } // namespace

    static Vector2 to_map(const Vector3 &p_position) {
        return Vector2(p_position.x, p_position.z);
    }

    /* The track's curve sampled into a polyline in world space (x, z); empty for none */
    static PackedVector2Array sample_curve(const Ref<TrackCurve> &p_curve) {
        PackedVector2Array points;
        if (p_curve.is_null()) {
            return points;
        }
        const Vector3 start = p_curve->get_p1();
        const Vector3 end = p_curve->get_p2();
        const Vector3 control_1 = start + p_curve->get_c1();
        const Vector3 control_2 = end + p_curve->get_c2();
        points.resize(CURVE_SEGMENTS + 1);
        for (int index = 0; index <= CURVE_SEGMENTS; ++index) {
            const real_t t = static_cast<real_t>(index) / CURVE_SEGMENTS;
            points.set(index, to_map(start.bezier_interpolate(control_1, control_2, end, t)));
        }
        return points;
    }

    /* The polyline's pieces appended to a draw_multiline() batch, two points each */
    static void append_segments(PackedVector2Array &p_segments, const PackedVector2Array &p_polyline) {
        for (int index = 1; index < p_polyline.size(); ++index) {
            p_segments.push_back(p_polyline[index - 1]);
            p_segments.push_back(p_polyline[index]);
        }
    }

    void RailMiniMap::_bind_methods() {
        ClassDB::bind_method(D_METHOD("zoom_in"), &RailMiniMap::zoom_in);
        ClassDB::bind_method(D_METHOD("zoom_out"), &RailMiniMap::zoom_out);
        ClassDB::bind_method(D_METHOD("zoom_reset"), &RailMiniMap::zoom_reset);

        ClassDB::bind_method(D_METHOD("set_track_color", "value"), &RailMiniMap::set_track_color);
        ClassDB::bind_method(D_METHOD("get_track_color"), &RailMiniMap::get_track_color);
        ADD_PROPERTY(PropertyInfo(Variant::COLOR, "track_color"), "set_track_color", "get_track_color");
        ClassDB::bind_method(D_METHOD("set_switch_active_color", "value"), &RailMiniMap::set_switch_active_color);
        ClassDB::bind_method(D_METHOD("get_switch_active_color"), &RailMiniMap::get_switch_active_color);
        ADD_PROPERTY(
                PropertyInfo(Variant::COLOR, "switch_active_color"), "set_switch_active_color",
                "get_switch_active_color");
        ClassDB::bind_method(D_METHOD("set_label_color", "value"), &RailMiniMap::set_label_color);
        ClassDB::bind_method(D_METHOD("get_label_color"), &RailMiniMap::get_label_color);
        ADD_PROPERTY(PropertyInfo(Variant::COLOR, "label_color"), "set_label_color", "get_label_color");
        ClassDB::bind_method(D_METHOD("set_vehicle_color", "value"), &RailMiniMap::set_vehicle_color);
        ClassDB::bind_method(D_METHOD("get_vehicle_color"), &RailMiniMap::get_vehicle_color);
        ADD_PROPERTY(PropertyInfo(Variant::COLOR, "vehicle_color"), "set_vehicle_color", "get_vehicle_color");
        ClassDB::bind_method(D_METHOD("set_background_color", "value"), &RailMiniMap::set_background_color);
        ClassDB::bind_method(D_METHOD("get_background_color"), &RailMiniMap::get_background_color);
        ADD_PROPERTY(PropertyInfo(Variant::COLOR, "background_color"), "set_background_color", "get_background_color");
        ClassDB::bind_method(D_METHOD("set_zoom", "value"), &RailMiniMap::set_zoom);
        ClassDB::bind_method(D_METHOD("get_zoom"), &RailMiniMap::get_zoom);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "zoom", PROPERTY_HINT_RANGE, "0.01,100,0.01,or_greater,suffix:px/m"),
                "set_zoom", "get_zoom");
        ClassDB::bind_method(D_METHOD("set_follow_camera_rotation", "value"), &RailMiniMap::set_follow_camera_rotation);
        ClassDB::bind_method(D_METHOD("get_follow_camera_rotation"), &RailMiniMap::get_follow_camera_rotation);
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "follow_camera_rotation"), "set_follow_camera_rotation",
                "get_follow_camera_rotation");
    }

    void RailMiniMap::_notification(const int p_what) {
        TrackServer *tracks = TrackServer::get_instance();
        switch (p_what) {
            case NOTIFICATION_ENTER_TREE: {
                ERR_FAIL_NULL(tracks);
                tracks->connect(
                        TrackServer::tracks_changed_signal, callable_mp(this, &RailMiniMap::_on_tracks_changed));
                tracks->connect(
                        TrackServer::topology_changed_signal, callable_mp(this, &RailMiniMap::_on_tracks_changed));
                tracks->connect(
                        TrackServer::switch_active_track_changed_signal,
                        callable_mp(this, &RailMiniMap::_on_switch_active_track_changed));
                set_process(is_visible_in_tree());
            } break;
            case NOTIFICATION_EXIT_TREE: {
                ERR_FAIL_NULL(tracks);
                tracks->disconnect(
                        TrackServer::tracks_changed_signal, callable_mp(this, &RailMiniMap::_on_tracks_changed));
                tracks->disconnect(
                        TrackServer::topology_changed_signal, callable_mp(this, &RailMiniMap::_on_tracks_changed));
                tracks->disconnect(
                        TrackServer::switch_active_track_changed_signal,
                        callable_mp(this, &RailMiniMap::_on_switch_active_track_changed));
            } break;
            case NOTIFICATION_VISIBILITY_CHANGED: {
                set_process(is_visible_in_tree());
            } break;
            // the view follows the camera at once, the vehicles every VEHICLES_REDRAW_INTERVAL
            case NOTIFICATION_PROCESS: {
                const Camera3D *camera = get_viewport()->get_camera_3d();
                if (camera == nullptr) {
                    return;
                }
                const Transform3D camera_transform = camera->get_global_transform();
                const Vector2 position = to_map(camera_transform.origin);
                const real_t rotation = follow_camera_rotation ? camera_transform.basis.get_euler().y : 0.0;
                vehicles_drawn_ago += get_process_delta_time();
                if (position == camera_position && rotation == camera_rotation &&
                    vehicles_drawn_ago < VEHICLES_REDRAW_INTERVAL) {
                    return;
                }
                camera_position = position;
                camera_rotation = rotation;
                vehicles_drawn_ago = 0.0;
                queue_redraw();
            } break;
            case NOTIFICATION_DRAW: {
                ERR_FAIL_NULL(tracks);
                const Rect2 own_rect = Rect2(Vector2(), get_size());
                draw_rect(own_rect, background_color);
                const Transform2D view_transform = _get_view_transform();
                const Rect2 view_rect = view_transform.affine_inverse().xform(own_rect);

                // the tracks around the view asked for again once the view leaves them; the lines
                // of a track still there are kept
                if (!query_rect.encloses(view_rect)) {
                    query_rect = view_rect.grow(MAX(view_rect.size.x, view_rect.size.y) * QUERY_MARGIN);
                    const TypedArray<RID> found = tracks->tracks_find_in_aabb(query_rect);
                    RIDMap<TrackLines> lines;
                    lines.reserve(found.size());
                    for (int index = 0; index < found.size(); ++index) {
                        const RID track = found[index];
                        // a rail map: roads and rivers are neither drawn nor named
                        if (tracks->track_get_group(track) != TrackServer::GROUP_RAIL) {
                            continue;
                        }
                        const TrackLines *kept = track_lines.getptr(track);
                        if (kept != nullptr) {
                            lines.insert(track, *kept);
                            continue;
                        }
                        TrackLines track_line;
                        track_line.common = sample_curve(tracks->track_get_curve(track, TrackServer::TRACK_COMMON));
                        track_line.name = tracks->track_get_name(track);
                        if (track_line.common.size() > CURVE_SEGMENTS / 2) {
                            track_line.label_point = track_line.common[CURVE_SEGMENTS / 2];
                        }
                        track_line.is_switch = tracks->track_is_switch(track);
                        if (track_line.is_switch) {
                            track_line.diverging =
                                    sample_curve(tracks->track_get_curve(track, TrackServer::TRACK_DIVERGING));
                            track_line.switch_rid = tracks->track_get_switch(track);
                            const int common_index = tracks->track_get_common_endpoint_index(track);
                            const PackedVector3Array endpoints = tracks->track_get_endpoints(track);
                            track_line.has_switch_point =
                                    common_index != TrackServer::POINT_NONE && common_index < endpoints.size();
                            if (track_line.has_switch_point) {
                                track_line.switch_point = to_map(endpoints[common_index]);
                            }
                        }
                        lines.insert(track, track_line);
                    }
                    track_lines = lines;
                    _build_batches();
                }

                // the tracks in world space, through the view's transform; a width of -1 keeps the
                // lines thin at any zoom
                draw_set_transform_matrix(view_transform);
                // Godot refuses an empty batch with an error; append_segments() keeps the count even
                if (!track_segments.is_empty()) {
                    draw_multiline(track_segments, track_color);
                }
                if (!active_segments.is_empty()) {
                    draw_multiline(active_segments, switch_active_color);
                }
                draw_set_transform_matrix(Transform2D());

                const Ref<Font> font = get_theme_default_font();
                const int font_size = get_theme_default_font_size();
                if (zoom >= TRACK_LABEL_MIN_ZOOM) {
                    for (const TrackLabel &label: track_labels) {
                        const Vector2 position = view_transform.xform(label.position);
                        if (!own_rect.has_point(position)) {
                            continue;
                        }
                        const Vector2 text_size =
                                font->get_string_size(label.name, HORIZONTAL_ALIGNMENT_LEFT, -1, font_size);
                        draw_string(
                                font, position + Vector2(-text_size.x, font->get_ascent(font_size)) * 0.5, label.name,
                                HORIZONTAL_ALIGNMENT_LEFT, -1, font_size, label_color);
                    }
                }
                if (zoom >= SWITCH_HANDLE_MIN_ZOOM) {
                    const Rect2 handles_rect = own_rect.grow(SWITCH_HANDLE_RADIUS);
                    for (int index = 0; index < static_cast<int>(switch_handles.size()); ++index) {
                        const Vector2 position = view_transform.xform(switch_handles[index].position);
                        if (!handles_rect.has_point(position)) {
                            continue;
                        }
                        Color color = tracks->switch_get_active_track(switch_handles[index].switch_rid) ==
                                                      TrackServer::TRACK_COMMON
                                              ? SWITCH_HANDLE_COMMON_COLOR
                                              : SWITCH_HANDLE_DIVERGING_COLOR;
                        if (index == hovered_switch) {
                            color = switch_active_color;
                        }
                        draw_circle(position, SWITCH_HANDLE_RADIUS, color);
                        draw_arc(
                                position, SWITCH_HANDLE_RADIUS, 0.0, Math::TAU, SWITCH_HANDLE_ARC_POINTS,
                                SWITCH_HANDLE_BORDER_COLOR);
                    }
                }

                // only a vehicle with somebody aboard or a cabin to drive from is marked and named
                const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
                const VehicleServer *vehicles = VehicleServer::get_instance();
                ERR_FAIL_NULL(rail_vehicles);
                ERR_FAIL_NULL(vehicles);
                const VehiclePersonRole::Role anybody = VehiclePersonRole::VEHICLE_PERSON_ROLE_ANY;
                const TypedArray<RID> in_view = rail_vehicles->vehicle_get_rids_in_rect(view_rect);
                for (int index = 0; index < in_view.size(); ++index) {
                    const RID vehicle = in_view[index];
                    if (!rail_vehicles->vehicle_get_front_cabin(vehicle).is_valid() &&
                        !rail_vehicles->vehicle_get_rear_cabin(vehicle).is_valid() &&
                        !rail_vehicles->vehicle_machine_room_has_person_role(vehicle, anybody)) {
                        continue;
                    }
                    const Vector2 position =
                            view_transform.xform(to_map(rail_vehicles->vehicle_get_transform(vehicle).origin));
                    const Vector2 marker_size = Vector2(VEHICLE_MARKER_SIZE, VEHICLE_MARKER_SIZE);
                    draw_rect(Rect2(position - marker_size * 0.5, marker_size), vehicle_color);
                    draw_string(
                            font, position + Vector2(VEHICLE_MARKER_SIZE, font->get_ascent(font_size) * 0.5),
                            vehicles->vehicle_get_name(vehicle), HORIZONTAL_ALIGNMENT_LEFT, -1, font_size,
                            vehicle_color);
                }
            } break;
            default:
                break;
        }
    }

    Transform2D RailMiniMap::_get_view_transform() const {
        // world (x, z) relative to the camera, turned with it, scaled by the zoom, centred
        return Transform2D(camera_rotation, Size2(zoom, zoom), 0.0, get_size() * 0.5) *
               Transform2D(0.0, -camera_position);
    }

    int RailMiniMap::_find_switch_handle(const Vector2 &p_view_position) const {
        if (zoom < SWITCH_HANDLE_MIN_ZOOM) {
            return -1;
        }
        const Transform2D view_transform = _get_view_transform();
        for (int index = 0; index < static_cast<int>(switch_handles.size()); ++index) {
            if (view_transform.xform(switch_handles[index].position).distance_to(p_view_position) <=
                SWITCH_HANDLE_RADIUS) {
                return index;
            }
        }
        return -1;
    }

    void RailMiniMap::_build_batches() {
        const TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        track_segments.clear();
        active_segments.clear();
        switch_handles.clear();
        track_labels.clear();
        hovered_switch = -1;
        for (const KeyValue<RID, TrackLines> &entry: track_lines) {
            const TrackLines &lines = entry.value;
            if (!lines.name.is_empty()) {
                track_labels.push_back({lines.name, lines.label_point});
            }
            if (!lines.is_switch) {
                append_segments(track_segments, lines.common);
                continue;
            }
            const bool common_active = tracks->switch_get_active_track(lines.switch_rid) == TrackServer::TRACK_COMMON;
            append_segments(common_active ? active_segments : track_segments, lines.common);
            append_segments(common_active ? track_segments : active_segments, lines.diverging);
            if (lines.has_switch_point) {
                switch_handles.push_back({lines.switch_rid, lines.switch_point});
            }
        }
    }

    void RailMiniMap::_on_tracks_changed() {
        track_lines.clear();
        query_rect = Rect2();
        queue_redraw();
    }

    void RailMiniMap::_on_switch_active_track_changed(const RID & /*p_switch*/, const int /*p_active_track*/) {
        _build_batches();
        queue_redraw();
    }

    void RailMiniMap::_gui_input(const Ref<InputEvent> &p_event) {
        const Ref<InputEventMouseMotion> motion = p_event;
        if (motion.is_valid()) {
            const int hovered = _find_switch_handle(motion->get_position());
            if (hovered != hovered_switch) {
                hovered_switch = hovered;
                queue_redraw();
            }
            return;
        }
        const Ref<InputEventMouseButton> button = p_event;
        if (button.is_null() || button->get_button_index() != MOUSE_BUTTON_LEFT || !button->is_pressed()) {
            return;
        }
        const int index = _find_switch_handle(button->get_position());
        TrackServer *tracks = TrackServer::get_instance();
        if (index < 0 || tracks == nullptr) {
            return;
        }
        const RID switch_rid = switch_handles[index].switch_rid;
        tracks->switch_set_active_track(
                switch_rid, tracks->switch_get_active_track(switch_rid) == TrackServer::TRACK_DIVERGING
                                    ? TrackServer::TRACK_COMMON
                                    : TrackServer::TRACK_DIVERGING);
        accept_event();
    }

    String RailMiniMap::_get_tooltip(const Vector2 &p_at_position) const {
        const int index = _find_switch_handle(p_at_position);
        const TrackServer *tracks = TrackServer::get_instance();
        if (index < 0 || tracks == nullptr) {
            return String();
        }
        const RID switch_rid = switch_handles[index].switch_rid;
        const String name = tracks->switch_get_name(switch_rid);
        const String state =
                tracks->switch_get_active_track(switch_rid) == TrackServer::TRACK_COMMON ? "Straight" : "Diverging";
        return vformat("Switch: %s\nState: %s", name.is_empty() ? String("unnamed") : name, state);
    }

    void RailMiniMap::zoom_in() {
        set_zoom(zoom * ZOOM_STEP);
    }

    void RailMiniMap::zoom_out() {
        set_zoom(zoom / ZOOM_STEP);
    }

    void RailMiniMap::zoom_reset() {
        set_zoom(1.0);
    }

    void RailMiniMap::set_track_color(const Color &p_value) {
        track_color = p_value;
        queue_redraw();
    }

    Color RailMiniMap::get_track_color() const {
        return track_color;
    }

    void RailMiniMap::set_switch_active_color(const Color &p_value) {
        switch_active_color = p_value;
        queue_redraw();
    }

    Color RailMiniMap::get_switch_active_color() const {
        return switch_active_color;
    }

    void RailMiniMap::set_label_color(const Color &p_value) {
        label_color = p_value;
        queue_redraw();
    }

    Color RailMiniMap::get_label_color() const {
        return label_color;
    }

    void RailMiniMap::set_vehicle_color(const Color &p_value) {
        vehicle_color = p_value;
        queue_redraw();
    }

    Color RailMiniMap::get_vehicle_color() const {
        return vehicle_color;
    }

    void RailMiniMap::set_background_color(const Color &p_value) {
        background_color = p_value;
        queue_redraw();
    }

    Color RailMiniMap::get_background_color() const {
        return background_color;
    }

    void RailMiniMap::set_zoom(const double p_value) {
        zoom = p_value;
        queue_redraw();
    }

    double RailMiniMap::get_zoom() const {
        return zoom;
    }

    void RailMiniMap::set_follow_camera_rotation(const bool p_value) {
        follow_camera_rotation = p_value;
        queue_redraw();
    }

    bool RailMiniMap::get_follow_camera_rotation() const {
        return follow_camera_rotation;
    }
} // namespace godot
