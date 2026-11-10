#include "DrawingTexture.hpp"
#include "rendering/DrawingServer.hpp"
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void DrawingTexture::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_texture_size", "texture_size"), &DrawingTexture::set_texture_size);
        ClassDB::bind_method(D_METHOD("get_texture_size"), &DrawingTexture::get_texture_size);
        ADD_PROPERTY(
                PropertyInfo(Variant::VECTOR2I, "texture_size", PROPERTY_HINT_NONE, "suffix:px"), "set_texture_size",
                "get_texture_size");
        ClassDB::bind_method(D_METHOD("set_transparent", "transparent"), &DrawingTexture::set_transparent);
        ClassDB::bind_method(D_METHOD("get_transparent"), &DrawingTexture::get_transparent);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "transparent"), "set_transparent", "get_transparent");
        ClassDB::bind_method(D_METHOD("set_flip_y", "flip_y"), &DrawingTexture::set_flip_y);
        ClassDB::bind_method(D_METHOD("get_flip_y"), &DrawingTexture::get_flip_y);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "flip_y"), "set_flip_y", "get_flip_y");
        ClassDB::bind_method(D_METHOD("attach_drawing", "drawing"), &DrawingTexture::attach_drawing);
    }

    /// The RenderingServer's own textures, not a resource loaded: made as the engine makes
    /// ViewportTexture's proxy
    DrawingTexture::DrawingTexture() {
        RenderingServer *rendering = RenderingServer::get_singleton();
        placeholder = rendering->texture_2d_placeholder_create();
        proxy = rendering->texture_proxy_create(placeholder);
    }

    DrawingTexture::~DrawingTexture() {
        RenderingServer *rendering = RenderingServer::get_singleton();
        ERR_FAIL_NULL(rendering);
        rendering->free_rid(proxy);
        rendering->free_rid(placeholder);
    }

    void DrawingTexture::set_texture_size(const Vector2i &p_texture_size) {
        texture_size = p_texture_size;
        emit_changed();
    }

    Vector2i DrawingTexture::get_texture_size() const {
        return texture_size;
    }

    void DrawingTexture::set_transparent(const bool p_transparent) {
        transparent = p_transparent;
        emit_changed();
    }

    bool DrawingTexture::get_transparent() const {
        return transparent;
    }

    void DrawingTexture::set_flip_y(const bool p_flip_y) {
        flip_y = p_flip_y;
    }

    bool DrawingTexture::get_flip_y() const {
        return flip_y;
    }

    void DrawingTexture::attach_drawing(const RID &p_drawing) {
        RID texture = placeholder;
        if (p_drawing.is_valid()) {
            DrawingServer *drawings = DrawingServer::get_instance();
            ERR_FAIL_NULL(drawings);
            texture = drawings->drawing_get_texture(p_drawing);
        }
        RenderingServer::get_singleton()->texture_proxy_update(proxy, texture);
    }

    int32_t DrawingTexture::_get_width() const {
        return texture_size.x;
    }

    int32_t DrawingTexture::_get_height() const {
        return texture_size.y;
    }

    bool DrawingTexture::_has_alpha() const {
        return transparent;
    }

    RID DrawingTexture::_get_rid() const {
        return proxy;
    }
} // namespace godot
