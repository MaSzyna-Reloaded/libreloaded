#pragma once
#include "macros.hpp"
#include "tracks/TrackData.hpp"
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
    /* The assets the core's tracks are drawn with (StandardTrackRenderingBackend): the materials of a
     * track's rails, bed, formation and of a road's carriageway and shoulders, and a sleeper of each
     * kind. A track says what it is (TrackData); its look is the renderer's. A kind with no sleeper
     * mesh here gets the backend's own. */
    class TrackRenderingLook : public Resource {
            GDCLASS(TrackRenderingLook, Resource)

        private:
            Ref<Material> rail_material;
            /* {TrackData.Bed: Material} */
            Dictionary bed_materials;
            Ref<Material> formation_material;
            /* {TrackData.SleeperKind: Mesh}, the sleeper's top at 0 and its length across X */
            Dictionary sleeper_meshes;
            /* {TrackData.SleeperKind: Material} */
            Dictionary sleeper_materials;
            /* {TrackData.Surface: Material} */
            Dictionary surface_materials;
            Ref<Material> shoulder_material;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS_NR(Ref<Material>, rail_material);
            MAKE_GS(Dictionary, bed_materials);
            MAKE_GS_NR(Ref<Material>, formation_material);
            MAKE_GS(Dictionary, sleeper_meshes);
            MAKE_GS(Dictionary, sleeper_materials);
            MAKE_GS(Dictionary, surface_materials);
            MAKE_GS_NR(Ref<Material>, shoulder_material);

            /// The material of a bed of p_bed; null when the look has none
            Ref<Material> bed_get_material(TrackData::Bed p_bed) const;
            /// The mesh of a sleeper of p_kind; null when the look has none
            Ref<Mesh> sleeper_get_mesh(TrackData::SleeperKind p_kind) const;
            Ref<Material> sleeper_get_material(TrackData::SleeperKind p_kind) const;
            Ref<Material> surface_get_material(TrackData::Surface p_surface) const;
    };
} // namespace godot
