#pragma once
#include "vehicles/base/VehicleRenderingServer.hpp"

namespace godot {
    /* What a rail vehicle looks like, drawn (VehicleRenderingServer): where RailVehicleServer places
     * it on its track, its bogies turned towards their own track and its wheels by the distance
     * rolled (RailVehicleAppearance). A plugin's renderer derives from it and stands in its place
     * under the same singleton. */
    class RailVehicleRenderingServer : public VehicleRenderingServer {
            GDCLASS(RailVehicleRenderingServer, VehicleRenderingServer)

        private:
            static RailVehicleRenderingServer *singleton;

        protected:
            static void _bind_methods() {}

            bool _vehicle_get_placement(const RID &p_vehicle, Transform3D &p_r_transform) const override;
            void _bind_running_gear(const RID &p_vehicle, Visual &p_visual) override;
            void _pose_running_gear(const RID &p_vehicle, Visual &p_visual) override;

        public:
            static RailVehicleRenderingServer *get_instance();

            RailVehicleRenderingServer();
            ~RailVehicleRenderingServer() override;
    };
} // namespace godot
