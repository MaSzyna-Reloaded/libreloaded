#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

namespace godot {
    /* Lines of a scenery - the wires of power lines, mostly: pairs of points, drawn by SceneryLines and
     * merged with their neighbours (SceneryLinesGeometry) */
    class LinesData : public Resource {
            GDCLASS(LinesData, Resource)

        private:
            /* The lines in the world, pairs of points - a strip or a loop turned into pairs as it was read */
            PackedVector3Array points;
            Color color = Color(0, 0, 0);
            /* The line's width; a negative one is a line that does not fade with the distance */
            double width = 0.0;
            double range_min = 0.0;
            /* Visible up to this distance from the lines' centre, 0 - no limit */
            double range_max = 0.0;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(PackedVector3Array, points);
            MAKE_GS(Color, color);
            MAKE_GS_NR(double, width);
            MAKE_GS_NR(double, range_min);
            MAKE_GS_NR(double, range_max);

            /// A copy of the lines moved by p_offset
            Ref<LinesData> translated(const Vector3 &p_offset) const;
    };
} // namespace godot
