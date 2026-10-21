#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* A station or a halt of a world (OSM's railway=station|halt), for a list of the world's
     * stations: its name, the codes it is known by and where it is */
    class StationData : public Resource {
            GDCLASS(StationData, Resource)

        private:
            String station_name;
            /* Its code in the signalling and the timetables (OSM's railway:ref, e.g. "Tz") */
            String ref;
            /* Its UIC code (OSM's uic_ref, e.g. "5100072") */
            String uic_ref;
            /* Latitude and longitude, degrees */
            Vector2 lat_lon;
            /* Its place in the world, on the ground */
            Vector3 position;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(String, station_name);
            MAKE_GS(String, ref);
            MAKE_GS(String, uic_ref);
            MAKE_GS(Vector2, lat_lon);
            MAKE_GS(Vector3, position);
    };
} // namespace godot
