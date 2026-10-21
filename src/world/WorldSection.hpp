#pragma once
#include "macros.hpp"
#include "world/WorldLayer.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /* What is drawn of one cell of a world - SceneryStreamingServer's cell of the grid (its
     * CHUNK_SIZE_M): its layers' meshes, models and lines, in the world's coordinates. Read when the
     * camera comes within reach of the cell and let go once it has left (WorldSectionProvider), as
     * libmaszyna's triangle chunks are. Saved by its class name, it names no script. */
    class WorldSection : public Resource {
            GDCLASS(WorldSection, Resource)

        private:
            /* The cell of the streaming grid it is */
            Vector2i cell;
            TypedArray<WorldLayer> layers;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(Vector2i, cell);
            MAKE_GS_NR(TypedArray<WorldLayer>, layers);
    };
} // namespace godot
