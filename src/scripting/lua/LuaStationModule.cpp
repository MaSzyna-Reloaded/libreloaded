#include "LuaHandle.hpp"
#include "LuaModules.hpp"
#include "simulation/SimulationServer.hpp"
#include "station/StationServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

namespace godot {
    /// A time of StationServer's [h], nil for NO_TIME
    static void push_time(lua_State *p_state, const double p_hours) {
        if (p_hours < 0.0) {
            lua_pushnil(p_state);
            return;
        }
        lua_pushnumber(p_state, p_hours);
    }

    /// The entry the script counts from 1 at p_index, as StationServer counts it from 0; raises an
    /// error past the entries of the timetable
    static int check_entry(lua_State *p_state, const int p_index, const RID &p_timetable) {
        const int64_t entry = luaL_checkinteger(p_state, p_index);
        const int count = LuaModules::server<StationServer>(p_state)->timetable_get_entry_count(p_timetable);
        luaL_argcheck(p_state, entry >= 1 && entry <= count, p_index, "no such entry");
        return static_cast<int>(entry - 1);
    }

    /// find(name) - the station of the name, nil when there is none
    static int station_find(lua_State *p_state) {
        const String name = String::utf8(luaL_checkstring(p_state, 1));
        LuaHandle::push(
                p_state, LuaModules::server<StationServer>(p_state)->station_find_by_name(name),
                ScriptHandleKind::STATION);
        return 1;
    }

    static int station_name(lua_State *p_state) {
        const RID station = LuaHandle::check(p_state, 1, ScriptHandleKind::STATION);
        lua_pushstring(
                p_state, LuaModules::server<StationServer>(p_state)->station_get_name(station).utf8().get_data());
        return 1;
    }

    /// run(v) - the timetable the vehicle's trainset runs, nil when it runs none
    static int station_run(lua_State *p_state) {
        const RID vehicle = LuaHandle::check(p_state, 1, ScriptHandleKind::VEHICLE);
        const RID trainset = LuaModules::server<RailVehicleServer>(p_state)->vehicle_get_trainset(vehicle);
        LuaHandle::push(
                p_state,
                trainset.is_valid() ? LuaModules::server<StationServer>(p_state)->run_find_by_trainset(trainset)
                                    : RID(),
                ScriptHandleKind::RUN);
        return 1;
    }

    static int station_timetable(lua_State *p_state) {
        const RID run = LuaHandle::check(p_state, 1, ScriptHandleKind::RUN);
        LuaHandle::push(
                p_state, LuaModules::server<StationServer>(p_state)->run_get_timetable(run),
                ScriptHandleKind::TIMETABLE);
        return 1;
    }

    /// offset(run) - the hours the run shifts its timetable's times by
    static int station_offset(lua_State *p_state) {
        const RID run = LuaHandle::check(p_state, 1, ScriptHandleKind::RUN);
        lua_pushnumber(p_state, LuaModules::server<StationServer>(p_state)->run_get_offset(run));
        return 1;
    }

    /// current_entry(run) - the entry the train drives to or stands at, from 1; the entry count + 1
    /// past the last
    static int station_current_entry(lua_State *p_state) {
        const RID run = LuaHandle::check(p_state, 1, ScriptHandleKind::RUN);
        lua_pushinteger(p_state, LuaModules::server<StationServer>(p_state)->run_get_current_entry(run) + 1);
        return 1;
    }

    /// arrived(run, i), departed(run, i) - when the train arrived at the entry, left or passed it
    /// [h], nil until it did
    static int station_arrived(lua_State *p_state) {
        const RID run = LuaHandle::check(p_state, 1, ScriptHandleKind::RUN);
        StationServer *stations = LuaModules::server<StationServer>(p_state);
        const int entry = check_entry(p_state, 2, stations->run_get_timetable(run));
        push_time(p_state, stations->run_entry_get_actual_arrival(run, entry));
        return 1;
    }

    static int station_departed(lua_State *p_state) {
        const RID run = LuaHandle::check(p_state, 1, ScriptHandleKind::RUN);
        StationServer *stations = LuaModules::server<StationServer>(p_state);
        const int entry = check_entry(p_state, 2, stations->run_get_timetable(run));
        push_time(p_state, stations->run_entry_get_actual_departure(run, entry));
        return 1;
    }

