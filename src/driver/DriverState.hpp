#pragma once
#include "vehicles/base/VehicleController.hpp"

#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /* What a driver does and wants now, as DriverServer answered a query (driver_get_state()),
     * filled by its implementation (DriverImplementation::get_state()). A snapshot - it does not
     * follow the driver, a holder asks again. Whatever the implementation thinks with, every field
     * means the same: what it shows the player of the driver. */
    class DriverState : public RefCounted {
            GDCLASS(DriverState, RefCounted)

        public:
            /// Why the driver keeps its train standing (the speed limit is 0)
            enum StopReason {
                /// It does not
                STOP_REASON_NONE,
                /// A stop ahead - the place it is to stand at
                STOP_REASON_AHEAD,
                /// Its vehicle is not ready to go (stop_reason_details says what is missing)
                STOP_REASON_NOT_READY,
                /// It has no order to go
                STOP_REASON_WAITING_FOR_ORDERS,
                /// A signal shows stop
                STOP_REASON_SIGNAL,
                /// The station has not dispatched the train yet
                STOP_REASON_DISPATCH,
            };

        private:
            String task;
            bool waiting_for_orders = true;
            VehicleController::Direction direction = VehicleController::DIRECTION_FORWARD;
            double speed_limit = 0.0;
            double next_speed_limit = 0.0;
            double next_speed_limit_distance = 0.0;
            StopReason stop_reason = STOP_REASON_NONE;
            PackedStringArray stop_reason_details;
            bool at_stop = false;
            double stop_time_left = 0.0;
            Array hints;

        protected:
            static void _bind_methods();

        public:
            DriverState();

            /// What the driver is doing, translated; empty for nothing it names
            void set_task(const String &p_task);
            String get_task() const;
            /// It has nothing to do but wait for an order
            void set_waiting_for_orders(bool p_waiting_for_orders);
            bool get_waiting_for_orders() const;
            /// The way it is to drive its vehicle
            void set_direction(VehicleController::Direction p_direction);
            VehicleController::Direction get_direction() const;
            /// The speed it may drive at now [km/h]: 0 when it is to stand, StationServer's
            /// NO_SPEED_LIMIT for no limit
            void set_speed_limit(double p_speed_limit);
            double get_speed_limit() const;
            /// The next change of the speed limit [km/h] (NO_SPEED_LIMIT: the limit ends) and the
            /// distance to it [m]; with no change ahead the speed limit itself, at INF
            void set_next_speed_limit(double p_next_speed_limit);
            double get_next_speed_limit() const;
            void set_next_speed_limit_distance(double p_next_speed_limit_distance);
            double get_next_speed_limit_distance() const;
            /// Why it keeps the train standing
            void set_stop_reason(StopReason p_stop_reason);
            StopReason get_stop_reason() const;
            /// What the stop reason comes from, translated: what the vehicle misses to be ready
            void set_stop_reason_details(const PackedStringArray &p_stop_reason_details);
            PackedStringArray get_stop_reason_details() const;
            /// It stands at a passenger stop; how long it is to stand yet [s], 0 when no wait holds it
            void set_at_stop(bool p_at_stop);
            bool get_at_stop() const;
            void set_stop_time_left(double p_stop_time_left);
            double get_stop_time_left() const;
            /// The steps it would take now, in the order it decided on them (DriverHint,
            /// addons/libreloaded/driver/driver_hint.gd); empty while it is aboard no vehicle
            void set_hints(const Array &p_hints);
            Array get_hints() const;
    };
} // namespace godot

VARIANT_ENUM_CAST(DriverState::StopReason)
