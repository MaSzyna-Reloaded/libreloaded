#pragma once
#include "tracks/TrackData.hpp"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* What TrackRenderingServer forwards the drawing of a track to: the renderer of one kind of track
     * data (TrackData.rendering_backend), registered by its name
     * (TrackRenderingServer::backend_register()). It keeps what it draws of each track under the
     * track's render RID, from track_create() to track_free(), and reads the track's curves from
     * TrackServer as it builds it. Its priority says how well it draws against the other backends
     * (TrackRenderingServer::AUTO_BACKEND). */
    class TrackRenderingBackend : public Object {
            GDCLASS(TrackRenderingBackend, Object)

        public:
            /// A backend's priority unless it gives its own (get_priority())
            static constexpr double DEFAULT_PRIORITY = 1.0;

        protected:
            static void _bind_methods();

        public:
            /// How well it draws against the other backends: a track is drawn by the backend of the
            /// highest priority, the one its data names on a tie (TrackRenderingServer::AUTO_BACKEND)
            virtual double get_priority() const;
            /// The track of TrackServer's p_track is drawn by this backend under p_track_render
            virtual void track_create(const RID &p_track_render, const RID &p_track) = 0;
            /// What the track drew goes, and the track with it
            virtual void track_free(const RID &p_track_render) = 0;
            /// What the track is; given before it is streamed or built
            virtual void track_set_description(const RID &p_track_render, const Ref<TrackData> &p_description) = 0;
            virtual void track_set_visible(const RID &p_track_render, bool p_visible) = 0;
            virtual void track_set_scenario(const RID &p_track_render, const RID &p_scenario) = 0;
            /// The blades of a switch at the switch's offsets
            virtual void
            track_set_switch_blade_offsets(const RID &p_track_render, double p_offset1, double p_offset2) = 0;
            /// Whether the backend streams the track itself (true) or TrackRenderingServer registers it
            /// with SceneryStreamingServer (false)
            virtual bool track_stream(const RID &p_track_render) = 0;
            /// Builds (or builds again) what the track draws
            virtual void track_build(const RID &p_track_render) = 0;
            /// What the track drew goes as it leaves the camera's range; the track stays
            virtual void track_clear(const RID &p_track_render) = 0;
            /// The joined tracks are other ones (TrackServer's topology_rebuilt)
            virtual void topology_rebuilt() = 0;
    };
} // namespace godot
