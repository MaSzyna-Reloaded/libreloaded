#include "rendering/TrackRenderingBackend.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void TrackRenderingBackend::_bind_methods() {
        ClassDB::bind_method(D_METHOD("track_create", "track_render", "track"), &TrackRenderingBackend::track_create);
        ClassDB::bind_method(D_METHOD("track_free", "track_render"), &TrackRenderingBackend::track_free);
        ClassDB::bind_method(
                D_METHOD("track_set_description", "track_render", "description"),
                &TrackRenderingBackend::track_set_description);
        ClassDB::bind_method(
                D_METHOD("track_set_visible", "track_render", "visible"), &TrackRenderingBackend::track_set_visible);
        ClassDB::bind_method(
                D_METHOD("track_set_scenario", "track_render", "scenario"), &TrackRenderingBackend::track_set_scenario);
        ClassDB::bind_method(
                D_METHOD("track_set_switch_blade_offsets", "track_render", "offset1", "offset2"),
                &TrackRenderingBackend::track_set_switch_blade_offsets);
        ClassDB::bind_method(D_METHOD("track_stream", "track_render"), &TrackRenderingBackend::track_stream);
        ClassDB::bind_method(D_METHOD("track_build", "track_render"), &TrackRenderingBackend::track_build);
        ClassDB::bind_method(D_METHOD("track_clear", "track_render"), &TrackRenderingBackend::track_clear);
        ClassDB::bind_method(D_METHOD("topology_rebuilt"), &TrackRenderingBackend::topology_rebuilt);
    }
} // namespace godot
