#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* One pantograph of a vehicle as it is built (RailVehiclePantographs.pantographs): the channel of
     * the power source it is raised by and feeds, and its geometry - where it stands and how its arms
     * are built. Configuration only; how far it is raised is the component's state. */
    class RailVehiclePantograph : public Resource {
            GDCLASS(RailVehiclePantograph, Resource);

        public:
            static void _bind_methods();

        private:
            RailVehicleEnginePowerSource::PantographSelector control = RailVehicleEnginePowerSource::PANTOGRAPH_FIRST;
            /* Whether the geometry below is known; one not known samples the wire where the vehicle
             * stands and always reaches it */
            bool has_geometry = false;
            /* Where the lower arm's pivot stands, in the vehicle's own space [m] */
            Vector3 position;
            double lower_length = 0.0;
            double upper_length = 0.0;
            /* The horizontal offset between the arms' ends [m] */
            double horizontal = 0.0;
            /* The arms' angles lowered [rad] */
            double lower_rest_angle = 0.0;
            double upper_rest_angle = 0.0;
            /* The slider's top over its pivot [m] */
            double slider_height = 0.0;

        public:
            MAKE_GS_NR(RailVehicleEnginePowerSource::PantographSelector, control);
            MAKE_GS(bool, has_geometry);
            MAKE_GS(Vector3, position);
            MAKE_GS(double, lower_length);
            MAKE_GS(double, upper_length);
            MAKE_GS(double, horizontal);
            MAKE_GS(double, lower_rest_angle);
            MAKE_GS(double, upper_rest_angle);
            MAKE_GS(double, slider_height);
    };
} // namespace godot
