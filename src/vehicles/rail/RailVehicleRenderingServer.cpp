#include "RailVehicleRenderingServer.hpp"
#include "rendering/ModelRenderingServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleAppearance.hpp"
#include "vehicles/rail/RailVehicleLighting.hpp"
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
        for (int axle = 0; axle < MAX_AXLE_CHANNELS; ++axle) {
            axle_channels.push_back(vformat("axle.%d.angle", axle));
        }
        for (const char *end: {"front", "rear"}) {
            for (const char *side: {"left", "right", "upper"}) {
                headlight_channels.push_back(vformat("headlight.%s.%s.level", end, side));
            }
        }
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

    /* The vehicle stands on a track: the body's transform is where RailVehicleServer placed it,
     * composed from the bogies, and nothing else */
    bool RailVehicleRenderingServer::_vehicle_get_placement(const RID &p_vehicle, Transform3D &p_r_transform) const {
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        if (server == nullptr ||
            !RID(server->vehicle_get_track_position(p_vehicle).get("track_rid", RID())).is_valid()) {
            return false;
        }
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicle_server, false);
        p_r_transform = vehicle_server->vehicle_get_transform(p_vehicle);
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
                server->bogie_get_transform(p_vehicle, RailVehicleWheels::BOGIE_FRONT),
                server->bogie_get_transform(p_vehicle, RailVehicleWheels::BOGIE_REAR)};
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
        // the wheels turn about the axis across the travel in the model's own frame: up x the vehicle's
        // front (-Z) as the model sees it (VehicleAppearance.model_transform) - X of a model whose front
        // faces +Z
        const Basis model_basis =
                p_visual.appearance.is_valid() ? p_visual.appearance->get_model_transform().basis : Basis();
        const Vector3 axle_axis =
                Vector3(0.0, 1.0, 0.0).cross(model_basis.xform_inv(Vector3(0.0, 0.0, -1.0))).normalized();
        const double angles[] = {
                wheels->get_angle_front_deg(), wheels->get_angle_powered_deg(), wheels->get_angle_rear_deg()};
        for (int group = 0; group < static_cast<int>(std::size(p_visual.wheels)); ++group) {
            const Basis pose(axle_axis, static_cast<real_t>(Math::deg_to_rad(angles[group])));
            for (const Part &wheel: p_visual.wheels[group]) {
                _pose(p_visual, wheel, pose);
            }
        }
        // the axles' angles as the model's parts driven by them take them [rad], by each axle's place
        const int axles = MIN(wheels->get_axle_count(), MAX_AXLE_CHANNELS);
        for (int axle = 0; axle < axles; ++axle) {
            p_visual.channels[axle_channels[axle]] = Math::deg_to_rad(wheels->get_axle_angle_deg(axle));
        }
    }

    /* The headlights' levels (the model format's headlight.<end>.<side>.level): 1 a headlight on,
     * the lighting's dimmed multiplier while its headlights are dimmed, 0 off.
     * TODO: the lamps' state is read again every frame the vehicle is drawn in detail, as a plugin's
     * renderer reads the lights of its own models; it is to be sent on an event of the lighting's
     * change instead
     * (TODO.md, "A vehicle's lights read every frame") */
    void RailVehicleRenderingServer::_fill_channels(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RailVehicleLighting> lighting =
                component<RailVehicleLighting>(p_vehicle, VehicleComponentType::COMPONENT_LIGHTING);
        if (lighting.is_null()) {
            return;
        }
        const double on = lighting->get_headlights_dimmed() ? lighting->get_head_light_dimmed_multiplier() : 1.0;
        const bool enabled[] = {
                lighting->get_front_headlight_left_enabled(),  lighting->get_front_headlight_right_enabled(),
                lighting->get_front_headlight_upper_enabled(), lighting->get_rear_headlight_left_enabled(),
                lighting->get_rear_headlight_right_enabled(),  lighting->get_rear_headlight_upper_enabled()};
        for (int index = 0; index < headlight_channels.size(); ++index) {
            p_visual.channels[headlight_channels[index]] = enabled[index] ? on : 0.0;
        }
    }

} // namespace godot
