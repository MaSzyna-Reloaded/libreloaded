#pragma once
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /// A texture showing a DrawingServer drawing, as a ViewportTexture shows a viewport. A material
    /// names it like any other texture, in a scene or in code; whoever owns the drawing attaches it
    /// (attach_drawing()). Its RID stays the same whether a drawing is attached or not, so what
    /// shows it follows without being told.
    class DrawingTexture : public Texture2D {
            GDCLASS(DrawingTexture, Texture2D)

        public:
            static constexpr int DEFAULT_SIDE = 256;

        private:
            Vector2i texture_size = Vector2i(DEFAULT_SIDE, DEFAULT_SIDE);
            bool transparent = false;
            bool flip_y = false;
            /// What it shows with no drawing attached
            RID placeholder;
            /// Its RID, proxying the placeholder or the drawing's texture
            RID proxy;

        protected:
            static void _bind_methods();

        public:
            DrawingTexture();
            ~DrawingTexture() override;

            void set_texture_size(const Vector2i &p_texture_size);
            Vector2i get_texture_size() const;
            void set_transparent(bool p_transparent);
            bool get_transparent() const;
            /// It holds its drawing upside down (DrawingServer::drawing_set_flip_y()) - for models
            /// whose texture coordinates count up from the bottom
            void set_flip_y(bool p_flip_y);
            bool get_flip_y() const;

            /// Shows the drawing's texture (DrawingServer::drawing_get_texture()); an invalid RID shows
            /// nothing again. Attached again after the drawing's material changed
            void attach_drawing(const RID &p_drawing);

            int32_t _get_width() const override;
            int32_t _get_height() const override;
            bool _has_alpha() const override;
            RID _get_rid() const override;
    };
} // namespace godot
