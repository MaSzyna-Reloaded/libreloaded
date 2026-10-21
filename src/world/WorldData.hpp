#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /* A world of the game's own format as it is saved (world.res, World.WORLD_FILE): its name, where
     * it lies, the clock it starts at and the cells its sections are drawn in. World is made of it
     * (World.open()); a plugin's subclass says more of a world it divides otherwise. Saved by its
     * class name, it names no script. */
    class WorldData : public Resource {
            GDCLASS(WorldData, Resource)

        public:
            /// A world that declares no clock to start at: the scenery's default (SceneryContentNode)
            static constexpr double NO_START_TIME = -1.0;

        private:
            /* The world's name: its .json and its signalling system's */
            String world_name;
            /* The projected metres (east, north) the world's origin lies at */
            Vector2 origin;
            /* The scenario clock the world declares for its start, hours; below 0 (NO_START_TIME) for none */
            double declared_start_time = NO_START_TIME;
            /* The cells of the streaming grid there are sections of (WorldSection) */
            TypedArray<Vector2i> section_cells;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(String, world_name);
            MAKE_GS(Vector2, origin);
            MAKE_GS_NR(double, declared_start_time);
            MAKE_GS_NR(TypedArray<Vector2i>, section_cells);
    };
} // namespace godot
