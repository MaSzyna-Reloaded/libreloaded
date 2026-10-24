#pragma once
#include "tracks/TrackServer.hpp"
#include "vehicles/rail/RailVehicleDoors.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/hash_set.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The stations of a scenery, the timetables of the trains calling at them, and the dispatch
    /// of a train at a stop.
    ///
    /// A station (station_*) has a name it is also found by and the tracks where a train stops or
    /// passes it - its stop points. A timetable (timetable_*) is a plan only: the train's name and
    /// its entries, each a station with its planned arrival and departure, any number of trains
    /// may run by it. A run (run_*) is a timetable run by a trainset: whoever drives the train moves
    /// it on from entry to entry, this server records when the train arrived at the current entry's
    /// station, left it or passed it, and works out from that how late the train is.
    ///
    /// The dispatch (odprawa, dispatch_*): its steps one after another, each over when the
    /// vehicles report it done - the cars' passengers exchanged, the train let go, its doors
    /// closed. A dispatch is kept under the vehicle its train is driven from. The cars open their
    /// doors and exchange by themselves (RailVehicleLoad); whoever drives works the cab's controls
    /// as the step asks. The original keeps it as a negative fStopTime of the driver and the
    /// driver's Doors() (Driver.cpp:1233-1241, 4266-4356, 6767-6801).
    class StationServer : public Object {
            GDCLASS(StationServer, Object)

        public:
            /// A time left out of a timetable: the arrival where a train passes without stopping
            static constexpr double NO_TIME = -1.0;
            /// No speed limit on the way to an entry, no radio channel from it on
            static constexpr double NO_SPEED_LIMIT = -1.0;
            static constexpr int NO_RADIO_CHANNEL = -1;
            /// An entry's kilometre the timetable does not give
            static constexpr double NO_KILOMETRE = -1.0;
            static constexpr double HOURS_PER_DAY = 24.0;
            static constexpr double MINUTES_PER_HOUR = 60.0;
            static constexpr double DAY_MINUTES = 1440.0;
            /// A difference over half a day is the other way round the clock
            static constexpr double HALF_DAY_MINUTES = 720.0;

            /// What a place a train stops at is: a station, or a passenger stop - a small one, where
            /// fewer get on and off
            enum StationType {
                STATION_TYPE_STATION,
                STATION_TYPE_PASSENGER_STOP,
            };

            enum DispatchStep {
                /// No dispatch
                DISPATCH_STEP_NONE,
                /// The passengers get off and on
                DISPATCH_STEP_EXCHANGE,
                /// The exchange is over, the train is not let go yet
                DISPATCH_STEP_WAIT_DEPARTURE,
                /// The train is let go: its doors close
                DISPATCH_STEP_CLOSE_DOORS,
            };

            /// The run arrived at its current entry, left it, passed it, moved on to another one
            /// (run: RID, entry: int); a run began, a run ended (run: RID)
            static const char *run_arrived_signal;
            static const char *run_departed_signal;
            static const char *run_passed_signal;
            static const char *run_entry_changed_signal;
            static const char *run_created_signal;
            static const char *run_freed_signal;
            /// The dispatch of the train driven from the vehicle went to another step
            /// (vehicle: RID, step: DispatchStep)
            static const char *dispatch_step_changed_signal;
            /// The dispatch is over, the train may go (vehicle: RID)
            static const char *dispatch_finished_signal;

            static StationServer *get_instance() {
                return singleton;
            }
            ~StationServer() override;

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline StationServer *singleton = nullptr;

            struct Station {
                    String name;
                    StationType type = STATION_TYPE_STATION;
                    /// The tracks where a train stops or passes the station
                    Vector<RID> stop_points;
            };
            /// A station of a timetable, the times planned there (hours since midnight) and what
            /// the timetable says of the way to it
            struct Entry {
                    RID station;
                    double arrival = NO_TIME;
                    double departure = NO_TIME;
                    /// The speed limit on the way to it [km/h]
                    double speed_limit = NO_SPEED_LIMIT;
                    /// The radio channel from it on
                    int radio_channel = NO_RADIO_CHANNEL;
                    /// Where it is on the line [km], NO_KILOMETRE for not given
                    double kilometre = NO_KILOMETRE;
                    /// The running time to it [min]
                    double running_time = 0.0;
                    /// A stop for the train's own sake: nobody gets on or off
                    bool technical_stop = false;
                    /// The train changes its direction there
                    bool direction_change = false;
                    /// The line's tracks on the way to it, 1 or 2
                    int track_count = 1;
                    Dictionary extra;
            };
            struct Timetable {
                    String train_name;
                    /// What kind of train it is (`Os`, `Ex`) and its own name, if it has one
                    String train_category;
                    String train_label;
                    /// The stations the train runs from and to - beyond the entries, where it runs on
                    RID origin;
                    RID destination;
                    /// The train's speed limit where no entry gives one [km/h]
                    double speed_limit = NO_SPEED_LIMIT;
                    /// What the train is to be: its locomotive's type, its mass [t] and its braked
                    /// weight [%]; empty and 0 for none required
                    String required_locomotive;
                    double required_mass = 0.0;
                    double required_brake_ratio = 0.0;
                    Vector<Entry> entries;
                    Dictionary extra;
            };
            /// A timetable run by a trainset: the entry it is at - driving to it or standing there -
            /// and what actually happened at each entry
            struct Run {
                    RID timetable;
                    RID trainset;
                    /// The timetable's times shifted by it [h]
                    double offset = 0.0;
                    int current_entry = 0;
                    /// Hours since midnight, NO_TIME until it happened; a departure from where the
                    /// train passes is its passing
                    Vector<double> actual_arrivals;
                    Vector<double> actual_departures;
                    /// The delay last recorded [min], late when positive
                    double recorded_delay = 0.0;
            };
            HashMap<RID, Station> stations;
            HashMap<String, RID> stations_by_name;
            HashMap<RID, Timetable> timetables;
            HashMap<RID, Run> runs;
            HashMap<RID, RID> runs_by_trainset;

            Entry *_get_entry(const RID &p_timetable, int p_entry);
            const Entry *_get_entry(const RID &p_timetable, int p_entry) const;
            bool _is_station_in_timetable(const RID &p_station) const;
            /// The run of the trainset the vehicle is in, nullptr for none - and its RID
            Run *_get_run_of_vehicle(const RID &p_vehicle, RID &p_run_rid);
            /// The trainset stands with a vehicle on a stop point of the station
            bool _is_standing_at(const RID &p_trainset, const RID &p_station) const;
            /// The run's current entry, nullptr past the last
            const Entry *_get_current_entry(const Run &p_run) const;
            /// Whether p_track is a stop point of the station of the run's current entry
            bool _is_at_current_station(const Run &p_run, const RID &p_track) const;
            /// A planned time of the run: the timetable's shifted by the run's offset [h]
            static double _shifted(double p_hours, double p_offset);
            /// p_to less p_from [min], the shorter way round the clock
            static double _minutes_between(double p_from, double p_to);
            void _on_track_vehicle_entered(
                    const RID &p_track, const RID &p_vehicle, TrackServer::TravelDirection p_direction);
            void _on_track_vehicle_stopped(const RID &p_track, const RID &p_vehicle);
            void _on_track_vehicle_started(
                    const RID &p_track, const RID &p_vehicle, TrackServer::TravelDirection p_direction);
            void _on_trainset_merged(const RID &p_trainset, const RID &p_absorbed);
            void _on_trainset_freed(const RID &p_trainset);

            /// A car of the dispatch and what it reports, followed while the dispatch lasts
            struct Car {
                    RID vehicle;
                    Ref<RailVehicleLoad> load;
                    Ref<RailVehicleDoors> doors;
            };
            struct Dispatch {
                    DispatchStep step = DISPATCH_STEP_NONE;
                    /// Let go while its passengers still exchanged: the doors close once they are done
                    bool departure_allowed = false;
                    Vector<Car> cars;
                    /// The cars still exchanging, and those whose doors are still not closed
                    HashSet<RID> exchanging;
                    HashSet<RID> doors_not_closed;
            };
            HashMap<RID, Dispatch> dispatches;

            void _set_step(const RID &p_vehicle, Dispatch &p_dispatch, DispatchStep p_step);
            void _close_doors(const RID &p_vehicle, Dispatch &p_dispatch);
            void _finish(const RID &p_vehicle);
            void _on_load_exchange_finished(const RID &p_vehicle, const RID &p_car);
            void _on_doors_closed(int p_side, const RID &p_vehicle, const RID &p_car);
            void _on_vehicle_freed(const RID &p_vehicle);

        protected:
            static void _bind_methods();

        public:
            StationServer();

            RID station_create();
            /// Fails while a timetable calls at the station or runs from or to it
            void station_free(const RID &p_station);
            /// A name another station had finds this one from now on
            void station_set_name(const RID &p_station, const String &p_name);
            String station_get_name(const RID &p_station) const;
            /// The station of the name, RID() for none
            RID station_find_by_name(const String &p_name) const;
            TypedArray<RID> station_list() const;
            void station_set_type(const RID &p_station, StationType p_type);
            StationType station_get_type(const RID &p_station) const;
            void station_add_stop_point(const RID &p_station, const RID &p_track);
            void station_clear_stop_points(const RID &p_station);
            TypedArray<RID> station_get_stop_points(const RID &p_station) const;

            RID timetable_create();
            void timetable_free(const RID &p_timetable);
            void timetable_set_train_name(const RID &p_timetable, const String &p_name);
            String timetable_get_train_name(const RID &p_timetable) const;
            /// What a plugin keeps of its own about the timetable - the core reads none of it
            void timetable_set_extra(const RID &p_timetable, const Dictionary &p_extra);
            Dictionary timetable_get_extra(const RID &p_timetable) const;
            /// What kind of train it is (`Os`, `Ex`), and its own name, empty for none
            void timetable_set_train_category(const RID &p_timetable, const String &p_category);
            String timetable_get_train_category(const RID &p_timetable) const;
            void timetable_set_train_label(const RID &p_timetable, const String &p_label);
            String timetable_get_train_label(const RID &p_timetable) const;
            /// The stations the train runs from and to - its relation; RID() for one not said
            void timetable_set_origin(const RID &p_timetable, const RID &p_station);
            RID timetable_get_origin(const RID &p_timetable) const;
            void timetable_set_destination(const RID &p_timetable, const RID &p_station);
            RID timetable_get_destination(const RID &p_timetable) const;
            /// The train's speed limit [km/h], NO_SPEED_LIMIT for none - of a timetable of no stations, the
            /// only limit it gives
            void timetable_set_speed_limit(const RID &p_timetable, double p_speed_limit);
            double timetable_get_speed_limit(const RID &p_timetable) const;
            /// What the train is to be - its locomotive's type, its mass [t] and its braked weight
            /// [%]; empty and 0 for none required. The trainset's own are its vehicles'
            void timetable_set_required_locomotive(const RID &p_timetable, const String &p_locomotive);
            String timetable_get_required_locomotive(const RID &p_timetable) const;
            void timetable_set_required_mass(const RID &p_timetable, double p_tonnes);
            double timetable_get_required_mass(const RID &p_timetable) const;
            void timetable_set_required_brake_ratio(const RID &p_timetable, double p_percent);
            double timetable_get_required_brake_ratio(const RID &p_timetable) const;
            /// A station at the end of the timetable, planned to arrive and depart there (hours
            /// since midnight; NO_TIME for an arrival where the train passes); its index
            int timetable_add_entry(const RID &p_timetable, const RID &p_station, double p_arrival, double p_departure);
            int timetable_get_entry_count(const RID &p_timetable) const;
            RID timetable_entry_get_station(const RID &p_timetable, int p_entry) const;
            double timetable_entry_get_arrival(const RID &p_timetable, int p_entry) const;
            double timetable_entry_get_departure(const RID &p_timetable, int p_entry) const;
            /// Whether the train stops there - an arrival is planned
            bool timetable_entry_is_stop(const RID &p_timetable, int p_entry) const;
            void timetable_entry_set_speed_limit(const RID &p_timetable, int p_entry, double p_speed_limit);
            double timetable_entry_get_speed_limit(const RID &p_timetable, int p_entry) const;
            void timetable_entry_set_radio_channel(const RID &p_timetable, int p_entry, int p_channel);
            int timetable_entry_get_radio_channel(const RID &p_timetable, int p_entry) const;
            void timetable_entry_set_kilometre(const RID &p_timetable, int p_entry, double p_kilometre);
            double timetable_entry_get_kilometre(const RID &p_timetable, int p_entry) const;
            void timetable_entry_set_running_time(const RID &p_timetable, int p_entry, double p_minutes);
            double timetable_entry_get_running_time(const RID &p_timetable, int p_entry) const;
            /// A stop for the train's own sake: nobody gets on or off
            void timetable_entry_set_technical_stop(const RID &p_timetable, int p_entry, bool p_technical);
            bool timetable_entry_is_technical_stop(const RID &p_timetable, int p_entry) const;
            /// The train changes its direction at the entry
            void timetable_entry_set_direction_change(const RID &p_timetable, int p_entry, bool p_change);
            bool timetable_entry_is_direction_change(const RID &p_timetable, int p_entry) const;
            /// The line's tracks on the way to the entry, 1 or 2
            void timetable_entry_set_track_count(const RID &p_timetable, int p_entry, int p_track_count);
            int timetable_entry_get_track_count(const RID &p_timetable, int p_entry) const;
            /// What a plugin keeps of its own about the entry
            void timetable_entry_set_extra(const RID &p_timetable, int p_entry, const Dictionary &p_extra);
            Dictionary timetable_entry_get_extra(const RID &p_timetable, int p_entry) const;

            /// The trainset runs the timetable, its times shifted by p_offset hours, from its first
            /// entry - standing there where no arrival is planned. A trainset has one run at a time;
            /// coupled into another, its run goes with it where that one has none, and ends where
            /// it has; it ends with the trainset.
            RID run_create(const RID &p_timetable, const RID &p_trainset, double p_offset = 0.0);
            void run_free(const RID &p_run);
            RID run_get_timetable(const RID &p_run) const;
            RID run_get_trainset(const RID &p_run) const;
            double run_get_offset(const RID &p_run) const;
            /// The run of the trainset, RID() for none
            RID run_find_by_trainset(const RID &p_trainset) const;
            /// On to the next entry, back to the one before - by whoever drives the train
            void run_next_entry(const RID &p_run);
            void run_previous_entry(const RID &p_run);
            /// The entry the train drives to or stands at; the entry count past the last
            int run_get_current_entry(const RID &p_run) const;
            /// When the train arrived at, and left or passed, the entry [h]; NO_TIME until it did
            double run_entry_get_actual_arrival(const RID &p_run, int p_entry) const;
            double run_entry_get_actual_departure(const RID &p_run, int p_entry) const;
            /// How late the train is at p_time [min], early when negative: the delay recorded last,
            /// or more once the clock has passed the planned time of where it has not got to - the
            /// current entry's arrival, or its departure where it passes or stands. A train early at
            /// a stop waits for its departure there: it is on time, not early
            double run_get_delay(const RID &p_run, double p_time) const;
            /// The minutes from p_time to the departure from the current entry, 0 where the train
            /// passes it
            double run_get_time_to_departure(const RID &p_run, double p_time) const;

            /// The dispatch of the train driven from the vehicle, of the cars given: those whose
            /// load is being exchanged (RailVehicleServer.load_add/load_remove) are waited for, the
            /// train's earlier dispatch is dropped
            void dispatch_start(const RID &p_vehicle, const TypedArray<RID> &p_cars);
            /// The train may go as far as its timetable says: once the passengers are done, its
            /// doors close
            void dispatch_depart(const RID &p_vehicle);
            /// The dispatch is dropped - the train left the stop
            void dispatch_cancel(const RID &p_vehicle);
            DispatchStep dispatch_get_step(const RID &p_vehicle) const;
            /// The seconds the longest exchange of the dispatch still takes, 0 when none
            double dispatch_get_exchange_time(const RID &p_vehicle) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(StationServer::StationType)
VARIANT_ENUM_CAST(StationServer::DispatchStep)
