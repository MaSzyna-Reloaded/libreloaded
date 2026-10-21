#include "StationServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *StationServer::run_arrived_signal = "run_arrived";
    const char *StationServer::run_departed_signal = "run_departed";
    const char *StationServer::run_passed_signal = "run_passed";
    const char *StationServer::run_entry_changed_signal = "run_entry_changed";
    const char *StationServer::run_created_signal = "run_created";
    const char *StationServer::run_freed_signal = "run_freed";
    const char *StationServer::dispatch_step_changed_signal = "dispatch_step_changed";
    const char *StationServer::dispatch_finished_signal = "dispatch_finished";

    void StationServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("station_create"), &StationServer::station_create);
        ClassDB::bind_method(D_METHOD("station_free", "station"), &StationServer::station_free);
        ClassDB::bind_method(D_METHOD("station_set_name", "station", "name"), &StationServer::station_set_name);
        ClassDB::bind_method(D_METHOD("station_get_name", "station"), &StationServer::station_get_name);
        ClassDB::bind_method(D_METHOD("station_find_by_name", "name"), &StationServer::station_find_by_name);
        ClassDB::bind_method(D_METHOD("station_list"), &StationServer::station_list);
        ClassDB::bind_method(D_METHOD("station_set_type", "station", "type"), &StationServer::station_set_type);
        ClassDB::bind_method(D_METHOD("station_get_type", "station"), &StationServer::station_get_type);
        ClassDB::bind_method(
                D_METHOD("station_add_stop_point", "station", "track"), &StationServer::station_add_stop_point);
        ClassDB::bind_method(
                D_METHOD("station_clear_stop_points", "station"), &StationServer::station_clear_stop_points);
        ClassDB::bind_method(D_METHOD("station_get_stop_points", "station"), &StationServer::station_get_stop_points);

        ClassDB::bind_method(D_METHOD("timetable_create"), &StationServer::timetable_create);
        ClassDB::bind_method(D_METHOD("timetable_free", "timetable"), &StationServer::timetable_free);
        ClassDB::bind_method(
                D_METHOD("timetable_set_train_name", "timetable", "name"), &StationServer::timetable_set_train_name);
        ClassDB::bind_method(
                D_METHOD("timetable_get_train_name", "timetable"), &StationServer::timetable_get_train_name);
        ClassDB::bind_method(
                D_METHOD("timetable_set_extra", "timetable", "extra"), &StationServer::timetable_set_extra);
        ClassDB::bind_method(D_METHOD("timetable_get_extra", "timetable"), &StationServer::timetable_get_extra);
        ClassDB::bind_method(
                D_METHOD("timetable_set_train_category", "timetable", "category"),
                &StationServer::timetable_set_train_category);
        ClassDB::bind_method(
                D_METHOD("timetable_get_train_category", "timetable"), &StationServer::timetable_get_train_category);
        ClassDB::bind_method(
                D_METHOD("timetable_set_train_label", "timetable", "label"), &StationServer::timetable_set_train_label);
        ClassDB::bind_method(
                D_METHOD("timetable_get_train_label", "timetable"), &StationServer::timetable_get_train_label);
        ClassDB::bind_method(
                D_METHOD("timetable_set_origin", "timetable", "station"), &StationServer::timetable_set_origin);
        ClassDB::bind_method(D_METHOD("timetable_get_origin", "timetable"), &StationServer::timetable_get_origin);
        ClassDB::bind_method(
                D_METHOD("timetable_set_destination", "timetable", "station"),
                &StationServer::timetable_set_destination);
        ClassDB::bind_method(
                D_METHOD("timetable_get_destination", "timetable"), &StationServer::timetable_get_destination);
        ClassDB::bind_method(
                D_METHOD("timetable_set_speed_limit", "timetable", "speed_limit"),
                &StationServer::timetable_set_speed_limit);
        ClassDB::bind_method(
                D_METHOD("timetable_get_speed_limit", "timetable"), &StationServer::timetable_get_speed_limit);
        ClassDB::bind_method(
                D_METHOD("timetable_set_required_locomotive", "timetable", "locomotive"),
                &StationServer::timetable_set_required_locomotive);
        ClassDB::bind_method(
                D_METHOD("timetable_get_required_locomotive", "timetable"),
                &StationServer::timetable_get_required_locomotive);
        ClassDB::bind_method(
                D_METHOD("timetable_set_required_mass", "timetable", "tonnes"),
                &StationServer::timetable_set_required_mass);
        ClassDB::bind_method(
                D_METHOD("timetable_get_required_mass", "timetable"), &StationServer::timetable_get_required_mass);
        ClassDB::bind_method(
                D_METHOD("timetable_set_required_brake_ratio", "timetable", "percent"),
                &StationServer::timetable_set_required_brake_ratio);
        ClassDB::bind_method(
                D_METHOD("timetable_get_required_brake_ratio", "timetable"),
                &StationServer::timetable_get_required_brake_ratio);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_set_track_count", "timetable", "entry", "track_count"),
                &StationServer::timetable_entry_set_track_count);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_get_track_count", "timetable", "entry"),
                &StationServer::timetable_entry_get_track_count);
        ClassDB::bind_method(
                D_METHOD("timetable_add_entry", "timetable", "station", "arrival", "departure"),
                &StationServer::timetable_add_entry);
        ClassDB::bind_method(
                D_METHOD("timetable_get_entry_count", "timetable"), &StationServer::timetable_get_entry_count);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_get_station", "timetable", "entry"),
                &StationServer::timetable_entry_get_station);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_get_arrival", "timetable", "entry"),
                &StationServer::timetable_entry_get_arrival);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_get_departure", "timetable", "entry"),
                &StationServer::timetable_entry_get_departure);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_is_stop", "timetable", "entry"), &StationServer::timetable_entry_is_stop);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_set_speed_limit", "timetable", "entry", "speed_limit"),
                &StationServer::timetable_entry_set_speed_limit);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_get_speed_limit", "timetable", "entry"),
                &StationServer::timetable_entry_get_speed_limit);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_set_radio_channel", "timetable", "entry", "channel"),
                &StationServer::timetable_entry_set_radio_channel);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_get_radio_channel", "timetable", "entry"),
                &StationServer::timetable_entry_get_radio_channel);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_set_kilometre", "timetable", "entry", "kilometre"),
                &StationServer::timetable_entry_set_kilometre);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_get_kilometre", "timetable", "entry"),
                &StationServer::timetable_entry_get_kilometre);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_set_running_time", "timetable", "entry", "minutes"),
                &StationServer::timetable_entry_set_running_time);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_get_running_time", "timetable", "entry"),
                &StationServer::timetable_entry_get_running_time);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_set_technical_stop", "timetable", "entry", "technical"),
                &StationServer::timetable_entry_set_technical_stop);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_is_technical_stop", "timetable", "entry"),
                &StationServer::timetable_entry_is_technical_stop);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_set_direction_change", "timetable", "entry", "change"),
                &StationServer::timetable_entry_set_direction_change);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_is_direction_change", "timetable", "entry"),
                &StationServer::timetable_entry_is_direction_change);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_set_extra", "timetable", "entry", "extra"),
                &StationServer::timetable_entry_set_extra);
        ClassDB::bind_method(
                D_METHOD("timetable_entry_get_extra", "timetable", "entry"), &StationServer::timetable_entry_get_extra);

        ClassDB::bind_method(
                D_METHOD("run_create", "timetable", "trainset", "offset"), &StationServer::run_create, DEFVAL(0.0));
        ClassDB::bind_method(D_METHOD("run_free", "run"), &StationServer::run_free);
        ClassDB::bind_method(D_METHOD("run_get_timetable", "run"), &StationServer::run_get_timetable);
        ClassDB::bind_method(D_METHOD("run_get_trainset", "run"), &StationServer::run_get_trainset);
        ClassDB::bind_method(D_METHOD("run_get_offset", "run"), &StationServer::run_get_offset);
        ClassDB::bind_method(D_METHOD("run_find_by_trainset", "trainset"), &StationServer::run_find_by_trainset);
        ClassDB::bind_method(D_METHOD("run_next_entry", "run"), &StationServer::run_next_entry);
        ClassDB::bind_method(D_METHOD("run_previous_entry", "run"), &StationServer::run_previous_entry);
        ClassDB::bind_method(D_METHOD("run_get_current_entry", "run"), &StationServer::run_get_current_entry);
        ClassDB::bind_method(
                D_METHOD("run_entry_get_actual_arrival", "run", "entry"), &StationServer::run_entry_get_actual_arrival);
        ClassDB::bind_method(
                D_METHOD("run_entry_get_actual_departure", "run", "entry"),
                &StationServer::run_entry_get_actual_departure);
        ClassDB::bind_method(D_METHOD("run_get_delay", "run", "time"), &StationServer::run_get_delay);
        ClassDB::bind_method(
                D_METHOD("run_get_time_to_departure", "run", "time"), &StationServer::run_get_time_to_departure);

        for (const char *signal:
             {run_arrived_signal, run_departed_signal, run_passed_signal, run_entry_changed_signal}) {
            ADD_SIGNAL(MethodInfo(signal, PropertyInfo(Variant::RID, "run"), PropertyInfo(Variant::INT, "entry")));
        }
        for (const char *signal: {run_created_signal, run_freed_signal}) {
            ADD_SIGNAL(MethodInfo(signal, PropertyInfo(Variant::RID, "run")));
        }

        BIND_ENUM_CONSTANT(STATION_TYPE_STATION);
        BIND_ENUM_CONSTANT(STATION_TYPE_PASSENGER_STOP);
        BIND_CONSTANT(NO_TIME);
        BIND_CONSTANT(NO_SPEED_LIMIT);
        BIND_CONSTANT(NO_RADIO_CHANNEL);
        BIND_CONSTANT(NO_KILOMETRE);
        ClassDB::bind_method(D_METHOD("dispatch_start", "vehicle", "cars"), &StationServer::dispatch_start);
        ClassDB::bind_method(D_METHOD("dispatch_depart", "vehicle"), &StationServer::dispatch_depart);
        ClassDB::bind_method(D_METHOD("dispatch_cancel", "vehicle"), &StationServer::dispatch_cancel);
        ClassDB::bind_method(D_METHOD("dispatch_get_step", "vehicle"), &StationServer::dispatch_get_step);
        ClassDB::bind_method(
                D_METHOD("dispatch_get_exchange_time", "vehicle"), &StationServer::dispatch_get_exchange_time);

        BIND_ENUM_CONSTANT(DISPATCH_STEP_NONE);
        BIND_ENUM_CONSTANT(DISPATCH_STEP_EXCHANGE);
        BIND_ENUM_CONSTANT(DISPATCH_STEP_WAIT_DEPARTURE);
        BIND_ENUM_CONSTANT(DISPATCH_STEP_CLOSE_DOORS);

        ADD_SIGNAL(MethodInfo(
                dispatch_step_changed_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::INT, "step", PROPERTY_HINT_ENUM, "None,Exchange,Wait departure,Close doors")));
        ADD_SIGNAL(MethodInfo(dispatch_finished_signal, PropertyInfo(Variant::RID, "vehicle")));
    }

    /// A freed vehicle takes its train's dispatch with it. No explicit disconnect: callable_mp
    /// reports this instance as the callable's object, so the engine drops the connection when it
    /// dies.
    /// A run hears its train on its tracks and follows its trainset.
    StationServer::StationServer() {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        vehicles->connect(VehicleServer::vehicle_freed_signal, callable_mp(this, &StationServer::_on_vehicle_freed));
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        tracks->connect(
                TrackServer::track_vehicle_entered_signal,
                callable_mp(this, &StationServer::_on_track_vehicle_entered));
        tracks->connect(
                TrackServer::track_vehicle_stopped_signal,
                callable_mp(this, &StationServer::_on_track_vehicle_stopped));
        tracks->connect(
                TrackServer::track_vehicle_started_signal,
                callable_mp(this, &StationServer::_on_track_vehicle_started));
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(rail_vehicles);
        rail_vehicles->connect(
                RailVehicleServer::trainset_merged_signal, callable_mp(this, &StationServer::_on_trainset_merged));
        rail_vehicles->connect(
                RailVehicleServer::trainset_freed_signal, callable_mp(this, &StationServer::_on_trainset_freed));
    }

    RID StationServer::station_create() {
        const RID station = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        stations.insert(station, Station());
        return station;
    }

    void StationServer::station_free(const RID &p_station) {
        const Station *station = stations.getptr(p_station);
        ERR_FAIL_NULL(station);
        ERR_FAIL_COND_MSG(_is_station_in_timetable(p_station), "A timetable calls at the station.");
        if (const RID *named = stations_by_name.getptr(station->name); named != nullptr && *named == p_station) {
            stations_by_name.erase(station->name);
        }
        stations.erase(p_station);
    }

    void StationServer::station_set_name(const RID &p_station, const String &p_name) {
        Station *station = stations.getptr(p_station);
        ERR_FAIL_NULL(station);
        if (const RID *named = stations_by_name.getptr(station->name); named != nullptr && *named == p_station) {
            stations_by_name.erase(station->name);
        }
        station->name = p_name;
        if (!p_name.is_empty()) {
            stations_by_name[p_name] = p_station;
        }
    }

    String StationServer::station_get_name(const RID &p_station) const {
        const Station *station = stations.getptr(p_station);
        ERR_FAIL_NULL_V(station, String());
        return station->name;
    }

    RID StationServer::station_find_by_name(const String &p_name) const {
        const RID *station = stations_by_name.getptr(p_name);
        return station != nullptr ? *station : RID();
    }

    TypedArray<RID> StationServer::station_list() const {
        TypedArray<RID> result;
        for (const KeyValue<RID, Station> &entry: stations) {
            result.push_back(entry.key);
        }
        return result;
    }

    void StationServer::station_set_type(const RID &p_station, const StationType p_type) {
        Station *station = stations.getptr(p_station);
        ERR_FAIL_NULL(station);
        station->type = p_type;
    }

    StationServer::StationType StationServer::station_get_type(const RID &p_station) const {
        const Station *station = stations.getptr(p_station);
        ERR_FAIL_NULL_V(station, STATION_TYPE_STATION);
        return station->type;
    }

    void StationServer::station_add_stop_point(const RID &p_station, const RID &p_track) {
        Station *station = stations.getptr(p_station);
        ERR_FAIL_NULL(station);
        if (!station->stop_points.has(p_track)) {
            station->stop_points.push_back(p_track);
        }
    }

    void StationServer::station_clear_stop_points(const RID &p_station) {
        Station *station = stations.getptr(p_station);
        ERR_FAIL_NULL(station);
        station->stop_points.clear();
    }

    TypedArray<RID> StationServer::station_get_stop_points(const RID &p_station) const {
        TypedArray<RID> result;
        const Station *station = stations.getptr(p_station);
        ERR_FAIL_NULL_V(station, result);
        for (const RID &track: station->stop_points) {
            result.push_back(track);
        }
        return result;
    }

    bool StationServer::_is_station_in_timetable(const RID &p_station) const {
        for (const KeyValue<RID, Timetable> &timetable: timetables) {
            if (timetable.value.origin == p_station || timetable.value.destination == p_station) {
                return true;
            }
            for (const Entry &entry: timetable.value.entries) {
                if (entry.station == p_station) {
                    return true;
                }
            }
        }
        return false;
    }

    RID StationServer::timetable_create() {
        const RID timetable = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        timetables.insert(timetable, Timetable());
        return timetable;
    }

    void StationServer::timetable_free(const RID &p_timetable) {
        ERR_FAIL_COND(!timetables.has(p_timetable));
        for (const KeyValue<RID, Run> &run: runs) {
            ERR_FAIL_COND_MSG(run.value.timetable == p_timetable, "A trainset runs the timetable.");
        }
        timetables.erase(p_timetable);
    }

    void StationServer::timetable_set_train_name(const RID &p_timetable, const String &p_name) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        timetable->train_name = p_name;
    }

    String StationServer::timetable_get_train_name(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, String());
        return timetable->train_name;
    }

    void StationServer::timetable_set_extra(const RID &p_timetable, const Dictionary &p_extra) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        timetable->extra = p_extra;
    }

    Dictionary StationServer::timetable_get_extra(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, Dictionary());
        return timetable->extra;
    }

    void StationServer::timetable_set_train_category(const RID &p_timetable, const String &p_category) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        timetable->train_category = p_category;
    }

    String StationServer::timetable_get_train_category(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, String());
        return timetable->train_category;
    }

    void StationServer::timetable_set_train_label(const RID &p_timetable, const String &p_label) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        timetable->train_label = p_label;
    }

    String StationServer::timetable_get_train_label(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, String());
        return timetable->train_label;
    }

    void StationServer::timetable_set_origin(const RID &p_timetable, const RID &p_station) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        ERR_FAIL_COND_MSG(p_station.is_valid() && !stations.has(p_station), "No such station.");
        timetable->origin = p_station;
    }

    RID StationServer::timetable_get_origin(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, RID());
        return timetable->origin;
    }

    void StationServer::timetable_set_destination(const RID &p_timetable, const RID &p_station) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        ERR_FAIL_COND_MSG(p_station.is_valid() && !stations.has(p_station), "No such station.");
        timetable->destination = p_station;
    }

    RID StationServer::timetable_get_destination(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, RID());
        return timetable->destination;
    }

    void StationServer::timetable_set_speed_limit(const RID &p_timetable, const double p_speed_limit) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        timetable->speed_limit = p_speed_limit;
    }

    double StationServer::timetable_get_speed_limit(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, NO_SPEED_LIMIT);
        return timetable->speed_limit;
    }

    void StationServer::timetable_set_required_locomotive(const RID &p_timetable, const String &p_locomotive) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        timetable->required_locomotive = p_locomotive;
    }

    String StationServer::timetable_get_required_locomotive(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, String());
        return timetable->required_locomotive;
    }

    void StationServer::timetable_set_required_mass(const RID &p_timetable, const double p_tonnes) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        timetable->required_mass = p_tonnes;
    }

    double StationServer::timetable_get_required_mass(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, 0.0);
        return timetable->required_mass;
    }

    void StationServer::timetable_set_required_brake_ratio(const RID &p_timetable, const double p_percent) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL(timetable);
        timetable->required_brake_ratio = p_percent;
    }

    double StationServer::timetable_get_required_brake_ratio(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, 0.0);
        return timetable->required_brake_ratio;
    }

    int StationServer::timetable_add_entry(
            const RID &p_timetable, const RID &p_station, const double p_arrival, const double p_departure) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, -1);
        ERR_FAIL_COND_V_MSG(!stations.has(p_station), -1, "No such station.");
        Entry entry;
        entry.station = p_station;
        entry.arrival = p_arrival;
        entry.departure = p_departure;
        timetable->entries.push_back(entry);
        return static_cast<int>(timetable->entries.size()) - 1;
    }

    int StationServer::timetable_get_entry_count(const RID &p_timetable) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, 0);
        return static_cast<int>(timetable->entries.size());
    }

    StationServer::Entry *StationServer::_get_entry(const RID &p_timetable, const int p_entry) {
        Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, nullptr);
        ERR_FAIL_INDEX_V(p_entry, timetable->entries.size(), nullptr);
        return &timetable->entries.write[p_entry];
    }

    const StationServer::Entry *StationServer::_get_entry(const RID &p_timetable, const int p_entry) const {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V(timetable, nullptr);
        ERR_FAIL_INDEX_V(p_entry, timetable->entries.size(), nullptr);
        return &timetable->entries[p_entry];
    }

    RID StationServer::timetable_entry_get_station(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr ? entry->station : RID();
    }

    double StationServer::timetable_entry_get_arrival(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr ? entry->arrival : NO_TIME;
    }

    double StationServer::timetable_entry_get_departure(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr ? entry->departure : NO_TIME;
    }

    bool StationServer::timetable_entry_is_stop(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr && entry->arrival >= 0.0;
    }

    void StationServer::timetable_entry_set_speed_limit(
            const RID &p_timetable, const int p_entry, const double p_speed_limit) {
        if (Entry *entry = _get_entry(p_timetable, p_entry); entry != nullptr) {
            entry->speed_limit = p_speed_limit;
        }
    }

    double StationServer::timetable_entry_get_speed_limit(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr ? entry->speed_limit : NO_SPEED_LIMIT;
    }

    void
    StationServer::timetable_entry_set_radio_channel(const RID &p_timetable, const int p_entry, const int p_channel) {
        if (Entry *entry = _get_entry(p_timetable, p_entry); entry != nullptr) {
            entry->radio_channel = p_channel;
        }
    }

    int StationServer::timetable_entry_get_radio_channel(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr ? entry->radio_channel : NO_RADIO_CHANNEL;
    }

    void
    StationServer::timetable_entry_set_kilometre(const RID &p_timetable, const int p_entry, const double p_kilometre) {
        if (Entry *entry = _get_entry(p_timetable, p_entry); entry != nullptr) {
            entry->kilometre = p_kilometre;
        }
    }

    double StationServer::timetable_entry_get_kilometre(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr ? entry->kilometre : NO_KILOMETRE;
    }

    void
    StationServer::timetable_entry_set_running_time(const RID &p_timetable, const int p_entry, const double p_minutes) {
        if (Entry *entry = _get_entry(p_timetable, p_entry); entry != nullptr) {
            entry->running_time = p_minutes;
        }
    }

    double StationServer::timetable_entry_get_running_time(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr ? entry->running_time : 0.0;
    }

    void StationServer::timetable_entry_set_technical_stop(
            const RID &p_timetable, const int p_entry, const bool p_technical) {
        if (Entry *entry = _get_entry(p_timetable, p_entry); entry != nullptr) {
            entry->technical_stop = p_technical;
        }
    }

    bool StationServer::timetable_entry_is_technical_stop(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr && entry->technical_stop;
    }

    void StationServer::timetable_entry_set_direction_change(
            const RID &p_timetable, const int p_entry, const bool p_change) {
        if (Entry *entry = _get_entry(p_timetable, p_entry); entry != nullptr) {
            entry->direction_change = p_change;
        }
    }

    bool StationServer::timetable_entry_is_direction_change(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr && entry->direction_change;
    }

    void
    StationServer::timetable_entry_set_track_count(const RID &p_timetable, const int p_entry, const int p_track_count) {
        if (Entry *entry = _get_entry(p_timetable, p_entry); entry != nullptr) {
            entry->track_count = p_track_count;
        }
    }

    int StationServer::timetable_entry_get_track_count(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr ? entry->track_count : 1;
    }

    void
    StationServer::timetable_entry_set_extra(const RID &p_timetable, const int p_entry, const Dictionary &p_extra) {
        if (Entry *entry = _get_entry(p_timetable, p_entry); entry != nullptr) {
            entry->extra = p_extra;
        }
    }

    Dictionary StationServer::timetable_entry_get_extra(const RID &p_timetable, const int p_entry) const {
        const Entry *entry = _get_entry(p_timetable, p_entry);
        return entry != nullptr ? entry->extra : Dictionary();
    }

    RID StationServer::run_create(const RID &p_timetable, const RID &p_trainset, const double p_offset) {
        const Timetable *timetable = timetables.getptr(p_timetable);
        ERR_FAIL_NULL_V_MSG(timetable, RID(), "No such timetable.");
        ERR_FAIL_COND_V_MSG(runs_by_trainset.has(p_trainset), RID(), "The trainset runs a timetable already.");
        const RID run_rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        Run run;
        run.timetable = p_timetable;
        run.trainset = p_trainset;
        run.offset = p_offset;
        run.actual_arrivals.resize(timetable->entries.size());
        run.actual_arrivals.fill(NO_TIME);
        run.actual_departures.resize(timetable->entries.size());
        run.actual_departures.fill(NO_TIME);
        // the train starts its run at the first entry where no arrival is planned there, or where it
        // stands at a stop point of its station already
        if (!timetable->entries.is_empty() &&
            (timetable->entries[0].arrival < 0.0 || _is_standing_at(p_trainset, timetable->entries[0].station))) {
            const SimulationServer *simulation = SimulationServer::get_instance();
            run.actual_arrivals.set(0, simulation != nullptr ? simulation->get_time_of_day() : 0.0);
        }
        runs.insert(run_rid, run);
        runs_by_trainset[p_trainset] = run_rid;
        emit_signal(run_created_signal, run_rid);
        return run_rid;
    }

    void StationServer::run_free(const RID &p_run) {
        const Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL(run);
        if (const RID *bound = runs_by_trainset.getptr(run->trainset); bound != nullptr && *bound == p_run) {
            runs_by_trainset.erase(run->trainset);
        }
        runs.erase(p_run);
        emit_signal(run_freed_signal, p_run);
    }

    RID StationServer::run_get_timetable(const RID &p_run) const {
        const Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL_V(run, RID());
        return run->timetable;
    }

    RID StationServer::run_get_trainset(const RID &p_run) const {
        const Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL_V(run, RID());
        return run->trainset;
    }

    double StationServer::run_get_offset(const RID &p_run) const {
        const Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL_V(run, 0.0);
        return run->offset;
    }

    RID StationServer::run_find_by_trainset(const RID &p_trainset) const {
        const RID *run = runs_by_trainset.getptr(p_trainset);
        return run != nullptr ? *run : RID();
    }

    void StationServer::run_next_entry(const RID &p_run) {
        Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL(run);
        if (run->current_entry >= run->actual_arrivals.size()) {
            return;
        }
        ++run->current_entry;
        emit_signal(run_entry_changed_signal, p_run, run->current_entry);
    }

    void StationServer::run_previous_entry(const RID &p_run) {
        Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL(run);
        if (run->current_entry <= 0) {
            return;
        }
        --run->current_entry;
        emit_signal(run_entry_changed_signal, p_run, run->current_entry);
    }

    int StationServer::run_get_current_entry(const RID &p_run) const {
        const Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL_V(run, 0);
        return run->current_entry;
    }

    double StationServer::run_entry_get_actual_arrival(const RID &p_run, const int p_entry) const {
        const Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL_V(run, NO_TIME);
        ERR_FAIL_INDEX_V(p_entry, run->actual_arrivals.size(), NO_TIME);
        return run->actual_arrivals[p_entry];
    }

    double StationServer::run_entry_get_actual_departure(const RID &p_run, const int p_entry) const {
        const Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL_V(run, NO_TIME);
        ERR_FAIL_INDEX_V(p_entry, run->actual_departures.size(), NO_TIME);
        return run->actual_departures[p_entry];
    }

    double StationServer::run_get_delay(const RID &p_run, const double p_time) const {
        const Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL_V(run, 0.0);
        const Entry *entry = _get_current_entry(*run);
        if (entry == nullptr || run->actual_departures[run->current_entry] >= 0.0) {
            return run->recorded_delay; // past the last, or on the way from the current one
        }
        // the planned time of where it has not got to: the arrival on the way to a stop, the
        // departure where it stands or passes
        const bool standing = run->actual_arrivals[run->current_entry] >= 0.0;
        const double planned = !standing && entry->arrival >= 0.0 ? entry->arrival : entry->departure;
        // a train early at a stop waits for its departure: it is on time there, not early
        const double recorded = standing ? MAX(0.0, run->recorded_delay) : run->recorded_delay;
        if (planned < 0.0) {
            return recorded;
        }
        return MAX(recorded, _minutes_between(_shifted(planned, run->offset), p_time));
    }

    double StationServer::run_get_time_to_departure(const RID &p_run, const double p_time) const {
        const Run *run = runs.getptr(p_run);
        ERR_FAIL_NULL_V(run, 0.0);
        const Entry *entry = _get_current_entry(*run);
        if (entry == nullptr || entry->arrival < 0.0 || entry->departure < 0.0) {
            return 0.0;
        }
        return _minutes_between(p_time, _shifted(entry->departure, run->offset));
    }

    bool StationServer::_is_standing_at(const RID &p_trainset, const RID &p_station) const {
        const Station *station = stations.getptr(p_station);
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        const TrackServer *tracks = TrackServer::get_instance();
        if (station == nullptr || rail_vehicles == nullptr || tracks == nullptr) {
            return false;
        }
        const TypedArray<RID> trainset_vehicles = rail_vehicles->trainset_get_vehicles(p_trainset);
        for (int index = 0; index < trainset_vehicles.size(); ++index) {
            const RID vehicle = trainset_vehicles[index];
            if (!rail_vehicles->vehicle_is_standing(vehicle)) {
                return false;
            }
            for (const RID &track: station->stop_points) {
                if (tracks->track_get_vehicle_list(track).has(vehicle)) {
                    return true;
                }
            }
        }
        return false;
    }

    const StationServer::Entry *StationServer::_get_current_entry(const Run &p_run) const {
        const Timetable *timetable = timetables.getptr(p_run.timetable);
        if (timetable == nullptr || p_run.current_entry >= timetable->entries.size()) {
            return nullptr;
        }
        return &timetable->entries[p_run.current_entry];
    }

    bool StationServer::_is_at_current_station(const Run &p_run, const RID &p_track) const {
        const Entry *entry = _get_current_entry(p_run);
        const Station *station = entry != nullptr ? stations.getptr(entry->station) : nullptr;
        return station != nullptr && station->stop_points.has(p_track);
    }

    double StationServer::_shifted(const double p_hours, const double p_offset) {
        return Math::fposmod(p_hours + p_offset, HOURS_PER_DAY);
    }

    double StationServer::_minutes_between(const double p_from, const double p_to) {
        double minutes = (p_to - p_from) * MINUTES_PER_HOUR;
        if (minutes < -HALF_DAY_MINUTES) {
            minutes += DAY_MINUTES;
        } else if (minutes > HALF_DAY_MINUTES) {
            minutes -= DAY_MINUTES;
        }
        return minutes;
    }

    StationServer::Run *StationServer::_get_run_of_vehicle(const RID &p_vehicle, RID &p_run_rid) {
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL_V(rail_vehicles, nullptr);
        const RID *run = runs_by_trainset.getptr(rail_vehicles->vehicle_get_trainset(p_vehicle));
        if (run == nullptr) {
            return nullptr;
        }
        p_run_rid = *run;
        return runs.getptr(*run);
    }

    /// A moving train coming onto a stop point of the station it passes: it passes it now
    void StationServer::_on_track_vehicle_entered(
            const RID &p_track, const RID &p_vehicle, const TrackServer::TravelDirection p_direction) {
        RID run_rid;
        Run *run = _get_run_of_vehicle(p_vehicle, run_rid);
        if (run == nullptr || p_direction == TrackServer::TRAVEL_NONE || !_is_at_current_station(*run, p_track)) {
            return;
        }
        const Entry *entry = _get_current_entry(*run);
        const int index = run->current_entry;
        if (entry->arrival >= 0.0 || run->actual_departures[index] >= 0.0) {
            return; // a stop is arrived at standing, a passing is recorded once
        }
        const SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        run->actual_departures.set(index, simulation->get_time_of_day());
        run->recorded_delay = _minutes_between(_shifted(entry->departure, run->offset), run->actual_departures[index]);
        emit_signal(run_passed_signal, run_rid, index);
    }

    /// The train stopping at a stop point of its current entry's station has arrived there
    void StationServer::_on_track_vehicle_stopped(const RID &p_track, const RID &p_vehicle) {
        RID run_rid;
        Run *run = _get_run_of_vehicle(p_vehicle, run_rid);
        if (run == nullptr || !_is_at_current_station(*run, p_track)) {
            return;
        }
        const int index = run->current_entry;
        if (run->actual_arrivals[index] >= 0.0 || run->actual_departures[index] >= 0.0) {
            return;
        }
        const Entry *entry = _get_current_entry(*run);
        const SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        run->actual_arrivals.set(index, simulation->get_time_of_day());
        const double planned = entry->arrival >= 0.0 ? entry->arrival : entry->departure;
        if (planned >= 0.0) {
            run->recorded_delay = _minutes_between(_shifted(planned, run->offset), run->actual_arrivals[index]);
        }
        emit_signal(run_arrived_signal, run_rid, index);
    }

    /// The train standing at its current entry starting off - wherever it stood - has left it
    void StationServer::_on_track_vehicle_started(
            const RID & /*p_track*/, const RID &p_vehicle, const TrackServer::TravelDirection /*p_direction*/) {
        RID run_rid;
        Run *run = _get_run_of_vehicle(p_vehicle, run_rid);
        if (run == nullptr) {
            return;
        }
        const int index = run->current_entry;
        const Entry *entry = _get_current_entry(*run);
        if (entry == nullptr || run->actual_arrivals[index] < 0.0 || run->actual_departures[index] >= 0.0) {
            return;
        }
        const SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        run->actual_departures.set(index, simulation->get_time_of_day());
        if (entry->departure >= 0.0) {
            run->recorded_delay =
                    _minutes_between(_shifted(entry->departure, run->offset), run->actual_departures[index]);
        }
        emit_signal(run_departed_signal, run_rid, index);
    }

    /// Coupled into another trainset, a run goes with it where that one has none, and ends where it has
    void StationServer::_on_trainset_merged(const RID &p_trainset, const RID &p_absorbed) {
        const RID *absorbed_run = runs_by_trainset.getptr(p_absorbed);
        if (absorbed_run == nullptr) {
            return;
        }
        const RID run_rid = *absorbed_run;
        if (runs_by_trainset.has(p_trainset)) {
            run_free(run_rid);
            return;
        }
        runs_by_trainset.erase(p_absorbed);
        runs_by_trainset[p_trainset] = run_rid;
        runs[run_rid].trainset = p_trainset;
    }

    void StationServer::_on_trainset_freed(const RID &p_trainset) {
        if (const RID *run = runs_by_trainset.getptr(p_trainset); run != nullptr) {
            run_free(*run);
        }
    }

    void StationServer::dispatch_start(const RID &p_vehicle, const TypedArray<RID> &p_cars) {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        dispatch_cancel(p_vehicle);
        Dispatch &dispatch = dispatches[p_vehicle];
        for (int index = 0; index < p_cars.size(); ++index) {
            Car car;
            car.vehicle = p_cars[index];
            car.load = vehicles->vehicle_component_get(car.vehicle, VehicleComponentType::COMPONENT_LOAD);
            car.doors = vehicles->vehicle_component_get(car.vehicle, VehicleComponentType::COMPONENT_DOORS);
            if (car.load.is_valid()) {
                car.load->connect(
                        RailVehicleLoad::load_exchange_finished_signal,
                        callable_mp(this, &StationServer::_on_load_exchange_finished).bind(p_vehicle, car.vehicle));
                if (car.load->get_load_exchange_time() > 0.0) {
                    dispatch.exchanging.insert(car.vehicle);
                }
            }
            if (car.doors.is_valid()) {
                car.doors->connect(
                        RailVehicleDoors::doors_closed_signal,
                        callable_mp(this, &StationServer::_on_doors_closed).bind(p_vehicle, car.vehicle));
            }
            dispatch.cars.push_back(car);
        }
        _set_step(
                p_vehicle, dispatch,
                dispatch.exchanging.is_empty() ? DISPATCH_STEP_WAIT_DEPARTURE : DISPATCH_STEP_EXCHANGE);
    }

    void StationServer::dispatch_depart(const RID &p_vehicle) {
        Dispatch *dispatch = dispatches.getptr(p_vehicle);
        if (dispatch == nullptr) {
            return;
        }
        dispatch->departure_allowed = true;
        if (dispatch->step == DISPATCH_STEP_WAIT_DEPARTURE) {
            _close_doors(p_vehicle, *dispatch);
        }
    }

    void StationServer::dispatch_cancel(const RID &p_vehicle) {
        Dispatch *dispatch = dispatches.getptr(p_vehicle);
        if (dispatch == nullptr) {
            return;
        }
        for (const Car &car: dispatch->cars) {
            if (car.load.is_valid()) {
                car.load->disconnect(
                        RailVehicleLoad::load_exchange_finished_signal,
                        callable_mp(this, &StationServer::_on_load_exchange_finished).bind(p_vehicle, car.vehicle));
            }
            if (car.doors.is_valid()) {
                car.doors->disconnect(
                        RailVehicleDoors::doors_closed_signal,
                        callable_mp(this, &StationServer::_on_doors_closed).bind(p_vehicle, car.vehicle));
            }
        }
        dispatches.erase(p_vehicle);
        emit_signal(dispatch_step_changed_signal, p_vehicle, DISPATCH_STEP_NONE);
    }

    StationServer::DispatchStep StationServer::dispatch_get_step(const RID &p_vehicle) const {
        const Dispatch *dispatch = dispatches.getptr(p_vehicle);
        return dispatch != nullptr ? dispatch->step : DISPATCH_STEP_NONE;
    }

    double StationServer::dispatch_get_exchange_time(const RID &p_vehicle) const {
        const Dispatch *dispatch = dispatches.getptr(p_vehicle);
        double exchange_time = 0.0;
        if (dispatch == nullptr) {
            return exchange_time;
        }
        for (const Car &car: dispatch->cars) {
            if (car.load.is_valid()) {
                exchange_time = MAX(exchange_time, car.load->get_load_exchange_time());
            }
        }
        return exchange_time;
    }

    void StationServer::_set_step(const RID &p_vehicle, Dispatch &p_dispatch, const DispatchStep p_step) {
        p_dispatch.step = p_step;
        emit_signal(dispatch_step_changed_signal, p_vehicle, p_step);
    }

    /// Every car whose doors are not closed yet is waited for; with none, the dispatch is over
    void StationServer::_close_doors(const RID &p_vehicle, Dispatch &p_dispatch) {
        for (const Car &car: p_dispatch.cars) {
            if (car.doors.is_valid() && !(car.doors->get_left_closed() && car.doors->get_right_closed())) {
                p_dispatch.doors_not_closed.insert(car.vehicle);
            }
        }
        if (p_dispatch.doors_not_closed.is_empty()) {
            _finish(p_vehicle);
            return;
        }
        _set_step(p_vehicle, p_dispatch, DISPATCH_STEP_CLOSE_DOORS);
    }

    void StationServer::_finish(const RID &p_vehicle) {
        dispatch_cancel(p_vehicle);
        emit_signal(dispatch_finished_signal, p_vehicle);
    }

    void StationServer::_on_load_exchange_finished(const RID &p_vehicle, const RID &p_car) {
        Dispatch *dispatch = dispatches.getptr(p_vehicle);
        ERR_FAIL_NULL(dispatch);
        dispatch->exchanging.erase(p_car);
        if (dispatch->step != DISPATCH_STEP_EXCHANGE || !dispatch->exchanging.is_empty()) {
            return;
        }
        if (dispatch->departure_allowed) {
            _close_doors(p_vehicle, *dispatch);
            return;
        }
        _set_step(p_vehicle, *dispatch, DISPATCH_STEP_WAIT_DEPARTURE);
    }

    void StationServer::_on_doors_closed(const int p_side, const RID &p_vehicle, const RID &p_car) {
        Dispatch *dispatch = dispatches.getptr(p_vehicle);
        ERR_FAIL_NULL(dispatch);
        if (dispatch->step != DISPATCH_STEP_CLOSE_DOORS) {
            return;
        }
        for (const Car &car: dispatch->cars) {
            if (car.vehicle == p_car && car.doors->get_left_closed() && car.doors->get_right_closed()) {
                dispatch->doors_not_closed.erase(p_car);
            }
        }
        if (dispatch->doors_not_closed.is_empty()) {
            _finish(p_vehicle);
        }
    }

    void StationServer::_on_vehicle_freed(const RID &p_vehicle) {
        Vector<RID> dropped;
        for (const KeyValue<RID, Dispatch> &entry: dispatches) {
            if (entry.key == p_vehicle) {
                dropped.push_back(entry.key);
                continue;
            }
            for (const Car &car: entry.value.cars) {
                if (car.vehicle == p_vehicle) {
                    dropped.push_back(entry.key);
                }
            }
        }
        for (const RID &vehicle: dropped) {
            dispatch_cancel(vehicle);
        }
    }
} // namespace godot
