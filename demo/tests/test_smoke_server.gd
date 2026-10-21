extends ReloadedGutTest

## What SmokeServer builds an emitter of: a SmokeSource's derived values, the flipbook of the core's
## own smoke and the settings of each kind of emitter.

const ATLAS:String = "res://vfx/smoke_atlas.png"

var _previous_atlas:String = ""
var _previous_dynamic_density:float = 1.0
var _previous_static_density:float = 1.0
var _previous_dynamic_lifetime:float = 1.0
var _previous_static_lifetime:float = 0.5


func before_each() -> void:
    _previous_atlas = ProjectSettings.get_setting("reloaded/smoke/modern/atlas", "")
    _previous_dynamic_density = ProjectSettings.get_setting("reloaded/smoke/dynamic/density", 1.0)
    _previous_static_density = ProjectSettings.get_setting("reloaded/smoke/static/density", 1.0)
    _previous_dynamic_lifetime = ProjectSettings.get_setting("reloaded/smoke/dynamic/lifetime", 1.0)
    _previous_static_lifetime = ProjectSettings.get_setting("reloaded/smoke/static/lifetime", 0.5)


func after_each() -> void:
    ProjectSettings.set_setting("reloaded/smoke/modern/atlas", _previous_atlas)
    ProjectSettings.set_setting("reloaded/smoke/dynamic/density", _previous_dynamic_density)
    ProjectSettings.set_setting("reloaded/smoke/static/density", _previous_static_density)
    ProjectSettings.set_setting("reloaded/smoke/dynamic/lifetime", _previous_dynamic_lifetime)
    ProjectSettings.set_setting("reloaded/smoke/static/lifetime", _previous_static_lifetime)


## 30 particles a second, 0.6 opacity fading at 0.15 a second, born 0.2-0.6 and growing by 0.8 a
## second up to 40
func _source() -> SmokeSource:
    var source := SmokeSource.new()
    source.spawn_rate = 30.0
    source.size_min = 0.2
    source.size_max = 0.6
    source.opacity_min = 0.3
    source.opacity_max = 0.6
    source.size_step = 0.8
    source.size_limit_min = 0.05
    source.size_limit_max = 40.0
    source.opacity_step = -0.15
    return source


func test_derives_the_lifetime_and_the_particle_budget() -> void:
    var source:SmokeSource = _source()

    # a particle dies when its opacity reaches zero
    assert_almost_eq(source.get_particle_lifetime(), 4.0, 0.001, "0.6 opacity faded at 0.15 per second")
    assert_eq(source.get_particle_amount(500, 1.0, 4.0), 120, "30 particles per second over one lifetime")
    assert_eq(source.get_particle_amount(64, 1.0, 4.0), 64, "the budget is capped")


func test_terminal_size_follows_the_linear_growth() -> void:
    var source:SmokeSource = _source()

    # mean initial size 0.4, growing by 0.8 per second over 4 seconds, under the 40.0 limit
    assert_almost_eq(source.get_terminal_size(4.0), 3.6, 0.001, "size grows linearly over the lifetime")
    assert_almost_eq(source.get_terminal_size(2.0), 2.0, 0.001, "half the lifetime grows half as far")


func test_the_core_smoke_draws_the_particles_with_the_flipbook() -> void:
    ProjectSettings.set_setting("reloaded/smoke/modern/atlas", ATLAS)

    var atlas:Texture2D = SmokeServer.smoke_get_atlas()

    assert_not_null(atlas, "The core's smoke draws the particles with the project's own atlas")
    var frames:Vector2i = ProjectSettings.get_setting("reloaded/smoke/modern/atlas_frames", Vector2i.ONE)
    assert_eq(atlas.get_width() % frames.x, 0, "The atlas splits into whole columns")
    assert_eq(atlas.get_height() % frames.y, 0, "The atlas splits into whole rows")


func test_a_vehicle_and_the_scenery_take_their_own_density() -> void:
    ProjectSettings.set_setting("reloaded/smoke/dynamic/density", 3.0)
    ProjectSettings.set_setting("reloaded/smoke/static/density", 1.0)

    assert_almost_eq(
        SmokeServer.smoke_get_density(SmokeServer.KIND_DYNAMIC), 3.0, 0.001,
        "A vehicle smokes at the vehicle density")
    assert_almost_eq(
        SmokeServer.smoke_get_density(SmokeServer.KIND_STATIC), 1.0, 0.001,
        "A chimney smokes at the scenery density")


func test_the_density_multiplies_the_particle_budget() -> void:
    var source:SmokeSource = _source()

    # the pool has to grow with the rate, or the emitter runs out of slots and spawns less
    assert_eq(source.get_particle_amount(500, 1.0, 4.0), 120, "30 particles per second over one lifetime")
    assert_eq(source.get_particle_amount(500, 3.0, 4.0), 360, "Three times as dense needs three times the pool")


func test_a_static_emitter_is_given_a_shorter_lifetime() -> void:
    ProjectSettings.set_setting("reloaded/smoke/dynamic/lifetime", 1.0)
    ProjectSettings.set_setting("reloaded/smoke/static/lifetime", 0.5)
    var source:SmokeSource = _source()

    assert_almost_eq(
        SmokeServer.smoke_get_lifetime(source, SmokeServer.KIND_DYNAMIC), 4.0, 0.001,
        "A vehicle keeps the source's own lifetime")
    assert_almost_eq(
        SmokeServer.smoke_get_lifetime(source, SmokeServer.KIND_STATIC), 2.0, 0.001,
        "A static emitter is given half of it")


func test_each_kind_has_its_own_particle_budget() -> void:
    assert_eq(
        SmokeServer.smoke_get_max_particles(SmokeServer.KIND_DYNAMIC),
        ProjectSettings.get_setting("reloaded/smoke/dynamic/max_particles", 2000),
        "A vehicle draws on the dynamic budget")
    assert_eq(
        SmokeServer.smoke_get_max_particles(SmokeServer.KIND_STATIC),
        ProjectSettings.get_setting("reloaded/smoke/static/max_particles", 500),
        "A static emitter draws on the static one")
