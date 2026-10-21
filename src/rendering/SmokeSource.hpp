#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/color.hpp>

namespace godot {
    /* What one particle emitter is made of (SmokeServer.smoke_set_source()): how many particles it
     * gives a second, in what cone and how fast they leave, how big, how opaque and of what colour
     * they are born, and how they grow and fade. A particle lives until it has faded out. */
    class SmokeSource : public Resource {
            GDCLASS(SmokeSource, Resource)

        public:
            /// The grey of a particle by default, of 255
            static constexpr int DEFAULT_COLOR_LEVEL = 16;

        private:
            /* Particles a second */
            double spawn_rate = 0.0;
            /* Off the emitter's up axis [deg] */
            double inclination_min = 0.0;
            double inclination_max = 0.0;
            /* The speed a particle leaves at [m/s] */
            double velocity_min = 1.0;
            double velocity_max = 1.0;
            /* The size a particle is born with, over the sprite's own */
            double size_min = 1.0;
            double size_max = 1.0;
            double opacity_min = 1.0;
            double opacity_max = 1.0;
            Color color = Color::from_rgba8(DEFAULT_COLOR_LEVEL, DEFAULT_COLOR_LEVEL, DEFAULT_COLOR_LEVEL);
            /* How much a particle grows a second, kept within the limits */
            double size_step = 0.0;
            double size_limit_min = 0.0;
            double size_limit_max = INFINITY;
            /* How much opacity a particle loses a second (below 0), kept within the limits */
            double opacity_step = 0.0;
            double opacity_limit_min = 0.0;
            double opacity_limit_max = 1.0;

        protected:
            static void _bind_methods();

        public:
            MAKE_GS(double, spawn_rate);
            MAKE_GS(double, inclination_min);
            MAKE_GS(double, inclination_max);
            MAKE_GS(double, velocity_min);
            MAKE_GS(double, velocity_max);
            MAKE_GS(double, size_min);
            MAKE_GS(double, size_max);
            MAKE_GS(double, opacity_min);
            MAKE_GS(double, opacity_max);
            MAKE_GS(Color, color);
            MAKE_GS(double, size_step);
            MAKE_GS(double, size_limit_min);
            MAKE_GS(double, size_limit_max);
            MAKE_GS(double, opacity_step);
            MAKE_GS(double, opacity_limit_min);
            MAKE_GS(double, opacity_limit_max);

            /* How long a particle lives [s]: until the most opaque one has faded out; 0 for a
             * source whose particles never fade */
            double get_particle_lifetime() const;
            /* How many particles the emitter holds at once: what it gives over `p_lifetime` at
             * `p_density` times its rate, at most `p_max_particles` */
            int get_particle_amount(int p_max_particles, double p_density, double p_lifetime) const;
            /* The size a particle has grown to after `p_lifetime` */
            double get_terminal_size(double p_lifetime) const;
            double get_mean_size() const;
    };
} // namespace godot