    /// delay(run) - how late the train is now [min], early when negative
    static int station_delay(lua_State *p_state) {
        const RID run = LuaHandle::check(p_state, 1, ScriptHandleKind::RUN);
        const double now = LuaModules::server<SimulationServer>(p_state)->get_time_of_day();
        lua_pushnumber(p_state, LuaModules::server<StationServer>(p_state)->run_get_delay(run, now));
        return 1;
    }

    /// train(timetable) - the train's name (its number), category and label
    static int station_train(lua_State *p_state) {
        const RID timetable = LuaHandle::check(p_state, 1, ScriptHandleKind::TIMETABLE);
        const StationServer *stations = LuaModules::server<StationServer>(p_state);
        lua_pushstring(p_state, stations->timetable_get_train_name(timetable).utf8().get_data());
        lua_pushstring(p_state, stations->timetable_get_train_category(timetable).utf8().get_data());
        lua_pushstring(p_state, stations->timetable_get_train_label(timetable).utf8().get_data());
        return 3;
    }

    /// origin(timetable), destination(timetable) - where the train runs from and to, nil for none
    static int station_origin(lua_State *p_state) {
        const RID timetable = LuaHandle::check(p_state, 1, ScriptHandleKind::TIMETABLE);
        LuaHandle::push(
                p_state, LuaModules::server<StationServer>(p_state)->timetable_get_origin(timetable),
                ScriptHandleKind::STATION);
        return 1;
    }

    static int station_destination(lua_State *p_state) {
        const RID timetable = LuaHandle::check(p_state, 1, ScriptHandleKind::TIMETABLE);
        LuaHandle::push(
                p_state, LuaModules::server<StationServer>(p_state)->timetable_get_destination(timetable),
                ScriptHandleKind::STATION);
        return 1;
    }

    static int station_entry_count(lua_State *p_state) {
        const RID timetable = LuaHandle::check(p_state, 1, ScriptHandleKind::TIMETABLE);
        lua_pushinteger(p_state, LuaModules::server<StationServer>(p_state)->timetable_get_entry_count(timetable));
        return 1;
    }

    /// entry(timetable, i) - the entry, from 1: {station =, arrival =, departure =, stop =,
    /// kilometre =, speed_limit =, track_count =}; the times as planned [h] - the run's are shifted
    /// by its offset - nil where none is given, as the speed limit
    static int station_entry(lua_State *p_state) {
        const RID timetable = LuaHandle::check(p_state, 1, ScriptHandleKind::TIMETABLE);
        const int entry = check_entry(p_state, 2, timetable);
        const StationServer *stations = LuaModules::server<StationServer>(p_state);
        lua_createtable(p_state, 0, 7);
        LuaHandle::push(p_state, stations->timetable_entry_get_station(timetable, entry), ScriptHandleKind::STATION);
        lua_setfield(p_state, -2, "station");
        push_time(p_state, stations->timetable_entry_get_arrival(timetable, entry));
        lua_setfield(p_state, -2, "arrival");
        push_time(p_state, stations->timetable_entry_get_departure(timetable, entry));
        lua_setfield(p_state, -2, "departure");
        lua_pushboolean(p_state, static_cast<int>(stations->timetable_entry_is_stop(timetable, entry)));
        lua_setfield(p_state, -2, "stop");
        lua_pushnumber(p_state, stations->timetable_entry_get_kilometre(timetable, entry));
        lua_setfield(p_state, -2, "kilometre");
        const double speed_limit = stations->timetable_entry_get_speed_limit(timetable, entry);
        if (speed_limit == StationServer::NO_SPEED_LIMIT) {
            lua_pushnil(p_state);
        } else {
            lua_pushnumber(p_state, speed_limit);
        }
        lua_setfield(p_state, -2, "speed_limit");
        lua_pushinteger(p_state, stations->timetable_entry_get_track_count(timetable, entry));
        lua_setfield(p_state, -2, "track_count");
        return 1;
    }

    const luaL_Reg LuaModules::STATION[] = {
            {"find", station_find},
            {"name", station_name},
            {"run", station_run},
            {"timetable", station_timetable},
            {"offset", station_offset},
            {"current_entry", station_current_entry},
            {"arrived", station_arrived},
            {"departed", station_departed},
            {"delay", station_delay},
            {"train", station_train},
            {"origin", station_origin},
            {"destination", station_destination},
            {"entry_count", station_entry_count},
            {"entry", station_entry},
            {nullptr, nullptr},
    };
} // namespace godot
