#include "TrackCurve.hpp"

namespace godot {
    void TrackCurve::_bind_methods() {
        BIND_PROPERTY(TrackCurve, Variant::VECTOR3, p1);
        BIND_PROPERTY(TrackCurve, Variant::VECTOR3, c1);
        BIND_PROPERTY(TrackCurve, Variant::VECTOR3, c2);
        BIND_PROPERTY(TrackCurve, Variant::VECTOR3, p2);
        BIND_PROPERTY(TrackCurve, Variant::FLOAT, roll1);
        BIND_PROPERTY(TrackCurve, Variant::FLOAT, roll2);
        BIND_PROPERTY(TrackCurve, Variant::FLOAT, radius);
        ClassDB::bind_method(D_METHOD("translated", "offset"), &TrackCurve::translated);
    }

    /// Each setter announces a change of its value only
#define TRACK_CURVE_FIELD(p_type, p_arg, p_name)                                                                       \
    p_type TrackCurve::get_##p_name() const {                                                                          \
        return p_name;                                                                                                 \
    }                                                                                                                  \
    void TrackCurve::set_##p_name(p_arg p_value) {                                                                     \
        if (p_name == p_value) {                                                                                       \
            return;                                                                                                    \
        }                                                                                                              \
        p_name = p_value;                                                                                              \
        emit_changed();                                                                                                \
    }

    TRACK_CURVE_FIELD(Vector3, const Vector3 &, p1)
    TRACK_CURVE_FIELD(Vector3, const Vector3 &, c1)
    TRACK_CURVE_FIELD(Vector3, const Vector3 &, c2)
    TRACK_CURVE_FIELD(Vector3, const Vector3 &, p2)
    TRACK_CURVE_FIELD(double, double, roll1)
    TRACK_CURVE_FIELD(double, double, roll2)
    TRACK_CURVE_FIELD(double, double, radius)
#undef TRACK_CURVE_FIELD

    Ref<TrackCurve> TrackCurve::translated(const Vector3 &p_offset) const {
        Ref<TrackCurve> moved = duplicate();
        moved->p1 = p1 + p_offset;
        moved->p2 = p2 + p_offset;
        return moved;
    }
} // namespace godot
