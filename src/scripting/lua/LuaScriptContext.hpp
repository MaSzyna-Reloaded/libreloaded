#pragma once
#include "scripting/ScriptRuntime.hpp"
#include <godot_cpp/classes/random_number_generator.hpp>
#include <godot_cpp/variant/rid.hpp>

// Lua is compiled as C++ (CMakeLists.txt), so its headers are included without extern "C"
#include "lua.h"

namespace godot {
    /// One ScenarioScriptServer context's Lua state (the original's lua::lua, lua.cpp:9-44), with
    /// the `maszyna` modules and the original's `eu07.events` open in it.
    ///
    /// A script gets the base, string, table, math, coroutine and utf8 libraries, without what
    /// reaches past the simulation: no files but the modules of its own directory (require), no
    /// binary chunks, no io, os, debug or package. A call that runs too long or a state that grows
    /// too big fails with an error instead of stopping the game.
    class LuaScriptContext : public ScriptRuntime {
        public:
            /// How many instructions run between two checks of the limit below
            static constexpr int INSTRUCTION_CHECK_STEP = 1000;
            /// A call into the script that runs longer fails (runaway loop)
            static constexpr int64_t MAX_INSTRUCTIONS_PER_CALL = 50000000;
            static constexpr size_t MAX_MEMORY_BYTES = static_cast<size_t>(64) * 1024 * 1024;
            static constexpr const char *SCRIPT_EXTENSION = ".lua";

        private:
            lua_State *state = nullptr;
            RID context;
            String base_dir;
            /// Where data_file_path() finds the data files; the base directory until it is set
            String data_dir;
            /// The files _load_file() read, in the order it read them
            PackedStringArray files;
            size_t memory_used = 0;
            int64_t instructions = 0;
            /// What math.random draws from - the script's own, so math.randomseed seeds nothing else
            Ref<RandomNumberGenerator> random_generator;

            static void *_allocate(void *p_context, void *p_block, size_t p_old_size, size_t p_new_size);
            static void _count_instructions(lua_State *p_state, lua_Debug *p_debug);
            static int _open_libraries(lua_State *p_state);
            static int _traceback(lua_State *p_state);
            static int _print(lua_State *p_state);
            static int _load(lua_State *p_state);
            static int _collect_garbage(lua_State *p_state);
            static int _math_random(lua_State *p_state);
            static int _math_random_seed(lua_State *p_state);
            static int _require(lua_State *p_state);
            /// Pushes the file's chunk, or its error; true when the chunk was pushed
            bool _load_file(lua_State *p_state, const String &p_path);
            /// The path of a file of the directory, as the data names it; empty for a path that leads
            /// out of it (absolute, a drive, a way up)
            static String _path_inside(const String &p_dir, const String &p_path);
            /// Calls what lies under the arguments on the stack; returns its error
            String _protected_call(int p_arguments);

        public:
            LuaScriptContext(const RID &p_context, const String &p_base_dir);
            ~LuaScriptContext() override;

            /// The context whose state this is - for the modules, which get only the state
            static LuaScriptContext *from_state(lua_State *p_state);
            RID get_rid() const;
            /// Keeps the function at p_index for a later call(); returns its id
            int64_t keep_function(lua_State *p_state, int p_index);
            /// The path of a data file the script names (a font, an image); raises a script error for
            /// a path that leads out of the data directory
            String data_file_path(lua_State *p_state, const String &p_path) const;

            String run_file(const String &p_path, const Vector<ScriptArgument> &p_arguments) override;
            void set_data_dir(const String &p_data_dir) override;
            String run_source(
                    const String &p_source, const String &p_chunk_name,
                    const Vector<ScriptArgument> &p_arguments) override;
            PackedStringArray get_files() const override;
            String check_source(const String &p_source, const String &p_chunk_name) override;
            String call(int64_t p_function, const Vector<ScriptArgument> &p_arguments) override;
            void release(int64_t p_function) override;
    };
} // namespace godot
