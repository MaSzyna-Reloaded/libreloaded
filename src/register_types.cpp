#include "cabin/Cabin3D.hpp"
#include "cabin/CabinHUDMouseSystem.hpp"
#include "cache/ResourceCache.hpp"
#include "driver/DriverImplementation.hpp"
#include "driver/DriverServer.hpp"
#include "driver/DriverState.hpp"
#include "game_data/GameDataServer.hpp"
#include "hud/HUDServer.hpp"
#include "loaders/OggVorbisFormatLoader.hpp"
#include "logging/GameLog.hpp"
#include "logging/GameLogFileHandler.hpp"
#include "logging/GameLogger.hpp"
#include "person/PersonServer.hpp"
#include "player/PlayerCameraServer.hpp"
#include "player/PlayerServer.hpp"
#include "player/TrainSoundListener3D.hpp"
#include "plugins/ReloadedPlugin.hpp"
#include "plugins/ReloadedPluginServer.hpp"
#include "radio/RadioServer.hpp"
#include "radio/SceneryVirtualRadio.hpp"
#include "register_types.h"
#include "rendering/ModelRenderingBackend.hpp"
#include "rendering/ModelRenderingServer.hpp"
#include "rendering/PlanarMirror3D.hpp"
#include "rendering/ReloadedMaterialServer.hpp"
#include "rendering/ReloadedModel3D.hpp"
#include "rendering/SceneModelRenderingBackend.hpp"
#include "rendering/SmokeServer.hpp"
#include "rendering/SmokeSource.hpp"
#include "rendering/StandardTrackRenderingBackend.hpp"
#include "rendering/TrackRenderingBackend.hpp"
#include "rendering/TrackRenderingLook.hpp"
#include "rendering/TrackRenderingServer.hpp"
#include "resources/ResourceLazyLoader.hpp"
#include "roads/RoadRouteSegment.hpp"
#include "roads/RoadServer.hpp"
#include "scenario/ScenarioEventAction.hpp"
#include "scenario/ScenarioEventCondition.hpp"
#include "scenario/ScenarioEventImplementation.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include "scenery/CompiledScenery.hpp"
#include "scenery/LinesData.hpp"
#include "scenery/ModelData.hpp"
#include "scenery/SceneryHUDMouseServer.hpp"
#include "scenery/SceneryLinesGeometry.hpp"
#include "scenery/SceneryMeshRenderingServer.hpp"
#include "scenery/SceneryModelPlacement.hpp"
#include "scenery/ScenerySoundPlacement.hpp"
#include "scenery/SceneryStreamingProvider.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "scenery/SignalBoxData.hpp"
#include "scenery/SoundData.hpp"
#include "scenery/StationData.hpp"
#include "scenery/TrainsetData.hpp"
#include "scenery/VehicleData.hpp"
#include "scripting/ScenarioScriptAction.hpp"
#include "scripting/ScenarioScriptCabinImplementation.hpp"
#include "scripting/ScenarioScriptServer.hpp"
#include "signalling/SignalAspect.hpp"
#include "signalling/SignalHeadKind.hpp"
#include "signalling/SignalHeadNode.hpp"
#include "signalling/SignallingImplementation.hpp"
#include "signalling/SignallingServer.hpp"
#include "signalling/SignallingSystemNode.hpp"
#include "simulation/SimulationRuntime.hpp"
#include "simulation/SimulationServer.hpp"
#include "station/StationServer.hpp"
#include "tracks/SpatialIndex.hpp"
#include "tracks/TrackCurve.hpp"
#include "tracks/TrackData.hpp"
#include "tracks/TrackEndpointRef.hpp"
#include "tracks/TrackServer.hpp"
#include "traction/PowerSourceData.hpp"
#include "traction/TractionData.hpp"
#include "traction/TractionServer.hpp"
#include "utils/ProcessMemory.hpp"
#include "utils/ReloadedTranslationServer.hpp"
#include "utils/Units.hpp"
#include "utils/UserSettings.hpp"
#include "utils/WorkerTaskQueue.hpp"
#include "vehicles/base/GenericVehicleComponent.hpp"
#include "vehicles/base/GenericVehicleComponentNode.hpp"
#include "vehicles/base/VehicleAppearance.hpp"
#include "vehicles/base/VehicleComponent.hpp"
#include "vehicles/base/VehicleComponentType.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/base/VehicleCurvePointItem.hpp"
#include "vehicles/base/VehicleImplementationServer.hpp"
#include "vehicles/base/VehiclePerson.hpp"
#include "vehicles/base/VehiclePersonRole.hpp"
#include "vehicles/base/VehiclePhysicsNode.hpp"
#include "vehicles/base/VehicleRenderingServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicle3D.hpp"
#include "vehicles/rail/RailVehicleAIHints.hpp"
#include "vehicles/rail/RailVehicleAppearance.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"
#include "vehicles/rail/RailVehicleBuffCoupl.hpp"
#include "vehicles/rail/RailVehicleCabinKind.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleComponentType.hpp"
#include "vehicles/rail/RailVehicleDieselElectricEngine.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"
#include "vehicles/rail/RailVehicleDimmerListItem.hpp"
#include "vehicles/rail/RailVehicleDoors.hpp"
#include "vehicles/rail/RailVehicleElectricEngine.hpp"
#include "vehicles/rail/RailVehicleElectricInductionEngine.hpp"
#include "vehicles/rail/RailVehicleElectricSeriesEngine.hpp"
#include "vehicles/rail/RailVehicleElectroPneumaticDynamicBrake.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"
#include "vehicles/rail/RailVehicleHeating.hpp"
#include "vehicles/rail/RailVehicleHorns.hpp"
#include "vehicles/rail/RailVehicleInverter.hpp"
#include "vehicles/rail/RailVehicleLightListItem.hpp"
#include "vehicles/rail/RailVehicleLighting.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"
#include "vehicles/rail/RailVehicleLoadListItem.hpp"
#include "vehicles/rail/RailVehicleMasterController.hpp"
#include "vehicles/rail/RailVehicleNeighbour.hpp"
#include "vehicles/rail/RailVehiclePhysicsNode.hpp"
#include "vehicles/rail/RailVehiclePowerSupply.hpp"
#include "vehicles/rail/RailVehicleRadio.hpp"
#include "vehicles/rail/RailVehicleRenderingServer.hpp"
#include "vehicles/rail/RailVehicleSecuritySystem.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include "vehicles/rail/RailVehicleSpeedControl.hpp"
#include "vehicles/rail/RailVehicleSpringBrake.hpp"
#include "vehicles/rail/RailVehicleSwitches.hpp"
#include "vehicles/rail/RailVehicleUniversalController.hpp"
#include "vehicles/rail/RailVehicleWheels.hpp"
#include "vehicles/rail/RailVehicleWiperListItem.hpp"
#include "vehicles/rail/RailVehicleWipers.hpp"
#include "vehicles/road/RoadVehicleAppearance.hpp"
#include "vehicles/road/RoadVehicleController.hpp"
#include "vehicles/road/RoadVehicleRenderingServer.hpp"
#include "vehicles/road/RoadVehicleServer.hpp"
#include "world/WorldData.hpp"
#include "world/WorldLayer.hpp"
#include "world/WorldSection.hpp"
#include <gdextension_interface.h>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

