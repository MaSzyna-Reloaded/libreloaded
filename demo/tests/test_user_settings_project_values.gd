extends ReloadedGutTest

## A player's value of a project setting the project had no value of yet - one a library registers
## as the game starts (ReloadedProjectSettings), after UserSettings applied the player's values -
## gives way to the setting's initial value when it is taken off, not to none: set to none, the
## setting was gone

const SETTING:String = "reloaded/tests/registered_after_the_player_s_value"
## UserSettings' section of the player's values of project settings
const PROJECT_SETTINGS_SECTION:String = "project_settings"
const PLAYER_VALUE:float = 5.0
const INITIAL_VALUE:float = 1.0
## A setting registered with ReloadedProjectSetting, and the value project.godot would give it
const REGISTERED_SETTING:String = "reloaded/tests/registered"
const PROJECT_VALUE:float = 3.0
## Settings the core reads in C++, registered by their owners as the library starts
const CORE_SETTINGS:Array[String] = [
    "reloaded/scenery/draw_distance",
    "reloaded/scenery/track_curve_bake_interval",
    "reloaded/scenery/tracks/distance",
    "reloaded/simulation/speed_change_time",
    "reloaded/smoke/dynamic/density",
    "reloaded/user_settings/declarations",
    "reloaded/vehicles/detail_distance",
]


func after_each() -> void:
    UserSettings.erase_setting(PROJECT_SETTINGS_SECTION, REGISTERED_SETTING)
    ProjectSettings.set_setting(SETTING, null)
    ProjectSettings.set_setting(REGISTERED_SETTING, null)


func test_a_setting_registered_later_goes_back_to_its_initial_value() -> void:
    assert_false(ProjectSettings.has_setting(SETTING), "the project has no value of it")
    UserSettings.set_setting(PROJECT_SETTINGS_SECTION, SETTING, PLAYER_VALUE)
    assert_eq(ProjectSettings.get_setting(SETTING), PLAYER_VALUE, "the player's value applied")
    # registered as the game starts, after the player's value
    ProjectSettings.set_initial_value(SETTING, INITIAL_VALUE)

    UserSettings.erase_setting(PROJECT_SETTINGS_SECTION, SETTING)

    assert_eq(ProjectSettings.get_setting(SETTING), INITIAL_VALUE, "the initial value, the setting kept")


func test_the_project_s_own_value_is_its_registered_default_whatever_the_player_set() -> void:
    ReloadedProjectSetting.add(REGISTERED_SETTING, INITIAL_VALUE, TYPE_FLOAT)
    UserSettings.set_setting(PROJECT_SETTINGS_SECTION, REGISTERED_SETTING, PLAYER_VALUE)

    assert_eq(ProjectSettings.get_setting(REGISTERED_SETTING), PLAYER_VALUE, "the player's value applied")
    assert_eq(UserSettings.get_project_value(REGISTERED_SETTING), INITIAL_VALUE, "the project's own value")


func test_the_project_s_own_value_is_the_one_project_godot_gives() -> void:
    # as project.godot gives it, before the registration
    ProjectSettings.set_setting(REGISTERED_SETTING, PROJECT_VALUE)
    ReloadedProjectSetting.add(REGISTERED_SETTING, INITIAL_VALUE, TYPE_FLOAT)
    UserSettings.set_setting(PROJECT_SETTINGS_SECTION, REGISTERED_SETTING, PLAYER_VALUE)

    assert_eq(UserSettings.get_project_value(REGISTERED_SETTING), PROJECT_VALUE, "project.godot's value")
    UserSettings.erase_setting(PROJECT_SETTINGS_SECTION, REGISTERED_SETTING)
    assert_eq(ProjectSettings.get_setting(REGISTERED_SETTING), PROJECT_VALUE, "project.godot's value back")


func test_the_settings_read_in_cpp_have_their_defaults_in_the_running_game() -> void:
    for setting: String in CORE_SETTINGS:
        assert_true(ProjectSettings.has_setting(setting), "%s has a value" % setting)
        assert_ne(ProjectSettings.property_get_revert(setting), null, "%s has its default" % setting)
