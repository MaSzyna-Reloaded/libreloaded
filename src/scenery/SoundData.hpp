#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* A sound of a scenery, played by a scenery's event: where it is, its file and how far it is
     * heard. Its name is the resource's name. */
    class SoundData : public Resource {
            GDCLASS(SoundData, Resource)

        private:
            Vector3 position;
            /* The file in the game's sounds, lower case, without its extension */
            String file;
            /* How far it is heard */
            double range_max = 0.0;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(Vector3, position);
            MAKE_GS(String, file);
            MAKE_GS_NR(double, range_max);
    };
} // namespace godot