ReloadedPluginServer *reloaded_plugin_server_singleton = nullptr;
ModelRenderingServer *model_rendering_server_singleton = nullptr;
TrackRenderingServer *track_rendering_server_singleton = nullptr;
StandardTrackRenderingBackend *standard_track_rendering_backend = nullptr;
/// The core's own model renderer: Godot's scenes (SceneModelRenderingBackend::NAME)
SceneModelRenderingBackend *scene_model_rendering_backend = nullptr;
ReloadedMaterialServer *reloaded_material_server_singleton = nullptr;
GameLog *game_log_singleton = nullptr;
UserSettings *user_settings_singleton = nullptr;
SimulationServer *simulation_server_singleton = nullptr;
GameDataServer *game_data_server_singleton = nullptr;
ResourceLazyLoader *resource_lazy_loader_singleton = nullptr;
TrackServer *track_server_singleton = nullptr;
PersonServer *person_server_singleton = nullptr;
VehicleServer *vehicle_server_singleton = nullptr;
RailVehicleServer *rail_vehicle_server_singleton = nullptr;
RoadServer *road_server_singleton = nullptr;
RoadVehicleServer *road_vehicle_server_singleton = nullptr;
RailVehicleRenderingServer *rail_vehicle_rendering_server_singleton = nullptr;
RoadVehicleRenderingServer *road_vehicle_rendering_server_singleton = nullptr;
TractionServer *traction_server_singleton = nullptr;
SceneryStreamingServer *scenery_streaming_server_singleton = nullptr;
SceneryMeshRenderingServer *scenery_mesh_rendering_server_singleton = nullptr;
SmokeServer *smoke_server_singleton = nullptr;
ReloadedTranslationServer *maszyna_translation_server_singleton = nullptr;
CabinHUDMouseSystem *cabin_hud_mouse_system_singleton = nullptr;
SignallingServer *signalling_server_singleton = nullptr;
ScenarioEventServer *scenario_event_server_singleton = nullptr;
SceneryHUDMouseServer *scenery_hud_mouse_server_singleton = nullptr;
ScenarioScriptServer *scenario_script_server_singleton = nullptr;
DriverServer *driver_server_singleton = nullptr;
PlayerServer *player_server_singleton = nullptr;
StationServer *station_server_singleton = nullptr;
RadioServer *radio_server_singleton = nullptr;
PlayerCameraServer *player_camera_server_singleton = nullptr;
HUDServer *hud_server_singleton = nullptr;
Ref<OggVorbisFormatLoader> ogg_vorbis_format_loader;

