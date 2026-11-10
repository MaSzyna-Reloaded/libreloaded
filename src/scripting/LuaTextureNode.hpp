#pragma once
#include "LuaNode.hpp"
#include "rendering/DrawingTexture.hpp"
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /// A Lua script that draws a texture: while the node is in the tree it owns a DrawingServer
    /// drawing, shown by its texture, and hands it to the script as the first argument of its file
    /// (`local drawing = ...`). A material in the scene names the same texture and shows what the
    /// script draws.
    ///
    /// With a material of its own the drawing is drawn through it into the texture - a canvas_item
    /// shader that gives the picture a look (a LED matrix, a screen's glow) at the texture's size,
    /// the drawing itself of drawing_size.
    class LuaTextureNode : public LuaNode {
            GDCLASS(LuaTextureNode, LuaNode)

        private:
            Ref<DrawingTexture> texture;
            Ref<Material> material;
            Vector2i drawing_size;
            RID drawing;

        protected:
            static void _bind_methods();

            void _script_starting(const RID &p_context) override;
            void _script_stopped() override;
            bool _is_running_in_editor() const override;
            Vector<ScriptArgument> _get_native_script_arguments() const override;

        public:
            void set_texture(const Ref<DrawingTexture> &p_texture);
            Ref<DrawingTexture> get_texture() const;
            void set_material(const Ref<Material> &p_material);
            Ref<Material> get_material() const;
            /// The size the script draws at when a material draws it into the texture; without a
            /// material it is the texture's
            void set_drawing_size(const Vector2i &p_drawing_size);
            Vector2i get_drawing_size() const;
    };
} // namespace godot
