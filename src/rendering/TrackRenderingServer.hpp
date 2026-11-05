#pragma once
#include "rendering/TrackRenderingBackend.hpp"
#include "utils/RIDHash.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* Draws the tracks and roads of TrackServer, each by a render RID of its own, while the camera is
     * near (SceneryStreamingServer). It keeps a track's streaming, scenario, visibility and
     * description (what the track is, a TrackData) and forwards its drawing to the backend its data
     * names (TrackData.rendering_backend; BACKEND_SETTING's for none): the core's
     * StandardTrackRenderingBackend, or one a plugin registers (backend_register()). */
    class TrackRenderingServer : public Object {
            GDCLASS(TrackRenderingServer, Object)

        public:
            /// How far the tracks, the roads and the traction over them are drawn by default [m]
            /// (the operator, 2026-10-17: under the 20 km draw distance they drew a world's whole
            /// network); a player's setting
            static constexpr double TRACKS_DISTANCE = 2000.0;
            static constexpr const char *TRACKS_DISTANCE_SETTING = "reloaded/scenery/tracks/distance";
            /// The backend a track's data names none for, by its name
            static constexpr const char *BACKEND_SETTING = "reloaded/tracks/backend";
            /// The core's own backend (StandardTrackRenderingBackend), the default one
            static constexpr const char *DEFAULT_BACKEND = "standard";

        private:
            struct Track {
                    RID track;
                    Ref<TrackData> description;
                    RID scenario;
                    bool visible = true;
                    /// What it is drawn with, by the backend (track_set_looks())
                    Dictionary looks;
                    /// The backend drawing it, by its name; empty until it has one
                    StringName backend;
                    /// SceneryStreamingServer's registration; invalid for a track built directly
                    RID stream_rid;
                    /// Within the camera's range while it is streamed
                    bool streamed = false;
            };
            RIDMap<Track> tracks;
            RIDMap<RID> track_renders_by_track;
            HashMap<StringName, TrackRenderingBackend *> backends;
            int stream_owner = 0;
            double tracks_distance = TRACKS_DISTANCE;

            /// The backend the track's description names, the default one for none
            StringName _get_backend_name(const Track &p_track) const;
            TrackRenderingBackend *_get_backend(const Track &p_track) const;
            /// The track handed to the backend its description names, taken from the one it had
            void _assign_backend(const RID &p_track_render, Track &p_track);
            void _stream_build(const RID &p_track_render, const Variant &p_preloaded);
            void _stream_clear(const RID &p_track_render);
            void _on_topology_rebuilt();
            void _on_switch_offset_changed(const RID &p_switch, double p_offset);
            void _on_data_reload_requested();

        protected:
            static void _bind_methods();

        public:
            TrackRenderingServer();
            ~TrackRenderingServer() override;

            static TrackRenderingServer *get_instance() {
                return singleton;
            }

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline TrackRenderingServer *singleton = nullptr;

        public:
            /// A backend tracks can be drawn by, by its name; it unregisters before it goes
            void backend_register(const StringName &p_name, uint64_t p_backend_id);
            /// Its tracks are given to the backends their data names now
            void backend_unregister(const StringName &p_name);
            /// The backend of the name, by its ObjectID; 0 for none
            uint64_t backend_get(const StringName &p_name) const;
            PackedStringArray backend_get_list() const;

            /// TrackServer's p_track drawn under the render RID returned
            RID track_create(const RID &p_track);
            void track_free(const RID &p_track_render);
            void track_free_many(const TypedArray<RID> &p_track_renders);
            /// What the track is: the backend its data names draws it. Given before the track is streamed
            /// or built
            void track_set_description(const RID &p_track_render, const Ref<TrackData> &p_description);
            void track_set_visible(const RID &p_track_render, bool p_visible);
            void track_set_scenario(const RID &p_track_render, const RID &p_scenario);
            /// What the track is drawn with, by the name of the backend drawing it (a StringName, the
            /// look a Resource of its kind: the standard one's TrackRenderingLook - its rails', bed's,
            /// sleepers' materials), given by whoever makes it (a scenery's,
            /// SceneryContentNode.track_looks); a backend with none draws bare surfaces. Nothing here
            /// loads one of its own
            void track_set_looks(const RID &p_track_render, const Dictionary &p_looks);
            /// Registers the track for streaming instead of building it: a scenery has thousands of tracks.
            /// It is built when the camera comes within TRACKS_DISTANCE_SETTING of it and cleared when it
            /// leaves - unless its backend streams it itself
            void track_stream(const RID &p_track_render);
            /// Builds the track again; a streamed one out of range is built when the camera comes back
            void track_rebuild(const RID &p_track_render);
            /// How far the tracks are drawn (TRACKS_DISTANCE_SETTING, read as the server is made) [m]; the
            /// traction over them is drawn as far
            double get_tracks_distance() const;

            /// TrackServer's track the render RID draws
            RID track_get_track(const RID &p_track_render) const;
            Ref<TrackData> track_get_description(const RID &p_track_render) const;
            /// The render RID of TrackServer's p_track; none for a track not drawn
            RID track_get_render_rid(const RID &p_track) const;
            TypedArray<RID> track_get_render_rids() const;
    };
} // namespace godot
