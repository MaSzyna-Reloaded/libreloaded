#pragma once
#include "utils/RIDHash.hpp"
#include <godot_cpp/classes/font_file.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/text_server.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/color.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/rect2.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /// Pictures drawn on the GPU, by RID - a cab's screen, a station's board - and what they are drawn
    /// with: fonts and images read from files.
    ///
    /// A drawing is a RenderingServer viewport with a 2D canvas. Every operation adds a command to
    /// the canvas; the viewport is drawn again once, in the next frame, after anything was added
    /// (UPDATE_ONCE), so nothing runs while nothing changes and nothing is copied through the CPU.
    /// Its owner may pass the picture through a material of its own (drawing_set_material()) - a
    /// second viewport draws the picture through it, at the size the owner asks for.
    ///
    /// The commands are grouped in layers, child canvas items drawn in their order: a layer is begun
    /// when a paste is sampled with another filter than the layer before it.
    class DrawingServer : public Object {
            GDCLASS(DrawingServer, Object)

        public:
            /// How a pasted image is sampled when it is scaled
            enum Filter {
                FILTER_NEAREST,
                /// Smooth, with mipmaps when it is made smaller
                FILTER_LINEAR,
            };

            /// How many segments an ellipse's or a pie slice's full turn is drawn with
            static constexpr int ARC_SEGMENTS_PER_TURN = 64;
            static constexpr double FULL_TURN_DEGREES = 360.0;
            /// A line, an outline and a pixel are drawn through the middle of the pixels they name
            static constexpr float PIXEL_CENTRE = 0.5;

            static DrawingServer *get_instance() {
                return singleton;
            }
            ~DrawingServer() override;

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline DrawingServer *singleton = nullptr;

            /// A viewport drawing one canvas
            struct Canvas {
                    RID viewport;
                    RID canvas;
                    /// The canvas item everything is drawn into, or the layers are children of
                    RID root;
            };

            struct Drawing {
                    Vector2i size;
                    bool transparent = false;
                    Canvas picture;
                    LocalVector<RID> layers;
                    Filter layer_filter = FILTER_NEAREST;
                    /// The picture drawn through the owner's material; no viewport without one
                    Canvas output;
                    Vector2i output_size;
                    /// Its texture holds the picture upside down (drawing_set_flip_y())
                    bool flip_y = false;
            };

            struct LoadedImage {
                    Ref<Image> image;
                    RID texture;
            };

            RIDMap<Drawing> drawings;
            RIDMap<Ref<FontFile>> fonts;
            RIDMap<LoadedImage> images;

            static Canvas _canvas_create(const Vector2i &p_size, bool p_transparent);
            static void _canvas_free(const Canvas &p_canvas);
            /// The layer the next command goes to; a paste asks for its filter, the other commands
            /// draw in any
            RID _layer(Drawing &p_drawing);
            RID _layer(Drawing &p_drawing, Filter p_filter);
            /// Drawn again in the next frame, through its material too
            static void _changed(const Drawing &p_drawing);
            /// The last canvas of the drawing - its output, else its picture - upside down or not
            static void _apply_flip_y(const Drawing &p_drawing);
            /// The texture of an image or of a drawing
            RID _source_texture(const RID &p_source) const;
            /// The points of the arc of the ellipse the rectangle bounds, from p_start to p_end
            /// [deg], clockwise from three o'clock as the y axis points down
            static PackedVector2Array _arc(const Rect2 &p_rect, double p_start, double p_end);
            /// The image made a texture, under a new RID
            RID _image_create(const Ref<Image> &p_image);

        protected:
            static void _bind_methods();

        public:
            DrawingServer();

            /// A drawing of the size, transparent or opaque black to begin with
            RID drawing_create(const Vector2i &p_size, bool p_transparent);
            Vector2i drawing_get_size(const RID &p_drawing) const;
            /// The picture passes through the material (a canvas_item shader) into a texture of
            /// p_output_size; an invalid material takes the pass away. drawing_get_texture() is
            /// another RID after it
            void drawing_set_material(const RID &p_drawing, const RID &p_material, const Vector2i &p_output_size);
            /// The texture holds the picture upside down - its first row is the picture's bottom, as
            /// models whose texture coordinates count up from the bottom (OpenGL's) take it; the
            /// drawing is drawn as ever, from the top
            void drawing_set_flip_y(const RID &p_drawing, bool p_flip_y);
            /// The RenderingServer texture the drawing ends in - its picture, or the picture through
            /// its material; the same RID until drawing_set_material()
            RID drawing_get_texture(const RID &p_drawing) const;
            /// Everything drawn on it goes; it is filled with the colour (opaque in an opaque drawing)
            void drawing_clear(const RID &p_drawing, const Color &p_color);
            void drawing_add_rect(const RID &p_drawing, const Rect2 &p_rect, const Color &p_color);
            /// The rectangle's edge, inside it
            void
            drawing_add_rect_outline(const RID &p_drawing, const Rect2 &p_rect, const Color &p_color, float p_width);
            /// A line through the pixels the points name, of the width [px]
            void drawing_add_line(
                    const RID &p_drawing, const PackedVector2Array &p_points, const Color &p_color, float p_width);
            /// The ellipse the rectangle bounds
            void drawing_add_ellipse(const RID &p_drawing, const Rect2 &p_rect, const Color &p_color);
            void
            drawing_add_ellipse_outline(const RID &p_drawing, const Rect2 &p_rect, const Color &p_color, float p_width);
            void drawing_add_polygon(const RID &p_drawing, const PackedVector2Array &p_points, const Color &p_color);
            void drawing_add_polygon_outline(
                    const RID &p_drawing, const PackedVector2Array &p_points, const Color &p_color, float p_width);
            /// The slice of the ellipse the rectangle bounds, from p_start to p_end [deg], clockwise
            /// from three o'clock
            void drawing_add_pieslice(
                    const RID &p_drawing, const Rect2 &p_rect, double p_start, double p_end, const Color &p_color);
            void drawing_add_pixel(const RID &p_drawing, const Vector2i &p_position, const Color &p_color);
            /// The text in the font of the size [px], its top left corner at the position
            void drawing_add_text(
                    const RID &p_drawing, const Vector2 &p_position, const String &p_text, const RID &p_font,
                    int p_size, const Color &p_color);
            /// The part of an image or of another drawing, scaled into the rectangle, tinted with the
            /// colour and mixed by its alpha; a negative size of the rectangle flips it where it stands
            void drawing_add_texture(
                    const RID &p_drawing, const RID &p_source, const Rect2 &p_source_rect, const Rect2 &p_rect,
                    const Color &p_modulate, Filter p_filter);
            /// An image or another drawing turned by the angle [deg, counter-clockwise] about its
            /// middle, its middle at p_center
            void drawing_add_texture_rotated(
                    const RID &p_drawing, const RID &p_source, const Vector2 &p_center, double p_angle,
                    Filter p_filter);

            /// A TrueType or OpenType font from the file, its edges smoothed as p_antialiasing says - none
            /// for a picture of whole pixels (a LED board); an invalid RID when it cannot be read
            RID font_load(
                    const String &p_path,
                    TextServer::FontAntialiasing p_antialiasing = TextServer::FONT_ANTIALIASING_GRAY);
            /// The size [px] the text takes in the font of the size
            Vector2 font_get_text_size(const RID &p_font, const String &p_text, int p_size) const;

            /// An image from the file (png, bmp, tga, dds, ...); an invalid RID when it cannot be read
            RID image_load(const String &p_path);
            /// An image from the file as a mask: white, as opaque as the file's alpha, or as its
            /// brightness when it has none - pasted, it takes the colour the paste is tinted with
            RID image_load_mask(const String &p_path);
            Vector2i image_get_size(const RID &p_image) const;
            Color image_get_pixel(const RID &p_image, const Vector2i &p_position) const;

            /// Frees a drawing, a font or an image
            void free_rid(const RID &p_rid);
            /// Whether the RID is a drawing, a font or an image of the server
            bool owns(const RID &p_rid) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(DrawingServer::Filter);
