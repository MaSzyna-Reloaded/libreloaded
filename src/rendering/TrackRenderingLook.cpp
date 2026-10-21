#include "rendering/TrackRenderingLook.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void TrackRenderingLook::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                TrackRenderingLook, Variant::OBJECT, rail_material, PROPERTY_HINT_RESOURCE_TYPE, "Material");
        BIND_PROPERTY(TrackRenderingLook, Variant::DICTIONARY, bed_materials);
        BIND_PROPERTY_W_HINT(
                TrackRenderingLook, Variant::OBJECT, formation_material, PROPERTY_HINT_RESOURCE_TYPE, "Material");
        BIND_PROPERTY(TrackRenderingLook, Variant::DICTIONARY, sleeper_meshes);
        BIND_PROPERTY(TrackRenderingLook, Variant::DICTIONARY, sleeper_materials);
        BIND_PROPERTY(TrackRenderingLook, Variant::DICTIONARY, surface_materials);
        BIND_PROPERTY_W_HINT(
                TrackRenderingLook, Variant::OBJECT, shoulder_material, PROPERTY_HINT_RESOURCE_TYPE, "Material");
        ClassDB::bind_method(D_METHOD("bed_get_material", "bed"), &TrackRenderingLook::bed_get_material);
        ClassDB::bind_method(D_METHOD("sleeper_get_mesh", "kind"), &TrackRenderingLook::sleeper_get_mesh);
        ClassDB::bind_method(D_METHOD("sleeper_get_material", "kind"), &TrackRenderingLook::sleeper_get_material);
        ClassDB::bind_method(D_METHOD("surface_get_material", "surface"), &TrackRenderingLook::surface_get_material);
    }

    Ref<Material> TrackRenderingLook::bed_get_material(const TrackData::Bed p_bed) const {
        return bed_materials.get(p_bed, Variant());
    }

    Ref<Mesh> TrackRenderingLook::sleeper_get_mesh(const TrackData::SleeperKind p_kind) const {
        return sleeper_meshes.get(p_kind, Variant());
    }

    Ref<Material> TrackRenderingLook::sleeper_get_material(const TrackData::SleeperKind p_kind) const {
        return sleeper_materials.get(p_kind, Variant());
    }

    Ref<Material> TrackRenderingLook::surface_get_material(const TrackData::Surface p_surface) const {
        return surface_materials.get(p_surface, Variant());
    }
} // namespace godot
