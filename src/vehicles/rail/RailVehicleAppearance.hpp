#pragma once
#include "vehicles/base/VehicleAppearance.hpp"

#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
    /* What a kind of rail vehicle looks like: its models (VehicleAppearance) and the submodels of
     * its exterior model the running gear turns - bogies and wheels, handed to
     * RailVehicleRenderingServer */
    class RailVehicleAppearance : public VehicleAppearance {
            GDCLASS(RailVehicleAppearance, VehicleAppearance)

        private:
            String front_bogie;
            String rear_bogie;
            PackedStringArray front_rolling_wheels;
            PackedStringArray powered_wheels;
            PackedStringArray rear_rolling_wheels;

        protected:
            static void _bind_methods();

        public:
            void set_front_bogie(const String &p_value);
            String get_front_bogie() const;
            void set_rear_bogie(const String &p_value);
            String get_rear_bogie() const;
            void set_front_rolling_wheels(const PackedStringArray &p_value);
            PackedStringArray get_front_rolling_wheels() const;
            void set_powered_wheels(const PackedStringArray &p_value);
            PackedStringArray get_powered_wheels() const;
            void set_rear_rolling_wheels(const PackedStringArray &p_value);
            PackedStringArray get_rear_rolling_wheels() const;
    };
} // namespace godot
