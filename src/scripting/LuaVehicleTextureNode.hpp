#pragma once
#include "LuaTextureNode.hpp"

namespace godot {
    /// A LuaTextureNode drawing for a vehicle - its destination sign, a screen of its cab: the script
    /// gets the vehicle after the drawing (`local drawing, vehicle = ...`), nil for none.
    class LuaVehicleTextureNode : public LuaTextureNode {
            GDCLASS(LuaVehicleTextureNode, LuaTextureNode)

        private:
            RID vehicle;

        protected:
            static void _bind_methods();

            Vector<ScriptArgument> _get_native_script_arguments() const override;

        public:
            /// The vehicle the script is handed as it starts
            void set_vehicle(const RID &p_vehicle);
            RID get_vehicle() const;
    };
} // namespace godot
