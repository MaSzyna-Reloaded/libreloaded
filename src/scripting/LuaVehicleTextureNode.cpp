#include "LuaVehicleTextureNode.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void LuaVehicleTextureNode::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_vehicle", "vehicle"), &LuaVehicleTextureNode::set_vehicle);
        ClassDB::bind_method(D_METHOD("get_vehicle"), &LuaVehicleTextureNode::get_vehicle);
    }

    Vector<ScriptArgument> LuaVehicleTextureNode::_get_native_script_arguments() const {
        Vector<ScriptArgument> arguments = LuaTextureNode::_get_native_script_arguments();
        arguments.push_back(ScriptArgument{vehicle, ScriptHandleKind::VEHICLE});
        return arguments;
    }

    void LuaVehicleTextureNode::set_vehicle(const RID &p_vehicle) {
        vehicle = p_vehicle;
    }

    RID LuaVehicleTextureNode::get_vehicle() const {
        return vehicle;
    }
} // namespace godot
