#pragma once
#include "vehicles/base/VehicleAppearance.hpp"

#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
    /* What a kind of road vehicle looks like: its models (VehicleAppearance) and the submodels of its
     * exterior model that are its wheels, turned as it runs by RoadVehicleRenderingServer */
    class RoadVehicleAppearance : public VehicleAppearance {
            GDCLASS(RoadVehicleAppearance, VehicleAppearance)

        private:
            PackedStringArray wheels;

        protected:
            static void _bind_methods();

        public:
            void set_wheels(const PackedStringArray &p_value);
            PackedStringArray get_wheels() const;
    };
} // namespace godot
