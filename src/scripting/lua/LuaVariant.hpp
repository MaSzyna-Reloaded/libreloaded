#pragma once
#include <godot_cpp/variant/variant.hpp>

#include "lua.h"

namespace godot {
    /// Godot values to Lua values and back. A RID becomes a handle of no particular kind (the
    /// modules push typed handles themselves), a vector a table {x, y, z}, an array a sequence and
    /// a resource a table of its stored properties. A Lua sequence becomes an Array, any other
    /// table a Dictionary.
    class LuaVariant {
        public:
            /// Deeper nesting is cut off - it also stops a table that contains itself
            static constexpr int MAX_CONVERSION_DEPTH = 16;

            /// The most a colour's channel is in a script: colours are {r, g, b[, a]} of 0..255
            static constexpr double CHANNEL_MAX = 255.0;

            static void push(lua_State *p_state, const Variant &p_value, int p_depth = 0);
            static Variant to_variant(lua_State *p_state, int p_index, int p_depth = 0);
            /// A colour as a script writes it, {r, g, b[, a]} of 0..255, opaque without a; raises a
            /// script error for anything else
            static Color check_color(lua_State *p_state, int p_index);
            /// Whether the value at p_index is written as a colour
            static bool is_color(lua_State *p_state, int p_index);
            static void push_color(lua_State *p_state, const Color &p_color);
    };
} // namespace godot
