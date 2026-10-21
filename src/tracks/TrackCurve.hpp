#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* A track's curve: from p1 to p2, its handles c1 and c2 relative to the ends, the rails rolled
     * roll1 at p1 and roll2 at p2; a radius other than 0 makes it an arc. Every change is announced
     * (Resource.changed). */
    class TrackCurve : public Resource {
            GDCLASS(TrackCurve, Resource)

        private:
            Vector3 p1;
            Vector3 c1;
            Vector3 c2;
            Vector3 p2;
            double roll1 = 0.0;
            double roll2 = 0.0;
            double radius = 0.0;

        protected:
            static void _bind_methods();

        public:
            Vector3 get_p1() const;
            void set_p1(const Vector3 &p_value);
            Vector3 get_c1() const;
            void set_c1(const Vector3 &p_value);
            Vector3 get_c2() const;
            void set_c2(const Vector3 &p_value);
            Vector3 get_p2() const;
            void set_p2(const Vector3 &p_value);
            double get_roll1() const;
            void set_roll1(double p_value);
            double get_roll2() const;
            void set_roll2(double p_value);
            double get_radius() const;
            void set_radius(double p_value);

            /// A copy of the curve moved by p_offset - its handles are relative to its ends
            Ref<TrackCurve> translated(const Vector3 &p_offset) const;
    };
} // namespace godot
