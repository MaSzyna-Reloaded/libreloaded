#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* A signal box of a world (OSM's railway=signal_box): its name, its code, its kind, where it is
     * and the station it works for */
    class SignalBoxData : public Resource {
            GDCLASS(SignalBoxData, Resource)

        private:
            String box_name;
            /* Its code (OSM's railway:ref, e.g. "Tz") */
            String ref;
            /* Its kind (OSM's railway:signal_box, e.g. "electronic", "relay", "mechanical") */
            String kind;
            /* Latitude and longitude, degrees */
            Vector2 lat_lon;
            /* Its place in the world, on the ground */
            Vector3 position;
            /* The ref of the station it works for (StationData.ref): by its own code, else the nearest's */
            String station_ref;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(String, box_name);
            MAKE_GS(String, ref);
            MAKE_GS(String, kind);
            MAKE_GS(Vector2, lat_lon);
            MAKE_GS(Vector3, position);
            MAKE_GS(String, station_ref);
    };
} // namespace godot
