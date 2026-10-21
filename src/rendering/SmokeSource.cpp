#include "SmokeSource.hpp"
#include <godot_cpp/core/math.hpp>

namespace godot {
    void SmokeSource::_bind_methods() {
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, spawn_rate);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, inclination_min);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, inclination_max);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, velocity_min);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, velocity_max);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, size_min);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, size_max);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, opacity_min);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, opacity_max);
        BIND_PROPERTY(SmokeSource, Variant::COLOR, color);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, size_step);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, size_limit_min);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, size_limit_max);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, opacity_step);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, opacity_limit_min);
        BIND_PROPERTY(SmokeSource, Variant::FLOAT, opacity_limit_max);
        ClassDB::bind_method(D_METHOD("get_particle_lifetime"), &SmokeSource::get_particle_lifetime);
        ClassDB::bind_method(
                D_METHOD("get_particle_amount", "max_particles", "density", "lifetime"),
                &SmokeSource::get_particle_amount);
        ClassDB::bind_method(D_METHOD("get_terminal_size", "lifetime"), &SmokeSource::get_terminal_size);
        ClassDB::bind_method(D_METHOD("get_mean_size"), &SmokeSource::get_mean_size);
    }

    double SmokeSource::get_particle_lifetime() const {
        if (Math::is_zero_approx(opacity_step)) {
            return 0.0;
        }
        return opacity_max / Math::abs(opacity_step);
    }

    int
    SmokeSource::get_particle_amount(const int p_max_particles, const double p_density, const double p_lifetime) const {
        return CLAMP(static_cast<int>(Math::ceil(spawn_rate * p_density * p_lifetime)), 0, p_max_particles);
    }

    double SmokeSource::get_terminal_size(const double p_lifetime) const {
        return CLAMP(get_mean_size() + (size_step * p_lifetime), size_limit_min, size_limit_max);
    }

    double SmokeSource::get_mean_size() const {
        return (size_min + size_max) * 0.5;
    }
} // namespace godot
