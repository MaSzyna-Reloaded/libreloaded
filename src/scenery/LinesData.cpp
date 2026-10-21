#include "LinesData.hpp"

namespace godot {
    void LinesData::_bind_methods() {
        BIND_PROPERTY(LinesData, Variant::PACKED_VECTOR3_ARRAY, points);
        BIND_PROPERTY(LinesData, Variant::COLOR, color);
        BIND_PROPERTY(LinesData, Variant::FLOAT, width);
        BIND_PROPERTY(LinesData, Variant::FLOAT, range_min);
        BIND_PROPERTY(LinesData, Variant::FLOAT, range_max);
        ClassDB::bind_method(D_METHOD("translated", "offset"), &LinesData::translated);
    }

    Ref<LinesData> LinesData::translated(const Vector3 &p_offset) const {
        Ref<LinesData> moved = duplicate();
        PackedVector3Array moved_points = points;
        for (int64_t index = 0; index < moved_points.size(); ++index) {
            moved_points.set(index, moved_points[index] + p_offset);
        }
        moved->points = moved_points;
        return moved;
    }
} // namespace godot
