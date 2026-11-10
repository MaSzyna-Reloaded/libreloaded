#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaScriptContext.hpp"
#include "scripting/ScenarioScriptServer.hpp"

namespace godot {
    static int time_create_timer(lua_State *p_state, const ScenarioScriptServer::Timer p_timer) {
        const double seconds = luaL_checknumber(p_state, 1);
        LuaScriptContext *context = LuaScriptContext::from_state(p_state);
        const int64_t function = context->keep_function(p_state, 2);
        ScenarioScriptServer *scripts = LuaModules::server<ScenarioScriptServer>(p_state);
        LuaHandle::push(
                p_state, scripts->script_clock_timer_create(context->get_rid(), seconds, function, p_timer),
                ScriptHandleKind::SUBSCRIPTION);
        return 1;
    }

    /// after(seconds, fn) - fn() once, after the seconds of the clock on the wall
    static int time_after(lua_State *p_state) {
        return time_create_timer(p_state, ScenarioScriptServer::TIMER_ONCE);
    }

    /// every(seconds, fn) - fn() every so many seconds of the clock on the wall
    static int time_every(lua_State *p_state) {
        return time_create_timer(p_state, ScenarioScriptServer::TIMER_REPEATING);
    }

    const luaL_Reg LuaModules::TIME[] = {
            {"after", time_after},
            {"every", time_every},
            {nullptr, nullptr},
    };
} // namespace godot
