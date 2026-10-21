#pragma once
#include "macros.hpp"
#include "scenery/LinesData.hpp"
#include "scenery/ModelData.hpp"
#include "scenery/SignalBoxData.hpp"
#include "scenery/SoundData.hpp"
#include "scenery/StationData.hpp"
#include "scenery/TrainsetData.hpp"
#include "tracks/TrackData.hpp"
#include "traction/PowerSourceData.hpp"
#include "traction/TractionData.hpp"
#include <godot_cpp/classes/packed_scene.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* What a scenery is made of, read whole and registered with the servers (SceneryServer): its
     * tracks, traction, models, sounds, lines, trainsets, stations, scripts and nodes. A plugin's
     * subclass carries what only its format has. */
    class CompiledScenery : public Resource {
            GDCLASS(CompiledScenery, Resource)

        private:
            Ref<PackedScene> nodes;
            TypedArray<TrackData> tracks;
            TypedArray<TractionData> traction;
            TypedArray<PowerSourceData> power_sources;
            TypedArray<ModelData> models;
            TypedArray<SoundData> sounds;
            /* The lines, drawn by SceneryLines (not nodes, so not in nodes) */
            TypedArray<LinesData> lines;
            /* The trainsets and their vehicles, built through the servers (not nodes, so not in nodes) */
            TypedArray<TrainsetData> trainsets;
            /* The scenario's scripts, relative to the scenery directory */
            TypedArray<String> scripts;
            /* The stations and signal boxes, for a list of them (not drawn) */
            TypedArray<StationData> stations;
            TypedArray<SignalBoxData> signal_boxes;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS_NR(Ref<PackedScene>, nodes);
            MAKE_GS_NR(TypedArray<TrackData>, tracks);
            MAKE_GS_NR(TypedArray<TractionData>, traction);
            MAKE_GS_NR(TypedArray<PowerSourceData>, power_sources);
            MAKE_GS_NR(TypedArray<ModelData>, models);
            MAKE_GS_NR(TypedArray<SoundData>, sounds);
            MAKE_GS_NR(TypedArray<LinesData>, lines);
            MAKE_GS_NR(TypedArray<TrainsetData>, trainsets);
            MAKE_GS_NR(TypedArray<String>, scripts);
            MAKE_GS_NR(TypedArray<StationData>, stations);
            MAKE_GS_NR(TypedArray<SignalBoxData>, signal_boxes);
    };
} // namespace godot
