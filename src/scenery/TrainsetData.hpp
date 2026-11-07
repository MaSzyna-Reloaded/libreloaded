#pragma once
#include "macros.hpp"
#include "scenery/TrainsetMemberData.hpp"
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
            /* Its vehicles and how they stand, front first */
            TypedArray<TrainsetMemberData> members;
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
            MAKE_GS_NR(TypedArray<TrainsetMemberData>, members);
            MAKE_GS(String, description);
            MAKE_GS_NR(bool, player_drivable);

            /// A driver sits in one of its vehicles, so the player can take the trainset - the
            /// original's launcher refuses one without ("Trainset not occupied", scenery_list.cpp:135-153)
            bool is_occupied() const;
            /// The vehicle the player starts in: the one with a driver in its front cab, else in its
            /// rear one, else the first; null for a trainset without vehicles
            Ref<VehicleData> get_driver_vehicle() const;
            /// Its members as p_arranged lists them (SceneryContentNode.trainset_override) when every
            /// declared member there stands at its place in this trainset, its declared ones otherwise.
            /// An entry is paired with its declared member by its place in the trainset
            /// (TrainsetMemberData.trainset_index) - an id may repeat. A declared member becomes the
            /// entry's vehicle, turned its way - its crew, load and gap stay. An added one (no place)
            /// is a copy of the vehicle before it (of the first declared one, when it comes first) with
            /// nobody aboard, no load, no gap, an id of its own and no name. Each couples to the next
            /// by the entry's coupling mode - the one the arrangement shows and the player chose.
            TypedArray<TrainsetMemberData> get_arranged_members(const TypedArray<TrainsetMemberData> &p_arranged) const;
    };
} // namespace godot
