#pragma once
#include "ScriptRuntime.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/templates/list.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /// A Lua script that runs while the node is in the tree: a ScenarioScriptServer context of its
    /// own, made as the node enters the tree and freed as it leaves it, with everything the script
    /// made. The script is a file (script_path) or the node's own code (source), never both; the
    /// file's directory - for its own code the scene's - is the context's base directory (require),
    /// its data files come from data_dir. A derived node hands the script what it works on, as the
    /// arguments of its chunk (`local drawing = ...`). A changed script runs again at once.
    ///
    /// The variables its script declares (reloaded.export_var()) are its properties
    /// script_variables/<name>, of their defaults' types, set in the inspector and saved with it.
    ///
    /// Its script runs in the game; a node that draws (LuaTextureNode) runs it in the editor too, so
    /// the picture is seen where the scene is made.
    ///
    /// Notifications, not the _enter_tree()/_exit_tree() virtuals: a GDScript subclass that defines
    /// them would replace the native ones (Cabin3D says the same).
    class LuaNode : public Node {
            GDCLASS(LuaNode, Node)

        private:
            String script_path;
            String source;
            String data_dir;
            /// What the variables its script declares are set to, by their names
            Dictionary script_variables;
            /// The types of the variables its script declared when it last ran, by their names - the
            /// inspector is told of them only when they change, not at every run
            Dictionary declared_variables;
            RID context;

            /// Its script runs, in a context of its own, with what the derived node hands it
            void _start();
            /// ...and goes with everything it made
            void _stop();
            /// A script already running runs again, as it is now set
            void _restart();
            /// Its context ran its code again: the variables its script declares may be others
            void _on_context_reloaded(const RID &p_context);
            /// The inspector shows the variables the script declared, when they are others than before
            void _show_declared_variables();

        protected:
            static void _bind_methods();

            /// The properties of the script's variables (script_variables/<name>)
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            bool _set(const StringName &p_name, const Variant &p_value);
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            bool _get(const StringName &p_name, Variant &p_value) const;
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _get_property_list(List<PropertyInfo> *p_list) const;
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            bool _property_can_revert(const StringName &p_name) const;
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            bool _property_get_revert(const StringName &p_name, Variant &p_value) const;

            /// What a GDScript subclass adds to the script's arguments, after the native ones
            GDVIRTUAL0RC(Array, _get_script_arguments)

            /// A derived node makes what it hands the script, with the context just made
            virtual void _script_starting(const RID &p_context);
            /// ...and lets it go once the context is freed
            virtual void _script_stopped();
            /// Whether the script runs in the editor as well
            virtual bool _is_running_in_editor() const;
            /// The arguments of the script's file, each RID as a handle of its kind
            virtual Vector<ScriptArgument> _get_native_script_arguments() const;

        public:
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _notification(int p_what);

            /// The script's file: res://, user:// or a path of the file system
            void set_script_path(const String &p_script_path);
            String get_script_path() const;
            /// The script's own code, kept in the scene, instead of a file
            void set_source(const String &p_source);
            String get_source() const;
            /// What the variables its script declares are set to, by their names
            void set_script_variables(const Dictionary &p_script_variables);
            Dictionary get_script_variables() const;
            /// Where the script's fonts and images are read from; empty for the script's directory
            void set_data_dir(const String &p_data_dir);
            String get_data_dir() const;
    };
} // namespace godot
