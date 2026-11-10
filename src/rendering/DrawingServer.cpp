#include "DrawingServer.hpp"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/transform2d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void DrawingServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("drawing_create", "size", "transparent"), &DrawingServer::drawing_create);
        ClassDB::bind_method(D_METHOD("drawing_get_size", "drawing"), &DrawingServer::drawing_get_size);
        ClassDB::bind_method(
                D_METHOD("drawing_set_material", "drawing", "material", "output_size"),
                &DrawingServer::drawing_set_material);
        ClassDB::bind_method(D_METHOD("drawing_set_flip_y", "drawing", "flip_y"), &DrawingServer::drawing_set_flip_y);
        ClassDB::bind_method(D_METHOD("drawing_get_texture", "drawing"), &DrawingServer::drawing_get_texture);
        ClassDB::bind_method(D_METHOD("drawing_clear", "drawing", "color"), &DrawingServer::drawing_clear);
        ClassDB::bind_method(
                D_METHOD("drawing_add_rect", "drawing", "rect", "color"), &DrawingServer::drawing_add_rect);
        ClassDB::bind_method(
                D_METHOD("drawing_add_rect_outline", "drawing", "rect", "color", "width"),
                &DrawingServer::drawing_add_rect_outline);
        ClassDB::bind_method(
                D_METHOD("drawing_add_line", "drawing", "points", "color", "width"), &DrawingServer::drawing_add_line);
        ClassDB::bind_method(
                D_METHOD("drawing_add_ellipse", "drawing", "rect", "color"), &DrawingServer::drawing_add_ellipse);
        ClassDB::bind_method(
                D_METHOD("drawing_add_ellipse_outline", "drawing", "rect", "color", "width"),
                &DrawingServer::drawing_add_ellipse_outline);
        ClassDB::bind_method(
                D_METHOD("drawing_add_polygon", "drawing", "points", "color"), &DrawingServer::drawing_add_polygon);
        ClassDB::bind_method(
                D_METHOD("drawing_add_polygon_outline", "drawing", "points", "color", "width"),
                &DrawingServer::drawing_add_polygon_outline);
        ClassDB::bind_method(
                D_METHOD("drawing_add_pieslice", "drawing", "rect", "start", "end", "color"),
                &DrawingServer::drawing_add_pieslice);
        ClassDB::bind_method(
                D_METHOD("drawing_add_pixel", "drawing", "position", "color"), &DrawingServer::drawing_add_pixel);
        ClassDB::bind_method(
                D_METHOD("drawing_add_text", "drawing", "position", "text", "font", "size", "color"),
                &DrawingServer::drawing_add_text);
        ClassDB::bind_method(
                D_METHOD("drawing_add_texture", "drawing", "source", "source_rect", "rect", "modulate", "filter"),
                &DrawingServer::drawing_add_texture);
        ClassDB::bind_method(
                D_METHOD("drawing_add_texture_rotated", "drawing", "source", "center", "angle", "filter"),
                &DrawingServer::drawing_add_texture_rotated);
        ClassDB::bind_method(
                D_METHOD("font_load", "path", "antialiasing"), &DrawingServer::font_load,
                DEFVAL(TextServer::FONT_ANTIALIASING_GRAY));
        ClassDB::bind_method(
                D_METHOD("font_get_text_size", "font", "text", "size"), &DrawingServer::font_get_text_size);
        ClassDB::bind_method(D_METHOD("image_load", "path"), &DrawingServer::image_load);
        ClassDB::bind_method(D_METHOD("image_load_mask", "path"), &DrawingServer::image_load_mask);
        ClassDB::bind_method(D_METHOD("image_get_size", "image"), &DrawingServer::image_get_size);
        ClassDB::bind_method(D_METHOD("image_get_pixel", "image", "position"), &DrawingServer::image_get_pixel);
        ClassDB::bind_method(D_METHOD("free_rid", "rid"), &DrawingServer::free_rid);
        ClassDB::bind_method(D_METHOD("owns", "rid"), &DrawingServer::owns);

        BIND_ENUM_CONSTANT(FILTER_NEAREST);
        BIND_ENUM_CONSTANT(FILTER_LINEAR);
    }

    DrawingServer::DrawingServer() {
        singleton = this;
    }

    DrawingServer::~DrawingServer() {
        LocalVector<RID> owned;
        for (const KeyValue<RID, Drawing> &drawing: drawings) {
            owned.push_back(drawing.key);
        }
        for (const KeyValue<RID, LoadedImage> &image: images) {
            owned.push_back(image.key);
        }
        for (const RID &rid: owned) {
            free_rid(rid);
        }
        singleton = nullptr;
    }

    // --- drawings ---

    DrawingServer::Canvas DrawingServer::_canvas_create(const Vector2i &p_size, const bool p_transparent) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        Canvas canvas;
        canvas.viewport = rendering->viewport_create();
        rendering->viewport_set_size(canvas.viewport, p_size.x, p_size.y);
        rendering->viewport_set_disable_3d(canvas.viewport, true);
        rendering->viewport_set_transparent_background(canvas.viewport, p_transparent);
        rendering->viewport_set_active(canvas.viewport, true);
        canvas.canvas = rendering->canvas_create();
        rendering->viewport_attach_canvas(canvas.viewport, canvas.canvas);
        canvas.root = rendering->canvas_item_create();
        rendering->canvas_item_set_parent(canvas.root, canvas.canvas);
        return canvas;
    }

    void DrawingServer::_canvas_free(const Canvas &p_canvas) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        rendering->free_rid(p_canvas.root);
        rendering->free_rid(p_canvas.canvas);
        rendering->free_rid(p_canvas.viewport);
    }

    RID DrawingServer::drawing_create(const Vector2i &p_size, const bool p_transparent) {
        ERR_FAIL_COND_V_MSG(p_size.x <= 0 || p_size.y <= 0, RID(), "A drawing needs a size");
        Drawing drawing;
        drawing.size = p_size;
        drawing.transparent = p_transparent;
        drawing.picture = _canvas_create(p_size, p_transparent);
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        drawings.insert(rid, drawing);
        drawing_clear(rid, Color(0, 0, 0, p_transparent ? 0 : 1));
        return rid;
    }

    Vector2i DrawingServer::drawing_get_size(const RID &p_drawing) const {
        const Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL_V(drawing, Vector2i());
        return drawing->size;
    }

    /// The picture is the output's child viewport, so it is drawn first in a frame both are drawn in
    void
    DrawingServer::drawing_set_material(const RID &p_drawing, const RID &p_material, const Vector2i &p_output_size) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        RenderingServer *rendering = RenderingServer::get_singleton();
        if (drawing->output.viewport.is_valid()) {
            rendering->viewport_set_parent_viewport(drawing->picture.viewport, RID());
            _canvas_free(drawing->output);
            drawing->output = Canvas();
            drawing->output_size = Vector2i();
        }
        if (!p_material.is_valid()) {
            return;
        }
        ERR_FAIL_COND_MSG(p_output_size.x <= 0 || p_output_size.y <= 0, "A drawing's output needs a size");
        drawing->output = _canvas_create(p_output_size, drawing->transparent);
        drawing->output_size = p_output_size;
        rendering->viewport_set_parent_viewport(drawing->picture.viewport, drawing->output.viewport);
        rendering->canvas_item_set_material(drawing->output.root, p_material);
        rendering->canvas_item_add_texture_rect(
                drawing->output.root, Rect2(Vector2(), p_output_size),
                rendering->viewport_get_texture(drawing->picture.viewport));
        _apply_flip_y(*drawing);
        _changed(*drawing);
    }

    void DrawingServer::drawing_set_flip_y(const RID &p_drawing, const bool p_flip_y) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        drawing->flip_y = p_flip_y;
        _apply_flip_y(*drawing);
        _changed(*drawing);
    }

    /// The picture is drawn into the output the right way up; the flip is the last canvas's
    void DrawingServer::_apply_flip_y(const Drawing &p_drawing) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        const bool through_material = p_drawing.output.viewport.is_valid();
        const Canvas &last = through_material ? p_drawing.output : p_drawing.picture;
        const double height = through_material ? p_drawing.output_size.y : p_drawing.size.y;
        rendering->viewport_set_canvas_transform(
                last.viewport, last.canvas,
                p_drawing.flip_y ? Transform2D(0.0, Size2(1.0, -1.0), 0.0, Vector2(0.0, height)) : Transform2D());
        if (through_material) {
            rendering->viewport_set_canvas_transform(
                    p_drawing.picture.viewport, p_drawing.picture.canvas, Transform2D());
        }
    }

    RID DrawingServer::drawing_get_texture(const RID &p_drawing) const {
        const Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL_V(drawing, RID());
        const Canvas &last = drawing->output.viewport.is_valid() ? drawing->output : drawing->picture;
        return RenderingServer::get_singleton()->viewport_get_texture(last.viewport);
    }

    void DrawingServer::_changed(const Drawing &p_drawing) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        rendering->viewport_set_update_mode(p_drawing.picture.viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);
        if (p_drawing.output.viewport.is_valid()) {
            rendering->viewport_set_update_mode(p_drawing.output.viewport, RenderingServer::VIEWPORT_UPDATE_ONCE);
        }
    }

    RID DrawingServer::_layer(Drawing &p_drawing) {
        if (p_drawing.layers.is_empty()) {
            return _layer(p_drawing, p_drawing.layer_filter);
        }
        return p_drawing.layers[p_drawing.layers.size() - 1];
    }

    RID DrawingServer::_layer(Drawing &p_drawing, const Filter p_filter) {
        if (!p_drawing.layers.is_empty() && p_drawing.layer_filter == p_filter) {
            return p_drawing.layers[p_drawing.layers.size() - 1];
        }
        RenderingServer *rendering = RenderingServer::get_singleton();
        const RID layer = rendering->canvas_item_create();
        rendering->canvas_item_set_parent(layer, p_drawing.picture.root);
        rendering->canvas_item_set_default_texture_filter(
                layer, p_filter == FILTER_NEAREST ? RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_NEAREST
                                                  : RenderingServer::CANVAS_ITEM_TEXTURE_FILTER_LINEAR_WITH_MIPMAPS);
        p_drawing.layers.push_back(layer);
        p_drawing.layer_filter = p_filter;
        return layer;
    }

    void DrawingServer::drawing_clear(const RID &p_drawing, const Color &p_color) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (const RID &layer: drawing->layers) {
            rendering->free_rid(layer);
        }
        drawing->layers.clear();
        rendering->canvas_item_clear(drawing->picture.root);
        Color color = p_color;
        if (!drawing->transparent) {
            color.a = 1;
        }
        rendering->canvas_item_add_rect(drawing->picture.root, Rect2(Vector2(), drawing->size), color);
        _changed(*drawing);
    }

    void DrawingServer::drawing_add_rect(const RID &p_drawing, const Rect2 &p_rect, const Color &p_color) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        RenderingServer::get_singleton()->canvas_item_add_rect(_layer(*drawing), p_rect, p_color);
        _changed(*drawing);
    }

    /// Drawn through the middle of its width, so its outer side lies on the rectangle's edge
    void DrawingServer::drawing_add_rect_outline(
            const RID &p_drawing, const Rect2 &p_rect, const Color &p_color, const float p_width) {
        const Rect2 inner = p_rect.grow(-p_width / 2);
        PackedVector2Array points;
        points.push_back(inner.position);
        points.push_back(Vector2(inner.get_end().x, inner.position.y));
        points.push_back(inner.get_end());
        points.push_back(Vector2(inner.position.x, inner.get_end().y));
        points.push_back(inner.position);
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        RenderingServer::get_singleton()->canvas_item_add_polyline(
                _layer(*drawing), points, PackedColorArray({p_color}), p_width);
        _changed(*drawing);
    }

    void DrawingServer::drawing_add_line(
            const RID &p_drawing, const PackedVector2Array &p_points, const Color &p_color, const float p_width) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        PackedVector2Array points = p_points;
        for (int64_t i = 0; i < points.size(); i++) {
            points.set(i, points[i] + Vector2(PIXEL_CENTRE, PIXEL_CENTRE));
        }
        RenderingServer::get_singleton()->canvas_item_add_polyline(
                _layer(*drawing), points, PackedColorArray({p_color}), p_width);
        _changed(*drawing);
    }

    PackedVector2Array DrawingServer::_arc(const Rect2 &p_rect, const double p_start, const double p_end) {
        const Vector2 center = p_rect.get_center();
        const Vector2 radius = p_rect.size / 2;
        const double sweep = p_end - p_start;
        const int segments =
                MAX(1, static_cast<int>(Math::ceil(Math::abs(sweep) / FULL_TURN_DEGREES * ARC_SEGMENTS_PER_TURN)));
        PackedVector2Array points;
        for (int i = 0; i <= segments; i++) {
            const double angle = Math::deg_to_rad(p_start + (sweep * i / segments));
            points.push_back(center + Vector2(radius.x * Math::cos(angle), radius.y * Math::sin(angle)));
        }
        return points;
    }

    void DrawingServer::drawing_add_ellipse(const RID &p_drawing, const Rect2 &p_rect, const Color &p_color) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        PackedVector2Array points = _arc(p_rect, 0, FULL_TURN_DEGREES);
        points.remove_at(points.size() - 1);
        RenderingServer::get_singleton()->canvas_item_add_polygon(
                _layer(*drawing), points, PackedColorArray({p_color}));
        _changed(*drawing);
    }

    void DrawingServer::drawing_add_ellipse_outline(
            const RID &p_drawing, const Rect2 &p_rect, const Color &p_color, const float p_width) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        RenderingServer::get_singleton()->canvas_item_add_polyline(
                _layer(*drawing), _arc(p_rect.grow(-p_width / 2), 0, FULL_TURN_DEGREES), PackedColorArray({p_color}),
                p_width);
        _changed(*drawing);
    }

    void
    DrawingServer::drawing_add_polygon(const RID &p_drawing, const PackedVector2Array &p_points, const Color &p_color) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        RenderingServer::get_singleton()->canvas_item_add_polygon(
                _layer(*drawing), p_points, PackedColorArray({p_color}));
        _changed(*drawing);
    }

    void DrawingServer::drawing_add_polygon_outline(
            const RID &p_drawing, const PackedVector2Array &p_points, const Color &p_color, const float p_width) {
        ERR_FAIL_COND(p_points.is_empty());
        PackedVector2Array closed = p_points;
        closed.push_back(p_points[0]);
        drawing_add_line(p_drawing, closed, p_color, p_width);
    }

    void DrawingServer::drawing_add_pieslice(
            const RID &p_drawing, const Rect2 &p_rect, const double p_start, const double p_end, const Color &p_color) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        PackedVector2Array points = _arc(p_rect, p_start, p_end);
        points.insert(0, p_rect.get_center());
        RenderingServer::get_singleton()->canvas_item_add_polygon(
                _layer(*drawing), points, PackedColorArray({p_color}));
        _changed(*drawing);
    }

    void DrawingServer::drawing_add_pixel(const RID &p_drawing, const Vector2i &p_position, const Color &p_color) {
        drawing_add_rect(p_drawing, Rect2(p_position, Vector2(1, 1)), p_color);
    }

    void DrawingServer::drawing_add_text(
            const RID &p_drawing, const Vector2 &p_position, const String &p_text, const RID &p_font, const int p_size,
            const Color &p_color) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        const Ref<FontFile> *font = fonts.getptr(p_font);
        ERR_FAIL_NULL_MSG(font, "Not a font of DrawingServer");
        (*font)->draw_string(
                _layer(*drawing), p_position + Vector2(0, (*font)->get_ascent(p_size)), p_text,
                HORIZONTAL_ALIGNMENT_LEFT, -1, p_size, p_color);
        _changed(*drawing);
    }

    RID DrawingServer::_source_texture(const RID &p_source) const {
        if (const LoadedImage *image = images.getptr(p_source)) {
            return image->texture;
        }
        return drawing_get_texture(p_source);
    }

    void DrawingServer::drawing_add_texture(
            const RID &p_drawing, const RID &p_source, const Rect2 &p_source_rect, const Rect2 &p_rect,
            const Color &p_modulate, const Filter p_filter) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        ERR_FAIL_COND_MSG(
                !(images.has(p_source) || drawings.has(p_source)), "Not an image nor a drawing of DrawingServer");
        RenderingServer::get_singleton()->canvas_item_add_texture_rect_region(
                _layer(*drawing, p_filter), p_rect, _source_texture(p_source), p_source_rect, p_modulate);
        _changed(*drawing);
    }

    /// The y axis points down, so a counter-clockwise turn is a negative angle of the transform
    void DrawingServer::drawing_add_texture_rotated(
            const RID &p_drawing, const RID &p_source, const Vector2 &p_center, const double p_angle,
            const Filter p_filter) {
        Drawing *drawing = drawings.getptr(p_drawing);
        ERR_FAIL_NULL(drawing);
        ERR_FAIL_COND_MSG(
                !(images.has(p_source) || drawings.has(p_source)), "Not an image nor a drawing of DrawingServer");
        const LoadedImage *image = images.getptr(p_source);
        const Vector2 size = image == nullptr ? Vector2(drawing_get_size(p_source)) : Vector2(image->image->get_size());
        RenderingServer *rendering = RenderingServer::get_singleton();
        const RID layer = _layer(*drawing, p_filter);
        rendering->canvas_item_add_set_transform(layer, Transform2D(-Math::deg_to_rad(p_angle), p_center));
        rendering->canvas_item_add_texture_rect(layer, Rect2(-size / 2, size), _source_texture(p_source));
        rendering->canvas_item_add_set_transform(layer, Transform2D());
        _changed(*drawing);
    }

    // --- fonts and images ---

    /// Without smoothing its glyphs stand on whole pixels as well
    RID DrawingServer::font_load(const String &p_path, const TextServer::FontAntialiasing p_antialiasing) {
        Ref<FontFile> font;
        font.instantiate();
        ERR_FAIL_COND_V_MSG(!(font->load_dynamic_font(p_path) == OK), RID(), "Cannot read the font " + p_path);
        // after loading, which sets the font's own options back to their defaults
        font->set_antialiasing(p_antialiasing);
        if (p_antialiasing == TextServer::FONT_ANTIALIASING_NONE) {
            font->set_subpixel_positioning(TextServer::SUBPIXEL_POSITIONING_DISABLED);
        }
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        fonts.insert(rid, font);
        return rid;
    }

    Vector2 DrawingServer::font_get_text_size(const RID &p_font, const String &p_text, const int p_size) const {
        const Ref<FontFile> *font = fonts.getptr(p_font);
        ERR_FAIL_NULL_V_MSG(font, Vector2(), "Not a font of DrawingServer");
        return (*font)->get_string_size(p_text, HORIZONTAL_ALIGNMENT_LEFT, -1, p_size);
    }

    RID DrawingServer::_image_create(const Ref<Image> &p_image) {
        p_image->generate_mipmaps();
        LoadedImage image;
        image.image = p_image;
        image.texture = RenderingServer::get_singleton()->texture_2d_create(p_image);
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        images.insert(rid, image);
        return rid;
    }

    /// Decompressed, so its pixels can be read (a dds is DXT-compressed)
    RID DrawingServer::image_load(const String &p_path) {
        const Ref<Image> image = Image::load_from_file(p_path);
        ERR_FAIL_COND_V_MSG(image.is_null(), RID(), "Cannot read the image " + p_path);
        image->decompress();
        return _image_create(image);
    }

    /// The mask is the alpha of an image that has one, the brightness of one that has none
    RID DrawingServer::image_load_mask(const String &p_path) {
        const Ref<Image> image = Image::load_from_file(p_path);
        ERR_FAIL_COND_V_MSG(image.is_null(), RID(), "Cannot read the image " + p_path);
        image->decompress();
        image->clear_mipmaps();
        const bool has_alpha = !(image->detect_alpha() == Image::ALPHA_NONE);
        image->convert(Image::FORMAT_RGBA8);
        for (int y = 0; y < image->get_height(); y++) {
            for (int x = 0; x < image->get_width(); x++) {
                const Color color = image->get_pixel(x, y);
                image->set_pixel(x, y, Color(1, 1, 1, has_alpha ? color.a : color.get_luminance()));
            }
        }
        return _image_create(image);
    }

    Vector2i DrawingServer::image_get_size(const RID &p_image) const {
        const LoadedImage *image = images.getptr(p_image);
        ERR_FAIL_NULL_V(image, Vector2i());
        return image->image->get_size();
    }

    Color DrawingServer::image_get_pixel(const RID &p_image, const Vector2i &p_position) const {
        const LoadedImage *image = images.getptr(p_image);
        ERR_FAIL_NULL_V(image, Color());
        return image->image->get_pixelv(p_position);
    }

    // --- every kind ---

    void DrawingServer::free_rid(const RID &p_rid) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        if (Drawing *drawing = drawings.getptr(p_rid)) {
            for (const RID &layer: drawing->layers) {
                rendering->free_rid(layer);
            }
            if (drawing->output.viewport.is_valid()) {
                _canvas_free(drawing->output);
            }
            _canvas_free(drawing->picture);
            drawings.erase(p_rid);
            return;
        }
        if (const LoadedImage *image = images.getptr(p_rid)) {
            rendering->free_rid(image->texture);
            images.erase(p_rid);
            return;
        }
        ERR_FAIL_COND_MSG(!fonts.erase(p_rid), "Not a drawing, a font nor an image of DrawingServer");
    }

    bool DrawingServer::owns(const RID &p_rid) const {
        return drawings.has(p_rid) || fonts.has(p_rid) || images.has(p_rid);
    }
} // namespace godot
