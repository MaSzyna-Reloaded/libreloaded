#include "LuaCabinTextureNode.hpp"
#include "ScenarioScriptServer.hpp"
#include "cabin/Cabin3D.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    void LuaCabinTextureNode::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_cabin_implementation", "cabin_implementation"),
                &LuaCabinTextureNode::set_cabin_implementation);
        ClassDB::bind_method(D_METHOD("get_cabin_implementation"), &LuaCabinTextureNode::get_cabin_implementation);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::OBJECT, "cabin_implementation", PROPERTY_HINT_RESOURCE_TYPE,
                        "ScenarioScriptCabinImplementation"),
                "set_cabin_implementation", "get_cabin_implementation");
    }

    /// The nearest Cabin3D above it, as a cab's elements take theirs (BaseCabinTool3D.cabin_of())
    void LuaCabinTextureNode::_script_starting(const RID &p_context) {
        LuaVehicleTextureNode::_script_starting(p_context);
        cabin = RID();
        set_vehicle(RID());
        for (Node *ancestor = get_parent(); ancestor != nullptr; ancestor = ancestor->get_parent()) {
            if (const Cabin3D *cab = Object::cast_to<Cabin3D>(ancestor)) {
                cabin = cab->get_cabin();
                set_vehicle(cab->get_vehicle_rid());
                break;
            }
        }
        if (cabin_implementation.is_valid()) {
            ScenarioScriptServer *scripts = ScenarioScriptServer::get_instance();
            ERR_FAIL_NULL(scripts);
            scripts->context_attach_cabin_implementation(p_context, cabin_implementation);
        }
    }

    Vector<ScriptArgument> LuaCabinTextureNode::_get_native_script_arguments() const {
        Vector<ScriptArgument> arguments = LuaVehicleTextureNode::_get_native_script_arguments();
        arguments.push_back(ScriptArgument{cabin, ScriptHandleKind::CABIN});
        return arguments;
    }

    void LuaCabinTextureNode::set_cabin_implementation(
            const Ref<ScenarioScriptCabinImplementation> &p_cabin_implementation) {
        cabin_implementation = p_cabin_implementation;
    }

    Ref<ScenarioScriptCabinImplementation> LuaCabinTextureNode::get_cabin_implementation() const {
        return cabin_implementation;
    }
} // namespace godot
