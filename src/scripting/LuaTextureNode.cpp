#include "LuaTextureNode.hpp"
#include "rendering/DrawingServer.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void LuaTextureNode::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_texture", "texture"), &LuaTextureNode::set_texture);
        ClassDB::bind_method(D_METHOD("get_texture"), &LuaTextureNode::get_texture);
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "texture", PROPERTY_HINT_RESOURCE_TYPE, "DrawingTexture"), "set_texture",
                "get_texture");
        ClassDB::bind_method(D_METHOD("set_material", "material"), &LuaTextureNode::set_material);
        ClassDB::bind_method(D_METHOD("get_material"), &LuaTextureNode::get_material);
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "material", PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial"),
                "set_material", "get_material");
        ClassDB::bind_method(D_METHOD("set_drawing_size", "drawing_size"), &LuaTextureNode::set_drawing_size);
        ClassDB::bind_method(D_METHOD("get_drawing_size"), &LuaTextureNode::get_drawing_size);
        ADD_PROPERTY(
                PropertyInfo(Variant::VECTOR2I, "drawing_size", PROPERTY_HINT_NONE, "suffix:px"), "set_drawing_size",
                "get_drawing_size");
    }

    void LuaTextureNode::_script_starting(const RID &p_context) {
        LuaNode::_script_starting(p_context);
        ERR_FAIL_COND_MSG(texture.is_null(), "LuaTextureNode " + get_name() + " has no texture");
        DrawingServer *drawings = DrawingServer::get_instance();
        ERR_FAIL_NULL(drawings);
        const bool through_material = material.is_valid();
        drawing = drawings->drawing_create(
                through_material ? drawing_size : texture->get_texture_size(), texture->get_transparent());
        if (through_material) {
            drawings->drawing_set_material(drawing, material->get_rid(), texture->get_texture_size());
        }
        drawings->drawing_set_flip_y(drawing, texture->get_flip_y());
        texture->attach_drawing(drawing);
    }

    void LuaTextureNode::_script_stopped() {
        if (drawing.is_valid()) {
            texture->attach_drawing(RID());
            DrawingServer *drawings = DrawingServer::get_instance();
            ERR_FAIL_NULL(drawings);
            drawings->free_rid(drawing);
            drawing = RID();
        }
        LuaNode::_script_stopped();
    }

    /// What it draws is seen in the editor as in the game
    bool LuaTextureNode::_is_running_in_editor() const {
        return true;
    }

    Vector<ScriptArgument> LuaTextureNode::_get_native_script_arguments() const {
        Vector<ScriptArgument> arguments = LuaNode::_get_native_script_arguments();
        arguments.push_back(ScriptArgument{drawing, ScriptHandleKind::DRAWING});
        return arguments;
    }

    void LuaTextureNode::set_texture(const Ref<DrawingTexture> &p_texture) {
        texture = p_texture;
    }

    Ref<DrawingTexture> LuaTextureNode::get_texture() const {
        return texture;
    }

    void LuaTextureNode::set_material(const Ref<Material> &p_material) {
        material = p_material;
    }

    Ref<Material> LuaTextureNode::get_material() const {
        return material;
    }

    void LuaTextureNode::set_drawing_size(const Vector2i &p_drawing_size) {
        drawing_size = p_drawing_size;
    }

    Vector2i LuaTextureNode::get_drawing_size() const {
        return drawing_size;
    }
} // namespace godot
