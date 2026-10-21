#pragma once
#include "macros.hpp"
#include "scenery/VehicleData.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* A trainset of a scenery - its vehicles stand on the track one after another from the offset and
     * are coupled (RailVehicleServer.trainset_place()); a vehicle outside any trainset is a trainset
     * of one, standing by its own track and offset. Its name - empty for a vehicle outside a
     * trainset - is the resource's name. */
    class TrainsetData : public Resource {
            GDCLASS(TrainsetData, Resource)

        private:
            /* The track its front stands on, and the distance along it [m] */
            String track_name;
            double offset = 0.0;
            /* The timetable of its driver and the velocity it starts with - the driver's first orders;
             * empty for a vehicle outside a trainset */
            String timetable;
            double velocity = 0.0;
            /* Its vehicles, front first */
            TypedArray<VehicleData> vehicles;
            /* What the scenario says the train is to do, and whether it offers the trainset to the
             * player (RailVehicleServer.trainset_set_description(), trainset_set_player_drivable()) */
            String description;
            bool player_drivable = true;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(String, track_name);
            MAKE_GS_NR(double, offset);
            MAKE_GS(String, timetable);
            MAKE_GS_NR(double, velocity);
            MAKE_GS_NR(TypedArray<VehicleData>, vehicles);
            MAKE_GS(String, description);
            MAKE_GS_NR(bool, player_drivable);

            /// A driver sits in one of its vehicles, so the player can take the trainset - the
            /// original's launcher refuses one without ("Trainset not occupied", scenery_list.cpp:135-153)
            bool is_occupied() const;
            /// The vehicle the player starts in: the one with a driver in its front cab, else in its
            /// rear one, else the first; null for a trainset without vehicles
            Ref<VehicleData> get_driver_vehicle() const;
            /// Its vehicles as p_arranged lists them (SceneryContentNode.trainset_override) when every
            /// declared vehicle there stands at its place in this trainset, its declared ones otherwise.
            /// An entry is paired with its declared vehicle by its place in the trainset
            /// (VehicleData.trainset_index), as the original couples a trainset by position
            /// (simulationstateserializer.cpp:818-834) - an id may repeat. A declared vehicle becomes
            /// the entry's vehicle, turned its way - its crew and load stay. An added one (no place)
            /// gets the velocity of the vehicle before it (of the first declared one, when it comes
            /// first), nobody aboard, no load, no gap, an id of its own and no name. The couplings are
            /// the places': each vehicle couples to the next as the declared vehicle at its place did,
            /// a vehicle moved from the end leaving no uncoupled joint behind it - past the declared
            /// joints as the last of them, the last vehicle as the declared last one.
            TypedArray<VehicleData> get_arranged_vehicles(const TypedArray<VehicleData> &p_arranged) const;
    };
} // namespace godot
