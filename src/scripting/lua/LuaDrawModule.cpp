#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaScriptContext.hpp"
#include "LuaVariant.hpp"
#include "rendering/DrawingServer.hpp"
#include "scripting/ScenarioScriptServer.hpp"

namespace godot {
    /// A line's or an outline's width when the script gives none [px]
    static constexpr double DEFAULT_WIDTH = 1.0;
    /// The coordinates of a point
    static constexpr int POINT_COORDINATES = 2;
    /// The fields of a part of a source, {x, y, w, h}
    static constexpr int RECT_FIELDS = 4;

    static DrawingServer *drawings(lua_State *p_state) {
        return LuaModules::server<DrawingServer>(p_state);
    }

    /// {x1, y1, x2, y2, ...}
    static PackedVector2Array check_points(lua_State *p_state, const int p_index) {
        luaL_checktype(p_state, p_index, LUA_TTABLE);
        const lua_Unsigned length = lua_rawlen(p_state, p_index);
        luaL_argcheck(
                p_state, length >= POINT_COORDINATES && length % POINT_COORDINATES == 0, p_index,
                "points are {x1, y1, x2, y2, ...}");
        PackedVector2Array points;
        for (lua_Unsigned i = 1; i < length; i += POINT_COORDINATES) {
            lua_rawgeti(p_state, p_index, static_cast<lua_Integer>(i));
            lua_rawgeti(p_state, p_index, static_cast<lua_Integer>(i + 1));
            points.push_back(Vector2(luaL_checknumber(p_state, -2), luaL_checknumber(p_state, -1)));
            lua_pop(p_state, POINT_COORDINATES);
        }
        return points;
    }

    /// x, y, w, h from p_index on
    static Rect2 check_rect(lua_State *p_state, const int p_index) {
        return {static_cast<real_t>(luaL_checknumber(p_state, p_index)),
                static_cast<real_t>(luaL_checknumber(p_state, p_index + 1)),
                static_cast<real_t>(luaL_checknumber(p_state, p_index + 2)),
                static_cast<real_t>(luaL_checknumber(p_state, p_index + 3))};
    }

    static float opt_width(lua_State *p_state, const int p_index) {
        return static_cast<float>(luaL_optnumber(p_state, p_index, DEFAULT_WIDTH));
    }

    static void push_size(lua_State *p_state, const Vector2 &p_size) {
        lua_pushnumber(p_state, p_size.x);
        lua_pushnumber(p_state, p_size.y);
    }

    /// Something the script made is its own, freed with the script
    static void push_kept(lua_State *p_state, const RID &p_resource, const ScriptHandleKind p_kind) {
        LuaModules::server<ScenarioScriptServer>(p_state)->script_drawing_resource_keep(
                LuaScriptContext::from_state(p_state)->get_rid(), p_resource);
        LuaHandle::push(p_state, p_resource, p_kind);
    }

    /// An image or a drawing - what can be pasted
    static RID check_source(lua_State *p_state, const int p_index) {
        const RID image = LuaHandle::test(p_state, p_index, ScriptHandleKind::IMAGE);
        return image.is_valid() ? image : LuaHandle::check(p_state, p_index, ScriptHandleKind::DRAWING);
    }

    /// The size of the source check_source() took at p_index
    static Vector2 source_size(lua_State *p_state, const int p_index, const RID &p_source) {
        const bool image = LuaHandle::test(p_state, p_index, ScriptHandleKind::IMAGE).is_valid();
        return image ? Vector2(drawings(p_state)->image_get_size(p_source))
                     : Vector2(drawings(p_state)->drawing_get_size(p_source));
    }

    static const char *const FILTER_NAMES[] = {"nearest", "linear", nullptr};

    static DrawingServer::Filter opt_filter(lua_State *p_state, const int p_index) {
        return static_cast<DrawingServer::Filter>(luaL_checkoption(p_state, p_index, "nearest", FILTER_NAMES));
    }

    static String check_data_path(lua_State *p_state, const int p_index) {
        return LuaScriptContext::from_state(p_state)->data_file_path(
                p_state, String::utf8(luaL_checkstring(p_state, p_index)));
    }

