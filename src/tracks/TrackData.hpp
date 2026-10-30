#pragma once
#include "macros.hpp"
#include "tracks/TrackCurve.hpp"
#include "tracks/TrackServer.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
    /* What a segment of track or road is and what it has - never how it is drawn: the backend that
     * draws it (rendering_backend) picks the materials and the section for its features. A plugin
     * keeps what is only its own in meta or in a subclass. Its name is the resource's name. */
    class TrackData : public Resource {
            GDCLASS(TrackData, Resource)

        public:
            /// A feature of the segment: it stands on a bridge or runs in a tunnel
            enum Structure {
                STRUCTURE_NONE,
                STRUCTURE_BRIDGE,
                STRUCTURE_TUNNEL,
            };
            /// What a rail track's bed is made of: crushed stone ballast, gravel, earth, a concrete slab,
            /// or the track is embedded in a street
            enum Bed {
                BED_CRUSHED_STONE,
                BED_GRAVEL,
                BED_DIRT,
                BED_SLAB,
                BED_EMBEDDED,
            };
            /// What a rail track's rails are laid on, where its bed has sleepers
            enum SleeperKind {
                SLEEPER_CONCRETE,
                SLEEPER_WOOD,
                SLEEPER_STEEL,
            };
            /// What a road is surfaced with
            enum Surface {
                SURFACE_ASPHALT,
                SURFACE_CONCRETE,
                SURFACE_PAVING,
                SURFACE_GRAVEL,
                SURFACE_DIRT,
            };

            /// A concrete sleeper track's: 1667 sleepers a kilometre
            static constexpr double DEFAULT_SLEEPER_SPACING = 0.6;
            /// An embankment's slopes, 1:1.5
            static constexpr double DEFAULT_FORMATION_SLOPE = 1.5;

        private:
            TrackServer::TrackType kind = TrackServer::TRACK_NORMAL;
            Ref<TrackCurve> curve;
            Ref<TrackCurve> diverging_curve;
            /* Gauge, or a road's width [m]: a standard gauge track's */
            double width = 1.5;
            double friction = TrackServer::DEFAULT_TRACK_FRICTION;
            /* Rail length between joints [m], -1 for a continuous rail */
            double joint_spacing = -1.0;
            int damage = TrackServer::TRACK_DAMAGE_NONE;
            bool visible = true;
            /* [km/h], -1 for none */
            double speed_limit = -1.0;
            Structure structure = STRUCTURE_NONE;
            Bed bed = BED_CRUSHED_STONE;
            Surface surface = SURFACE_ASPHALT;
            bool centre_line = false;
            bool shoulder = false;
            /* The formation the bed lies on: from its crown down to the ground [m] - 0 for none, below 0
             * a cutting - its crown beyond the foot of the ballast [m] and its slopes, 1:n */
            double formation_height = 0.0;
            double formation_width = 0.0;
            double formation_slope = DEFAULT_FORMATION_SLOPE;
            /* From one sleeper to the next [m]; a slab or an embedded bed has none */
            double sleeper_spacing = DEFAULT_SLEEPER_SPACING;
            SleeperKind sleeper_kind = SLEEPER_CONCRETE;
            /* What a plugin keeps of its own about the segment; handed to TrackServer with it
             * (track_set_meta()) */
            Dictionary meta;
            /* The name of the TrackRenderingServer backend that draws the segment; empty for the one
             * TrackRenderingServer picks (TrackRenderingServer::BACKEND_SETTING) */
            StringName rendering_backend;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS_NR(TrackServer::TrackType, kind);
            MAKE_GS_NR(Ref<TrackCurve>, curve);
            MAKE_GS_NR(Ref<TrackCurve>, diverging_curve);
            MAKE_GS_NR(double, width);
            MAKE_GS_NR(double, friction);
            MAKE_GS_NR(double, joint_spacing);
            MAKE_GS_NR(int, damage);
            MAKE_GS_NR(bool, visible);
            MAKE_GS_NR(double, speed_limit);
            MAKE_GS_NR(Structure, structure);
            MAKE_GS_NR(Bed, bed);
            MAKE_GS_NR(Surface, surface);
            MAKE_GS_NR(bool, centre_line);
            MAKE_GS_NR(bool, shoulder);
            MAKE_GS(Dictionary, meta);
            MAKE_GS_NR(double, formation_height);
            MAKE_GS_NR(double, formation_width);
            MAKE_GS_NR(double, formation_slope);
            MAKE_GS_NR(double, sleeper_spacing);
            MAKE_GS_NR(SleeperKind, sleeper_kind);
            MAKE_GS(StringName, rendering_backend);

            /// A copy of the segment moved by p_offset (both its curves)
            Ref<TrackData> translated(const Vector3 &p_offset) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(TrackData::Structure)
VARIANT_ENUM_CAST(TrackData::Bed)
VARIANT_ENUM_CAST(TrackData::Surface)
VARIANT_ENUM_CAST(TrackData::SleeperKind)
