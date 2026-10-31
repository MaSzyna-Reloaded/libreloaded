#pragma once
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/variant/string.hpp>

#include "utils/UserSettingsProvider.hpp"

namespace godot {
    /* A plugin of Rails Reloaded, compiled into this library (CMake LIBRELOADED_PLUGINS): what it is
     * for the player - its name, description and logo - and what it adds to the core at two points
     * of the library's start. Its classes come once every core class is registered, since they
     * derive from them; its servers once the vehicle servers exist and before the servers that
     * follow model instances. They go in reverse at the same point of the library's end.
     * ReloadedPluginServer creates and holds the plugins. Its part of the game - the providers it
     * gives the core - is a ReloadedRuntimePlugin of its addon (runtime_script), which the core's
     * ReloadedPlugins puts in the tree before the main scene, as the editor holds EditorPlugins. */
    class ReloadedPlugin : public Object {
            GDCLASS(ReloadedPlugin, Object)

        protected:
            /* Set by the plugin when it is created, the logo once its servers are - a texture is
             * loaded from the plugin's addon, which needs the resource loader */
            String name;
            String description;
            Ref<Texture2D> logo;
            /* Set by the plugin when it is created: the path of its ReloadedRuntimePlugin script;
             * empty for none */
            String runtime_script;

            static void _bind_methods();

        public:
            /* Registers the plugin's classes */
            virtual void register_classes() {}
            /* Creates the plugin's servers and hands them to the core */
            virtual void create_servers() {}
            /* Frees what create_servers() made, in reverse */
            virtual void free_servers() {}
            /* The plugin's settings, added to the game's (UserSettings) as it starts, after the
             * plugins' classes and before their servers; none - null */
            virtual Ref<UserSettingsProvider> get_user_settings_provider() const {
                return {};
            }

            String get_name() const;
            String get_description() const;
            Ref<Texture2D> get_logo() const;
            String get_runtime_script() const;
    };
} // namespace godot
