#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* A power source of the catenary - a substation or a section point - its supply and its fuses.
     * Its name is the resource's name. */
    class PowerSourceData : public Resource {
            GDCLASS(PowerSourceData, Resource)

        private:
            Vector3 position;
            double nominal_voltage = 0.0;
            double voltage_frequency = 0.0;
            double internal_resistance = 0.0;
            double max_output_current = 0.0;
            double fast_fuse_timeout = 0.0;
            double fast_fuse_repetition = 0.0;
            double slow_fuse_timeout = 0.0;
            bool is_section = false;
            bool recuperation = false;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(Vector3, position);
            MAKE_GS_NR(double, nominal_voltage);
            MAKE_GS_NR(double, voltage_frequency);
            MAKE_GS_NR(double, internal_resistance);
            MAKE_GS_NR(double, max_output_current);
            MAKE_GS_NR(double, fast_fuse_timeout);
            MAKE_GS_NR(double, fast_fuse_repetition);
            MAKE_GS_NR(double, slow_fuse_timeout);
            MAKE_GS_NR(bool, is_section);
            MAKE_GS_NR(bool, recuperation);

            /// A copy of the power source moved by p_offset
            Ref<PowerSourceData> translated(const Vector3 &p_offset) const;
    };
} // namespace godot
