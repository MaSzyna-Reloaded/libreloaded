#include "rendering/TrackRenderingServer.hpp"

#include "game_data/GameDataServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "tracks/TrackCurve.hpp"
#include "tracks/TrackServer.hpp"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void TrackRenderingServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("backend_register", "name", "backend_id"), &TrackRenderingServer::backend_register);
        ClassDB::bind_method(D_METHOD("backend_unregister", "name"), &TrackRenderingServer::backend_unregister);
        ClassDB::bind_method(D_METHOD("backend_get", "name"), &TrackRenderingServer::backend_get);
        ClassDB::bind_method(D_METHOD("backend_get_list"), &TrackRenderingServer::backend_get_list);
        ClassDB::bind_method(D_METHOD("track_create", "track"), &TrackRenderingServer::track_create);
        ClassDB::bind_method(D_METHOD("track_free", "track_render"), &TrackRenderingServer::track_free);
        ClassDB::bind_method(D_METHOD("track_free_many", "track_renders"), &TrackRenderingServer::track_free_many);
        ClassDB::bind_method(
                D_METHOD("track_set_description", "track_render", "description"),
                &TrackRenderingServer::track_set_description);
        ClassDB::bind_method(
                D_METHOD("track_set_preferred_backend", "track_render", "backend"),
                &TrackRenderingServer::track_set_preferred_backend);
        ClassDB::bind_method(
                D_METHOD("track_set_visible", "track_render", "visible"), &TrackRenderingServer::track_set_visible);
        ClassDB::bind_method(
                D_METHOD("track_set_scenario", "track_render", "scenario"), &TrackRenderingServer::track_set_scenario);
        ClassDB::bind_method(D_METHOD("track_stream", "track_render"), &TrackRenderingServer::track_stream);
        ClassDB::bind_method(D_METHOD("track_rebuild", "track_render"), &TrackRenderingServer::track_rebuild);
        ClassDB::bind_method(D_METHOD("track_get_track", "track_render"), &TrackRenderingServer::track_get_track);
        ClassDB::bind_method(
                D_METHOD("track_get_description", "track_render"), &TrackRenderingServer::track_get_description);
        ClassDB::bind_method(D_METHOD("track_get_render_rid", "track"), &TrackRenderingServer::track_get_render_rid);
        ClassDB::bind_method(D_METHOD("track_get_render_rids"), &TrackRenderingServer::track_get_render_rids);

        ClassDB::bind_method(D_METHOD("get_tracks_distance"), &TrackRenderingServer::get_tracks_distance);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "tracks_distance", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_READ_ONLY), "",
                "get_tracks_distance");

        BIND_CONSTANT(TRACKS_DISTANCE);
    }

    TrackRenderingServer::TrackRenderingServer() {
        singleton = this;
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL(streaming);
        // the owner lives as long as the streaming does
        stream_owner = streaming->owner_create(
                "tracks", Callable(), callable_mp(this, &TrackRenderingServer::_stream_build),
                callable_mp(this, &TrackRenderingServer::_stream_clear));
        tracks_distance = ProjectSettings::get_singleton()->get_setting(TRACKS_DISTANCE_SETTING, TRACKS_DISTANCE);
        if (TrackServer *track_server = TrackServer::get_instance(); track_server != nullptr) {
            track_server->connect(
                    TrackServer::topology_rebuilt_signal,
                    callable_mp(this, &TrackRenderingServer::_on_topology_rebuilt));
            track_server->connect(
                    TrackServer::switch_offset_changed_signal,
                    callable_mp(this, &TrackRenderingServer::_on_switch_offset_changed));
        }
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            game_data->connect(
                    GameDataServer::data_reload_requested_signal,
                    callable_mp(this, &TrackRenderingServer::_on_data_reload_requested));
        }
    }

    TrackRenderingServer::~TrackRenderingServer() {
        if (TrackServer *track_server = TrackServer::get_instance(); track_server != nullptr) {
            track_server->disconnect(
                    TrackServer::topology_rebuilt_signal,
                    callable_mp(this, &TrackRenderingServer::_on_topology_rebuilt));
            track_server->disconnect(
                    TrackServer::switch_offset_changed_signal,
                    callable_mp(this, &TrackRenderingServer::_on_switch_offset_changed));
        }
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            game_data->disconnect(
                    GameDataServer::data_reload_requested_signal,
                    callable_mp(this, &TrackRenderingServer::_on_data_reload_requested));
        }
        singleton = nullptr;
    }

    void TrackRenderingServer::backend_register(const StringName &p_name, const uint64_t p_backend_id) {
        TrackRenderingBackend *backend =
                Object::cast_to<TrackRenderingBackend>(ObjectDB::get_instance(ObjectID(p_backend_id)));
        ERR_FAIL_NULL_MSG(backend, "TrackRenderingServer: " + String(p_name) + " is no TrackRenderingBackend");
        ERR_FAIL_COND_MSG(backends.has(p_name), "TrackRenderingServer: backend " + String(p_name) + " is registered");
        backends[p_name] = backend;
    }

    void TrackRenderingServer::backend_unregister(const StringName &p_name) {
        TrackRenderingBackend **backend = backends.getptr(p_name);
        ERR_FAIL_NULL(backend);
        for (KeyValue<RID, Track> &item: tracks) {
            if (item.value.backend == p_name) {
                (*backend)->track_free(item.key);
                item.value.backend = StringName();
            }
        }
        backends.erase(p_name);
    }

    uint64_t TrackRenderingServer::backend_get(const StringName &p_name) const {
        TrackRenderingBackend *const *backend = backends.getptr(p_name);
        return backend != nullptr ? (*backend)->get_instance_id() : 0;
    }

    PackedStringArray TrackRenderingServer::backend_get_list() const {
        PackedStringArray result;
        for (const KeyValue<StringName, TrackRenderingBackend *> &entry: backends) {
            result.push_back(entry.key);
        }
        return result;
    }

    RID TrackRenderingServer::track_create(const RID &p_track) {
        const RID track_render = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        Track track;
        track.track = p_track;
        tracks.insert(track_render, track);
        track_renders_by_track[p_track] = track_render;
        _assign_backend(track_render, tracks[track_render]);
        return track_render;
    }

    void TrackRenderingServer::track_free(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        if (track->stream_rid.is_valid()) {
            if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
                streaming->stream_free(track->stream_rid);
            }
        }
        if (TrackRenderingBackend *backend = _get_backend(*track); backend != nullptr) {
            backend->track_free(p_track_render);
        }
        if (const RID *drawn = track_renders_by_track.getptr(track->track);
            drawn != nullptr && *drawn == p_track_render) {
            track_renders_by_track.erase(track->track);
        }
        tracks.erase(p_track_render);
    }

    void TrackRenderingServer::track_free_many(const TypedArray<RID> &p_track_renders) {
        for (int index = 0; index < p_track_renders.size(); index++) {
            track_free(p_track_renders[index]);
        }
    }

    void TrackRenderingServer::track_set_description(const RID &p_track_render, const Ref<TrackData> &p_description) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        track->description = p_description;
        if (_get_backend_name(*track) != track->backend) {
            _assign_backend(p_track_render, *track);
            return;
        }
        if (TrackRenderingBackend *backend = _get_backend(*track); backend != nullptr) {
            backend->track_set_description(p_track_render, p_description);
        }
    }

    void TrackRenderingServer::track_set_preferred_backend(const RID &p_track_render, const StringName &p_backend) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        track->preferred_backend = p_backend;
        if (_get_backend_name(*track) != track->backend) {
            _assign_backend(p_track_render, *track);
        }
    }

    void TrackRenderingServer::track_set_visible(const RID &p_track_render, const bool p_visible) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        track->visible = p_visible;
        if (TrackRenderingBackend *backend = _get_backend(*track); backend != nullptr) {
            backend->track_set_visible(p_track_render, p_visible);
        }
    }

    void TrackRenderingServer::track_set_scenario(const RID &p_track_render, const RID &p_scenario) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        track->scenario = p_scenario;
        if (TrackRenderingBackend *backend = _get_backend(*track); backend != nullptr) {
            backend->track_set_scenario(p_track_render, p_scenario);
        }
    }

    void TrackRenderingServer::track_stream(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        TrackRenderingBackend *backend = _get_backend(*track);
        TrackServer *track_server = TrackServer::get_instance();
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        if (backend == nullptr || track_server == nullptr || streaming == nullptr || track->stream_rid.is_valid()) {
            return;
        }
        const Ref<TrackCurve> curve = track_server->track_get_curve(track->track);
        if (curve.is_null() || backend->track_stream(p_track_render)) {
            return;
        }
        track->stream_rid = streaming->stream_register(
                stream_owner, p_track_render, curve->get_p1(), static_cast<float>(tracks_distance));
    }

    void TrackRenderingServer::track_rebuild(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        TrackRenderingBackend *backend = _get_backend(*track);
        // out of range: built by _stream_build() when the camera comes back
        if (backend == nullptr || (track->stream_rid.is_valid() && !track->streamed)) {
            return;
        }
        backend->track_build(p_track_render);
    }

    double TrackRenderingServer::get_tracks_distance() const {
        return tracks_distance;
    }

    RID TrackRenderingServer::track_get_track(const RID &p_track_render) const {
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, RID());
        return track->track;
    }

    Ref<TrackData> TrackRenderingServer::track_get_description(const RID &p_track_render) const {
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, Ref<TrackData>());
        return track->description;
    }

    RID TrackRenderingServer::track_get_render_rid(const RID &p_track) const {
        const RID *track_render = track_renders_by_track.getptr(p_track);
        return track_render != nullptr ? *track_render : RID();
    }

    TypedArray<RID> TrackRenderingServer::track_get_render_rids() const {
        TypedArray<RID> track_renders;
        for (const KeyValue<RID, Track> &item: tracks) {
            track_renders.push_back(item.key);
        }
        return track_renders;
    }

    StringName TrackRenderingServer::_get_backend_name(const Track &p_track) const {
        StringName named = p_track.description.is_valid() ? p_track.description->get_rendering_backend() : StringName();
        if (named != StringName() && backends.has(named)) {
            return named;
        }
        StringName setting = ProjectSettings::get_singleton()->get_setting(BACKEND_SETTING, AUTO_BACKEND);
        if (setting != StringName(AUTO_BACKEND) && backends.has(setting)) {
            return setting;
        }
        StringName best;
        double best_priority = -Math::INF;
        for (const KeyValue<StringName, TrackRenderingBackend *> &entry: backends) {
            const double priority = entry.value->get_priority();
            if (priority > best_priority || (priority == best_priority && entry.key == p_track.preferred_backend)) {
                best = entry.key;
                best_priority = priority;
            }
        }
        return best;
    }

    TrackRenderingBackend *TrackRenderingServer::_get_backend(const Track &p_track) const {
        TrackRenderingBackend *const *backend = backends.getptr(p_track.backend);
        return backend != nullptr ? *backend : nullptr;
    }

    void TrackRenderingServer::_assign_backend(const RID &p_track_render, Track &p_track) {
        if (TrackRenderingBackend *old = _get_backend(p_track); old != nullptr) {
            old->track_free(p_track_render);
        }
        p_track.backend = _get_backend_name(p_track);
        TrackRenderingBackend *backend = _get_backend(p_track);
        ERR_FAIL_NULL_MSG(backend, "TrackRenderingServer: no backend " + String(p_track.backend));
        backend->track_create(p_track_render, p_track.track);
        backend->track_set_scenario(p_track_render, p_track.scenario);
        backend->track_set_visible(p_track_render, p_track.visible);
        if (p_track.description.is_valid()) {
            backend->track_set_description(p_track_render, p_track.description);
        }
    }

    void TrackRenderingServer::_stream_build(const RID &p_track_render, const Variant & /* p_preloaded */) {
        Track *track = tracks.getptr(p_track_render);
        if (track == nullptr) {
            return;
        }
        track->streamed = true;
        track_rebuild(p_track_render);
    }

    void TrackRenderingServer::_stream_clear(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        if (track == nullptr) {
            return;
        }
        track->streamed = false;
        if (TrackRenderingBackend *backend = _get_backend(*track); backend != nullptr) {
            backend->track_clear(p_track_render);
        }
    }

    void TrackRenderingServer::_on_topology_rebuilt() {
        for (const KeyValue<StringName, TrackRenderingBackend *> &item: backends) {
            item.value->topology_rebuilt();
        }
    }

    /// The blades follow the switch's animation step for every switch, a scenery's too - a scenery
    /// has no TrackSwitch3D nodes to forward it
    void TrackRenderingServer::_on_switch_offset_changed(const RID &p_switch, const double /* p_offset */) {
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        const RID track_render = track_get_render_rid(track_server->switch_get_track(p_switch));
        Track *track = tracks.getptr(track_render);
        if (track == nullptr) {
            return;
        }
        if (TrackRenderingBackend *backend = _get_backend(*track); backend != nullptr) {
            backend->track_set_switch_blade_offsets(
                    track_render, track_server->switch_get_f_offset1(p_switch),
                    track_server->switch_get_f_offset2(p_switch));
        }
    }

    /// The data read again: every track built again - a streamed one as it is streamed, one built
    /// directly at once
    void TrackRenderingServer::_on_data_reload_requested() {
        for (const KeyValue<RID, Track> &item: tracks) {
            if (!item.value.stream_rid.is_valid()) {
                track_rebuild(item.key);
            }
        }
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            streaming->owner_rebuild(stream_owner);
        }
    }
} // namespace godot
