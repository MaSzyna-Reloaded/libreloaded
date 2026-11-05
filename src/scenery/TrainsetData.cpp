#include "TrainsetData.hpp"

namespace godot {
    void TrainsetData::_bind_methods() {
        BIND_PROPERTY(TrainsetData, Variant::STRING, track_name);
        BIND_PROPERTY(TrainsetData, Variant::FLOAT, offset);
        // the resource's own name (resource_name), bound as the property name too: Resource's own
        // accessors, bound as this class's (godot-cpp binds a method to the class it is of)
        ClassDB::bind_method(
                D_METHOD("set_name", "name"), static_cast<void (TrainsetData::*)(const String &)>(&Resource::set_name));
        ClassDB::bind_method(D_METHOD("get_name"), static_cast<String (TrainsetData::*)() const>(&Resource::get_name));
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR), "set_name",
                "get_name");
        BIND_PROPERTY(TrainsetData, Variant::STRING, timetable);
        BIND_PROPERTY(TrainsetData, Variant::FLOAT, velocity);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                TrainsetData, Variant::ARRAY, members, PROPERTY_HINT_TYPE_STRING, "TrainsetMemberData");
        BIND_PROPERTY(TrainsetData, Variant::STRING, description);
        BIND_PROPERTY(TrainsetData, Variant::BOOL, player_drivable);
        ClassDB::bind_method(D_METHOD("is_occupied"), &TrainsetData::is_occupied);
        ClassDB::bind_method(D_METHOD("get_driver_vehicle"), &TrainsetData::get_driver_vehicle);
        ClassDB::bind_method(D_METHOD("get_arranged_members", "arranged"), &TrainsetData::get_arranged_members);
    }

    bool TrainsetData::is_occupied() const {
        for (int64_t index = 0; index < members.size(); ++index) {
            const Ref<TrainsetMemberData> member = members[index];
            if (!(member->get_vehicle()->get_driver_type() == VehicleData::DRIVER_NOBODY)) {
                return true;
            }
        }
        return false;
    }

    Ref<VehicleData> TrainsetData::get_driver_vehicle() const {
        for (const VehicleData::DriverType driver_type: {VehicleData::DRIVER_HEAD, VehicleData::DRIVER_REAR}) {
            for (int64_t index = 0; index < members.size(); ++index) {
                const Ref<TrainsetMemberData> member = members[index];
                if (member->get_vehicle()->get_driver_type() == driver_type) {
                    return member->get_vehicle();
                }
            }
        }
        return members.is_empty() ? Ref<VehicleData>() : Ref<TrainsetMemberData>(members[0])->get_vehicle();
    }

    TypedArray<TrainsetMemberData>
    TrainsetData::get_arranged_members(const TypedArray<TrainsetMemberData> &p_arranged) const {
        Ref<TrainsetMemberData> first_declared;
        for (int64_t index = 0; index < p_arranged.size(); ++index) {
            const Ref<TrainsetMemberData> entry = p_arranged[index];
            if (entry->get_trainset_index() < 0) {
                continue;
            }
            if (entry->get_trainset_index() >= members.size() ||
                !(Ref<TrainsetMemberData>(members[entry->get_trainset_index()])->get_vehicle()->get_vehicle_id() ==
                  entry->get_vehicle()->get_vehicle_id())) {
                return members;
            }
            if (first_declared.is_null()) {
                first_declared = members[entry->get_trainset_index()];
            }
        }
        if (first_declared.is_null()) {
            return members;
        }
        TypedArray<TrainsetMemberData> result;
        for (int64_t index = 0; index < p_arranged.size(); ++index) {
            const Ref<TrainsetMemberData> entry = p_arranged[index];
            Ref<TrainsetMemberData> member;
            if (entry->get_trainset_index() >= 0) {
                const Ref<TrainsetMemberData> declared = members[entry->get_trainset_index()];
                member = declared->duplicate();
                member->set_vehicle(declared->get_vehicle()->duplicate());
            } else {
                const Ref<TrainsetMemberData> before =
                        result.is_empty() ? first_declared : Ref<TrainsetMemberData>(result.back());
                const Ref<VehicleData> vehicle = before->get_vehicle()->duplicate();
                vehicle->set_vehicle_id(vformat("%s_%d", first_declared->get_vehicle()->get_vehicle_id(), index));
                vehicle->set_name("");
                vehicle->set_driver_type(VehicleData::DRIVER_NOBODY);
                vehicle->set_load_name("");
                vehicle->set_load_amount(0.0);
                member.instantiate();
                member->set_vehicle(vehicle);
            }
            member->get_vehicle()->take_vehicle_of(entry->get_vehicle());
            member->set_direction(entry->get_direction());
            member->set_trainset_index(entry->get_trainset_index());
            member->set_coupling_mode(entry->get_coupling_mode());
            result.append(member);
        }
        return result;
    }
} // namespace godot
