#include "TrainsetMemberData.hpp"

namespace godot {
    void TrainsetMemberData::_bind_methods() {
        BIND_PROPERTY_W_HINT(TrainsetMemberData, Variant::OBJECT, vehicle, PROPERTY_HINT_RESOURCE_TYPE, "VehicleData");
        ClassDB::bind_method(D_METHOD("set_direction", "value"), &TrainsetMemberData::set_direction);
        ClassDB::bind_method(D_METHOD("get_direction"), &TrainsetMemberData::get_direction);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "direction", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "TrackServer.Direction"),
                "set_direction", "get_direction");
        BIND_PROPERTY(TrainsetMemberData, Variant::FLOAT, gap);
        ClassDB::bind_method(D_METHOD("set_coupling_mode", "value"), &TrainsetMemberData::set_coupling_mode);
        ClassDB::bind_method(D_METHOD("get_coupling_mode"), &TrainsetMemberData::get_coupling_mode);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "coupling_mode", PROPERTY_HINT_ENUM, RailVehicleController::coupling_mode_hint(),
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "RailVehicleController.CouplingMode"),
                "set_coupling_mode", "get_coupling_mode");
        BIND_PROPERTY(TrainsetMemberData, Variant::INT, trainset_index);
    }
} // namespace godot
