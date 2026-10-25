#pragma once
#include "macros.hpp"
#include "utils/Units.hpp"
#include "vehicles/base/VehicleController.hpp"

#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
    /// A vehicle of the road: no rails, couplers or cabs, and no backend - it moves by its own
    /// kinematics, its velocity going towards the one RoadVehicleServer allows within its
    /// acceleration limit. Where it is on the road, what is ahead of it and how fast that lets it go
    /// are RoadVehicleServer's, which steps every road vehicle in one loop (IMPLEMENTATION_NAME of
    /// VehicleServer). What is driven from outside - the velocity wanted and the place across the
    /// road - comes as commands.
    class RoadVehicleController : public VehicleController {
            GDCLASS(RoadVehicleController, VehicleController)

        public:
            /* Its lights: the head lights at its front, the tail lights at its rear */
            enum Lights {
                LIGHTS_NONE = 0,
                LIGHTS_HEAD = 1,
                LIGHTS_TAIL = 2,
            };
            /* Its horns sounding: the low and the high one */
            enum Horns {
                HORNS_NONE = 0,
                HORNS_LOW = 1,
                HORNS_HIGH = 2,
            };
            /* km/h in one m/s - the velocities wanted and allowed are in km/h, as a track's limit */
            static constexpr double KMH_PER_MPS =
                    static_cast<double>(Units::SECONDS_PER_HOUR) / static_cast<double>(Units::METRES_PER_KILOMETRE);
            /* What the configuration starts with, for a vehicle described without it */
            static constexpr double DEFAULT_FOLLOWING_GAP = 2.0;
            static constexpr double DEFAULT_ACCELERATION_LIMIT = 1.0;
            static constexpr double DEFAULT_OBSTACLE_RANGE = 100.0;

        private:
            /* The configuration: the gap kept to the vehicle ahead [m], the acceleration and
             * deceleration it never exceeds [m/s2], and how far ahead it looks for what stops it [m] */
            double following_gap = DEFAULT_FOLLOWING_GAP;
            double acceleration_limit = DEFAULT_ACCELERATION_LIMIT;
            double obstacle_range = DEFAULT_OBSTACLE_RANGE;
            /* Its wheels' diameter [m] - none turn without one - and how many times its engine
             * turns for a turn of the wheels */
            double wheel_diameter = 0.0;
            double transmission_ratio = 1.0;

            /* The state: the velocity it wants [km/h] and its place across the road, from the
             * road's centre line to the right of the way it travels [m] - both commanded - then its
             * velocity along its front [m/s], its acceleration [m/s2] and the distance run [m] */
            double wanted_velocity = 0.0;
            double side_offset = 0.0;
            BitField<Lights> lights = LIGHTS_NONE;
            BitField<Horns> horns = HORNS_NONE;
            bool engine_running = false;
            double velocity = 0.0;
            double acceleration = 0.0;
            double total_distance = 0.0;
            bool simulation_ready = false;

        protected:
            static void _bind_methods();
            void _initialize_simulation() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _fill_state_dictionary(Dictionary &p_state) const override;
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            RoadVehicleController();

            MAKE_GS(double, following_gap);
            MAKE_GS(double, acceleration_limit);
            MAKE_GS(double, obstacle_range);
            MAKE_GS(double, wheel_diameter);
            MAKE_GS(double, transmission_ratio);

            /// Commands: the velocity it wants [km/h, 0 or more], its place across the road [m] and
            /// the lights it has on
            void wanted_velocity_set(double p_velocity);
            void side_offset_set(double p_offset);
            void lights_set(BitField<Lights> p_lights);
            BitField<Lights> get_lights() const;
            /// Commands: the horns sounding now, and its engine started or stopped
            void horns_set(BitField<Horns> p_horns);
            BitField<Horns> get_horns() const;
            void engine_start();
            void engine_stop();
            bool is_engine_running() const;
            /// The engine's revolutions [1/s] - the wheels' times the transmission ratio
            double get_engine_revolutions() const;
            /// How much it accelerates, and how hard it brakes, against its acceleration limit
            /// [0-1] - what a driver's pedals would give: it has no engine or brakes of its own
            double get_throttle() const;
            double get_brake_force_ratio() const;
            double get_wanted_velocity() const;
            double get_side_offset() const;
            /// How far its wheels have turned by the distance run [deg, 0-360)
            double get_wheels_angle_deg() const;

            /// One step of p_delta [s]: the velocity goes towards p_allowed_velocity [m/s] within the
            /// acceleration limit; the distance it ran [m] is returned, for RoadVehicleServer to move
            /// it by. Called by RoadVehicleServer only.
            double process_movement(double p_delta, double p_allowed_velocity);

            void release() override;
            bool is_simulation_ready() const override;
            double get_velocity() const override;
            double get_speed() const override;
            double get_acceleration() const override;
            double get_mass_total() const override;
            double get_total_distance() const override;
            Direction get_direction() const override;
            void apply_config() override {}
            bool is_physics_active() const override;
    };
} // namespace godot

VARIANT_BITFIELD_CAST(RoadVehicleController::Lights);
VARIANT_BITFIELD_CAST(RoadVehicleController::Horns);
