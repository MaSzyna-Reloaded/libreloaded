#include "LuaNode.hpp"
#include "ScenarioScriptServer.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
    void LuaNode::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_script_path", "script_path"), &LuaNode::set_script_path);
        ClassDB::bind_method(D_METHOD("get_script_path"), &LuaNode::get_script_path);
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "script_path", PROPERTY_HINT_FILE, "*.lua"), "set_script_path",
                "get_script_path");
        ClassDB::bind_method(D_METHOD("set_source", "source"), &LuaNode::set_source);
        ClassDB::bind_method(D_METHOD("get_source"), &LuaNode::get_source);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "source", PROPERTY_HINT_MULTILINE_TEXT), "set_source", "get_source");
        ClassDB::bind_method(D_METHOD("set_script_variables", "script_variables"), &LuaNode::set_script_variables);
        ClassDB::bind_method(D_METHOD("get_script_variables"), &LuaNode::get_script_variables);
        // saved, shown by the properties of the variables (_get_property_list())
        ADD_PROPERTY(
                PropertyInfo(Variant::DICTIONARY, "script_variables", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_STORAGE),
                "set_script_variables", "get_script_variables");
        ClassDB::bind_method(D_METHOD("set_data_dir", "data_dir"), &LuaNode::set_data_dir);
        ClassDB::bind_method(D_METHOD("get_data_dir"), &LuaNode::get_data_dir);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "data_dir", PROPERTY_HINT_DIR), "set_data_dir", "get_data_dir");
        GDVIRTUAL_BIND(_get_script_arguments);
    }

    /// The unit the node's own code runs as
    static constexpr const char *SOURCE_UNIT = "source";
    /// The properties of the script's variables: script_variables/<name>
    static constexpr const char *VARIABLES_PREFIX = "script_variables/";

    /// The variable a property stands for; empty for another property
    static String variable_of(const StringName &p_property) {
        const String property = p_property;
        return property.begins_with(VARIABLES_PREFIX) ? property.trim_prefix(VARIABLES_PREFIX) : String();
    }

    void LuaNode::_notification(const int p_what) {
        const bool tree_changed = p_what == NOTIFICATION_ENTER_TREE || p_what == NOTIFICATION_EXIT_TREE;
        if (!tree_changed || (Engine::get_singleton()->is_editor_hint() && !_is_running_in_editor())) {
            return;
        }
        if (p_what == NOTIFICATION_ENTER_TREE) {
            _start();
            return;
        }
        _stop();
    }

    /// The node's own code has the directory of the scene it is saved in
    void LuaNode::_start() {
        ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
        ERR_FAIL_NULL(scripts);
        ERR_FAIL_COND_MSG(
                script_path.is_empty() == source.is_empty(),
                "LuaNode " + get_name() + " needs a script file or code of its own");
        const Node *scene = get_owner() == nullptr ? this : get_owner();
        const String base_dir =
                source.is_empty() ? script_path.get_base_dir() : scene->get_scene_file_path().get_base_dir();
        context = scripts->context_create(base_dir);
        scripts->connect(
                ScenarioScriptServer::context_reloaded_signal, callable_mp(this, &LuaNode::_on_context_reloaded));
        scripts->context_set_data_dir(context, data_dir.is_empty() ? base_dir : data_dir);
        scripts->context_set_variables(context, script_variables);
        _script_starting(context);
        Vector<ScriptArgument> arguments = _get_native_script_arguments();
        Array added;
        if (GDVIRTUAL_CALL(_get_script_arguments, added)) {
            for (int64_t i = 0; i < added.size(); i++) {
                arguments.push_back(ScriptArgument{added[i]});
            }
        }
        if (source.is_empty()) {
            scripts->context_run_file(context, script_path.get_file(), arguments);
        } else {
            scripts->context_apply_source(context, SOURCE_UNIT, source, arguments);
        }
        _show_declared_variables();
    }

    /// Told at every run, the inspector would rebuild itself while a value is being picked in it
    void LuaNode::_show_declared_variables() {
        ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
        ERR_FAIL_NULL(scripts);
        const Dictionary exports = scripts->context_get_exports(context);
        Dictionary declared;
        const Array names = exports.keys();
        for (int64_t i = 0; i < names.size(); i++) {
            declared[names[i]] = exports[names[i]].get_type();
        }
        if (!(declared == declared_variables)) {
            declared_variables = declared;
            notify_property_list_changed();
        }
    }

    void LuaNode::_stop() {
        if (!context.is_valid()) {
            return;
        }
        ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
        ERR_FAIL_NULL(scripts);
        scripts->disconnect(
                ScenarioScriptServer::context_reloaded_signal, callable_mp(this, &LuaNode::_on_context_reloaded));
        scripts->context_free(context);
        context = RID();
        _script_stopped();
    }

    void LuaNode::_on_context_reloaded(const RID &p_context) {
        if (p_context == context) {
            _show_declared_variables();
        }
    }

    void LuaNode::_restart() {
        if (context.is_valid()) {
            _stop();
            _start();
        }
    }

    /// A value set to the script's own default is kept as well: the script's default may change
    bool LuaNode::_set(const StringName &p_name, const Variant &p_value) {
        const String variable = variable_of(p_name);
        if (variable.is_empty()) {
            return false;
        }
        script_variables[variable] = p_value;
        // the script runs again with it, in its context - what the node handed it stays
        if (context.is_valid()) {
            ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
            ERR_FAIL_NULL_V(scripts, true);
            scripts->context_set_variables(context, script_variables);
            scripts->context_reload(context);
        }
        return true;
    }

    bool LuaNode::_get(const StringName &p_name, Variant &p_value) const {
        const String variable = variable_of(p_name);
        if (variable.is_empty()) {
            return false;
        }
        ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
        const Dictionary exports =
                context.is_valid() && scripts != nullptr ? scripts->context_get_exports(context) : Dictionary();
        p_value = script_variables.get(variable, exports.get(variable, Variant()));
        return true;
    }

    /// The variables the running script declared; while it does not run, the ones set on the node
    void LuaNode::_get_property_list(List<PropertyInfo> *p_list) const {
        ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
        const Dictionary declared =
                context.is_valid() && scripts != nullptr ? scripts->context_get_exports(context) : script_variables;
        const Array names = declared.keys();
        for (int64_t i = 0; i < names.size(); i++) {
            const String name = names[i];
            const Variant value = script_variables.get(name, declared[name]);
            // saved in script_variables, not each on its own
            p_list->push_back(PropertyInfo(
                    value.get_type(), VARIABLES_PREFIX + name, PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR));
        }
    }

    bool LuaNode::_property_can_revert(const StringName &p_name) const {
        return script_variables.has(variable_of(p_name));
    }

    bool LuaNode::_property_get_revert(const StringName &p_name, Variant &p_value) const {
        ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
        if (!context.is_valid() || scripts == nullptr) {
            return false;
        }
        const Dictionary exports = scripts->context_get_exports(context);
        const String variable = variable_of(p_name);
        if (!exports.has(variable)) {
            return false;
        }
        p_value = exports[variable];
        return true;
    }

    void LuaNode::_script_starting(const RID &p_context) {}

    void LuaNode::_script_stopped() {}

    bool LuaNode::_is_running_in_editor() const {
        return false;
    }

    Vector<ScriptArgument> LuaNode::_get_native_script_arguments() const {
        return {};
    }

    void LuaNode::set_script_path(const String &p_script_path) {
        script_path = p_script_path;
        _restart();
    }

    String LuaNode::get_script_path() const {
        return script_path;
    }

    void LuaNode::set_source(const String &p_source) {
        source = p_source;
        _restart();
    }

    String LuaNode::get_source() const {
        return source;
    }

    void LuaNode::set_script_variables(const Dictionary &p_script_variables) {
        script_variables = p_script_variables;
        _restart();
    }

    Dictionary LuaNode::get_script_variables() const {
        return script_variables;
    }

    void LuaNode::set_data_dir(const String &p_data_dir) {
        data_dir = p_data_dir;
        _restart();
    }

    String LuaNode::get_data_dir() const {
        return data_dir;
    }
} // namespace godot
