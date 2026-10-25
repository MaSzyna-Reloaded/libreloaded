#include "RailVehicleRenderingServer.hpp"
#include "rendering/ModelRenderingServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleAppearance.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include "vehicles/rail/RailVehicleWheels.hpp"

#include <godot_cpp/core/math.hpp>

namespace godot {
    RailVehicleRenderingServer *RailVehicleRenderingServer::singleton = nullptr;

    template<typename T>
    static Ref<T> component(const RID &p_vehicle, const VehicleComponentType::Type p_type) {
        const VehicleServer *server = VehicleServer::get_instance();
        return server != nullptr ? Ref<T>(server->vehicle_component_get(p_vehicle, p_type)) : Ref<T>();
    }

    RailVehicleRenderingServer *RailVehicleRenderingServer::get_instance() {
        return singleton;
    }

    RailVehicleRenderingServer::RailVehicleRenderingServer() {
        singleton = this;
        if (RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance(); rail_vehicles != nullptr) {
            _follow_placements(
                    rail_vehicles, RailVehicleServer::vehicle_placed_signal,
                    RailVehicleServer::vehicle_placement_changed_signal);
        }
    }

    RailVehicleRenderingServer::~RailVehicleRenderingServer() {
        _detach_all();
        singleton = nullptr;
    }

    /* The vehicle stands on a track: the body's transform is RailVehicleServer's answer and nothing
     * else - it composes it from the bogies */
    bool RailVehicleRenderingServer::_vehicle_get_placement(const RID &p_vehicle, Transform3D &p_r_transform) const {
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        if (server == nullptr ||
            !RID(server->vehicle_get_track_position(p_vehicle).get("track_rid", RID())).is_valid()) {
            return false;
        }
        p_r_transform = server->vehicle_get_transform(p_vehicle);
        return true;
    }

    void RailVehicleRenderingServer::_bind_running_gear(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RailVehicleAppearance> appearance = p_visual.appearance;
        if (appearance.is_null()) {
            return;
        }
        p_visual.bogies[RailVehicleWheels::BOGIE_FRONT] = _part(p_visual, appearance->get_front_bogie());
        p_visual.bogies[RailVehicleWheels::BOGIE_REAR] = _part(p_visual, appearance->get_rear_bogie());
        p_visual.wheels[0] = _parts(p_visual, appearance->get_front_rolling_wheels());
        p_visual.wheels[1] = _parts(p_visual, appearance->get_powered_wheels());
        p_visual.wheels[2] = _parts(p_visual, appearance->get_rear_rolling_wheels());
    }

    /* The bogies turned towards their own track (where each sits on it is RailVehicleServer's,
     * by the wheels' pivot spacing), and the wheels turned by the distance rolled - the same
     * sign as the original's UpdateAxle() (DynObj.cpp:489). A bogie turns about the vehicle's
     * vertical, which in its own frame is its rest basis seen from the vehicle. */
    void RailVehicleRenderingServer::_pose_running_gear(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RailVehicleWheels> wheels =
                component<RailVehicleWheels>(p_vehicle, VehicleComponentType::COMPONENT_WHEELS);
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        if (wheels.is_null() || server == nullptr) {
            return;
        }
        const Transform3D bogie_transforms[] = {
                server->vehicle_get_bogie_transform(p_vehicle, RailVehicleWheels::BOGIE_FRONT),
                server->vehicle_get_bogie_transform(p_vehicle, RailVehicleWheels::BOGIE_REAR)};
        Vector3 body_forward = bogie_transforms[0].origin - bogie_transforms[1].origin;
        if (!body_forward.is_zero_approx()) {
            body_forward.normalize();
            const double body_yaw = Math::atan2(-body_forward.x, body_forward.z);
            for (int index = 0; index < static_cast<int>(std::size(p_visual.bogies)); ++index) {
                const Vector3 bogie_forward = -bogie_transforms[index].basis.get_column(2).normalized();
                const double bogie_yaw = Math::atan2(-bogie_forward.x, bogie_forward.z);
                const Basis &rest = p_visual.bogies[index].rest.basis;
                _pose(p_visual, p_visual.bogies[index],
                      rest.inverse() * Basis(Vector3(0.0, 1.0, 0.0), static_cast<real_t>(body_yaw - bogie_yaw)) * rest);
            }
        }
        const double angles[] = {
                wheels->get_angle_front_deg(), wheels->get_angle_powered_deg(), wheels->get_angle_rear_deg()};
        for (int group = 0; group < static_cast<int>(std::size(p_visual.wheels)); ++group) {
            const Basis pose(Vector3(1.0, 0.0, 0.0), static_cast<real_t>(Math::deg_to_rad(angles[group])));
            for (const Part &wheel: p_visual.wheels[group]) {
                _pose(p_visual, wheel, pose);
            }
        }
    }

} // namespace godot