void initialize_libreloaded_module(const ModuleInitializationLevel p_level) {
    UtilityFunctions::print("Initializing libreloaded module on level " + String::num(p_level) + "...");

    if (p_level == MODULE_INITIALIZATION_LEVEL_EDITOR) {
        //         GDREGISTER_CLASS(DieselEngineMasterControllerPowerItemEditor);
    }

    if (p_level == MODULE_INITIALIZATION_LEVEL_SCENE) {
        GDREGISTER_ABSTRACT_CLASS(UserSettings);
        GDREGISTER_CLASS(ProcessMemory);
        GDREGISTER_CLASS(SimulationServer);
        GDREGISTER_CLASS(SimulationRuntime);
        GDREGISTER_CLASS(GameDataServer);
        GDREGISTER_ABSTRACT_CLASS(ReloadedTranslationServer);
        GDREGISTER_CLASS(ResourceCache);
        GDREGISTER_CLASS(ResourceLazyLoader);
        GDREGISTER_CLASS(PlanarMirror3D);
        GDREGISTER_CLASS(ModelRenderingServer);
        GDREGISTER_ABSTRACT_CLASS(ModelRenderingBackend);
        GDREGISTER_ABSTRACT_CLASS(TrackRenderingBackend);
        GDREGISTER_ABSTRACT_CLASS(StandardTrackRenderingBackend);
        GDREGISTER_ABSTRACT_CLASS(TrackRenderingServer);
        GDREGISTER_CLASS(TrackRenderingLook);
        GDREGISTER_CLASS(SceneModelRenderingBackend);
        GDREGISTER_CLASS(ReloadedMaterialServer);
        GDREGISTER_CLASS(ReloadedModel3D);
        GDREGISTER_CLASS(SmokeSource);
        GDREGISTER_CLASS(SmokeServer);
        GDREGISTER_ABSTRACT_CLASS(PersonServer);
        GDREGISTER_ABSTRACT_CLASS(VehicleServer);
        GDREGISTER_CLASS(VehiclePerson);
        GDREGISTER_ABSTRACT_CLASS(VehicleImplementationServer);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleServer);
        GDREGISTER_ABSTRACT_CLASS(RoadServer);
        GDREGISTER_CLASS(RoadRouteSegment);
        GDREGISTER_ABSTRACT_CLASS(RoadVehicleServer);
        GDREGISTER_CLASS(RailVehicleNeighbour);
        GDREGISTER_CLASS(VehicleAppearance);
        GDREGISTER_ABSTRACT_CLASS(VehicleRenderingServer);
        GDREGISTER_CLASS(RailVehicleAppearance);
        GDREGISTER_CLASS(RailVehicleRenderingServer);
        GDREGISTER_CLASS(RoadVehicleAppearance);
        GDREGISTER_CLASS(RoadVehicleRenderingServer);
        GDREGISTER_ABSTRACT_CLASS(TractionServer);
        GDREGISTER_CLASS(SpatialIndex);
        GDREGISTER_CLASS(TrackEndpointRef);
        GDREGISTER_CLASS(TrackRouteSegment);
        GDREGISTER_CLASS(TrackBranchNeighbors);
        GDREGISTER_ABSTRACT_CLASS(TrackServer);
        GDREGISTER_ABSTRACT_CLASS(SignallingServer);
        GDREGISTER_CLASS(SignalAspect);
        GDREGISTER_CLASS(SignalHeadKind);
        // the scenery's data, saved by their class names (CODE_STYLE.md, "Saved data names no script")
        GDREGISTER_CLASS(TrackCurve);
        GDREGISTER_CLASS(TrackData);
        GDREGISTER_CLASS(TractionData);
        GDREGISTER_CLASS(PowerSourceData);
        GDREGISTER_CLASS(ModelData);
        GDREGISTER_CLASS(LinesData);
        GDREGISTER_CLASS(StationData);
        GDREGISTER_CLASS(SignalBoxData);
        GDREGISTER_CLASS(SoundData);
        GDREGISTER_CLASS(VehicleData);
        GDREGISTER_CLASS(TrainsetData);
        GDREGISTER_CLASS(CompiledScenery);
        GDREGISTER_CLASS(WorldLayer);
        GDREGISTER_CLASS(WorldSection);
        GDREGISTER_CLASS(WorldData);
        GDREGISTER_VIRTUAL_CLASS(SignallingImplementation);
        GDREGISTER_CLASS(SignalHeadNode);
        GDREGISTER_CLASS(SignallingSystemNode);
        GDREGISTER_ABSTRACT_CLASS(ScenarioEventServer);
        GDREGISTER_ABSTRACT_CLASS(SceneryHUDMouseServer);
        GDREGISTER_ABSTRACT_CLASS(DriverServer);
        GDREGISTER_VIRTUAL_CLASS(DriverImplementation);
        GDREGISTER_CLASS(DriverState);
        GDREGISTER_VIRTUAL_CLASS(ScenarioEventAction);
        GDREGISTER_VIRTUAL_CLASS(ScenarioEventCondition);
        GDREGISTER_VIRTUAL_CLASS(ScenarioEventImplementation);
        GDREGISTER_ABSTRACT_CLASS(ScenarioScriptServer);
        GDREGISTER_VIRTUAL_CLASS(ScenarioScriptCabinImplementation);
        GDREGISTER_ABSTRACT_CLASS(PlayerServer);
        GDREGISTER_ABSTRACT_CLASS(StationServer);
        GDREGISTER_ABSTRACT_CLASS(RadioServer);
        GDREGISTER_CLASS(SceneryVirtualRadio);
        GDREGISTER_ABSTRACT_CLASS(PlayerCameraServer);
        GDREGISTER_ABSTRACT_CLASS(HUDServer);
        GDREGISTER_INTERNAL_CLASS(ScenarioScriptAction);
        GDREGISTER_CLASS(WorkerTaskQueue);
        GDREGISTER_VIRTUAL_CLASS(SceneryStreamingProvider);
        GDREGISTER_CLASS(SceneryModelPlacement);
        GDREGISTER_CLASS(ScenerySoundPlacement);
        GDREGISTER_ABSTRACT_CLASS(SceneryStreamingServer);
        GDREGISTER_CLASS(SceneryLinesGeometry);
        GDREGISTER_ABSTRACT_CLASS(SceneryMeshRenderingServer);
        GDREGISTER_CLASS(OggVorbisFormatLoader);
        GDREGISTER_ABSTRACT_CLASS(VehicleComponentType);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleComponentType);
        GDREGISTER_ABSTRACT_CLASS(VehiclePersonRole);
        GDREGISTER_ABSTRACT_CLASS(Units);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleCabinKind);
        GDREGISTER_CLASS(VehiclePhysicsNode);
        GDREGISTER_CLASS(RailVehiclePhysicsNode);
        GDREGISTER_ABSTRACT_CLASS(VehicleComponent);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleComponent);
        GDREGISTER_CLASS(GenericVehicleComponent);
        GDREGISTER_CLASS(GenericVehicleComponentNode);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleBrake);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleSpringBrake);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleDoors);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleDieselEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleDieselElectricEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleElectricEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleElectricSeriesEngine);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleElectricInductionEngine);
        GDREGISTER_ABSTRACT_CLASS(VehicleController);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleController);
        GDREGISTER_CLASS(RoadVehicleController);
        GDREGISTER_CLASS(Cabin3D);
        GDREGISTER_CLASS(TrainSoundListener3D);
        GDREGISTER_ABSTRACT_CLASS(CabinHUDMouseSystem);
        GDREGISTER_CLASS(RailVehicle3D);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleHeating);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleRadio);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleWheels);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleSecuritySystem);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleHorns);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleAIHints);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleLighting);
        GDREGISTER_ABSTRACT_CLASS(GameLog);
        GDREGISTER_CLASS(GameLogger);
        GDREGISTER_CLASS(GameLogHandler);
        GDREGISTER_CLASS(GameLogFileHandler);
        GDREGISTER_CLASS(RailVehicleInverter);
        GDREGISTER_CLASS(RailVehicleLightListItem);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleElectroPneumaticDynamicBrake);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleLoad);
        GDREGISTER_CLASS(RailVehicleLoadListItem);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleBuffCoupl);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleSpeedControl);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleUniversalController);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleMasterController);
        GDREGISTER_ABSTRACT_CLASS(RailVehiclePowerSupply);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleEnginePowerSource);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleWipers);
        GDREGISTER_CLASS(RailVehicleWiperListItem);
        GDREGISTER_ABSTRACT_CLASS(RailVehicleSwitches);
        GDREGISTER_CLASS(RailVehicleDimmerListItem);
        GDREGISTER_CLASS(VehicleCurvePointItem);
        GDREGISTER_ABSTRACT_CLASS(ReloadedPlugin);
        GDREGISTER_CLASS(ReloadedPluginServer);

        // after every core class: the plugins' classes derive from them
        reloaded_plugin_server_singleton = memnew(ReloadedPluginServer);
        Engine::get_singleton()->register_singleton("ReloadedPluginServer", reloaded_plugin_server_singleton); // 0
        reloaded_plugin_server_singleton->plugins_register_classes();

        user_settings_singleton = memnew(UserSettings);
        Engine::get_singleton()->register_singleton("UserSettings", user_settings_singleton); // 1
        // after UserSettings is registered: the constructor follows the game directory; before
        // every server whose constructor creates a ResourceCache, which follows its cache clearing
        game_data_server_singleton = memnew(GameDataServer);
        Engine::get_singleton()->register_singleton("GameDataServer", game_data_server_singleton); // 1a
        // follows nothing; the plugins give it their materials' provider
        reloaded_material_server_singleton = memnew(ReloadedMaterialServer);
        Engine::get_singleton()->register_singleton("ReloadedMaterialServer", reloaded_material_server_singleton); // 1c
        // after GameDataServer is registered: the constructor follows its unloading; before the
        // streamed servers, which load through it
        resource_lazy_loader_singleton = memnew(ResourceLazyLoader);
        Engine::get_singleton()->register_singleton("ResourceLazyLoader", resource_lazy_loader_singleton); // 1b
        simulation_server_singleton = memnew(SimulationServer);
        game_log_singleton = memnew(GameLog);
        scenery_streaming_server_singleton = memnew(SceneryStreamingServer);
        track_server_singleton = memnew(TrackServer);
        cabin_hud_mouse_system_singleton = memnew(CabinHUDMouseSystem);

        Engine::get_singleton()->register_singleton("GameLog", game_log_singleton);                                // 3
        Engine::get_singleton()->register_singleton("SceneryStreamingServer", scenery_streaming_server_singleton); // 5
        // after SceneryStreamingServer is registered: the constructor makes its streaming owner
        scenery_mesh_rendering_server_singleton = memnew(SceneryMeshRenderingServer);
        Engine::get_singleton()->register_singleton(
                "SceneryMeshRenderingServer", scenery_mesh_rendering_server_singleton);               // 6b
        Engine::get_singleton()->register_singleton("SimulationServer", simulation_server_singleton); // 7
        Engine::get_singleton()->register_singleton("TrackServer", track_server_singleton);           // 8
        // after SimulationServer, SceneryStreamingServer and GameDataServer are registered: the
        // constructor follows the wind, makes its streaming owner and follows the data's unloading;
        // before the plugins' servers, whose models give off smoke
        smoke_server_singleton = memnew(SmokeServer);
        Engine::get_singleton()->register_singleton("SmokeServer", smoke_server_singleton); // 8a
        // after SimulationServer is registered: the constructor follows its clock
        person_server_singleton = memnew(PersonServer);
        Engine::get_singleton()->register_singleton("PersonServer", person_server_singleton); // 8b
        // after PersonServer is registered: the constructor follows its freed persons
        vehicle_server_singleton = memnew(VehicleServer);
        Engine::get_singleton()->register_singleton("VehicleServer", vehicle_server_singleton); // 9
        // after VehicleServer: a radio in a vehicle is where the vehicle is; before the servers that
        // wire radios or follow the ether
        radio_server_singleton = memnew(RadioServer);
        Engine::get_singleton()->register_singleton("RadioServer", radio_server_singleton); // 9a
        // after VehicleServer is registered: the constructor follows the vehicles' lifetime
        rail_vehicle_server_singleton = memnew(RailVehicleServer);
        Engine::get_singleton()->register_singleton("RailVehicleServer", rail_vehicle_server_singleton); // 10
        // after TrackServer is registered: a road laid along a track has its limit
        road_server_singleton = memnew(RoadServer);
        Engine::get_singleton()->register_singleton("RoadServer", road_server_singleton); // 10d
        // after VehicleServer and RoadServer are registered: the constructor follows the vehicles'
        // lifetime, and it steps the road vehicles
        road_vehicle_server_singleton = memnew(RoadVehicleServer);
        Engine::get_singleton()->register_singleton("RoadVehicleServer", road_vehicle_server_singleton); // 10e
        vehicle_server_singleton->implementation_register(
                RoadVehicleServer::IMPLEMENTATION_NAME, road_vehicle_server_singleton->get_instance_id());
        // after TrackServer is registered: the constructor follows its switches; before the plugins'
        // servers, which draw the signal heads' lights
        signalling_server_singleton = memnew(SignallingServer);
        Engine::get_singleton()->register_singleton("SignallingServer", signalling_server_singleton); // 10a
        // after SimulationServer, RailVehicleServer and TrackServer are registered: the constructor follows
        // their pause and speed, vehicles and isolated sections; before the plugins' servers, whose
        // animations queue its events
        scenario_event_server_singleton = memnew(ScenarioEventServer);
        Engine::get_singleton()->register_singleton("ScenarioEventServer", scenario_event_server_singleton); // 10b
        // after UserSettings and GameDataServer are registered: the constructor follows the
        // language and the data's reloads; before the plugins' servers, which give it the data's
        // catalogues
        maszyna_translation_server_singleton = memnew(ReloadedTranslationServer);
        Engine::get_singleton()->register_singleton(
                "ReloadedTranslationServer", maszyna_translation_server_singleton); // 13
        // before the plugins' servers, which register their model renderers with it
        model_rendering_server_singleton = memnew(ModelRenderingServer);
        Engine::get_singleton()->register_singleton("ModelRenderingServer", model_rendering_server_singleton); // 13a
        // after SceneryStreamingServer, which streams its placements
        scene_model_rendering_backend = memnew(SceneModelRenderingBackend);
        model_rendering_server_singleton->backend_register(
                SceneModelRenderingBackend::NAME, scene_model_rendering_backend->get_instance_id()); // 13b
        // after SceneryStreamingServer, TrackServer and GameDataServer, whose signals it follows; before
        // the plugins' servers, which register their track renderers with it
        track_rendering_server_singleton = memnew(TrackRenderingServer);
        Engine::get_singleton()->register_singleton("TrackRenderingServer", track_rendering_server_singleton); // 13c
        standard_track_rendering_backend = memnew(StandardTrackRenderingBackend);
        track_rendering_server_singleton->backend_register(
                StandardTrackRenderingBackend::NAME, standard_track_rendering_backend->get_instance_id()); // 13d
        // after ModelRenderingServer is registered: it outlines and picks its instances; before the
        // plugins' servers, whose vehicle renderer frees its vehicles' pickables when it is freed
        scenery_hud_mouse_server_singleton = memnew(SceneryHUDMouseServer);
        Engine::get_singleton()->register_singleton("SceneryHUDMouseServer", scenery_hud_mouse_server_singleton); // 13e
        /* after RailVehicleServer, SceneryStreamingServer, SignallingServer and ScenarioEventServer are
         * registered, before the servers that follow model instances: the plugins' servers (a vehicle
         * backend, a model renderer) */
        reloaded_plugin_server_singleton->plugins_create_servers(); // 10c
        // a plugin's network registered itself under the name already (10c)
        if (!Engine::get_singleton()->has_singleton("TractionServer")) {
            traction_server_singleton = memnew(TractionServer);
            Engine::get_singleton()->register_singleton("TractionServer", traction_server_singleton); // 11
        }
        Engine::get_singleton()->register_singleton("CabinHUDMouseSystem", cabin_hud_mouse_system_singleton); // 14
        // after RailVehicleServer is registered: the constructor follows its freed vehicles
        driver_server_singleton = memnew(DriverServer);
        Engine::get_singleton()->register_singleton("DriverServer", driver_server_singleton); // 17
        // after RailVehicleServer, the model renderer and SceneryHUDMouseServer: the constructor follows
        // their vehicles and instances, and it registers what the player picks. A plugin's renderer
        // registered itself under the name already (10c); without one the core draws the models
        if (RailVehicleRenderingServer::get_instance() == nullptr) {
            rail_vehicle_rendering_server_singleton = memnew(RailVehicleRenderingServer);
            Engine::get_singleton()->register_singleton(
                    "RailVehicleRenderingServer", rail_vehicle_rendering_server_singleton); // 18a
        }
        // the road vehicles' renderer, as the rail ones'
        if (RoadVehicleRenderingServer::get_instance() == nullptr) {
            road_vehicle_rendering_server_singleton = memnew(RoadVehicleRenderingServer);
            Engine::get_singleton()->register_singleton(
                    "RoadVehicleRenderingServer", road_vehicle_rendering_server_singleton); // 18c
        }
        // after VehicleServer: the constructor follows its freed vehicles
        station_server_singleton = memnew(StationServer);
        Engine::get_singleton()->register_singleton("StationServer", station_server_singleton); // 18b
        // after RailVehicleServer and DriverServer: it follows freed vehicles and hands trainsets
        // over to their drivers
        player_server_singleton = memnew(PlayerServer);
        Engine::get_singleton()->register_singleton("PlayerServer", player_server_singleton); // 19
        // after PlayerServer: the view follows what the player drives
        player_camera_server_singleton = memnew(PlayerCameraServer);
        Engine::get_singleton()->register_singleton("PlayerCameraServer", player_camera_server_singleton); // 20
        hud_server_singleton = memnew(HUDServer);
        Engine::get_singleton()->register_singleton("HUDServer", hud_server_singleton); // 21
        // last: the constructor follows what the servers above report to the scripts
        scenario_script_server_singleton = memnew(ScenarioScriptServer);
        Engine::get_singleton()->register_singleton("ScenarioScriptServer", scenario_script_server_singleton); // 22

        ogg_vorbis_format_loader.instantiate();
        ResourceLoader::get_singleton()->add_resource_format_loader(ogg_vorbis_format_loader);
    }
}

