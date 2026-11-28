#pragma once

#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"

#include <godot_cpp/templates/vector.hpp>

namespace godot {
    class RailVehicleWheels : public RailVehicleComponent {
            GDCLASS(RailVehicleWheels, RailVehicleComponent)


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_WHEELS;
            }

        private:
            static void _bind_methods();
            /* The diameter TMoverParameters keeps when the data gives none (MOVER.h:1595) */
            static constexpr double DEFAULT_WHEEL_DIAMETER = 1.0;

        protected:
            /* The diameters the data gives, the powered wheels' for a rolling one it leaves out
             * (LoadFIZ_Wheels) [m] */
            double _get_resolved_powered_wheel_diameter() const;
            double _get_resolved_front_rolling_wheel_diameter() const;
            double _get_resolved_rear_rolling_wheel_diameter() const;

        public:
            /* How far each group of wheels has turned [deg] */
            virtual double get_angle_front_deg() const = 0;
            virtual double get_angle_powered_deg() const = 0;
            virtual double get_angle_rear_deg() const = 0;
            /* The axles the arrangement counts, and how far one of them has turned by its place
             * from the front [deg]: its group's angle - a powered axle's, a rolling one's before
             * the powered ones or after them; 0 for an axle past the last */
            virtual int get_axle_count() const;
            virtual double get_axle_angle_deg(int p_axle) const;
            /* Live state, read straight from the backend - nothing is stored. */
            virtual double get_rotation_speed_rps() const = 0;
            virtual double get_rotation_acceleration_rps2() const = 0;
            virtual bool get_slipping() const = 0;
            virtual double get_flat() const = 0;
            /// Which of the vehicle's two bogies (RailVehicleServer::bogie_get_transform()).
            enum Bogie {
                BOGIE_FRONT = 0,
                BOGIE_REAR = 1,
            };

            enum BearingType {
                BEARING_TYPE_SLIDE = 0,
                BEARING_TYPE_ROLL = 1,
            };

        private:
            double powered_wheel_diameter = 0.0;
            double front_rolling_wheel_diameter = 0.0;
            double rear_rolling_wheel_diameter = 0.0;
            double axle_inertial_moment = 0.0;
            double track_width = 1.435;
            String axle_arrangement = "";
            double bogie_axle_spacing = 0.0;
            double bogie_pivot_spacing = 0.0;
            double minimum_curve_radius = 0.0;
            int bearing_type = BEARING_TYPE_ROLL;
            /* Each axle's group by its place from the front, as the arrangement reads
             * (set_axle_arrangement()) */
            enum AxleGroup {
                AXLE_GROUP_FRONT_ROLLING,
                AXLE_GROUP_POWERED,
                AXLE_GROUP_REAR_ROLLING,
            };
            Vector<AxleGroup> axle_groups;

        public:
            MAKE_GS(double, powered_wheel_diameter);
            MAKE_GS(double, front_rolling_wheel_diameter);
            MAKE_GS(double, rear_rolling_wheel_diameter);
            MAKE_GS(double, axle_inertial_moment);
            MAKE_GS(double, track_width);
            const String &get_axle_arrangement() const;
            void set_axle_arrangement(const String &p_axle_arrangement);
            MAKE_GS(double, bogie_axle_spacing);
            MAKE_GS(double, bogie_pivot_spacing);
            MAKE_GS(double, minimum_curve_radius);
            MAKE_GS(int, bearing_type);
    };
} // namespace godot

VARIANT_ENUM_CAST(godot::RailVehicleWheels::BearingType);
VARIANT_ENUM_CAST(godot::RailVehicleWheels::Bogie);
