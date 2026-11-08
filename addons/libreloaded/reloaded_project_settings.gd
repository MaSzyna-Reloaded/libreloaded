@tool
class_name ReloadedProjectSettings
extends RefCounted

## libreloaded's project settings read in GDScript, each with its default value - its owner's
## constant - and its hint, registered by the editor plugin (libreloaded.gd) and as the game starts
## (ReloadedPlugins, the first autoload), so the game knows their default values too: a setting at its
## default value is not saved in project.godot. The settings read in C++ are registered by their
## owners as the library starts (initialize_libreloaded_module(), ReloadedProjectSetting)

const TRACTION: GDScript = preload("traction/traction_rendering_server.gd")
const VEHICLE_CATALOG: GDScript = preload("vehicles/vehicle_catalog_server.gd")


static func register() -> void:
    # The original renders shadow maps with front faces culled (opengl33renderer.cpp:1758) against
    # self-shadowing acne; off by default like in Godot, because an open single-sided model then casts
    # no shadow and the acne only moves to the unlit side
    ReloadedProjectSetting.add(SkyEnvironment.REVERSE_CULL_FACE_SETTING, SkyEnvironment.REVERSE_CULL_FACE, TYPE_BOOL)
    # The cab gets a sun of its own with sharp shadows over its few metres, and the world's sun keeps
    # its far cascades in the cab view too; costs a second shadow map and a twice larger atlas
    ReloadedProjectSetting.add(
        SkyEnvironment.CABIN_SHADOWS_IMPROVED_SETTING, SkyEnvironment.CABIN_SHADOWS_IMPROVED, TYPE_BOOL
    )
    # How far the camera sees (Player.FAR_DISTANCE_SETTING): a world's far terrain, drawn outside the
    # streaming, past its draw distance
    ReloadedProjectSetting.add(
        Player.FAR_DISTANCE_SETTING, Player.FAR_DISTANCE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,50000.0,10.0,suffix:m"
    )
    # The backend a traction whose data names none is drawn by (TractionData.rendering_backend)
    ReloadedProjectSetting.add(TRACTION.RENDERING_BACKEND_SETTING, TRACTION.PLAIN_BACKEND, TYPE_STRING_NAME)
    # How close and how far the zoom takes the external view (Shift+F4) from what it looks at
    ReloadedProjectSetting.add(
        FollowView.MIN_DISTANCE_SETTING, FollowView.MIN_DISTANCE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,20.0,0.1,suffix:m"
    )
    ReloadedProjectSetting.add(
        FollowView.MAX_DISTANCE_SETTING, FollowView.MAX_DISTANCE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "10.0,2000.0,10.0,suffix:m"
    )
    # A vehicle to follow farther than this from the view is not flown to: the view jumps beside it
    # (the vehicle card's crosshair) and follows from there
    ReloadedProjectSetting.add(
        Player.FOLLOW_JUMP_DISTANCE_SETTING, Player.FOLLOW_JUMP_DISTANCE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "100.0,100000.0,100.0,suffix:m"
    )
    # Skydome's volumetric fog volume is 8 m deep by day and 3 m at night, and it is measured from
    # the camera - a light shaft is only visible while its lamp is inside it. This stretches the
    # volume while the density is divided by the same factor, which leaves the optical depth, and
    # so the look of the fog, alone. At 24 that is 72 m by night and 192 m by day.
    ReloadedProjectSetting.add(
        SkyEnvironment.FOG_VOLUMETRIC_LENGTH_SCALE_SETTING, SkyEnvironment.FOG_VOLUMETRIC_LENGTH_SCALE_DEFAULT,
        TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "1.0,64.0,0.5"
    )
    # Floor under fog_volumetric_far_falloff. A scenery asking for a fog of kilometres drives that
    # falloff to 0.01-0.04 and leaves no haze by the camera at all, so a street lamp has nothing to
    # scatter in and casts no visible shaft. Night air is never that clean.
    ReloadedProjectSetting.add(
        SkyEnvironment.FOG_VOLUMETRIC_MINIMUM_SETTING, SkyEnvironment.FOG_VOLUMETRIC_MINIMUM_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    # how sharp the shadows are (SkyEnvironment.get_shadow_atlas_size())
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_TEXTURE_QUALITY_SETTING, SkyEnvironment.SHADOW_TEXTURE_QUALITY, TYPE_INT,
        PROPERTY_HINT_ENUM, ",".join(PackedStringArray(SkyEnvironment.ShadowTextureQuality.keys().map(
            func(name: String) -> String: return name.capitalize())))
    )
    ReloadedProjectSetting.add(SkyEnvironment.SHADOW_SCENERY_ENABLED_SETTING, SkyEnvironment.SHADOW_ENABLED, TYPE_BOOL)
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_SCENERY_MODE_SETTING, SkyEnvironment.SHADOW_SCENERY_MODE, TYPE_INT,
        PROPERTY_HINT_ENUM, "Orthogonal,PSSM 2 Splits,PSSM 4 Splits"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_SCENERY_BLUR_SETTING, SkyEnvironment.SHADOW_SCENERY_BLUR, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.01,or_greater"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_SCENERY_OPACITY_SETTING, SkyEnvironment.SHADOW_OPACITY, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_SCENERY_BIAS_SETTING, SkyEnvironment.SHADOW_BIAS, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.001,or_greater"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_SCENERY_NORMAL_BIAS_SETTING, SkyEnvironment.SHADOW_SCENERY_NORMAL_BIAS, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,32.0,0.001,or_greater"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_SCENERY_MAX_DISTANCE_SETTING, SkyEnvironment.SHADOW_SCENERY_MAX_DISTANCE, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10000.0,1.0,suffix:m"
    )
    ReloadedProjectSetting.add(SkyEnvironment.SHADOW_SCENERY_BLEND_SPLITS_SETTING, SkyEnvironment.SHADOW_BLEND_SPLITS, TYPE_BOOL)
    ReloadedProjectSetting.add(SkyEnvironment.SHADOW_CABIN_ENABLED_SETTING, SkyEnvironment.SHADOW_ENABLED, TYPE_BOOL)
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_CABIN_MODE_SETTING, SkyEnvironment.SHADOW_CABIN_MODE, TYPE_INT,
        PROPERTY_HINT_ENUM, "Orthogonal,PSSM 2 Splits,PSSM 4 Splits"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_CABIN_BLUR_SETTING, SkyEnvironment.SHADOW_CABIN_BLUR, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.01,or_greater"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_CABIN_OPACITY_SETTING, SkyEnvironment.SHADOW_OPACITY, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_CABIN_BIAS_SETTING, SkyEnvironment.SHADOW_BIAS, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10.0,0.001,or_greater"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_CABIN_NORMAL_BIAS_SETTING, SkyEnvironment.SHADOW_CABIN_NORMAL_BIAS, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,32.0,0.001,or_greater"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.SHADOW_CABIN_MAX_DISTANCE_SETTING, SkyEnvironment.SHADOW_CABIN_MAX_DISTANCE, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,10000.0,1.0,suffix:m"
    )
    ReloadedProjectSetting.add(SkyEnvironment.SHADOW_CABIN_BLEND_SPLITS_SETTING, SkyEnvironment.SHADOW_BLEND_SPLITS, TYPE_BOOL)
    for i: int in SkyEnvironment.SHADOW_SCENERY_SPLIT_SETTINGS.size():
        ReloadedProjectSetting.add(
            SkyEnvironment.SHADOW_SCENERY_SPLIT_SETTINGS[i], SkyEnvironment.SHADOW_SCENERY_SPLITS[i],
            TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.001"
        )
        ReloadedProjectSetting.add(
            SkyEnvironment.SHADOW_CABIN_SPLIT_SETTINGS[i], SkyEnvironment.SHADOW_CABIN_SPLITS[i],
            TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.001"
        )
    ReloadedProjectSetting.add(
        SkyEnvironment.VOLUMETRIC_FOG_ENERGY_SETTING, SkyEnvironment.VOLUMETRIC_FOG_ENERGY_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,16.0,0.001,or_greater"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.FOG_CURVE_SETTING, SkyEnvironment.FOG_CURVE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_EXP_EASING, "positive_only"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.FOG_AERIAL_PERSPECTIVE_SETTING,
        SkyEnvironment.FOG_AERIAL_PERSPECTIVE_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.RAIN_FOG_DISTANCE_SETTING, SkyEnvironment.RAIN_FOG_DISTANCE_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "10.0,5000.0,10.0,or_greater,suffix:m"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.RAIN_FOG_DENSITY_SETTING, SkyEnvironment.RAIN_FOG_DENSITY_DEFAULT,
        TYPE_FLOAT, PROPERTY_HINT_RANGE, "0.0,1.0,0.01"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.FOG_VOLUMETRIC_FAR_FALLOFF_SETTING,
        SkyEnvironment.FOG_VOLUMETRIC_FAR_FALLOFF_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "1.0,4.0,0.05"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_SETTING,
        SkyEnvironment.FOG_SCENERY_DISTANCE_FACTOR_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.1,4.0,0.01,or_greater"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.FOG_DAY_DISTANCE_FACTOR_SETTING,
        SkyEnvironment.FOG_DAY_DISTANCE_FACTOR_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.01,4.0,0.0001,or_greater"
    )
    ReloadedProjectSetting.add(
        SkyEnvironment.FOG_NIGHT_DISTANCE_FACTOR_SETTING,
        SkyEnvironment.FOG_NIGHT_DISTANCE_FACTOR_DEFAULT, TYPE_FLOAT,
        PROPERTY_HINT_RANGE, "0.01,4.0,0.0001,or_greater"
    )
    # The provider a vehicle of no provider named is the one of, and where the player's liveries are
    # kept (VehicleCatalogServer)
    ReloadedProjectSetting.add(VEHICLE_CATALOG.PROVIDER_SETTING, &"", TYPE_STRING_NAME)
    ReloadedProjectSetting.add(
        VEHICLE_CATALOG.LIVERIES_DIRECTORY_SETTING, VEHICLE_CATALOG.LIVERIES_DIRECTORY, TYPE_STRING, PROPERTY_HINT_DIR
    )
