#include "SmokeServer.hpp"
#include "game_data/GameDataServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "utils/Units.hpp"
#include <godot_cpp/classes/curve.hpp>
#include <godot_cpp/classes/curve_texture.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/gpu_particles3d.hpp>
#include <godot_cpp/classes/gradient.hpp>
#include <godot_cpp/classes/gradient_texture1_d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    SmokeServer *SmokeServer::singleton = nullptr;

    /// Every setting an emitter is built of: a change of one builds them all again
    static const char *const SETTING_NAMES[] = {
            SmokeServer::ENABLED_SETTING,
            SmokeServer::DYNAMIC_DISTANCE_SETTING,
            SmokeServer::STATIC_DISTANCE_SETTING,
            SmokeServer::DYNAMIC_DENSITY_SETTING,
            SmokeServer::STATIC_DENSITY_SETTING,
            SmokeServer::DYNAMIC_LIFETIME_SETTING,
            SmokeServer::STATIC_LIFETIME_SETTING,
            SmokeServer::DYNAMIC_MAX_PARTICLES_SETTING,
            SmokeServer::STATIC_MAX_PARTICLES_SETTING,
            SmokeServer::ATLAS_SETTING,
            SmokeServer::ATLAS_FRAMES_SETTING,
    };

    static Array read_settings() {
        const ProjectSettings *project_settings = ProjectSettings::get_singleton();
        Array values;
        for (const char *name: SETTING_NAMES) {
            values.push_back(project_settings->get_setting(name));
        }
        return values;
    }

    /// A linear change from `p_from` to `p_to` over a particle's lifetime
    static Ref<CurveTexture> make_curve(const double p_from, const double p_to) {
        Ref<Curve> curve;
        curve.instantiate();
        curve->set_min_value(MIN(p_from, p_to));
        curve->set_max_value(MAX(p_from, p_to));
        curve->add_point(Vector2(0.0, p_from), 0.0, 0.0, Curve::TANGENT_LINEAR, Curve::TANGENT_LINEAR);
        curve->add_point(Vector2(1.0, p_to), 0.0, 0.0, Curve::TANGENT_LINEAR, Curve::TANGENT_LINEAR);
        Ref<CurveTexture> texture;
        texture.instantiate();
        texture->set_curve(curve);
        return texture;
    }

    /// A particle's opacity at birth, picked between the source's two; divided by the density,
    /// so more particles add up to the same plume, not to more soot
    static Ref<GradientTexture1D> make_opacity_ramp(const SmokeSource &p_source, const double p_density) {
        const double scale = 1.0 / MAX(p_density, SmokeServer::MIN_DENSITY);
        Ref<Gradient> gradient;
        gradient.instantiate();
        gradient->set_color(0, Color(1.0, 1.0, 1.0, static_cast<float>(p_source.get_opacity_min() * scale)));
        gradient->set_color(1, Color(1.0, 1.0, 1.0, static_cast<float>(p_source.get_opacity_max() * scale)));
        Ref<GradientTexture1D> texture;
        texture.instantiate();
        texture->set_gradient(gradient);
        return texture;
    }

    void SmokeServer::_bind_methods() {
        BIND_ENUM_CONSTANT(KIND_STATIC);
        BIND_ENUM_CONSTANT(KIND_DYNAMIC);
        ClassDB::bind_method(D_METHOD("smoke_create", "kind"), &SmokeServer::smoke_create);
        ClassDB::bind_method(D_METHOD("smoke_register", "kind", "transform"), &SmokeServer::smoke_register);
        ClassDB::bind_method(D_METHOD("smoke_free", "emitter"), &SmokeServer::smoke_free);
        ClassDB::bind_method(D_METHOD("smoke_set_source", "emitter", "source"), &SmokeServer::smoke_set_source);
        ClassDB::bind_method(D_METHOD("smoke_set_sprite", "emitter", "sprite"), &SmokeServer::smoke_set_sprite);
        ClassDB::bind_method(D_METHOD("smoke_set_scenario", "emitter", "scenario"), &SmokeServer::smoke_set_scenario);
        ClassDB::bind_method(
                D_METHOD("smoke_set_transform", "emitter", "transform"), &SmokeServer::smoke_set_transform);
        ClassDB::bind_method(D_METHOD("smoke_set_visible", "emitter", "visible"), &SmokeServer::smoke_set_visible);
        ClassDB::bind_method(
                D_METHOD("smoke_set_intensity", "emitter", "intensity"), &SmokeServer::smoke_set_intensity);
        ClassDB::bind_method(D_METHOD("smoke_get_statistics"), &SmokeServer::smoke_get_statistics);
        ClassDB::bind_method(D_METHOD("smoke_get_atlas"), &SmokeServer::smoke_get_atlas);
        ClassDB::bind_method(D_METHOD("smoke_get_density", "kind"), &SmokeServer::smoke_get_density);
        ClassDB::bind_method(D_METHOD("smoke_get_lifetime", "source", "kind"), &SmokeServer::smoke_get_lifetime);
        ClassDB::bind_method(D_METHOD("smoke_get_max_particles", "kind"), &SmokeServer::smoke_get_max_particles);
    }

    SmokeServer *SmokeServer::get_instance() {
        return singleton;
    }

    /// After SimulationServer, SceneryStreamingServer and GameDataServer: it follows the wind, streams
    /// the standing emitters and lets go of what the data's sources were built into
    SmokeServer::SmokeServer() : time(Time::get_singleton()) {
        singleton = this;
        settings = read_settings();
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &SmokeServer::_on_settings_changed));
        SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        simulation->connect(
                SimulationServer::weather_changed_signal, callable_mp(this, &SmokeServer::_on_weather_changed));
        _on_weather_changed();
        GameDataServer *game_data = GameDataServer::get_instance();
        ERR_FAIL_NULL(game_data);
        game_data->connect(
                GameDataServer::data_unload_requested_signal,
                callable_mp(this, &SmokeServer::_on_data_unload_requested));
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL(streaming);
        stream_owner = streaming->owner_create(
                "smoke", Callable(), callable_mp(this, &SmokeServer::_stream_build),
                callable_mp(this, &SmokeServer::_stream_clear));
    }

    SmokeServer::~SmokeServer() {
        for (KeyValue<RID, Emitter> &item: emitters) {
            _clear(item.value);
        }
        emitters.clear();
        spawning.clear();
        _set_processing(false);
        singleton = nullptr;
    }

    RID SmokeServer::_emitter_make(const Kind p_kind) {
        ERR_FAIL_INDEX_V(p_kind, KIND_COUNT, RID());
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        emitters[rid].kind = p_kind;
        return rid;
    }

    RID SmokeServer::smoke_create(const Kind p_kind) {
        return _emitter_make(p_kind);
    }

    RID SmokeServer::smoke_register(const Kind p_kind, const Transform3D &p_transform) {
        RID rid = _emitter_make(p_kind);
        Emitter *emitter = emitters.getptr(rid);
        ERR_FAIL_NULL_V(emitter, RID());
        emitter->transform = p_transform;
        emitter->registered = true;
        _register_stream(rid, *emitter);
        return rid;
    }

    void SmokeServer::_register_stream(const RID &p_emitter, Emitter &p_data) {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL(streaming);
        if (p_data.stream_rid.is_valid()) {
            streaming->stream_free(p_data.stream_rid);
        }
        const float distance = ProjectSettings::get_singleton()->get_setting(
                p_data.kind == KIND_DYNAMIC ? DYNAMIC_DISTANCE_SETTING : STATIC_DISTANCE_SETTING, DEFAULT_DISTANCE);
        p_data.stream_rid = streaming->stream_register(stream_owner, p_emitter, p_data.transform.origin, distance);
    }

    void SmokeServer::smoke_free(const RID &p_emitter) {
        Emitter *emitter = emitters.getptr(p_emitter);
        ERR_FAIL_NULL(emitter);
        if (emitter->stream_rid.is_valid()) {
            if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
                streaming->stream_free(emitter->stream_rid);
            }
        }
        // out of spawning with its particles, before its element goes
        _clear(*emitter);
        emitters.erase(p_emitter);
    }

    void SmokeServer::smoke_set_source(const RID &p_emitter, const Ref<SmokeSource> &p_source) {
        Emitter *emitter = emitters.getptr(p_emitter);
        ERR_FAIL_NULL(emitter);
        const bool built = emitter->particles.is_valid();
        _clear(*emitter);
        emitter->source = p_source;
        if (!emitter->registered || built) {
            _build(*emitter);
        }
    }

    void SmokeServer::smoke_set_sprite(const RID &p_emitter, const Ref<Texture2D> &p_sprite) {
        Emitter *emitter = emitters.getptr(p_emitter);
        ERR_FAIL_NULL(emitter);
        emitter->sprite = p_sprite;
        if (emitter->particles.is_valid()) {
            _clear(*emitter);
            _build(*emitter);
        }
    }

    void SmokeServer::smoke_set_scenario(const RID &p_emitter, const RID &p_scenario) {
        Emitter *emitter = emitters.getptr(p_emitter);
        ERR_FAIL_NULL(emitter);
        emitter->scenario = p_scenario;
        if (emitter->particles_instance.is_valid()) {
            RenderingServer::get_singleton()->instance_set_scenario(emitter->particles_instance, p_scenario);
        }
    }

    void SmokeServer::smoke_set_transform(const RID &p_emitter, const Transform3D &p_transform) {
        Emitter *emitter = emitters.getptr(p_emitter);
        ERR_FAIL_NULL(emitter);
        emitter->transform = p_transform;
        if (emitter->particles_instance.is_valid()) {
            RenderingServer::get_singleton()->instance_set_transform(emitter->particles_instance, p_transform);
        }
    }

    void SmokeServer::smoke_set_visible(const RID &p_emitter, const bool p_visible) {
        Emitter *emitter = emitters.getptr(p_emitter);
        ERR_FAIL_NULL(emitter);
        emitter->visible = p_visible;
        if (emitter->particles.is_valid()) {
            RenderingServer *rs = RenderingServer::get_singleton();
            if (emitter->kind == KIND_STATIC) {
                rs->particles_set_emitting(emitter->particles, p_visible);
            }
            rs->instance_set_visible(emitter->particles_instance, p_visible);
        }
        _refresh_spawning(*emitter);
    }

    void SmokeServer::smoke_set_intensity(const RID &p_emitter, const float p_intensity) {
        Emitter *emitter = emitters.getptr(p_emitter);
        ERR_FAIL_NULL(emitter);
        emitter->intensity = p_intensity;
        _refresh_spawning(*emitter);
    }

    Dictionary SmokeServer::smoke_get_statistics() const {
        int built = 0;
        for (const KeyValue<RID, Emitter> &item: emitters) {
            if (item.value.particles.is_valid()) {
                built++;
            }
        }
        Dictionary statistics;
        statistics["total"] = static_cast<int>(emitters.size());
        statistics["built"] = built;
        return statistics;
    }

    Ref<Texture2D> SmokeServer::smoke_get_atlas() const {
        const String path = ProjectSettings::get_singleton()->get_setting(ATLAS_SETTING, "");
        if (path.is_empty()) {
            return Ref<Texture2D>();
        }
        return ResourceLoader::get_singleton()->load(path);
    }

    double SmokeServer::smoke_get_density(const Kind p_kind) const {
        const double density = ProjectSettings::get_singleton()->get_setting(
                p_kind == KIND_DYNAMIC ? DYNAMIC_DENSITY_SETTING : STATIC_DENSITY_SETTING, DEFAULT_DENSITY);
        return MAX(density, 0.0);
    }

    double SmokeServer::smoke_get_lifetime(const Ref<SmokeSource> &p_source, const Kind p_kind) const {
        ERR_FAIL_COND_V(p_source.is_null(), 0.0);
        const ProjectSettings *project_settings = ProjectSettings::get_singleton();
        const double scale = p_kind == KIND_DYNAMIC ? static_cast<double>(project_settings->get_setting(
                                                              DYNAMIC_LIFETIME_SETTING, DEFAULT_DYNAMIC_LIFETIME))
                                                    : static_cast<double>(project_settings->get_setting(
                                                              STATIC_LIFETIME_SETTING, DEFAULT_STATIC_LIFETIME));
        return p_source->get_particle_lifetime() * MAX(scale, 0.0);
    }

    int SmokeServer::smoke_get_max_particles(const Kind p_kind) const {
        const ProjectSettings *project_settings = ProjectSettings::get_singleton();
        if (p_kind == KIND_DYNAMIC) {
            return project_settings->get_setting(DYNAMIC_MAX_PARTICLES_SETTING, DEFAULT_DYNAMIC_MAX_PARTICLES);
        }
        return project_settings->get_setting(STATIC_MAX_PARTICLES_SETTING, DEFAULT_STATIC_MAX_PARTICLES);
    }

    /// Made once per kind, source and sprite at the settings now; null when the source gives no
    /// particle (it never fades, or its rate is none)
    const SmokeServer::RenderData *SmokeServer::_get_render_data(const Emitter &p_emitter) {
        HashMap<uint64_t, RenderData> &by_sprite = render_data[p_emitter.kind][p_emitter.source->get_instance_id()];
        const uint64_t sprite_id = p_emitter.sprite.is_valid() ? p_emitter.sprite->get_instance_id() : 0;
        if (const RenderData *known = by_sprite.getptr(sprite_id); known != nullptr) {
            return known->mesh.is_valid() ? known : nullptr;
        }
        RenderData &data = by_sprite[sprite_id];
        const SmokeSource &source = *p_emitter.source.ptr();
        const double density = smoke_get_density(p_emitter.kind);
        const double lifetime = smoke_get_lifetime(p_emitter.source, p_emitter.kind);
        const int amount = source.get_particle_amount(smoke_get_max_particles(p_emitter.kind), density, lifetime);
        if (Math::is_zero_approx(lifetime) || amount == 0) {
            WARN_PRINT(vformat("[SmokeServer] The smoke source emits nothing: %s", source.get_name()));
            return nullptr;
        }
        const double terminal_size = source.get_terminal_size(lifetime);
        const double reach = (source.get_velocity_max() * lifetime) + terminal_size;

        Ref<ParticleProcessMaterial> material;
        material.instantiate();
        // every particle leaves along the emitter's up axis, within the cone
        material->set_direction(Vector3(0.0, 1.0, 0.0));
        material->set_spread(static_cast<float>(source.get_inclination_max()));
        material->set_param_min(
                ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, static_cast<float>(source.get_velocity_min()));
        material->set_param_max(
                ParticleProcessMaterial::PARAM_INITIAL_LINEAR_VELOCITY, static_cast<float>(source.get_velocity_max()));
        material->set_param_min(ParticleProcessMaterial::PARAM_ANGLE, 0.0);
        material->set_param_max(ParticleProcessMaterial::PARAM_ANGLE, FULL_TURN_DEGREES);
        material->set_param_min(ParticleProcessMaterial::PARAM_SCALE, static_cast<float>(source.get_size_min()));
        material->set_param_max(ParticleProcessMaterial::PARAM_SCALE, static_cast<float>(source.get_size_max()));
        material->set_param_texture(
                ParticleProcessMaterial::PARAM_SCALE,
                make_curve(1.0, terminal_size / MAX(source.get_mean_size(), MIN_MEAN_SIZE)));
        material->set_color(source.get_color());
        material->set_color_initial_ramp(make_opacity_ramp(source, density));
        material->set_alpha_curve(make_curve(1.0, 0.0));
        // the flipbook is walked exactly once over a particle's lifetime
        material->set_param_min(ParticleProcessMaterial::PARAM_ANIM_SPEED, 1.0);
        material->set_param_max(ParticleProcessMaterial::PARAM_ANIM_SPEED, 1.0);
        // no gravity: the wind's drift takes its place (_apply_wind())
        material->set_gravity(Vector3());

        Ref<StandardMaterial3D> sprite_material;
        sprite_material.instantiate();
        sprite_material->set_transparency(BaseMaterial3D::TRANSPARENCY_ALPHA);
        sprite_material->set_blend_mode(BaseMaterial3D::BLEND_MODE_MIX);
        sprite_material->set_shading_mode(BaseMaterial3D::SHADING_MODE_UNSHADED);
        sprite_material->set_billboard_mode(BaseMaterial3D::BILLBOARD_PARTICLES);
        sprite_material->set_flag(BaseMaterial3D::FLAG_BILLBOARD_KEEP_SCALE, true);
        sprite_material->set_flag(BaseMaterial3D::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
        sprite_material->set_cull_mode(BaseMaterial3D::CULL_DISABLED);
        // a plume does not hide what is behind its particles
        sprite_material->set_depth_draw_mode(BaseMaterial3D::DEPTH_DRAW_DISABLED);
        if (p_emitter.sprite.is_valid()) {
            sprite_material->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, p_emitter.sprite);
        } else if (const Ref<Texture2D> atlas = smoke_get_atlas(); atlas.is_valid()) {
            const Vector2i frames = ProjectSettings::get_singleton()->get_setting(
                    ATLAS_FRAMES_SETTING, Vector2i(DEFAULT_ATLAS_FRAMES, DEFAULT_ATLAS_FRAMES));
            sprite_material->set_texture(BaseMaterial3D::TEXTURE_ALBEDO, atlas);
            sprite_material->set_particles_anim_h_frames(MAX(frames.x, 1));
            sprite_material->set_particles_anim_v_frames(MAX(frames.y, 1));
            // one pass over the flipbook per particle, not a loop
            sprite_material->set_particles_anim_loop(false);
        } else {
            WARN_PRINT(
                    vformat("[SmokeServer] The core's smoke needs %s; the particles are drawn plain", ATLAS_SETTING));
        }
        Ref<QuadMesh> mesh;
        mesh.instantiate();
        mesh->set_size(Vector2(1.0, 1.0));
        mesh->set_material(sprite_material);

        data.process_material = material;
        data.mesh = mesh;
        data.amount = amount;
        data.lifetime = lifetime;
        data.spawn_rate = source.get_spawn_rate() * density;
        data.preprocess = MIN(lifetime, PREPROCESS_SECONDS);
        data.aabb =
                AABB(Vector3(-reach, -terminal_size, -reach),
                     Vector3(reach * 2.0, reach + (terminal_size * 2.0), reach * 2.0));
        return &data;
    }

    void SmokeServer::_build(Emitter &p_emitter) {
        if (p_emitter.particles.is_valid() || p_emitter.source.is_null() ||
            !static_cast<bool>(ProjectSettings::get_singleton()->get_setting(ENABLED_SETTING, DEFAULT_ENABLED))) {
            return;
        }
        const RenderData *data = _get_render_data(p_emitter);
        if (data == nullptr) {
            return;
        }
        p_emitter.process_material = data->process_material;
        _apply_wind(p_emitter);
        p_emitter.amount = data->amount;
        p_emitter.spawn_rate = static_cast<float>(data->spawn_rate);

        RenderingServer *rs = RenderingServer::get_singleton();
        p_emitter.particles = rs->particles_create();
        rs->particles_set_mode(p_emitter.particles, RenderingServer::PARTICLES_MODE_3D);
        // the particles stay where they were born, the plume trails behind a moving emitter
        rs->particles_set_use_local_coordinates(p_emitter.particles, false);
        rs->particles_set_amount(p_emitter.particles, data->amount);
        rs->particles_set_lifetime(p_emitter.particles, data->lifetime);
        rs->particles_set_process_material(p_emitter.particles, data->process_material->get_rid());
        rs->particles_set_draw_passes(p_emitter.particles, 1);
        rs->particles_set_draw_pass_mesh(p_emitter.particles, 0, data->mesh->get_rid());
        rs->particles_set_draw_order(p_emitter.particles, RenderingServer::PARTICLES_DRAW_ORDER_VIEW_DEPTH);
        // the box follows the emitter, not the plume: a fast vehicle's trail is culled with it
        rs->particles_set_custom_aabb(p_emitter.particles, data->aabb);
        if (p_emitter.kind == KIND_DYNAMIC) {
            // the rate follows the owner, so _process_smoke() spawns by hand
            rs->particles_set_emitting(p_emitter.particles, false);
        } else {
            // a standing emitter's rate never changes: the engine emits for it, its plume already
            // in the air when it is built
            rs->particles_set_pre_process_time(p_emitter.particles, data->preprocess);
            rs->particles_set_emitting(p_emitter.particles, p_emitter.visible);
        }
        p_emitter.particles_instance = rs->instance_create();
        rs->instance_set_base(p_emitter.particles_instance, p_emitter.particles);
        rs->instance_set_scenario(p_emitter.particles_instance, p_emitter.scenario);
        rs->instance_set_visible(p_emitter.particles_instance, p_emitter.visible);
        rs->instance_set_transform(p_emitter.particles_instance, p_emitter.transform);
        _refresh_spawning(p_emitter);
    }

    void SmokeServer::_clear(Emitter &p_emitter) {
        p_emitter.process_material.unref();
        RenderingServer *rs = RenderingServer::get_singleton();
        if (p_emitter.particles_instance.is_valid()) {
            rs->free_rid(p_emitter.particles_instance);
            p_emitter.particles_instance = RID();
        }
        if (p_emitter.particles.is_valid()) {
            rs->free_rid(p_emitter.particles);
            p_emitter.particles = RID();
        }
        _refresh_spawning(p_emitter);
    }

    void SmokeServer::_stream_build(const RID &p_emitter, const Variant & /*p_preloaded*/) {
        if (Emitter *emitter = emitters.getptr(p_emitter); emitter != nullptr) {
            _build(*emitter);
        }
    }

    void SmokeServer::_stream_clear(const RID &p_emitter) {
        if (Emitter *emitter = emitters.getptr(p_emitter); emitter != nullptr) {
            _clear(*emitter);
        }
    }

    /// The emitter spawns while it is moving, visible, built and of an intensity above zero
    void SmokeServer::_refresh_spawning(Emitter &p_emitter) {
        const bool spawns = p_emitter.kind == KIND_DYNAMIC && p_emitter.visible && p_emitter.particles.is_valid() &&
                            p_emitter.intensity > 0.0;
        if (spawns == p_emitter.ordered) {
            return;
        }
        p_emitter.ordered = spawns;
        if (spawns) {
            // nothing is owed for the time it did not spawn
            p_emitter.last_spawn_usec = time->get_ticks_usec();
            spawning.push_back(&p_emitter);
        } else {
            spawning.erase(&p_emitter);
        }
        _set_processing(!spawning.is_empty());
    }

    void SmokeServer::_apply_wind(const Emitter &p_emitter) const {
        if (p_emitter.process_material.is_valid()) {
            p_emitter.process_material->set_gravity(wind * WIND_ACCELERATION);
        }
    }

    /// The tick runs only while an emitter spawns
    void SmokeServer::_set_processing(const bool p_processing) {
        if (processing == p_processing) {
            return;
        }
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree == nullptr) {
            return;
        }
        processing = p_processing;
        if (p_processing) {
            tree->connect("process_frame", callable_mp(this, &SmokeServer::_process_smoke));
            return;
        }
        tree->disconnect("process_frame", callable_mp(this, &SmokeServer::_process_smoke));
    }

    /// Spawns the particles the moving emitters owe this frame, by hand: their rate follows their
    /// owner, and Godot's only knob for a rate - amount_ratio - takes live particles away instead of
    /// spawning fewer, which cut a whole plume off at once. Fractional particles are carried over,
    /// so a rate below one a second still spawns.
    void SmokeServer::_process_smoke() {
        const int size = static_cast<int>(spawning.size());
        if (size == 0) {
            return;
        }
        const uint64_t now = time->get_ticks_usec();
        RenderingServer *rs = RenderingServer::get_singleton();
        const int visited = MIN(size, MAX_EMITTERS_PER_FRAME);
        for (int i = 0; i < visited; i++) {
            if (spawn_cursor >= size) {
                spawn_cursor = 0;
            }
            Emitter &emitter = *spawning[spawn_cursor];
            spawn_cursor++;
            const double delta = static_cast<double>(now - emitter.last_spawn_usec) / Units::USEC_PER_SECOND;
            emitter.last_spawn_usec = now;
            emitter.spawn_backlog =
                    static_cast<float>(emitter.spawn_backlog + (emitter.spawn_rate * emitter.intensity * delta));
            const int count = MIN(static_cast<int>(emitter.spawn_backlog), emitter.amount);
            if (count < 1) {
                continue;
            }
            emitter.spawn_backlog -= static_cast<float>(count);
            // only where a particle is born is given; its velocity, size, roll and colour are the
            // process material's
            for (int particle = 0; particle < count; particle++) {
                rs->particles_emit(
                        emitter.particles, emitter.transform, Vector3(), Color(), Color(),
                        GPUParticles3D::EMIT_FLAG_POSITION);
            }
        }
    }

    /// A change of what the emitters are built of builds them all again; a registered one is
    /// registered again, the distance of its kind may be another
    void SmokeServer::_on_settings_changed() {
        const Array now = read_settings();
        if (now == settings) {
            return;
        }
        settings = now;
        for (HashMap<uint64_t, HashMap<uint64_t, RenderData>> &by_kind: render_data) {
            by_kind.clear();
        }
        for (KeyValue<RID, Emitter> &item: emitters) {
            _clear(item.value);
            if (item.value.registered) {
                _register_stream(item.key, item.value);
            } else {
                _build(item.value);
            }
        }
    }

    /// The whole world shares one wind, so it reaches every emitter - and the materials the
    /// standing ones share
    void SmokeServer::_on_weather_changed() {
        const SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        const Vector3 now =
                simulation->get_wind_direction().normalized() * static_cast<float>(simulation->get_wind_speed());
        if (wind.is_equal_approx(now)) {
            return;
        }
        wind = now;
        for (const KeyValue<RID, Emitter> &item: emitters) {
            _apply_wind(item.value);
        }
    }

    /// The sources were the old data's: what they were built into goes with them, and the emitters
    /// built of them are built again by their owners
    void SmokeServer::_on_data_unload_requested() {
        for (HashMap<uint64_t, HashMap<uint64_t, RenderData>> &by_kind: render_data) {
            by_kind.clear();
        }
    }
} // namespace godot
