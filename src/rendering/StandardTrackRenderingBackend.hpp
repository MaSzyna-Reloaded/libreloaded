#pragma once
#include "rendering/TrackRenderingBackend.hpp"
#include "rendering/TrackRenderingLook.hpp"
#include "tracks/TrackCurve.hpp"
#include "utils/RIDHash.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* The core's tracks: what a TrackData says, drawn with the renderer's own assets
     * (TrackRenderingLook, LOOK_SETTING). A rail track is two rails on a bed of its kind - a ballast
     * prism, a slab, or a street the rails are embedded in - over its formation, banked or cut as
     * its formation_* say, with its sleepers (TrackData.sleeper_*) in MultiMesh chunks near the
     * camera; a road its carriageway and shoulders. Each curve of a switch is drawn as a track of its
     * own, on the common curve's sleepers; the blades are not drawn. */
    class StandardTrackRenderingBackend : public TrackRenderingBackend {
            GDCLASS(StandardTrackRenderingBackend, TrackRenderingBackend)

        public:
            /// Its name with TrackRenderingServer, the default backend's
            static constexpr const char *NAME = "standard";
            /// The look the tracks are drawn with, by its path
            static constexpr const char *LOOK_SETTING = "reloaded/tracks/look";
            static constexpr const char *DEFAULT_LOOK = "res://addons/libreloaded/tracks/track_look.tres";

            /// A curve is drawn in sections this far apart [m], a straight one in sections of up to
            /// STRAIGHT_SEGMENT_LENGTH
            static constexpr double SEGMENT_LENGTH = 2.0;
            static constexpr double STRAIGHT_SEGMENT_LENGTH = 25.0;
            /// The rail's section, 60E1 (PN-EN 13674-1): its height, the core's rail height
            /// (TrackServer::RAIL_HEIGHT), is scaled from it [m]
            static constexpr double RAIL_PROFILE_HEIGHT = 0.172;
            /// A concrete sleeper's length and height (PS-94) [m]: the ballast is laid around it
            static constexpr double SLEEPER_LENGTH = 2.6;
            static constexpr double SLEEPER_HEIGHT = 0.2;
            /// The ballast prism (Id-1, PKP PLK): its crest beyond the sleepers' ends, its depth under
            /// them [m] and its slopes, 1:n. The crest lies a little under the sleepers' tops
            static constexpr double BALLAST_SHOULDER = 0.4;
            static constexpr double BALLAST_DEPTH = 0.3;
            static constexpr double BALLAST_SLOPE = 1.5;
            static constexpr double BALLAST_CREST_DROP = 0.03;
            /// A slab track's slab: its half width and thickness [m]
            static constexpr double SLAB_HALF_WIDTH = 1.4;
            static constexpr double SLAB_THICKNESS = 0.3;
            /// An embedded track's street beyond its rails [m]
            static constexpr double EMBEDDED_MARGIN = 1.0;
            /// A road's shoulder: its width and its drop under the carriageway [m]
            static constexpr double SHOULDER_WIDTH = 0.75;
            static constexpr double SHOULDER_DROP = 0.05;
            /// The diverging curve's bed lies this much under the common one's, so the two do not
            /// fight where they overlap [m]
            static constexpr double DIVERGING_BED_DROP = 0.01;
            /// A track's bed and formation reach this far past its ends, under the next one's [m]
            static constexpr double BED_JOINT_OVERLAP = 0.02;
            /// The sleepers are drawn within this distance of the camera, fading out past it by the
            /// margins, in MultiMesh chunks of a quarter of it [m] (DoS, libmaszyna 655cdcda
            /// track_rendering_server.gd: sleepers_multimesh_max_distance, _create_sleeper_chunks())
            static constexpr double SLEEPERS_DISTANCE = 150.0;
            static constexpr double SLEEPERS_BEGIN_MARGIN = 0.5;
            static constexpr double SLEEPERS_END_MARGIN = 2.0;
            static constexpr double SLEEPER_CHUNK_LENGTH = 0.25 * SLEEPERS_DISTANCE;
            /// The base sleepers' sections [m]: a concrete one (PS-94) narrower at the top, a wooden one
            /// square, a steel one a low trough
            static constexpr double CONCRETE_SLEEPER_TOP_WIDTH = 0.17;
            static constexpr double CONCRETE_SLEEPER_BOTTOM_WIDTH = 0.3;
            static constexpr double WOOD_SLEEPER_WIDTH = 0.26;
            static constexpr double WOOD_SLEEPER_HEIGHT = 0.16;
            static constexpr double STEEL_SLEEPER_WIDTH = 0.26;
            static constexpr double STEEL_SLEEPER_HEIGHT = 0.1;

        private:
            struct Track {
                    RID track;
                    Ref<TrackData> description;
                    RID scenario;
                    bool visible = true;
                    /// The RenderingServer instance while the track is built
                    RID instance;
                    Ref<ArrayMesh> mesh;
                    /// The sleepers' MultiMeshes and their instances, a chunk each, while it is built
                    Vector<RID> sleeper_multimeshes;
                    Vector<RID> sleeper_instances;
            };
            /// A place along a curve: its point, and the track's side and up there, rolled
            struct Frame {
                    Vector3 origin;
                    Vector3 side;
                    Vector3 up;
                    Vector3 forward;
                    double distance = 0.0;
            };
            /// One surface of the track's mesh being built, of one material
            struct Surface {
                    Ref<Material> material;
                    PackedVector3Array vertices;
                    PackedVector3Array normals;
                    PackedVector2Array uvs;
                    PackedInt32Array indices;
            };

            RIDMap<Track> tracks;
            Ref<TrackRenderingLook> look;
            /// The base sleeper of each kind, for a kind the look gives no mesh of
            HashMap<int, Ref<ArrayMesh>> base_sleepers;

            static Ref<Curve3D> _build_curve(const Ref<Curve3D> &p_domain_curve);
            static Vector<Frame> _build_frames(const Ref<Curve3D> &p_curve, const Ref<TrackCurve> &p_track_curve);
            static Frame
            _frame_at(const Ref<Curve3D> &p_curve, const Ref<TrackCurve> &p_track_curve, double p_distance);
            /// A sleeper of p_kind as a box of its section, its length across X, its top at 0
            static Ref<ArrayMesh> _build_base_sleeper(TrackData::SleeperKind p_kind);
            /// Where the track's sleepers lie along p_curve, from its start to its end; none on a slab or
            /// an embedded bed
            static Vector<Transform3D> _place_sleepers(
                    const Ref<Curve3D> &p_curve, const Ref<TrackCurve> &p_track_curve, const TrackData &p_description);
            /// The track's sleepers along p_curve, MultiMesh chunks of SLEEPER_CHUNK_LENGTH
            void _add_sleepers(
                    Track &p_track, const Ref<Curve3D> &p_curve, const Ref<TrackCurve> &p_track_curve,
                    const TrackData &p_description) const;
            /// The section p_points (x across, y up) swept along p_frames into the surface of p_material
            static void
            _sweep(const Vector<Frame> &p_frames, const PackedVector2Array &p_points, const Ref<Material> &p_material,
                   Vector<Surface> &p_surfaces);
            void _add_rail_track(
                    const Vector<Frame> &p_frames, const TrackData &p_description, double p_bed_drop,
                    Vector<Surface> &p_surfaces) const;
            void
            _add_road(const Vector<Frame> &p_frames, const TrackData &p_description, Vector<Surface> &p_surfaces) const;

        protected:
            static void _bind_methods();

        public:
            StandardTrackRenderingBackend();
            ~StandardTrackRenderingBackend() override;

            void track_create(const RID &p_track_render, const RID &p_track) override;
            void track_free(const RID &p_track_render) override;
            void track_set_description(const RID &p_track_render, const Ref<TrackData> &p_description) override;
            void track_set_visible(const RID &p_track_render, bool p_visible) override;
            void track_set_scenario(const RID &p_track_render, const RID &p_scenario) override;
            void track_set_switch_blade_offsets(const RID &p_track_render, double p_offset1, double p_offset2) override;
            bool track_stream(const RID &p_track_render) override;
            void track_build(const RID &p_track_render) override;
            void track_clear(const RID &p_track_render) override;
            void topology_rebuilt() override;

            /// The mesh the track is drawn with while it is built; null otherwise
            Ref<ArrayMesh> track_get_mesh(const RID &p_track_render) const;
            /// The RenderingServer MultiMeshes of the track's sleepers while it is built, a chunk each
            TypedArray<RID> track_get_sleeper_multimeshes(const RID &p_track_render) const;
            /// Where the track's sleepers lie, as they are drawn
            Array track_get_sleeper_transforms(const RID &p_track_render) const;
    };
} // namespace godot
