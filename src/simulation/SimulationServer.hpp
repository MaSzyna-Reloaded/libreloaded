#pragma once
#include <godot_cpp/variant/vector3.hpp>

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {

    /// The state of the world the whole simulation shares - the light level and the air
    /// temperature as the environment publishes them (Global.fLuminance, Global.AirTemperature) -
    /// and runs the one clock of the simulation (Timer::UpdateTimers(), Timer.cpp:70-92): the
    /// physics, the events, the drivers and the time of day all advance by the same seconds, read
    /// here.
    class SimulationServer : public Object {
            GDCLASS(SimulationServer, Object)

        public:
            /* What falls from the sky (Global.Weather, simulationenvironment.cpp:198) */
            enum Weather {
                WEATHER_CLEAR,
                WEATHER_CLOUDY,
                WEATHER_RAIN,
                WEATHER_SNOW,
            };

            /* The season of the year, by the day of it (SEASON_*_LAST_DAY) */
            enum Season {
                SEASON_WINTER,
                SEASON_SPRING,
                SEASON_SUMMER,
                SEASON_AUTUMN,
            };

        private:
            static SimulationServer *singleton;

            /// A frame counts at most this much real time [s], then times the simulation speed. The
            /// original caps the simulated time instead, at 1 s (Timer.cpp:84), because its Mover
            /// went wrong on long frames; here the couplers are refreshed every physics step and
            /// the drivers and events run in slices, so a long frame simulates the same - x100 is
            /// x100, whatever it costs in frames. What stays capped is the real time a frame may
            /// count: one hitch (loading, a debugger) is not taken whole and multiplied, so the
            /// simulation cannot spiral into ever longer frames; under 4 fps it runs slower.
            static constexpr double MAX_FRAME_DELTA = 0.25;
            /// The running speed this near the one set is at it
            static constexpr double SPEED_SETTLED = 0.001;
            /// A frame is simulated in slices no longer than this [s], each announced on its own:
            /// what reacts on the clock between frames - a driver reacts every 0.1 s at the
            /// quickest (ReactionTime, Driver.cpp:7501), an event runs in its own pass - then does
            /// the same whether a frame is short or long. The physics steps each slice in its own
            /// 0.01 s steps.
            static constexpr double MAX_SLICE_TIME = 0.1;

            double time_of_day = DEFAULT_TIME_OF_DAY;
            double simulation_time = 0.0;
            double simulation_speed = 1.0;
            /// The speed the clock runs at, going to simulation_speed over speed_change_time
            double current_simulation_speed = 1.0;
            /// [s], taken from the Project Setting when a speed is set
            double speed_change_time = 0.0;
            /// Who needs the clock running, and whether it runs - on SceneTree's `process_frame`,
            /// which comes before any node's `_process`, so every reader sees this frame's step
            int clock_holders = 0;
            bool clock_running = false;
            /// The SimulationRuntime nodes in the tree: the clock runs under at least one; with
            /// none (the editor, a viewer) it stands, whoever holds it
            int runtimes = 0;
            /* The calendar day the time of day belongs to (date_set()), carried over past midnight */
            int date_year = DEFAULT_YEAR;
            int date_month = DEFAULT_MONTH;
            int date_day = DEFAULT_DAY;
            /* Hours from UTC of the time of day */
            int timezone_offset = DEFAULT_TIMEZONE_OFFSET;
            /* Where the sun is seen from [deg] */
            double latitude = DEFAULT_LATITUDE;
            double longitude = DEFAULT_LONGITUDE;
            /* 0-1, the weather's; dims the daylight */
            double cloud_cover = DEFAULT_CLOUD_COVER;
            /* Global.fFriction: the scenery's rail friction multiplier (Globals.h:95) */
            double friction = DEFAULT_FRICTION;
            Weather weather = WEATHER_CLEAR;
            /* 0-1, how hard it rains or snows */
            double precipitation = 0.0;
            /* [m/s], and the horizontal unit vector it blows along */
            double wind_speed = 0.0;
            Vector3 wind_direction = Vector3(1.0, 0.0, 0.0);
            /* Computed from the date, never set (_update_season()) */
            Season season = SEASON_SUMMER;
            double air_temperature = DEFAULT_AIR_TEMPERATURE;
            /* The time of day and the date follow the system's clock instead of the simulation's */
            bool use_system_time = false;
            /* Seconds the system's local time is ahead of UTC, taken when use_system_time is set */
            int64_t system_time_bias = 0;
            /* Computed, never set (_update_light_level()) */
            double light_level = 1.0;
            /* LIGHT_LEVEL_NIGHT_ALTITUDE_SETTING and LIGHT_LEVEL_DAY_ALTITUDE_SETTING */
            double light_night_altitude = LIGHT_LEVEL_NIGHT_ALTITUDE_DEFAULT;
            double light_day_altitude = LIGHT_LEVEL_DAY_ALTITUDE_DEFAULT;
            bool paused = false;

            void _refresh_clock();
            void _on_process_frame();
            /* The time of day moved from p_previous: a minute, an hour or a day passed is said so,
             * and the light follows the sun */
            void _time_of_day_moved(double p_previous);
            /* The light level of the sun where and when it is, dimmed by the clouds; announced on a
             * change */
            void _update_light_level();
            void _update_season();
            /// The day of the year of the date, 1 on the 1st of January
            int _get_day_of_year() const;

            /// The speed's change time follows its setting
            void _on_project_settings_changed();

        protected:
            static void _bind_methods();

        public:
            SimulationServer();
            ~SimulationServer() override;
            static const char *simulation_advanced_signal;
            static const char *simulation_paused_signal;
            static const char *simulation_unpaused_signal;
            static const char *simulation_speed_changed_signal;
            static const char *simulation_current_speed_changed_signal;
            /// The Project Setting of how long the running speed takes to reach one set [s]
            static constexpr const char *SPEED_CHANGE_TIME_SETTING = "reloaded/simulation/speed_change_time";
            /// ... when the project does not set it (libmaszyna.gd registers the same) [s]
            static constexpr double SPEED_CHANGE_TIME_DEFAULT = 0.4;
            static const char *time_of_day_changed_signal;
            static const char *time_of_day_hour_changed_signal;
            static const char *date_changed_signal;
            static const char *light_level_changed_signal;
            /// The weather, the precipitation or the wind changed
            static const char *weather_changed_signal;
            /// The date reached another season
            static const char *season_changed_signal;
            /// The last day of the year of each season but the second winter - the year's day the
            /// materials of the scenery change their season at
            static constexpr int SEASON_WINTER_LAST_DAY = 65;
            static constexpr int SEASON_SPRING_LAST_DAY = 158;
            static constexpr int SEASON_SUMMER_LAST_DAY = 252;
            static constexpr int SEASON_AUTUMN_LAST_DAY = 341;
            /// Sun altitude [deg] between which the light level ramps from night to full day. The
            /// original lights a scenery light set to "on when dark" below a light level of 0.325
            /// (AnimModel.cpp:598), which on this ramp falls at about 1.4 degrees below the
            /// horizon. Both ends have to stay clear of a winter noon - at 50 N the sun peaks at
            /// 16-19 degrees in January (FINDINGS.md). libmaszyna.gd registers the same.
            static constexpr const char *LIGHT_LEVEL_NIGHT_ALTITUDE_SETTING = "reloaded/lights/night_altitude";
            static constexpr const char *LIGHT_LEVEL_DAY_ALTITUDE_SETTING = "reloaded/lights/day_altitude";
            static constexpr double LIGHT_LEVEL_NIGHT_ALTITUDE_DEFAULT = -6.0;
            static constexpr double LIGHT_LEVEL_DAY_ALTITUDE_DEFAULT = 6.0;
            /// How much a full cloud cover dims the daylight (LIBMASZYNA_SPLIT_SPECS.md, 5)
            static constexpr double LIGHT_LEVEL_OVERCAST_FACTOR = 0.65;
            /// What the environment starts with until a scenery sets its own - EnvironmentNode's
            /// defaults
            static constexpr double DEFAULT_TIME_OF_DAY = 8.0;
            static constexpr int DEFAULT_YEAR = 2026;
            static constexpr int DEFAULT_MONTH = 5;
            static constexpr int DEFAULT_DAY = 3;
            static constexpr int DEFAULT_TIMEZONE_OFFSET = 1;
            static constexpr double DEFAULT_LATITUDE = 50.271;
            static constexpr double DEFAULT_LONGITUDE = 19.04;
            static constexpr double DEFAULT_CLOUD_COVER = 0.5;
            static constexpr double DEFAULT_FRICTION = 1.0;
            /* Global.FrictionWeatherFactor of rain and of snow (simulationenvironment.cpp:198-201) */
            static constexpr double RAIN_FRICTION_FACTOR = 0.85;
            static constexpr double SNOW_FRICTION_FACTOR = 0.75;
            static constexpr double DEFAULT_AIR_TEMPERATURE = 15.0;

            static SimulationServer *get_instance() {
                return singleton;
            }

            /// Hours since midnight, fractional. Setting it jumps the clock (a scenario's start, the
            /// player's change, the system time); running, it advances with the simulation.
            void set_time_of_day(double p_hours);
            double get_time_of_day() const;
            /// Simulated seconds so far (fSimulationTime, Timer.cpp:87)
            double simulation_get_time() const;
            /// The clock runs while somebody holds it and the runtime is not paused: whoever needs
            /// time to pass holds it while it does, and lets it go
            void clock_hold();
            void clock_release();
            /// A SimulationRuntime node entering and leaving the tree: the clock runs only while
            /// one is attached
            void runtime_attach();
            void runtime_detach();
            /// Holds the clock and receives every `simulation_advanced` slice until unsubscribed
            void clock_subscribe(const Callable &p_on_advanced);
            void clock_unsubscribe(const Callable &p_on_advanced);
            /// One frame of the clock, `p_frame_delta` real seconds (Timer::UpdateTimers(),
            /// Timer.cpp:79-87): at most MAX_FRAME_DELTA of it, times the simulation speed, in
            /// slices of at most MAX_SLICE_TIME - each added to the simulation time and the time
            /// of day, then `simulation_advanced(seconds)`. Called by the clock's node, processed
            /// before every other.
            void simulation_advance(double p_frame_delta);
            /// How many simulated seconds pass in one real second (Global.fTimeSpeed, Timer.cpp:80) -
            /// the speed set; the clock gets to it like a tape's motor, over the Project Setting's
            /// SPEED_CHANGE_TIME_SETTING (simulation_get_current_speed())
            void set_simulation_speed(double p_speed);
            double get_simulation_speed() const;
            /// The speed the clock runs at now, on its way to the one set
            double simulation_get_current_speed() const;
            /// The wall clock's speed, at once rather than over the speed change time - a scenery
            /// starts at it, and the menu is heard at it
            void simulation_reset_speed();
            /// How bright the scene is, 0-1: daylight from the sun's altitude, dimmed by the cloud
            /// cover - computed here, never set; light_level_changed says when it moves
            double get_light_level() const;
            /// Degrees Celsius
            void set_air_temperature(double p_temperature);
            double get_air_temperature() const;
            /// The day the time of day belongs to; a day or a month past its end carries over (the
            /// 32nd of January is the 1st of February). date_changed when it is set or the clock
            /// passes midnight.
            void date_set(int p_year, int p_month, int p_day);
            int date_get_year() const;
            int date_get_month() const;
            int date_get_day() const;
            void set_timezone_offset(int p_hours);
            int get_timezone_offset() const;
            /// Decimal degrees, north and east positive
            void set_latitude(double p_degrees);
            double get_latitude() const;
            void set_longitude(double p_degrees);
            double get_longitude() const;
            /// 0-1
            void set_cloud_cover(double p_cover);
            double get_cloud_cover() const;
            /// The scenery's rail friction multiplier (Global.fFriction; the scenery's "friction" event)
            void set_friction(double p_friction);
            double get_friction() const;
            /// What falls from the sky
            void set_weather(Weather p_weather);
            Weather get_weather() const;
            /// 0-1, how hard it rains or snows
            void set_precipitation(double p_precipitation);
            double get_precipitation() const;
            /// The wind [m/s], and the horizontal unit vector it blows along
            void set_wind(double p_speed, const Vector3 &p_direction);
            double get_wind_speed() const;
            Vector3 get_wind_direction() const;
            /// The season of the date
            Season get_season() const;
            /// The rails' friction under the weather: wet and snowy rails grip less
            /// (Global.FrictionWeatherFactor, simulationenvironment.cpp:198-201)
            double simulation_get_weather_friction_factor() const;
            void set_use_system_time(bool p_use);
            bool get_use_system_time() const;
            /// The time of day, the date, the location, the cloud cover and the temperature back to
            /// the defaults (DEFAULT_*) - what an environment starts from before its scene sets its
            /// own (EnvironmentNode)
            void environment_reset();
            /// Stops the world while something covers it (a loading screen, the spinner of "Exit to
            /// menu"): whoever runs a part of the simulation, or its sound, holds it on "simulation_paused"
            /// and lets it go on "simulation_unpaused"
            void simulation_pause();
            void simulation_unpause();
            bool simulation_is_paused() const;
    };

} // namespace godot

VARIANT_ENUM_CAST(SimulationServer::Weather);
VARIANT_ENUM_CAST(SimulationServer::Season);