    /// create(width, height[, transparent]) - a drawing, opaque black or transparent
    static int draw_create(lua_State *p_state) {
        const Vector2i size(
                static_cast<int32_t>(luaL_checkinteger(p_state, 1)),
                static_cast<int32_t>(luaL_checkinteger(p_state, 2)));
        luaL_argcheck(p_state, size.x > 0 && size.y > 0, 1, "a drawing needs a size");
        push_kept(
                p_state, drawings(p_state)->drawing_create(size, lua_toboolean(p_state, 3) != 0),
                ScriptHandleKind::DRAWING);
        return 1;
    }

    /// size(drawing) - its width and height
    static int draw_size(lua_State *p_state) {
        push_size(
                p_state, drawings(p_state)->drawing_get_size(LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING)));
        return 2;
    }

    /// clear(drawing, colour) - everything drawn goes, it is filled with the colour
    static int draw_clear(lua_State *p_state) {
        drawings(p_state)->drawing_clear(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), LuaVariant::check_color(p_state, 2));
        return 0;
    }

    /// rect(drawing, x, y, w, h, colour)
    static int draw_rect(lua_State *p_state) {
        drawings(p_state)->drawing_add_rect(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), check_rect(p_state, 2),
                LuaVariant::check_color(p_state, 6));
        return 0;
    }

    /// rect_outline(drawing, x, y, w, h, colour[, width]) - the edge, inside the rectangle
    static int draw_rect_outline(lua_State *p_state) {
        drawings(p_state)->drawing_add_rect_outline(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), check_rect(p_state, 2),
                LuaVariant::check_color(p_state, 6), opt_width(p_state, 7));
        return 0;
    }

    /// line(drawing, points, colour[, width]) - through the pixels the points name
    static int draw_line(lua_State *p_state) {
        drawings(p_state)->drawing_add_line(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), check_points(p_state, 2),
                LuaVariant::check_color(p_state, 3), opt_width(p_state, 4));
        return 0;
    }

    /// ellipse(drawing, x, y, w, h, colour) - the ellipse the rectangle bounds
    static int draw_ellipse(lua_State *p_state) {
        drawings(p_state)->drawing_add_ellipse(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), check_rect(p_state, 2),
                LuaVariant::check_color(p_state, 6));
        return 0;
    }

    /// ellipse_outline(drawing, x, y, w, h, colour[, width])
    static int draw_ellipse_outline(lua_State *p_state) {
        drawings(p_state)->drawing_add_ellipse_outline(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), check_rect(p_state, 2),
                LuaVariant::check_color(p_state, 6), opt_width(p_state, 7));
        return 0;
    }

    /// polygon(drawing, points, colour)
    static int draw_polygon(lua_State *p_state) {
        drawings(p_state)->drawing_add_polygon(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), check_points(p_state, 2),
                LuaVariant::check_color(p_state, 3));
        return 0;
    }

    /// polygon_outline(drawing, points, colour[, width])
    static int draw_polygon_outline(lua_State *p_state) {
        drawings(p_state)->drawing_add_polygon_outline(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), check_points(p_state, 2),
                LuaVariant::check_color(p_state, 3), opt_width(p_state, 4));
        return 0;
    }

    /// pieslice(drawing, x, y, w, h, start, end, colour) - angles in degrees, clockwise from three
    /// o'clock
    static int draw_pieslice(lua_State *p_state) {
        constexpr int START_INDEX = 6;
        drawings(p_state)->drawing_add_pieslice(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), check_rect(p_state, 2),
                luaL_checknumber(p_state, START_INDEX), luaL_checknumber(p_state, START_INDEX + 1),
                LuaVariant::check_color(p_state, START_INDEX + 2));
        return 0;
    }

    /// pixel(drawing, x, y, colour)
    static int draw_pixel(lua_State *p_state) {
        drawings(p_state)->drawing_add_pixel(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING),
                Vector2i(
                        static_cast<int32_t>(luaL_checkinteger(p_state, 2)),
                        static_cast<int32_t>(luaL_checkinteger(p_state, 3))),
                LuaVariant::check_color(p_state, 4));
        return 0;
    }

    /// text(drawing, x, y, text, font, size, colour) - the text's top left corner at x, y
    static int draw_text(lua_State *p_state) {
        constexpr int FONT_INDEX = 5;
        drawings(p_state)->drawing_add_text(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING),
                Vector2(luaL_checknumber(p_state, 2), luaL_checknumber(p_state, 3)),
                String::utf8(luaL_checkstring(p_state, 4)),
                LuaHandle::check(p_state, FONT_INDEX, ScriptHandleKind::FONT),
                static_cast<int>(luaL_checkinteger(p_state, FONT_INDEX + 1)),
                LuaVariant::check_color(p_state, FONT_INDEX + 2));
        return 0;
    }

    /// text_size(font, text, size) - the width and height the text takes
    static int draw_text_size(lua_State *p_state) {
        push_size(
                p_state,
                drawings(p_state)->font_get_text_size(
                        LuaHandle::check(p_state, 1, ScriptHandleKind::FONT),
                        String::utf8(luaL_checkstring(p_state, 2)), static_cast<int>(luaL_checkinteger(p_state, 3))));
        return 2;
    }

    /// How a font's edges are smoothed, as a script names it (TextServer::FontAntialiasing)
    static const char *const ANTIALIASING_NAMES[] = {"none", "gray", "lcd", nullptr};

    /// load_font(path[, antialiasing]) - a TrueType or OpenType font of the data directory, its edges
    /// smoothed ("gray") or not ("none", whole pixels for a LED board)
    static int draw_load_font(lua_State *p_state) {
        const String path = check_data_path(p_state, 1);
        const auto antialiasing =
                static_cast<TextServer::FontAntialiasing>(luaL_checkoption(p_state, 2, "gray", ANTIALIASING_NAMES));
        const RID font = drawings(p_state)->font_load(path, antialiasing);
        if (!font.is_valid()) {
            // luaL_error is the Lua C API's vararg error call
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            return luaL_error(p_state, "cannot read the font %s", lua_tostring(p_state, 1));
        }
        push_kept(p_state, font, ScriptHandleKind::FONT);
        return 1;
    }

    /// The image of the data directory at argument 1, read by p_load
    static int load_image(lua_State *p_state, RID (DrawingServer::*p_load)(const String &)) {
        const String path = check_data_path(p_state, 1);
        const RID image = (drawings(p_state)->*p_load)(path);
        if (!image.is_valid()) {
            // luaL_error is the Lua C API's vararg error call
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
            return luaL_error(p_state, "cannot read the image %s", lua_tostring(p_state, 1));
        }
        push_kept(p_state, image, ScriptHandleKind::IMAGE);
        return 1;
    }

    /// load_image(path) - an image of the data directory (png, bmp, tga, dds, ...)
    static int draw_load_image(lua_State *p_state) {
        return load_image(p_state, &DrawingServer::image_load);
    }

    /// load_mask(path) - an image as a mask: pasted with a tint, it is drawn in the tint's colour,
    /// as opaque as the image's alpha, or as its brightness when it has none
    static int draw_load_mask(lua_State *p_state) {
        return load_image(p_state, &DrawingServer::image_load_mask);
    }

    /// image_size(image) - its width and height
    static int draw_image_size(lua_State *p_state) {
        push_size(p_state, drawings(p_state)->image_get_size(LuaHandle::check(p_state, 1, ScriptHandleKind::IMAGE)));
        return 2;
    }

    /// image_pixel(image, x, y) - the colour of its pixel, {r, g, b, a}
    static int draw_image_pixel(lua_State *p_state) {
        const Color color = drawings(p_state)->image_get_pixel(
                LuaHandle::check(p_state, 1, ScriptHandleKind::IMAGE),
                Vector2i(
                        static_cast<int32_t>(luaL_checkinteger(p_state, 2)),
                        static_cast<int32_t>(luaL_checkinteger(p_state, 3))));
        LuaVariant::push_color(p_state, color);
        return 1;
    }

    /// A number field of the options table at p_index, p_default without it
    static double option_number(lua_State *p_state, const int p_index, const char *p_name, const double p_default) {
        lua_getfield(p_state, p_index, p_name);
        const double value = lua_isnil(p_state, -1) ? p_default : luaL_checknumber(p_state, -1);
        lua_pop(p_state, 1);
        return value;
    }

    static bool option_flag(lua_State *p_state, const int p_index, const char *p_name) {
        lua_getfield(p_state, p_index, p_name);
        const bool value = lua_toboolean(p_state, -1) != 0;
        lua_pop(p_state, 1);
        return value;
    }

    /// paste(drawing, source, x, y[, options]) - an image or another drawing, mixed by its alpha.
    /// options: w, h (scaled to them), source = {x, y, w, h} (a part of it), filter = "nearest" or
    /// "linear", flip_x, flip_y, tint = colour
    static int draw_paste(lua_State *p_state) {
        constexpr int OPTIONS_INDEX = 5;
        const RID drawing = LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING);
        const RID source = check_source(p_state, 2);
        const Vector2 position(luaL_checknumber(p_state, 3), luaL_checknumber(p_state, 4));
        Rect2 source_rect(Vector2(), source_size(p_state, 2, source));
        Vector2 size = source_rect.size;
        Color tint(1, 1, 1, 1);
        DrawingServer::Filter filter = DrawingServer::FILTER_NEAREST;
        bool flip_x = false;
        bool flip_y = false;
        if (!lua_isnoneornil(p_state, OPTIONS_INDEX)) {
            luaL_checktype(p_state, OPTIONS_INDEX, LUA_TTABLE);
            if (lua_getfield(p_state, OPTIONS_INDEX, "source") != LUA_TNIL) {
                const int part = lua_gettop(p_state);
                luaL_checktype(p_state, part, LUA_TTABLE);
                for (int i = 1; i <= RECT_FIELDS; i++) {
                    lua_rawgeti(p_state, part, i);
                }
                source_rect = check_rect(p_state, part + 1);
                lua_pop(p_state, RECT_FIELDS);
                size = source_rect.size;
            }
            lua_pop(p_state, 1);
            size =
                    Vector2(option_number(p_state, OPTIONS_INDEX, "w", size.x),
                            option_number(p_state, OPTIONS_INDEX, "h", size.y));
            if (lua_getfield(p_state, OPTIONS_INDEX, "tint") != LUA_TNIL) {
                tint = LuaVariant::check_color(p_state, lua_gettop(p_state));
            }
            lua_pop(p_state, 1);
            lua_getfield(p_state, OPTIONS_INDEX, "filter");
            filter = opt_filter(p_state, lua_gettop(p_state));
            lua_pop(p_state, 1);
            flip_x = option_flag(p_state, OPTIONS_INDEX, "flip_x");
            flip_y = option_flag(p_state, OPTIONS_INDEX, "flip_y");
        }
        // a negative size flips the image where it stands
        const Rect2 rect(position, Vector2(flip_x ? -size.x : size.x, flip_y ? -size.y : size.y));
        drawings(p_state)->drawing_add_texture(drawing, source, source_rect, rect, tint, filter);
        return 0;
    }

    /// paste_rotated(drawing, source, cx, cy, angle[, filter]) - turned by the angle [deg,
    /// counter-clockwise] about its middle, its middle at cx, cy
    static int draw_paste_rotated(lua_State *p_state) {
        constexpr int ANGLE_INDEX = 5;
        drawings(p_state)->drawing_add_texture_rotated(
                LuaHandle::check(p_state, 1, ScriptHandleKind::DRAWING), check_source(p_state, 2),
                Vector2(luaL_checknumber(p_state, 3), luaL_checknumber(p_state, 4)),
                luaL_checknumber(p_state, ANGLE_INDEX), opt_filter(p_state, ANGLE_INDEX + 1));
        return 0;
    }

    /// free(handle) - a drawing, a font or an image the script made, before the script ends; false
    /// when the script did not make it
    static int draw_free(lua_State *p_state) {
        const RID resource = LuaHandle::to_rid(p_state, 1);
        luaL_argcheck(p_state, resource.is_valid(), 1, "a drawing, a font or an image");
        lua_pushboolean(
                p_state,
                static_cast<int>(LuaModules::server<ScenarioScriptServer>(p_state)->script_drawing_resource_free(
                        LuaScriptContext::from_state(p_state)->get_rid(), resource)));
        return 1;
    }

    const luaL_Reg LuaModules::DRAW[] = {
            {"create", draw_create},
            {"size", draw_size},
            {"clear", draw_clear},
            {"rect", draw_rect},
            {"rect_outline", draw_rect_outline},
            {"line", draw_line},
            {"ellipse", draw_ellipse},
            {"ellipse_outline", draw_ellipse_outline},
            {"polygon", draw_polygon},
            {"polygon_outline", draw_polygon_outline},
            {"pieslice", draw_pieslice},
            {"pixel", draw_pixel},
            {"text", draw_text},
            {"text_size", draw_text_size},
            {"load_font", draw_load_font},
            {"load_image", draw_load_image},
            {"load_mask", draw_load_mask},
            {"image_size", draw_image_size},
            {"image_pixel", draw_image_pixel},
            {"paste", draw_paste},
            {"paste_rotated", draw_paste_rotated},
            {"free", draw_free},
            {nullptr, nullptr},
    };
} // namespace godot
