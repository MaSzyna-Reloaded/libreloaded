#include "RailVehiclePantographs.hpp"

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>

namespace godot {
    void RailVehiclePantographs::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_pantographs"), &RailVehiclePantographs::get_pantographs);
        ClassDB::bind_method(D_METHOD("set_pantographs", "pantographs"), &RailVehiclePantographs::set_pantographs);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::ARRAY, "pantographs", PROPERTY_HINT_TYPE_STRING,
                        String::num_int64(Variant::OBJECT) + "/" + String::num_int64(PROPERTY_HINT_RESOURCE_TYPE) +
                                ":RailVehiclePantograph"),
                "set_pantographs", "get_pantographs");

        ClassDB::bind_method(D_METHOD("get_pantograph_count"), &RailVehiclePantographs::get_pantograph_count);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_control", "index"), &RailVehiclePantographs::get_pantograph_control);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_has_geometry", "index"), &RailVehiclePantographs::get_pantograph_has_geometry);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_position", "index"), &RailVehiclePantographs::get_pantograph_position);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_height", "index"), &RailVehiclePantographs::get_pantograph_height);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_reaches_wire", "index"), &RailVehiclePantographs::get_pantograph_reaches_wire);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_lower_raise", "index"), &RailVehiclePantographs::get_pantograph_lower_raise);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_upper_raise", "index"), &RailVehiclePantographs::get_pantograph_upper_raise);
        ClassDB::bind_method(
                D_METHOD("pantograph_step", "index", "gap", "active", "speed_factor", "delta"),
                &RailVehiclePantographs::pantograph_step);
        ClassDB::bind_method(
                D_METHOD("pantograph_fit", "index", "max_height"), &RailVehiclePantographs::pantograph_fit);
    }

    TypedArray<RailVehiclePantograph> RailVehiclePantographs::get_pantographs() const {
        return pantographs;
    }

    void RailVehiclePantographs::set_pantographs(const TypedArray<RailVehiclePantograph> &p_pantographs) {
        pantographs = p_pantographs;
    }

    /* A pantograph without geometry is not raised. One with it keeps its slider's height as far as
     * its arms - new ones too - allow it; one new, or one that gets its geometry only now, has no
     * height yet and is fit lowered, its arms at rest */
    void RailVehiclePantographs::_apply_configuration() {
        states.resize(pantographs.size());
        for (uint32_t index = 0; index < states.size(); ++index) {
            if (get_pantograph_has_geometry(static_cast<int>(index))) {
                pantograph_fit(static_cast<int>(index), Math::INF);
            } else {
                states[index] = Pantograph();
            }
        }
    }

    void RailVehiclePantographs::_fill_state_dictionary(Dictionary &p_state) const {
        PackedFloat64Array heights;
        PackedFloat64Array lower_raises;
        PackedFloat64Array upper_raises;
        for (uint32_t index = 0; index < states.size(); ++index) {
            heights.push_back(get_pantograph_height(static_cast<int>(index)));
            lower_raises.push_back(get_pantograph_lower_raise(static_cast<int>(index)));
            upper_raises.push_back(get_pantograph_upper_raise(static_cast<int>(index)));
        }
        p_state["pantograph_heights"] = heights;
        p_state["pantograph_lower_raises"] = lower_raises;
        p_state["pantograph_upper_raises"] = upper_raises;
    }

    Ref<RailVehiclePantograph> RailVehiclePantographs::_get_configuration(const int p_index) const {
        ERR_FAIL_INDEX_V(p_index, pantographs.size(), Ref<RailVehiclePantograph>());
        return pantographs[p_index];
    }

    bool RailVehiclePantographs::_upper_angle(
            const Ref<RailVehiclePantograph> &p_config, const double p_lower_angle, double &p_r_upper_angle) {
        const double reach = ((p_config->get_lower_length() * Math::cos(p_lower_angle)) + p_config->get_horizontal()) /
                             p_config->get_upper_length();
        if (reach < -1.0 || reach > 1.0) {
            return false;
        }
        p_r_upper_angle = Math::acos(reach);
        return p_lower_angle + p_r_upper_angle < Math::PI;
    }

    double RailVehiclePantographs::_height(
            const Ref<RailVehiclePantograph> &p_config, const double p_lower_angle, const double p_upper_angle) {
        return (p_config->get_lower_length() * Math::sin(p_lower_angle)) +
               (p_config->get_upper_length() * Math::sin(p_upper_angle)) + p_config->get_slider_height();
    }

    int RailVehiclePantographs::get_pantograph_count() const {
        return static_cast<int>(pantographs.size());
    }

    RailVehicleEnginePowerSource::PantographSelector
    RailVehiclePantographs::get_pantograph_control(const int p_index) const {
        const Ref<RailVehiclePantograph> config = _get_configuration(p_index);
        return config.is_valid() ? config->get_control() : RailVehicleEnginePowerSource::PANTOGRAPH_FIRST;
    }

    bool RailVehiclePantographs::get_pantograph_has_geometry(const int p_index) const {
        const Ref<RailVehiclePantograph> config = _get_configuration(p_index);
        return config.is_valid() && config->get_has_geometry();
    }

    Vector3 RailVehiclePantographs::get_pantograph_position(const int p_index) const {
        return get_pantograph_has_geometry(p_index) ? _get_configuration(p_index)->get_position() : Vector3();
    }

    double RailVehiclePantographs::get_pantograph_height(const int p_index) const {
        ERR_FAIL_INDEX_V(p_index, static_cast<int>(states.size()), 0.0);
        return states[p_index].height;
    }

    bool RailVehiclePantographs::get_pantograph_reaches_wire(const int p_index) const {
        ERR_FAIL_INDEX_V(p_index, static_cast<int>(states.size()), false);
        return !get_pantograph_has_geometry(p_index) || states[p_index].reaches_wire;
    }

    double RailVehiclePantographs::get_pantograph_lower_raise(const int p_index) const {
        ERR_FAIL_INDEX_V(p_index, static_cast<int>(states.size()), 0.0);
        if (!get_pantograph_has_geometry(p_index)) {
            return 0.0;
        }
        return states[p_index].lower_angle - _get_configuration(p_index)->get_lower_rest_angle();
    }

    double RailVehiclePantographs::get_pantograph_upper_raise(const int p_index) const {
        ERR_FAIL_INDEX_V(p_index, static_cast<int>(states.size()), 0.0);
        if (!get_pantograph_has_geometry(p_index)) {
            return 0.0;
        }
        return states[p_index].upper_angle - _get_configuration(p_index)->get_upper_rest_angle();
    }

    void RailVehiclePantographs::pantograph_step(
            const int p_index, const double p_gap, const bool p_active, const double p_speed_factor,
            const double p_delta) {
        ERR_FAIL_INDEX(p_index, static_cast<int>(states.size()));
        if (!get_pantograph_has_geometry(p_index)) {
            return;
        }
        const Ref<RailVehiclePantograph> config = _get_configuration(p_index);
        Pantograph &pantograph = states[p_index];
        double angle = pantograph.lower_angle;
        if (p_speed_factor > 0.0 && p_active) {
            if (p_gap > SETTLED_GAP) {
                angle += MIN(p_speed_factor, RISE_SHARE * p_gap);
            } else if (p_gap < -SETTLED_GAP) {
                angle += PRESS_SHARE * p_gap;
            }
        } else {
            if (angle > config->get_lower_rest_angle()) {
                angle -= FALL_RATE * p_delta;
            }
            angle = MAX(angle, config->get_lower_rest_angle());
        }
        double upper = 0.0;
        if (!Math::is_equal_approx(angle, pantograph.lower_angle) && _upper_angle(config, angle, upper)) {
            pantograph.lower_angle = angle;
            pantograph.upper_angle = upper;
            pantograph.height = _height(config, angle, upper);
        }
        pantograph.reaches_wire = p_active && p_gap < CONTACT_GAP;
    }

    /* The lower arm's range is halved twice: first to its highest angle the arms can stand at, then
     * to the angle of the height wanted - the height grows with the angle over the arms' range */
    void RailVehiclePantographs::pantograph_fit(const int p_index, const double p_max_height) {
        ERR_FAIL_INDEX(p_index, static_cast<int>(states.size()));
        if (!get_pantograph_has_geometry(p_index)) {
            return;
        }
        const Ref<RailVehiclePantograph> config = _get_configuration(p_index);
        const double rest_angle = config->get_lower_rest_angle();
        Pantograph &pantograph = states[p_index];
        double upper = 0.0;
        if (!_upper_angle(config, rest_angle, upper)) {
            return;
        }
        const double rest = _height(config, rest_angle, upper);
        double low = rest_angle;
        double high = Math::PI;
        for (int step = 0; step < FIT_STEPS; ++step) {
            const double middle = 0.5 * (low + high);
            (_upper_angle(config, middle, upper) ? low : high) = middle;
        }
        const double highest = low;
        _upper_angle(config, highest, upper);
        const double reach = MIN(_height(config, highest, upper), p_max_height);
        const double wanted = MAX(MIN(pantograph.height, reach), rest);
        low = rest_angle;
        high = highest;
        for (int step = 0; step < FIT_STEPS; ++step) {
            const double middle = 0.5 * (low + high);
            _upper_angle(config, middle, upper);
            (_height(config, middle, upper) < wanted ? low : high) = middle;
        }
        _upper_angle(config, low, upper);
        pantograph.lower_angle = low;
        pantograph.upper_angle = upper;
        pantograph.height = _height(config, low, upper);
    }
} // namespace godot
