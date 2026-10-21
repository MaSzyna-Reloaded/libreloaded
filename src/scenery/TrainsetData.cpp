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
                TrainsetData, Variant::ARRAY, vehicles, PROPERTY_HINT_TYPE_STRING, "VehicleData");
        BIND_PROPERTY(TrainsetData, Variant::STRING, description);
        BIND_PROPERTY(TrainsetData, Variant::BOOL, player_drivable);
        ClassDB::bind_method(D_METHOD("is_occupied"), &TrainsetData::is_occupied);
        ClassDB::bind_method(D_METHOD("get_driver_vehicle"), &TrainsetData::get_driver_vehicle);
        ClassDB::bind_method(D_METHOD("get_arranged_vehicles", "arranged"), &TrainsetData::get_arranged_vehicles);
    }

    bool TrainsetData::is_occupied() const {
        for (int64_t index = 0; index < vehicles.size(); ++index) {
            const Ref<VehicleData> vehicle = vehicles[index];
            if (!(vehicle->get_driver_type() == VehicleData::DRIVER_NOBODY)) {
                return true;
            }
        }
        return false;
    }

    Ref<VehicleData> TrainsetData::get_driver_vehicle() const {
        for (const VehicleData::DriverType driver_type: {VehicleData::DRIVER_HEAD, VehicleData::DRIVER_REAR}) {
            for (int64_t index = 0; index < vehicles.size(); ++index) {
                const Ref<VehicleData> vehicle = vehicles[index];
                if (vehicle->get_driver_type() == driver_type) {
                    return vehicle;
                }
            }
        }
        return vehicles.is_empty() ? Ref<VehicleData>() : Ref<VehicleData>(vehicles[0]);
    }

    TypedArray<VehicleData> TrainsetData::get_arranged_vehicles(const TypedArray<VehicleData> &p_arranged) const {
        Ref<VehicleData> first_declared;
        for (int64_t index = 0; index < p_arranged.size(); ++index) {
            const Ref<VehicleData> entry = p_arranged[index];
            if (entry->get_trainset_index() < 0) {
                continue;
            }
            if (entry->get_trainset_index() >= vehicles.size() ||
                !(Ref<VehicleData>(vehicles[entry->get_trainset_index()])->get_vehicle_id() ==
                  entry->get_vehicle_id())) {
                return vehicles;
            }
            if (first_declared.is_null()) {
                first_declared = vehicles[entry->get_trainset_index()];
            }
        }
        if (first_declared.is_null()) {
            return vehicles;
        }
        TypedArray<VehicleData> result;
        for (int64_t index = 0; index < p_arranged.size(); ++index) {
            const Ref<VehicleData> entry = p_arranged[index];
            Ref<VehicleData> vehicle;
            if (entry->get_trainset_index() >= 0) {
                vehicle = Ref<VehicleData>(vehicles[entry->get_trainset_index()])->duplicate();
            } else {
                const Ref<VehicleData> before = result.is_empty() ? first_declared : Ref<VehicleData>(result.back());
                vehicle = before->duplicate();
                vehicle->set_vehicle_id(vformat("%s_%d", first_declared->get_vehicle_id(), index));
                vehicle->set_name("");
                vehicle->set_driver_type(VehicleData::DRIVER_NOBODY);
                vehicle->set_load_name("");
                vehicle->set_load_amount(0.0);
                vehicle->set_gap(0.0);
            }
            vehicle->take_vehicle_of(entry);
            result.append(vehicle);
        }
        const int64_t joints = vehicles.size() - 1;
        for (int64_t index = 0; index < result.size(); ++index) {
            const Ref<VehicleData> vehicle = result[index];
            if (index == result.size() - 1) {
                vehicle->set_coupling(Ref<VehicleData>(vehicles.back())->get_coupling());
            } else if (index < joints) {
                vehicle->set_coupling(Ref<VehicleData>(vehicles[index])->get_coupling());
            } else if (joints > 0) {
                vehicle->set_coupling(Ref<VehicleData>(vehicles[joints - 1])->get_coupling());
            }
        }
        return result;
    }
} // namespace godot
