#include "ResourceCacheServer.hpp"
#include "utils/ReloadedProjectSetting.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/json.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <utility>
#include <vector>

namespace godot {
    namespace {
        constexpr int64_t GIBIBYTE = 1024LL * 1024LL * 1024LL;
        /* The maximum sizes the player chooses from [GiB], smallest first */
        constexpr int64_t MAX_SIZE_CHOICES_GIB[] = {1, 2, 5, 10, 20, 50, 100};
        /* The default maximum size: a share of the disk's free space as the game starts, at most the
         * cap - one of the choices either way */
        constexpr int64_t DEFAULT_MAX_SIZE_CAP = 20 * GIBIBYTE;
        constexpr double DEFAULT_FREE_SPACE_SHARE = 0.25;
        /* The disk's free space the caches never take, whatever their maximum size */
        constexpr int64_t MIN_FREE_SPACE = 5 * GIBIBYTE;
        /* The entries' last uses in root, by key: {"<key>": unix time [ms]} */
        constexpr const char *INDEX_FILE = "last_used.json";
        constexpr int64_t MSEC_PER_SECOND = 1000;
        constexpr const char *SIZE_UNIT = " GB";
        constexpr const char *UNLIMITED_NAME = "No limit";
    } // namespace

    static int64_t now_msec() {
        return static_cast<int64_t>(Time::get_singleton()->get_unix_time_from_system() * MSEC_PER_SECOND);
    }

    ResourceCacheServer *ResourceCacheServer::singleton = nullptr;

    void ResourceCacheServer::project_settings_register() {
        const int64_t share = static_cast<int64_t>(
                static_cast<double>(_get_space_left(RESOURCE_CACHE_DIR_PREFIX)) * DEFAULT_FREE_SPACE_SHARE);
        const int64_t limit = std::min(DEFAULT_MAX_SIZE_CAP, share);
        const PackedInt64Array choices = max_size_get_choices();
        int64_t default_max_size = choices[0];
        String hint;
        for (const int64_t choice: choices) {
            if (choice != MAX_SIZE_UNLIMITED && choice <= limit) {
                default_max_size = choice;
            }
            const String name = choice == MAX_SIZE_UNLIMITED ? String(UNLIMITED_NAME)
                                                             : String::num_int64(choice / GIBIBYTE) + SIZE_UNIT;
            hint += (hint.is_empty() ? "" : ",") + name + ":" + String::num_int64(choice);
        }
        ReloadedProjectSetting::add(MAX_SIZE_SETTING, default_max_size, Variant::INT, PROPERTY_HINT_ENUM, hint);
    }

    ResourceCacheServer *ResourceCacheServer::get_instance() {
        return singleton;
    }