void uninitialize_libreloaded_module(const ModuleInitializationLevel p_level) {
    UtilityFunctions::print("De-initializing libreloaded module on level " + String::num(p_level) + "...");

    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    if (ogg_vorbis_format_loader.is_valid()) {
        ResourceLoader::get_singleton()->remove_resource_format_loader(ogg_vorbis_format_loader);
        ogg_vorbis_format_loader.unref();
    }

    // in reverse order of creation, which is not the order of registration: a singleton created
    // early and registered late (SimulationServer) still outlives the ones created after it
    if (Engine::get_singleton()->has_singleton("ScenarioScriptServer")) {
        Engine::get_singleton()->unregister_singleton("ScenarioScriptServer"); // 22
    }
    if (scenario_script_server_singleton != nullptr) {
        memdelete(scenario_script_server_singleton);
        scenario_script_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("HUDServer")) {
        Engine::get_singleton()->unregister_singleton("HUDServer"); // 21
    }
    if (hud_server_singleton != nullptr) {
        memdelete(hud_server_singleton);
        hud_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("PlayerCameraServer")) {
        Engine::get_singleton()->unregister_singleton("PlayerCameraServer"); // 20
    }
    if (player_camera_server_singleton != nullptr) {
        memdelete(player_camera_server_singleton);
        player_camera_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("PlayerServer")) {
        Engine::get_singleton()->unregister_singleton("PlayerServer"); // 19
    }
    if (player_server_singleton != nullptr) {
        memdelete(player_server_singleton);
        player_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("StationServer")) {
        Engine::get_singleton()->unregister_singleton("StationServer"); // 18b
    }
    if (station_server_singleton != nullptr) {
        memdelete(station_server_singleton);
        station_server_singleton = nullptr;
    }

    // a plugin's renderer is the plugin's to free (10c)
    if (road_vehicle_rendering_server_singleton != nullptr) {
        Engine::get_singleton()->unregister_singleton("RoadVehicleRenderingServer"); // 18c
        memdelete(road_vehicle_rendering_server_singleton);
        road_vehicle_rendering_server_singleton = nullptr;
    }
    if (rail_vehicle_rendering_server_singleton != nullptr) {
        Engine::get_singleton()->unregister_singleton("RailVehicleRenderingServer"); // 18a
        memdelete(rail_vehicle_rendering_server_singleton);
        rail_vehicle_rendering_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("DriverServer")) {
        Engine::get_singleton()->unregister_singleton("DriverServer"); // 17
    }
    if (driver_server_singleton != nullptr) {
        memdelete(driver_server_singleton);
        driver_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("ReloadedTranslationServer")) {
        Engine::get_singleton()->unregister_singleton("ReloadedTranslationServer"); // 13
    }
    if (maszyna_translation_server_singleton != nullptr) {
        memdelete(maszyna_translation_server_singleton);
        maszyna_translation_server_singleton = nullptr;
    }

    if (reloaded_plugin_server_singleton != nullptr) {
        reloaded_plugin_server_singleton->plugins_free_servers(); // 10c
    }

    if (Engine::get_singleton()->has_singleton("SceneryHUDMouseServer")) {
        Engine::get_singleton()->unregister_singleton("SceneryHUDMouseServer"); // 13e
    }
    if (scenery_hud_mouse_server_singleton != nullptr) {
        memdelete(scenery_hud_mouse_server_singleton);
        scenery_hud_mouse_server_singleton = nullptr;
    }

    if (standard_track_rendering_backend != nullptr) {
        if (track_rendering_server_singleton != nullptr) {
            track_rendering_server_singleton->backend_unregister(StandardTrackRenderingBackend::NAME); // 13d
        }
        memdelete(standard_track_rendering_backend);
        standard_track_rendering_backend = nullptr;
    }
    if (Engine::get_singleton()->has_singleton("TrackRenderingServer")) {
        Engine::get_singleton()->unregister_singleton("TrackRenderingServer"); // 13c
    }
    if (track_rendering_server_singleton != nullptr) {
        memdelete(track_rendering_server_singleton);
        track_rendering_server_singleton = nullptr;
    }
    if (scene_model_rendering_backend != nullptr) {
        if (model_rendering_server_singleton != nullptr) {
            model_rendering_server_singleton->backend_unregister(SceneModelRenderingBackend::NAME); // 13b
        }
        memdelete(scene_model_rendering_backend);
        scene_model_rendering_backend = nullptr;
    }
    if (Engine::get_singleton()->has_singleton("ModelRenderingServer")) {
        Engine::get_singleton()->unregister_singleton("ModelRenderingServer"); // 13a
    }
    if (model_rendering_server_singleton != nullptr) {
        memdelete(model_rendering_server_singleton);
        model_rendering_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("ScenarioEventServer")) {
        Engine::get_singleton()->unregister_singleton("ScenarioEventServer"); // 10b
    }
    if (scenario_event_server_singleton != nullptr) {
        memdelete(scenario_event_server_singleton);
        scenario_event_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("SignallingServer")) {
        Engine::get_singleton()->unregister_singleton("SignallingServer"); // 10a
    }
    if (signalling_server_singleton != nullptr) {
        memdelete(signalling_server_singleton);
        signalling_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("RoadVehicleServer")) {
        Engine::get_singleton()->unregister_singleton("RoadVehicleServer"); // 10e
    }
    if (road_vehicle_server_singleton != nullptr) {
        if (vehicle_server_singleton != nullptr) {
            vehicle_server_singleton->implementation_unregister(RoadVehicleServer::IMPLEMENTATION_NAME);
        }
        memdelete(road_vehicle_server_singleton);
        road_vehicle_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("RoadServer")) {
        Engine::get_singleton()->unregister_singleton("RoadServer"); // 10d
    }
    if (road_server_singleton != nullptr) {
        memdelete(road_server_singleton);
        road_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("RailVehicleServer")) {
        Engine::get_singleton()->unregister_singleton("RailVehicleServer"); // 10
    }
    if (rail_vehicle_server_singleton != nullptr) {
        memdelete(rail_vehicle_server_singleton);
        rail_vehicle_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("RadioServer")) {
        Engine::get_singleton()->unregister_singleton("RadioServer"); // 9a
    }
    if (radio_server_singleton != nullptr) {
        memdelete(radio_server_singleton);
        radio_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("VehicleServer")) {
        Engine::get_singleton()->unregister_singleton("VehicleServer"); // 9
    }
    if (vehicle_server_singleton != nullptr) {
        memdelete(vehicle_server_singleton);
        vehicle_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("PersonServer")) {
        Engine::get_singleton()->unregister_singleton("PersonServer"); // 8b
    }
    if (person_server_singleton != nullptr) {
        memdelete(person_server_singleton);
        person_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("SmokeServer")) {
        Engine::get_singleton()->unregister_singleton("SmokeServer"); // 8a
    }
    if (smoke_server_singleton != nullptr) {
        memdelete(smoke_server_singleton);
        smoke_server_singleton = nullptr;
    }

    // a plugin's network is the plugin's to free (10c)
    if (traction_server_singleton != nullptr) {
        Engine::get_singleton()->unregister_singleton("TractionServer"); // 11
        memdelete(traction_server_singleton);
        traction_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("SceneryMeshRenderingServer")) {
        Engine::get_singleton()->unregister_singleton("SceneryMeshRenderingServer"); // 6b
    }
    if (scenery_mesh_rendering_server_singleton != nullptr) {
        memdelete(scenery_mesh_rendering_server_singleton);
        scenery_mesh_rendering_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("CabinHUDMouseSystem")) {
        Engine::get_singleton()->unregister_singleton("CabinHUDMouseSystem"); // 14
    }
    if (cabin_hud_mouse_system_singleton != nullptr) {
        memdelete(cabin_hud_mouse_system_singleton);
        cabin_hud_mouse_system_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("TrackServer")) {
        Engine::get_singleton()->unregister_singleton("TrackServer"); // 8
    }
    if (track_server_singleton != nullptr) {
        memdelete(track_server_singleton);
        track_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("SceneryStreamingServer")) {
        Engine::get_singleton()->unregister_singleton("SceneryStreamingServer"); // 5
    }
    if (scenery_streaming_server_singleton != nullptr) {
        memdelete(scenery_streaming_server_singleton);
        scenery_streaming_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("GameLog")) {
        Engine::get_singleton()->unregister_singleton("GameLog"); // 3
    }
    if (game_log_singleton != nullptr) {
        memdelete(game_log_singleton);
        game_log_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("SimulationServer")) {
        Engine::get_singleton()->unregister_singleton("SimulationServer"); // 7
    }
    if (simulation_server_singleton != nullptr) {
        memdelete(simulation_server_singleton);
        simulation_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("ResourceLazyLoader")) {
        Engine::get_singleton()->unregister_singleton("ResourceLazyLoader"); // 1b
    }
    if (resource_lazy_loader_singleton != nullptr) {
        memdelete(resource_lazy_loader_singleton);
        resource_lazy_loader_singleton = nullptr;
    }

    if (reloaded_material_server_singleton != nullptr) {
        Engine::get_singleton()->unregister_singleton("ReloadedMaterialServer"); // 1c
        memdelete(reloaded_material_server_singleton);
        reloaded_material_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("GameDataServer")) {
        Engine::get_singleton()->unregister_singleton("GameDataServer"); // 1a
    }
    if (game_data_server_singleton != nullptr) {
        memdelete(game_data_server_singleton);
        game_data_server_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("UserSettings")) {
        Engine::get_singleton()->unregister_singleton("UserSettings"); // 1
    }
    if (user_settings_singleton != nullptr) {
        memdelete(user_settings_singleton);
        user_settings_singleton = nullptr;
    }

    if (Engine::get_singleton()->has_singleton("ReloadedPluginServer")) {
        Engine::get_singleton()->unregister_singleton("ReloadedPluginServer"); // 0
    }
    if (reloaded_plugin_server_singleton != nullptr) {
        memdelete(reloaded_plugin_server_singleton);
        reloaded_plugin_server_singleton = nullptr;
    }
}
extern "C" {
    // Initialization.
    GDExtensionBool GDE_EXPORT libreloaded_library_init(
            const GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library,
            GDExtensionInitialization *p_r_initialization) {
        const GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, p_r_initialization);

        init_obj.register_initializer(initialize_libreloaded_module);
        init_obj.register_terminator(uninitialize_libreloaded_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}
