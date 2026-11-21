#pragma once

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_int64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

#ifndef RESOURCE_CACHE_DIR_PREFIX
#define RESOURCE_CACHE_DIR_PREFIX "user://cache/"
#endif

namespace godot {
    /// The caches on disk as a whole - every ResourceCache keeps its files under root - and their
    /// size: the least recently used entries go first when the caches grow past the player's maximum
    /// size or would take the last of the disk's free space. An entry is what a cache keeps under one
    /// key: its file, the file's hash and the directory of files it writes beside it
    /// (ResourceCache::get_file_path()). An entry used in this session is never removed - what is
    /// streamed or loaded lazily reads its files later. Trims as it is made, before anything reads
    /// the caches - as GameDataServer clears them on a new build - after every entry written and when
    /// the player changes the maximum size; not in the editor, which shares the game's user directory
    /// and would remove what a running game uses.
    class ResourceCacheServer : public Object {
            GDCLASS(ResourceCacheServer, Object)

        public:
            /// The player's maximum size of the caches, in bytes
            static constexpr const char *MAX_SIZE_SETTING = "reloaded/cache/max_size";
            /// The maximum size that sets no maximum: only the disk's free space limits the caches
            static constexpr int64_t MAX_SIZE_UNLIMITED = 0;
            /// The file beside an entry's file holding the hash it was written for
            static constexpr const char *HASH_SUFFIX = ".hash";

        private:
            struct Entry {
                    int64_t size = 0;
                    /* Unix time of its last use [ms] */
                    int64_t last_used = 0;
                    /* Its files beside its directory, relative to root: its file and its hash */
                    PackedStringArray files;
            };

            static ResourceCacheServer *singleton;

            String root = RESOURCE_CACHE_DIR_PREFIX;
            /* Off in the editor: nothing is measured, trimmed or written to the index */
            bool trimming = false;
            /* The maximum size the caches were last trimmed to, to trim again when it changes */
            int64_t max_size = MAX_SIZE_UNLIMITED;
            HashMap<String, Entry> entries;
            /* The keys of the entries used in this session */
            HashSet<String> session_entries;
            int64_t used = 0;
            /* Entries, session entries and used: the caches are used from workers */
            Mutex mutex;

            static int64_t _get_space_left(const String &p_directory);
            static int64_t _get_max_size();
            static void _tree_measure(const String &p_directory, Entry &p_entry);
            static void _tree_remove(const String &p_directory);

            /* The entries under root, their last uses from the index */
            void _measure();
            void _scan(const String &p_relative, const Dictionary &p_index);
            /* With the mutex held */
            void _trim();
            void _index_save() const;
            void _on_settings_changed();

        protected:
            static void _bind_methods();

        public:
            /// Registers its project settings with their defaults and hints as the library starts
            /// (initialize_libreloaded_module(), ReloadedProjectSetting)
            static void project_settings_register();
            static ResourceCacheServer *get_instance();

            ResourceCacheServer();
            ~ResourceCacheServer() override;

            /// Where every cache keeps its files. Another root (the tests') is measured at once, as a
            /// new session: nothing in it counts as used yet
            void set_root(const String &p_root);
            String get_root() const;

            /// Removes the least recently used entries not used in this session until the caches fit
            /// what they may take (budget_get())
            void cache_trim();
            /// Every file under p_directory of root, and the whole root for an empty one
            void cache_clear(const String &p_directory);
            /// What the caches may take: the maximum size, or less when the disk's free space would
            /// fall under the reserve the caches never take; never less than nothing
            static int64_t budget_get(int64_t p_max_size, int64_t p_used, int64_t p_free_space);
            /// The maximum sizes the player chooses from, in bytes, smallest first, and
            /// MAX_SIZE_UNLIMITED last
            static PackedInt64Array max_size_get_choices();

            /// The key of the entry a file belongs to, both relative to root: the file's path without
            /// its hash suffix and its extension
            static String entry_get_key(const String &p_file);
            /// The entry of p_key is used now, and stays for this session
            void entry_mark_used(const String &p_key);
            /// The entry of p_file (relative to root) was written: measured again, kept for this
            /// session, and the caches trimmed
            void entry_written(const String &p_file);
    };
} // namespace godot
