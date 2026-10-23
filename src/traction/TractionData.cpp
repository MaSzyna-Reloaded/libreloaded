#include "TractionData.hpp"

namespace godot {
    void TractionData::_bind_methods() {
        BIND_PROPERTY(TractionData, Variant::STRING, power_supply_name);
        BIND_PROPERTY(TractionData, Variant::FLOAT, nominal_voltage);
        BIND_PROPERTY(TractionData, Variant::FLOAT, max_current);
        BIND_PROPERTY(TractionData, Variant::FLOAT, resistivity);
        BIND_PROPERTY(TractionData, Variant::FLOAT, wire_thickness);
        BIND_PROPERTY(TractionData, Variant::INT, damage_flag);
        BIND_PROPERTY(TractionData, Variant::VECTOR3, contact_p1);
        BIND_PROPERTY(TractionData, Variant::VECTOR3, contact_p2);
        BIND_PROPERTY(TractionData, Variant::VECTOR3, support_p1);
        BIND_PROPERTY(TractionData, Variant::VECTOR3, support_p2);
        BIND_PROPERTY(TractionData, Variant::FLOAT, min_height);
        BIND_PROPERTY(TractionData, Variant::FLOAT, segment_length);
        BIND_PROPERTY(TractionData, Variant::INT, wires);
        BIND_PROPERTY(TractionData, Variant::FLOAT, wire_offset);
        BIND_PROPERTY(TractionData, Variant::BOOL, visible);
        BIND_PROPERTY(TractionData, Variant::STRING, parallel);
        BIND_PROPERTY(TractionData, Variant::DICTIONARY, meta);
        BIND_PROPERTY(TractionData, Variant::STRING_NAME, rendering_backend);
        ClassDB::bind_method(D_METHOD("translated", "offset"), &TractionData::translated);
    }

    Ref<TractionData> TractionData::translated(const Vector3 &p_offset) const {
        Ref<TractionData> moved = duplicate();
        moved->contact_p1 = contact_p1 + p_offset;
        moved->contact_p2 = contact_p2 + p_offset;
        moved->support_p1 = support_p1 + p_offset;
        moved->support_p2 = support_p2 + p_offset;
        return moved;
    }
} // namespace godot