    void ResourceCacheServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_root", "root"), &ResourceCacheServer::set_root);
        ClassDB::bind_method(D_METHOD("get_root"), &ResourceCacheServer::get_root);
        ClassDB::bind_method(D_METHOD("cache_trim"), &ResourceCacheServer::cache_trim);
        ClassDB::bind_method(D_METHOD("cache_clear", "directory"), &ResourceCacheServer::cache_clear);
        ClassDB::bind_static_method(
                "ResourceCacheServer", D_METHOD("budget_get", "max_size", "used", "free_space"),
                &ResourceCacheServer::budget_get);
        ClassDB::bind_static_method(
                "ResourceCacheServer", D_METHOD("max_size_get_choices"), &ResourceCacheServer::max_size_get_choices);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "root"), "set_root", "get_root");
        BIND_CONSTANT(MAX_SIZE_UNLIMITED);
    }

    /* Measured and trimmed before anything reads the caches - nothing is in use yet - the way
     * GameDataServer clears them on a new build */
    ResourceCacheServer::ResourceCacheServer() : trimming(!Engine::get_singleton()->is_editor_hint()) {
        singleton = this;
        if (!trimming) {
            return;
        }
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &ResourceCacheServer::_on_settings_changed));
        _measure();
        MutexLock lock(mutex);
        _trim();
    }

    // the uses of this session, for the next one to know what was used last
    ResourceCacheServer::~ResourceCacheServer() {
        if (trimming) {
            _index_save();
        }
        singleton = nullptr;
    }

    void ResourceCacheServer::set_root(const String &p_root) {
        if (trimming) {
            MutexLock lock(mutex);
            _index_save();
        }
        root = p_root;
        if (trimming) {
            _measure();
        }
    }

    String ResourceCacheServer::get_root() const {
        return root;
    }

    int64_t ResourceCacheServer::_get_space_left(const String &p_directory) {
        DirAccess::make_dir_recursive_absolute(p_directory);
        const Ref<DirAccess> directory = DirAccess::open(p_directory);
        ERR_FAIL_COND_V(directory.is_null(), 0);
        return static_cast<int64_t>(directory->get_space_left());
    }

    int64_t ResourceCacheServer::_get_max_size() {
        return ProjectSettings::get_singleton()->get_setting(MAX_SIZE_SETTING);
    }

    int64_t
    ResourceCacheServer::budget_get(const int64_t p_max_size, const int64_t p_used, const int64_t p_free_space) {
        const int64_t disk_budget = std::max<int64_t>(p_used + p_free_space - MIN_FREE_SPACE, 0);
        return p_max_size == MAX_SIZE_UNLIMITED ? disk_budget : std::min(p_max_size, disk_budget);
    }

    PackedInt64Array ResourceCacheServer::max_size_get_choices() {
        PackedInt64Array choices;
        for (const int64_t choice: MAX_SIZE_CHOICES_GIB) {
            choices.push_back(choice * GIBIBYTE);
        }
        choices.push_back(MAX_SIZE_UNLIMITED);
        return choices;
    }

    String ResourceCacheServer::entry_get_key(const String &p_file) {
        return p_file.trim_suffix(HASH_SUFFIX).get_basename();
    }

    void ResourceCacheServer::_tree_measure(const String &p_directory, Entry &p_entry) {
        for (const String &file: DirAccess::get_files_at(p_directory)) {
            const String path = p_directory.path_join(file);
            p_entry.size += FileAccess::get_size(path);
            p_entry.last_used = std::max(
                    p_entry.last_used, static_cast<int64_t>(FileAccess::get_modified_time(path)) * MSEC_PER_SECOND);
        }
        for (const String &directory: DirAccess::get_directories_at(p_directory)) {
            _tree_measure(p_directory.path_join(directory), p_entry);
        }
    }

    void ResourceCacheServer::_tree_remove(const String &p_directory) {
        if (!DirAccess::dir_exists_absolute(p_directory)) {
            return;
        }
        UtilityFunctions::print_verbose("[ResourceCacheServer] Clearing cache for " + p_directory + "...");
        for (const String &file: DirAccess::get_files_at(p_directory)) {
            DirAccess::remove_absolute(p_directory.path_join(file));
        }
        for (const String &directory: DirAccess::get_directories_at(p_directory)) {
            _tree_remove(p_directory.path_join(directory));
        }
        DirAccess::remove_absolute(p_directory);
    }

    void ResourceCacheServer::_measure() {
        MutexLock lock(mutex);
        entries.clear();
        session_entries.clear();
        used = 0;
        if (!DirAccess::dir_exists_absolute(root)) {
            return;
        }
        const String index_path = root.path_join(INDEX_FILE);
        const Variant parsed = FileAccess::file_exists(index_path)
                                       ? JSON::parse_string(FileAccess::get_file_as_string(index_path))
                                       : Variant();
        const Dictionary index = parsed.get_type() == Variant::DICTIONARY ? Dictionary(parsed) : Dictionary();
        _scan("", index);
        // an entry the index does not know was last used when it was last written
        for (KeyValue<String, Entry> &entry: entries) {
            if (index.has(entry.key)) {
                entry.value.last_used = index[entry.key];
            }
            used += entry.value.size;
        }
    }

    /* A file is its entry's; a directory is the entry's whose key it has - its file's stem, or a key
     * the index knows (files written beside an entry that has no file) - or else a directory of
     * entries */
    void ResourceCacheServer::_scan(const String &p_relative, const Dictionary &p_index) {
        const String directory = root.path_join(p_relative);
        HashSet<String> keys;
        for (const String &file: DirAccess::get_files_at(directory)) {
            const String relative = p_relative.path_join(file);
            if (relative == INDEX_FILE) {
                continue;
            }
            const String key = entry_get_key(relative);
            const String path = directory.path_join(file);
            Entry &entry = entries[key];
            entry.files.push_back(relative);
            entry.size += FileAccess::get_size(path);
            entry.last_used = std::max(
                    entry.last_used, static_cast<int64_t>(FileAccess::get_modified_time(path)) * MSEC_PER_SECOND);
            keys.insert(key);
        }
        for (const String &name: DirAccess::get_directories_at(directory)) {
            const String relative = p_relative.path_join(name);
            if (keys.has(relative) || p_index.has(relative)) {
                _tree_measure(directory.path_join(name), entries[relative]);
            } else {
                _scan(relative, p_index);
            }
        }
    }

    void ResourceCacheServer::cache_trim() {
        if (!trimming) {
            return;
        }
        MutexLock lock(mutex);
        _trim();
    }

    void ResourceCacheServer::_trim() {
        max_size = _get_max_size();
        const int64_t budget = budget_get(max_size, used, _get_space_left(root));
        if (used <= budget) {
            return;
        }
        std::vector<std::pair<int64_t, String>> unused;
        for (const KeyValue<String, Entry> &entry: entries) {
            if (!session_entries.has(entry.key)) {
                unused.emplace_back(entry.value.last_used, entry.key);
            }
        }
        std::sort(unused.begin(), unused.end(), [](const auto &p_a, const auto &p_b) { return p_a.first < p_b.first; });
        for (const std::pair<int64_t, String> &candidate: unused) {
            if (used <= budget) {
                break;
            }
            const Entry &entry = entries[candidate.second];
            for (const String &file: entry.files) {
                DirAccess::remove_absolute(root.path_join(file));
            }
            _tree_remove(root.path_join(candidate.second));
            used -= entry.size;
            entries.erase(candidate.second);
        }
        UtilityFunctions::print_verbose(
                "[ResourceCacheServer] Trimmed to " + String::num_int64(used) + " of " + String::num_int64(budget) +
                " bytes");
        _index_save();
    }

    void ResourceCacheServer::cache_clear(const String &p_directory) {
        MutexLock lock(mutex);
        _tree_remove(root.path_join(p_directory));
        const String prefix = p_directory.is_empty() ? String() : p_directory + String("/");
        Vector<String> cleared;
        for (const KeyValue<String, Entry> &entry: entries) {
            if (entry.key.begins_with(prefix)) {
                cleared.push_back(entry.key);
                used -= entry.value.size;
            }
        }
        for (const String &key: cleared) {
            entries.erase(key);
        }
    }

    void ResourceCacheServer::entry_mark_used(const String &p_key) {
        if (!trimming) {
            return;
        }
        MutexLock lock(mutex);
        session_entries.insert(p_key);
        if (Entry *entry = entries.getptr(p_key); entry != nullptr) {
            entry->last_used = now_msec();
        }
    }

    void ResourceCacheServer::entry_written(const String &p_file) {
        if (!trimming) {
            return;
        }
        const String key = entry_get_key(p_file);
        Entry written;
        for (const String &file: {p_file, p_file + String(HASH_SUFFIX)}) {
            if (FileAccess::file_exists(root.path_join(file))) {
                written.files.push_back(file);
                written.size += FileAccess::get_size(root.path_join(file));
            }
        }
        if (DirAccess::dir_exists_absolute(root.path_join(key))) {
            _tree_measure(root.path_join(key), written);
        }
        written.last_used = now_msec();
        MutexLock lock(mutex);
        if (const Entry *previous = entries.getptr(key); previous != nullptr) {
            used -= previous->size;
        }
        used += written.size;
        entries[key] = std::move(written);
        session_entries.insert(key);
        _trim();
    }

    // none for no entries: a root cleared away is not made again for an empty index
    void ResourceCacheServer::_index_save() const {
        const String path = root.path_join(INDEX_FILE);
        if (entries.is_empty()) {
            if (FileAccess::file_exists(path)) {
                DirAccess::remove_absolute(path);
            }
            return;
        }
        Dictionary index;
        for (const KeyValue<String, Entry> &entry: entries) {
            index[entry.key] = entry.value.last_used;
        }
        DirAccess::make_dir_recursive_absolute(root);
        const Ref<FileAccess> file = FileAccess::open(path, FileAccess::WRITE);
        ERR_FAIL_COND_MSG(file.is_null(), "[ResourceCacheServer] Failed to save the cache index in " + root);
        file->store_string(JSON::stringify(index));
    }

    // the player's maximum size is in effect at once: a smaller one trims the caches now
    void ResourceCacheServer::_on_settings_changed() {
        MutexLock lock(mutex);
        if (_get_max_size() != max_size) {
            _trim();
        }
    }
} // namespace godot
