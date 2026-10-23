#pragma once
#include "macros.hpp"
#include "scenery/LinesData.hpp"
#include "scenery/ModelData.hpp"
#include "tracks/TrackData.hpp"
#include "tracks/TrackServer.hpp"
#include "traction/PowerSourceData.hpp"
#include "traction/TractionData.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* One layer of a world - its railways, its roads, its buildings, its catenary, its wires... - as
     * data: of a section of a world (WorldSection), or of a sector of a world a plugin divides into
     * sectors. Its name is the layer's (the resource's own name), what a tool keeps a layer by. The
     * simulation's part of a world - tracks, traction, power sources, signals - is loaded whole with
     * the world; a layer of a section holds what is only drawn (meshes, models, lines). */
    class WorldLayer : public Resource {
            GDCLASS(WorldLayer, Resource)

        private:
            /* What its tracks are (TrackServer.track_get_group() of each) */
            TrackServer::TrackTypeGroup group = TrackServer::GROUP_NONE;
            TypedArray<TrackData> tracks;
            TypedArray<TractionData> traction;
            TypedArray<PowerSourceData> power_sources;
            /* Geometry, its surfaces named after the game's material they are drawn with
             * (ReloadedMaterialServer) or carrying a material of their own */
            TypedArray<ArrayMesh> meshes;
            /* How far the meshes are drawn from [m]; 0: the draw distance */
            double range_end = 0.0;
            /* Models of whichever model renderer's backend (ModelData.rendering_backend) */
            TypedArray<ModelData> models;
            /* Lines - the wires of power lines */
            TypedArray<LinesData> lines;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS_NR(TrackServer::TrackTypeGroup, group);
            MAKE_GS_NR(TypedArray<TrackData>, tracks);
            MAKE_GS_NR(TypedArray<TractionData>, traction);
            MAKE_GS_NR(TypedArray<PowerSourceData>, power_sources);
            MAKE_GS_NR(TypedArray<ArrayMesh>, meshes);
            MAKE_GS_NR(double, range_end);
            MAKE_GS_NR(TypedArray<ModelData>, models);
            MAKE_GS_NR(TypedArray<LinesData>, lines);

            /// It holds nothing
            bool is_empty() const;
    };
} // namespace godot
