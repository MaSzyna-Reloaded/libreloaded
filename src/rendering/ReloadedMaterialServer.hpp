#pragma once
#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /* The materials the world names - a track's rails and bed, a mesh's surfaces - by the name the
     * data gives them. The core knows no material data: a plugin provides them (material_set_provider())
     * and without one nothing is found, so whatever asks draws with its own default. */
    class ReloadedMaterialServer : public Object {
            GDCLASS(ReloadedMaterialServer, Object)

        public:
            /* An option of material_get(): the material draws both faces (Dictionary key, bool) */
            static constexpr const char *OPTION_CULL_DISABLED = "cull_disabled";

        private:
            static ReloadedMaterialServer *singleton;
            Callable material_provider;
            Callable texture_size_provider;
            Callable texture_length_provider;

        protected:
            static void _bind_methods();

        public:
            static ReloadedMaterialServer *get_instance();

            ReloadedMaterialServer();
            ~ReloadedMaterialServer() override;

            /* What finds the materials: p_material(name, options) -> Material, p_texture_size(name) ->
             * Vector2i, p_texture_length(name) -> float */
            void material_set_provider(
                    const Callable &p_material, const Callable &p_texture_size, const Callable &p_texture_length);
            /* The material of the name, null when there is none or no provider */
            Ref<Material> material_get(const String &p_name, const Dictionary &p_options = Dictionary()) const;
            /* The size of the material's texture in texels, zero when unknown */
            Vector2i material_get_texture_size(const String &p_name) const;
            /* The length the material's texture covers along a track [m], -1 when the material declares
             * none */
            double material_get_texture_length(const String &p_name) const;
    };
} // namespace godot
