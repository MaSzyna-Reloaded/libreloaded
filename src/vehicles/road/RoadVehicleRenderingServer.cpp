#include "RoadVehicleRenderingServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/road/RoadVehicleAppearance.hpp"
#include "vehicles/road/RoadVehicleController.hpp"
#include "vehicles/road/RoadVehicleServer.hpp"

#include <godot_cpp/core/math.hpp>

namespace godot {
    RoadVehicleRenderingServer *RoadVehicleRenderingServer::get_instance() {
        return singleton;
    }

    RoadVehicleRenderingServer::RoadVehicleRenderingServer() {
        singleton = this;
        if (RoadVehicleServer *road_vehicles = RoadVehicleServer::get_instance(); road_vehicles != nullptr) {
            _follow_placements(
                    road_vehicles, RoadVehicleServer::vehicle_placed_signal,
                    RoadVehicleServer::vehicle_placement_changed_signal);
        }
    }

    RoadVehicleRenderingServer::~RoadVehicleRenderingServer() {
        _detach_all();
        singleton = nullptr;
    }

    bool RoadVehicleRenderingServer::_vehicle_get_placement(const RID &p_vehicle, Transform3D &p_r_transform) const {
        const RoadVehicleServer *server = RoadVehicleServer::get_instance();
        if (server == nullptr || !server->vehicle_get_road(p_vehicle).is_valid()) {
            return false;
        }
        p_r_transform = server->vehicle_get_transform(p_vehicle);
        return true;
    }

    void RoadVehicleRenderingServer::_bind_running_gear(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RoadVehicleAppearance> appearance = p_visual.appearance;
        if (appearance.is_valid()) {
            p_visual.wheels[0] = _parts(p_visual, appearance->get_wheels());
        }
    }

    /* Every wheel turned by the distance run, about its own axle */
    void RoadVehicleRenderingServer::_pose_running_gear(const RID &p_vehicle, Visual &p_visual) {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const Ref<RoadVehicleController> controller = vehicle_server != nullptr
                                                              ? vehicle_server->vehicle_get_controller(p_vehicle)
                                                              : Ref<VehicleController>();
        if (controller.is_null()) {
            return;
        }
        const Basis pose(
                Vector3(1.0, 0.0, 0.0), static_cast<real_t>(Math::deg_to_rad(controller->get_wheels_angle_deg())));
        for (const Part &wheel: p_visual.wheels[0]) {
            _pose(p_visual, wheel, pose);
        }
    }
} // namespace godot
