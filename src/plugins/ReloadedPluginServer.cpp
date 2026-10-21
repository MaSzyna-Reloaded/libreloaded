#include "plugins/ReloadedPluginServer.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>

namespace godot {
    ReloadedPluginServer *ReloadedPluginServer::singleton = nullptr;

    void ReloadedPluginServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("plugin_get_list"), &ReloadedPluginServer::plugin_get_list);
        ClassDB::bind_method(
                D_METHOD("data_path_resolve", "base_dir", "relative_path"), &ReloadedPluginServer::data_path_resolve);
    }

    ReloadedPluginServer *ReloadedPluginServer::get_instance() {
        return singleton;
    }

    ReloadedPluginServer::ReloadedPluginServer() {
        singleton = this;
        plugins = _create_plugins();
    }

    ReloadedPluginServer::~ReloadedPluginServer() {
        for (ReloadedPlugin *plugin: plugins) {
            memdelete(plugin);
        }
        plugins.clear();
        singleton = nullptr;
    }

    void ReloadedPluginServer::plugins_register_classes() {
        for (ReloadedPlugin *plugin: plugins) {
            plugin->register_classes();
        }
    }

    void ReloadedPluginServer::plugins_create_servers() {
        for (ReloadedPlugin *plugin: plugins) {
            plugin->create_servers();
        }
    }

    void ReloadedPluginServer::plugins_free_servers() {
        for (int64_t i = plugins.size() - 1; i >= 0; i--) {
            plugins[i]->free_servers();
        }
    }

    TypedArray<ReloadedPlugin> ReloadedPluginServer::plugin_get_list() const {
        TypedArray<ReloadedPlugin> list;
        for (ReloadedPlugin *plugin: plugins) {
            list.push_back(plugin);
        }
        return list;
    }

    String ReloadedPluginServer::data_path_resolve(const String &p_base_dir, const String &p_relative_path) const {
        if (!data_path_resolver.is_valid()) {
            return p_relative_path;
        }
        return data_path_resolver.call(p_base_dir, p_relative_path);
    }

    void ReloadedPluginServer::data_path_set_resolver(const Callable &p_resolver) {
        ERR_FAIL_COND_MSG(data_path_resolver.is_valid(), "A plugin already resolves the data's paths.");
        data_path_resolver = p_resolver;
    }
} // namespace godot
