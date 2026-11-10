#pragma once
#include "LuaVehicleTextureNode.hpp"
#include "ScenarioScriptCabinImplementation.hpp"

namespace godot {
    /// A LuaVehicleTextureNode of a driver's cab - a screen of its dashboard. It sits under the cab's
    /// Cabin3D and hands the script the cab's vehicle and cabin after the drawing
    /// (`local drawing, vehicle, cabin = ...`); with a cabin implementation the script reaches the
    /// cab's controls too (reloaded.cabin).
    class LuaCabinTextureNode : public LuaVehicleTextureNode {
            GDCLASS(LuaCabinTextureNode, LuaVehicleTextureNode)

        private:
            Ref<ScenarioScriptCabinImplementation> cabin_implementation;
            /// The cabin of the Cabin3D it sits in, taken as the script starts with its vehicle
            RID cabin;

        protected:
            static void _bind_methods();

            void _script_starting(const RID &p_context) override;
            Vector<ScriptArgument> _get_native_script_arguments() const override;

        public:
            void set_cabin_implementation(const Ref<ScenarioScriptCabinImplementation> &p_cabin_implementation);
            Ref<ScenarioScriptCabinImplementation> get_cabin_implementation() const;
    };
} // namespace godot
