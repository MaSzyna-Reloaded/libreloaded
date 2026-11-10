#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "LuaScriptContext.hpp"
#include "LuaVariant.hpp"
#include "scripting/ScenarioScriptServer.hpp"

namespace godot {
    /// reloaded.cancel(subscription) - stops a timer or a subscription of this script
    static int cancel(lua_State *p_state) {
        const RID subscription = LuaHandle::check(p_state, 1, ScriptHandleKind::SUBSCRIPTION);
        ScenarioScriptServer *scripts = LuaModules::server<ScenarioScriptServer>(p_state);
        lua_pushboolean(
                p_state, static_cast<int>(scripts->script_cancel(
                                 LuaScriptContext::from_state(p_state)->get_rid(), subscription)));
        return 1;
    }

    int LuaModules::subscribe(
            lua_State *p_state, const ScenarioScriptServer::ScriptSignal p_signal, const ScriptHandleKind p_target) {
        const RID target = LuaHandle::check(p_state, 1, p_target);
        LuaScriptContext *context = LuaScriptContext::from_state(p_state);
        const int64_t function = context->keep_function(p_state, 2);
        ScenarioScriptServer *scripts = server<ScenarioScriptServer>(p_state);
        LuaHandle::push(
                p_state, scripts->script_subscribe(context->get_rid(), p_signal, target, function),
                ScriptHandleKind::SUBSCRIPTION);
        return 1;
    }

    /// export_var(name, default) - a variable of the script its owner sets (a LuaNode in the
    /// inspector): its value, or the default. A boolean, an integer, a number, a string or a colour
    /// {r, g, b[, a]} - its type is the default's
    static int export_var(lua_State *p_state) {
        const String name = String::utf8(luaL_checkstring(p_state, 1));
        luaL_checkany(p_state, 2);
        const bool color = LuaVariant::is_color(p_state, 2);
        const int type = lua_type(p_state, 2);
        luaL_argcheck(
                p_state, color || type == LUA_TBOOLEAN || type == LUA_TNUMBER || type == LUA_TSTRING, 2,
                "a boolean, a number, a string or a colour");
        const Variant fallback =
                color ? Variant(LuaVariant::check_color(p_state, 2)) : LuaVariant::to_variant(p_state, 2);
        const Variant value = LuaModules::server<ScenarioScriptServer>(p_state)->script_variable_export(
                LuaScriptContext::from_state(p_state)->get_rid(), name, fallback);
        if (color) {
            LuaVariant::push_color(p_state, value);
        } else {
            LuaVariant::push(p_state, value);
        }
        return 1;
    }

    const luaL_Reg LuaModules::ROOT[] = {
            {"cancel", cancel},
            {"export_var", export_var},
            {nullptr, nullptr},
    };

    // Only the addresses of the module tables are taken, which is a constant initialization
    // NOLINTNEXTLINE(cppcoreguidelines-interfaces-global-init)
    const LuaModules::Module LuaModules::MODULES[] = {
            {"sim", SIM},     {"vehicle", VEHICLE}, {"cabin", CABIN}, {"driver", DRIVER},   {"event", EVENT},
            {"track", TRACK}, {"signal", SIGNAL},   {"log", LOG},     {"player", PLAYER},   {"camera", CAMERA},
            {"hud", HUD},     {"draw", DRAW},       {"time", TIME},   {"station", STATION}, {nullptr, nullptr},
    };

    /// The plugins' modules (LuaModules::opener_register())
    static Vector<LuaModules::Opener> &openers() {
        static Vector<LuaModules::Opener> registered;
        return registered;
    }

    void LuaModules::opener_register(const Opener p_opener) {
        openers().push_back(p_opener);
    }

    const Vector<LuaModules::Opener> &LuaModules::get_openers() {
        return openers();
    }
} // namespace godot
