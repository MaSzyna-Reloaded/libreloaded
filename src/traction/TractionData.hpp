#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* One span of the catenary: where its wires hang, how they are strung and what feeds them. A
     * plugin keeps what is only its own in meta or in a subclass. */
    class TractionData : public Resource {
            GDCLASS(TractionData, Resource)

        private:
            /* The power source feeding the span, by its name */
            String power_supply_name;
            /* [V] */
            double nominal_voltage = 0.0;
            /* [A] */
            double max_current = 0.0;
            /* [ohm/m] */
            double resistivity = 0.0;
            /* The contact wire's thickness [m] */
            double wire_thickness = 0.0;
            int damage_flag = 0;
            Vector3 contact_p1;
            Vector3 contact_p2;
            Vector3 support_p1;
            Vector3 support_p2;
            /* The lowest the contact wire hangs between its supports [m] */
            double min_height = 0.0;
            /* The dropper spacing [m] */
            double segment_length = 0.0;
            /* How the wires are strung (TractionRenderingServer.Wires), the contact wire alone by default */
            int wires = 0;
            double wire_offset = 0.0;
            bool visible = true;
            /* The span it runs beside, by its name */
            String parallel;
            /* What a plugin keeps of its own about the span */
            Dictionary meta;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(String, power_supply_name);
            MAKE_GS_NR(double, nominal_voltage);
            MAKE_GS_NR(double, max_current);
            MAKE_GS_NR(double, resistivity);
            MAKE_GS_NR(double, wire_thickness);
            MAKE_GS_NR(int, damage_flag);
            MAKE_GS(Vector3, contact_p1);
            MAKE_GS(Vector3, contact_p2);
            MAKE_GS(Vector3, support_p1);
            MAKE_GS(Vector3, support_p2);
            MAKE_GS_NR(double, min_height);
            MAKE_GS_NR(double, segment_length);
            MAKE_GS_NR(int, wires);
            MAKE_GS_NR(double, wire_offset);
            MAKE_GS_NR(bool, visible);
            MAKE_GS(String, parallel);
            MAKE_GS(Dictionary, meta);

            /// A copy of the span moved by p_offset
            Ref<TractionData> translated(const Vector3 &p_offset) const;
    };
} // namespace godot
