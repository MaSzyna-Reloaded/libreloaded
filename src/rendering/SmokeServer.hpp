#pragma once
#include "rendering/SmokeSource.hpp"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/particle_process_material.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    /* The particle emitters of the world, by RID: smoke a vehicle or the scenery gives off. An
     * emitter is made of a SmokeSource and drawn with a sprite - by default the flipbook of
     * ATLAS_SETTING, the core's own smoke. A moving emitter (KIND_DYNAMIC) spawns its particles
     * at the rate its owner sets (smoke_set_intensity()); a standing one (KIND_STATIC) emits on its
     * own, its plume in the air the moment it is built. Every emitter drifts with the wind of
     * SimulationServer. */
    class SmokeServer : public Object {
            GDCLASS(SmokeServer, Object)

        public:
            /* A vehicle's emitter against one of the scenery: each kind has its own settings */
            enum Kind {
                KIND_STATIC,
                KIND_DYNAMIC,
                KIND_COUNT,
            };

            /// With it off no emitter is built
            static constexpr const char *ENABLED_SETTING = "reloaded/smoke/enabled";
            static constexpr bool DEFAULT_ENABLED = true;
            /// How far from the camera a registered emitter is built (smoke_register()) [m]
            static constexpr const char *DYNAMIC_DISTANCE_SETTING = "reloaded/smoke/dynamic/distance";
            static constexpr const char *STATIC_DISTANCE_SETTING = "reloaded/smoke/static/distance";
            static constexpr float DEFAULT_DISTANCE = 1500.0;
            /// How many more particles an emitter gives than its source asks for; each is fainter by as
            /// much, so a denser plume is smoother, not darker
            static constexpr const char *DYNAMIC_DENSITY_SETTING = "reloaded/smoke/dynamic/density";
            static constexpr const char *STATIC_DENSITY_SETTING = "reloaded/smoke/static/density";
            static constexpr double DEFAULT_DENSITY = 1.0;
            /// How long a particle lives, over what its source makes of it: a chimney needs no minute
            /// of smoke hanging over it, and the lifetime decides how many particles are in the air
            static constexpr const char *DYNAMIC_LIFETIME_SETTING = "reloaded/smoke/dynamic/lifetime";
            static constexpr const char *STATIC_LIFETIME_SETTING = "reloaded/smoke/static/lifetime";
            static constexpr double DEFAULT_DYNAMIC_LIFETIME = 1.0;
            static constexpr double DEFAULT_STATIC_LIFETIME = 0.5;
            /// The particles one emitter holds at most; room has to be left for the density
            static constexpr const char *DYNAMIC_MAX_PARTICLES_SETTING = "reloaded/smoke/dynamic/max_particles";
            static constexpr const char *STATIC_MAX_PARTICLES_SETTING = "reloaded/smoke/static/max_particles";
            static constexpr int DEFAULT_DYNAMIC_MAX_PARTICLES = 2000;
            static constexpr int DEFAULT_STATIC_MAX_PARTICLES = 500;
            /// The flipbook a particle walks over its lifetime: it wells up, breaks into wisps and
            /// dissolves. The project fills it in - the core ships none
            static constexpr const char *ATLAS_SETTING = "reloaded/smoke/modern/atlas";
            /// Its columns and rows
            static constexpr const char *ATLAS_FRAMES_SETTING = "reloaded/smoke/modern/atlas_frames";
            static constexpr int DEFAULT_ATLAS_FRAMES = 4;
            /// How much of a standing emitter's plume is in the air when it is built [s]; bounded,
            /// as the engine works it out in steps of a frame
            static constexpr double PREPROCESS_SECONDS = 5.0;
            /// The acceleration a particle drifts with, of the wind's speed: the drift grows with the
            /// square of the particle's age
            static constexpr float WIND_ACCELERATION = 0.1;
            /// Moving emitters visited a frame. Below it every one is visited every frame, which
            /// spreads its particles evenly; above it the tick carries on where it left off, and an
            /// emitter spawns the backlog its clock owes it a few frames later
            static constexpr int MAX_EMITTERS_PER_FRAME = 64;
            /// A particle is born rolled anywhere around the full turn [deg]
            static constexpr float FULL_TURN_DEGREES = 360.0;
            /// Below this a particle's mean size counts as none (the growth is divided by it)
            static constexpr double MIN_MEAN_SIZE = 0.001;
            /// Below this the density counts as none (a particle's opacity is divided by it)
            static constexpr double MIN_DENSITY = 0.001;

        private:
            static SmokeServer *singleton;

            /* What an emitter of a source, drawn with a sprite, is built of at the settings now -
             * shared by the emitters of the same source, sprite and kind */
            struct RenderData {
                    Ref<ParticleProcessMaterial> process_material;
                    Ref<QuadMesh> mesh;
                    int amount = 0;
                    double lifetime = 0.0;
                    double spawn_rate = 0.0;
                    double preprocess = 0.0;
                    /* The reach of the plume around the emitter, in its own space */
                    AABB aabb;
            };

            struct Emitter {
                    Kind kind = KIND_STATIC;
                    Ref<SmokeSource> source;
                    Ref<Texture2D> sprite;
                    RID scenario;
                    Transform3D transform;
                    bool visible = true;
                    /* The spawn rate over the source's, a moving emitter's only */
                    float intensity = 1.0;
                    /* Registered with SceneryStreamingServer (smoke_register()), built while near */
                    bool registered = false;
                    RID stream_rid;
                    RID particles;
                    RID particles_instance;
                    Ref<ParticleProcessMaterial> process_material;
                    float spawn_rate = 0.0;
                    /* Fractional particles carried to the next tick */
                    float spawn_backlog = 0.0;
                    /* Its own clock, so a skipped frame costs nothing */
                    uint64_t last_spawn_usec = 0;
                    /* The pool, the cap on one tick's spawns */
                    int amount = 0;
                    /* In spawning: moving, visible, built and of an intensity above zero */
                    bool ordered = false;
            };

            HashMap<RID, Emitter> emitters;
            /* The emitters that spawn now, walked round-robin by _process_smoke(); a HashMap's
             * elements do not move, so they are held by pointer */
            LocalVector<Emitter *> spawning;
            int spawn_cursor = 0;
            bool processing = false;
            /* By kind, source and sprite (their ObjectIDs) */
            HashMap<uint64_t, HashMap<uint64_t, RenderData>> render_data[KIND_COUNT];
            /* The vector the particles drift with [m/s] */
            Vector3 wind;
            int stream_owner = -1;
            /* What the emitters were built with, in the order of the settings' names */
            Array settings;
            /* Taken once: the tick reads the clock every frame */
            Time *time = nullptr;

            RID _emitter_make(Kind p_kind);
            const RenderData *_get_render_data(const Emitter &p_emitter);
            void _build(Emitter &p_emitter);
            void _clear(Emitter &p_emitter);
            /* A registered emitter registered again, with the distance of its kind now */
            void _register_stream(const RID &p_emitter, Emitter &p_data);
            void _refresh_spawning(Emitter &p_emitter);
            void _apply_wind(const Emitter &p_emitter) const;
            void _set_processing(bool p_processing);
            void _process_smoke();
            void _stream_build(const RID &p_emitter, const Variant &p_preloaded);
            void _stream_clear(const RID &p_emitter);
            void _on_settings_changed();
            void _on_weather_changed();
            void _on_data_unload_requested();

        protected:
            static void _bind_methods();

        public:
            static SmokeServer *get_instance();

            SmokeServer();
            ~SmokeServer() override;

            /* An emitter built as soon as it has a source */
            RID smoke_create(Kind p_kind);
            /* An emitter standing at `p_transform`, built while the camera is within its kind's
             * distance */
            RID smoke_register(Kind p_kind, const Transform3D &p_transform);
            void smoke_free(const RID &p_emitter);
            /* What the emitter is made of; it is built of it again */
            void smoke_set_source(const RID &p_emitter, const Ref<SmokeSource> &p_source);
            /* What a particle is drawn with; null for the core's own smoke (ATLAS_SETTING) */
            void smoke_set_sprite(const RID &p_emitter, const Ref<Texture2D> &p_sprite);
            void smoke_set_scenario(const RID &p_emitter, const RID &p_scenario);
            /* Where the particles are born; they go up along the transform's up axis */
            void smoke_set_transform(const RID &p_emitter, const Transform3D &p_transform);
            /* A hidden emitter spawns nothing */
            void smoke_set_visible(const RID &p_emitter, bool p_visible);
            /* A moving emitter's spawn rate over its source's, 0..1 */
            void smoke_set_intensity(const RID &p_emitter, float p_intensity);
            /* `total` emitters, `built` of them */
            Dictionary smoke_get_statistics() const;
            /* The flipbook of ATLAS_SETTING; null when the project gives none */
            Ref<Texture2D> smoke_get_atlas() const;
            double smoke_get_density(Kind p_kind) const;
            /* How long a particle of `p_source` lives in an emitter of `p_kind` [s] */
            double smoke_get_lifetime(const Ref<SmokeSource> &p_source, Kind p_kind) const;
            int smoke_get_max_particles(Kind p_kind) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(SmokeServer::Kind)
