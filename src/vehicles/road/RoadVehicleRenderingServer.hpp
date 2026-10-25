#pragma once
#include "vehicles/base/VehicleRenderingServer.hpp"

namespace godot {
    /* What a road vehicle looks like, drawn (VehicleRenderingServer): where RoadVehicleServer places
     * it on its road, its wheels turned by the distance run (RoadVehicleAppearance). A plugin's
     * renderer derives from it and stands in its place under the same singleton. */
    class RoadVehicleRenderingServer : public VehicleRenderingServer {
            GDCLASS(RoadVehicleRenderingServer, VehicleRenderingServer)

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline RoadVehicleRenderingServer *singleton = nullptr;

        protected:
            static void _bind_methods() {}

            bool _vehicle_get_placement(const RID &p_vehicle, Transform3D &p_r_transform) const override;
            void _bind_running_gear(const RID &p_vehicle, Visual &p_visual) override;
            void _pose_running_gear(const RID &p_vehicle, Visual &p_visual) override;

        public:
            static RoadVehicleRenderingServer *get_instance();

            RoadVehicleRenderingServer();
            ~RoadVehicleRenderingServer() override;
    };
} // namespace godot
