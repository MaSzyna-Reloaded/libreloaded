extends ReloadedGutTest

## A preset of the user settings sets all its values at once, each as the type its setting declares,
## and the settings tell which preset they match - a setting never set by its declared default

## Settings of the tests' own, declared by the fixture, and the file's preset of them
const FIXTURE: String = "res://tests/fixtures/user_settings_presets.json"
const SECTION: String = "tests_presets"
const QUALITY: String = "quality"
const SHADOWS: String = "shadows"
const DISTANCE: String = "reloaded/tests/presets_distance"
const FILE_PRESET: String = "tests_from_file"
## UserSettings' section of the player's values of project settings
const PROJECT_SETTINGS_SECTION: String = "project_settings"
const LOW: String = "tests_low"
const DEFAULTS: String = "tests_defaults"
const MERGED: String = "tests_merged"
## The fixture's declared defaults
const DEFAULT_QUALITY: int = 1
const DEFAULT_DISTANCE: float = 100.0
const LOW_DISTANCE: float = 50.0


## Adds its value of SHADOWS to the preset MERGED
class PresetsProvider extends UserSettingsProvider:
    func _get_presets() -> Array:
        return [{"name": MERGED, "values": {SECTION: {SHADOWS: false}}}]


func before_all() -> void:
    var provider: UserSettingsFileProvider = UserSettingsFileProvider.new()
    provider.path = FIXTURE
    UserSettings.provider_add(provider)


func after_each() -> void:
    for preset: String in [LOW, DEFAULTS, MERGED]:
        if UserSettings.preset_list().has(preset):
            UserSettings.preset_delete(preset)
    UserSettings.erase_setting(SECTION, QUALITY)
    UserSettings.erase_setting(SECTION, SHADOWS)
    UserSettings.erase_setting(PROJECT_SETTINGS_SECTION, DISTANCE)


func after_all() -> void:
    UserSettings.preset_delete(FILE_PRESET)


func _create_low() -> void:
    # numbers as JSON gives them, all floats
    UserSettings.preset_create(LOW, "Low", {
        SECTION: {QUALITY: 0.0, SHADOWS: false},
        PROJECT_SETTINGS_SECTION: {DISTANCE: LOW_DISTANCE},
    })


func test_a_provider_s_file_gives_its_presets() -> void:
    assert_has(UserSettings.preset_list(), FILE_PRESET)
    var preset: Dictionary = UserSettings.preset_get(FILE_PRESET)
    assert_eq(preset["title"], "From file")
    assert_eq(preset["values"][SECTION][QUALITY], 2.0, "the file's value")


func test_a_preset_sets_every_value_at_once_as_its_declared_type() -> void:
    _create_low()
    watch_signals(UserSettings)

    UserSettings.preset_set(LOW)

    var quality: Variant = UserSettings.get_setting(SECTION, QUALITY)
    assert_eq(typeof(quality), TYPE_INT, "an int setting is an int")
    assert_eq(quality, 0)
    assert_eq(UserSettings.get_setting(SECTION, SHADOWS), false)
    assert_eq(ProjectSettings.get_setting(DISTANCE), LOW_DISTANCE, "a project setting on ProjectSettings")
    assert_signal_emit_count(UserSettings, "setting_changed", 3, "each change announced")
    assert_signal_emit_count(UserSettings, "config_changed", 1, "the configuration's change once")


func test_the_settings_match_the_preset_set_until_one_changes() -> void:
    _create_low()
    UserSettings.preset_set(LOW)
    assert_eq(UserSettings.preset_get_current(), LOW)

    UserSettings.set_setting(SECTION, SHADOWS, true)

    assert_ne(UserSettings.preset_get_current(), LOW, "no longer the preset")


func test_a_setting_never_set_matches_by_its_declared_default() -> void:
    UserSettings.preset_create(DEFAULTS, "Defaults", {
        SECTION: {QUALITY: DEFAULT_QUALITY, SHADOWS: true},
        PROJECT_SETTINGS_SECTION: {DISTANCE: DEFAULT_DISTANCE},
    })

    assert_eq(UserSettings.preset_get_current(), DEFAULTS)


func test_the_settings_match_no_preset() -> void:
    _create_low()
    UserSettings.set_setting(SECTION, QUALITY, DEFAULT_QUALITY + 1)

    assert_eq(UserSettings.preset_get_current(), "")


func test_a_provider_s_values_join_the_preset_of_their_name() -> void:
    UserSettings.preset_create(MERGED, "Merged", {SECTION: {QUALITY: 0}})

    UserSettings.provider_add(PresetsProvider.new())

    var preset: Dictionary = UserSettings.preset_get(MERGED)
    assert_eq(preset["title"], "Merged", "the title kept")
    assert_eq(preset["values"][SECTION], {QUALITY: 0, SHADOWS: false}, "both values")


func test_a_deleted_preset_is_gone() -> void:
    _create_low()

    UserSettings.preset_delete(LOW)

    assert_does_not_have(UserSettings.preset_list(), LOW)
