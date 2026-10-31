#include "game_data/GameDataServer.hpp"
#include "rendering/ModelRenderingServer.hpp"
#include "resources/ResourceLazyLoader.hpp"
#include "scenery/SceneryHUDMouseServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "vehicles/base/VehicleRenderingServer.hpp"
#include "vehicles/base/VehicleServer.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *VehicleRenderingServer::vehicle_model_built_signal = "vehicle_model_built";
    const char *VehicleRenderingServer::builds_finished_signal = "builds_finished";

    /* A vehicle far away is drawn from instances, a near one as nodes - shown and owned in the
     * editor when it is editable */
    static ModelRenderingServer::Instancer detail_instancer(const bool p_detailed, const bool p_editable) {
        if (!p_detailed) {
            return ModelRenderingServer::INSTANCER_OPTIMIZED;
        }
        return p_editable ? ModelRenderingServer::INSTANCER_EDITABLE_NODES : ModelRenderingServer::INSTANCER_NODES;
    }

    VehicleRenderingServer::VehicleRenderingServer() {
        preload_queue.instantiate();
        preload_queue->connect(
                WorkerTaskQueue::task_finished_signal,
                callable_mp(this, &VehicleRenderingServer::_on_preload_finished));
        _on_project_settings_changed();
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &VehicleRenderingServer::_on_project_settings_changed));
        if (VehicleServer *vehicle_server = VehicleServer::get_instance(); vehicle_server != nullptr) {
            vehicle_server->connect(
                    VehicleServer::vehicle_freed_signal, callable_mp(this, &VehicleRenderingServer::_on_vehicle_freed));
            vehicle_server->connect(
                    VehicleServer::vehicle_config_changed_signal,
                    callable_mp(this, &VehicleRenderingServer::_on_vehicle_config_changed));
        }
        if (ModelRenderingServer *models = ModelRenderingServer::get_instance(); models != nullptr) {
            models->connect(
                    ModelRenderingServer::instance_built_signal,
                    callable_mp(this, &VehicleRenderingServer::_on_instance_built));
        }
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            game_data->connect(
                    GameDataServer::data_reload_requested_signal,
                    callable_mp(this, &VehicleRenderingServer::_on_data_reload_requested));
        }
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            streaming->connect(
                    SceneryStreamingServer::streaming_camera_changed_signal,
                    callable_mp(this, &VehicleRenderingServer::_on_streaming_camera_changed));
        }
    }

    void VehicleRenderingServer::_on_project_settings_changed() {
        detail_distance =
                ProjectSettings::get_singleton()->get_setting(DETAIL_DISTANCE_SETTING, DEFAULT_DETAIL_DISTANCE);
    }

    /* Every vehicle's place against the new camera at once - the ones within the draw distance
     * wait for their build from now on (builds_get_pending_count()), not from the sweep's turn */
    void VehicleRenderingServer::_on_streaming_camera_changed() {
        for (const RID &vehicle: visit_order) {
            _update_detail(vehicle, vehicles[vehicle]);
        }
    }

    void VehicleRenderingServer::preloads_drain() {
        preload_queue->drain();
        preload_vehicles.clear();
        for (KeyValue<RID, Visual> &item: vehicles) {
            item.value.preload_task = -1;
        }
    }

    /* Every model file the vehicle's build takes - the appearance's, and what a plugin's renderer
     * adds - as [resource, data path, skins], what its materials are found by */
    Array VehicleRenderingServer::_get_preload_requests(const RID &p_vehicle, const Visual &p_visual) const {
        Array requests;
        const String data_path = p_visual.appearance.is_valid() ? p_visual.appearance->get_data_path() : String();
        const PackedStringArray skins =
                p_visual.appearance.is_valid() ? p_visual.appearance->get_skins() : PackedStringArray();
        const auto request =
                [&requests](const RID &p_resource, const String &p_data_path, const PackedStringArray &p_skins) {
                    if (p_resource.is_valid()) {
                        requests.push_back(Array::make(p_resource, p_data_path, p_skins));
                    }
                };
        for (const KeyValue<String, RID> &resource: p_visual.model_resources) {
            request(resource.value, data_path, skins);
        }
        _vehicle_add_preload_requests(p_vehicle, requests);
        return requests;
    }

    /// On a preload worker: the files loaded by ResourceLazyLoader, which keeps them without lazy
    /// loading and loads one file once however many vehicles ask for it, and their materials' files
    /// read (ModelRenderingServer::material_preload()). [{resource: model}, [what holds the materials]]
    Array VehicleRenderingServer::_load_resources(const Array &p_requests) {
        Dictionary models;
        Array materials;
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        ModelRenderingServer *model_rendering = ModelRenderingServer::get_instance();
        ERR_FAIL_NULL_V(lazy_loader, Array());
        ERR_FAIL_NULL_V(model_rendering, Array());
        for (const Variant &item: p_requests) {
            const Array request = item;
            const Ref<Resource> model = lazy_loader->resource_load(request[0]);
            models[request[0]] = model;
            // a vehicle's models draw with the project's texture size limit (_create_models()), by the
            // default backend: a VehicleAppearance names none
            materials.push_back(model_rendering->material_preload(StringName(), model, request[1], request[2], 0));
        }
        return Array::make(models, materials);
    }

    /* The vehicle's model files loaded off the main thread - the parse of a model stalled it for
     * tens of milliseconds, a frame and more. What is resident already is taken at once, and so is
     * everything with the dummy renderer, which loads on the main thread (ResourceLazyLoader) */
    void VehicleRenderingServer::_preload(const RID &p_vehicle, Visual &p_visual) {
        if (p_visual.preload_task >= 0) {
            return;
        }
        const Array requested = _get_preload_requests(p_vehicle, p_visual);
        bool resident = true;
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        ERR_FAIL_NULL(lazy_loader);
        for (const Variant &request: requested) {
            resident = resident && lazy_loader->resource_is_resident(Array(request)[0]);
        }
        // the dummy renderer takes no resource from another thread: loaded here (ResourceLazyLoader)
        if (resident || !lazy_loader->get_loads_off_main_thread()) {
            _apply_preloaded(p_vehicle, p_visual, _load_resources(requested));
            return;
        }
        p_visual.preload_task =
                preload_queue->submit(callable_mp_static(&VehicleRenderingServer::_load_resources).bind(requested));
        preload_vehicles[p_visual.preload_task] = p_vehicle;
    }

    void VehicleRenderingServer::_on_preload_finished(const int p_task) {
        const RID *vehicle = preload_vehicles.getptr(p_task);
        if (vehicle == nullptr) {
            return;
        }
        const RID vehicle_rid = *vehicle;
        preload_vehicles.erase(p_task);
        Visual *visual = vehicles.getptr(vehicle_rid);
        const Array loaded = preload_queue->wait(p_task);
        if (visual == nullptr || visual->preload_task != p_task) {
            return;
        }
        visual->preload_task = -1;
        _apply_preloaded(vehicle_rid, *visual, loaded);
    }

    /* The loaded files wait for the build; what depends on the model file alone is done now, by a
     * plugin's renderer */
    void VehicleRenderingServer::_apply_preloaded(const RID &p_vehicle, Visual &p_visual, const Array &p_loaded) {
        p_visual.preloaded.clear();
        ERR_FAIL_COND(p_loaded.size() < 2);
        const Dictionary models = p_loaded[0];
        for (const Variant &resource: models.keys()) {
            p_visual.preloaded[resource] = models[resource];
        }
        p_visual.preloaded_materials = p_loaded[1];
        _vehicle_preloaded(p_vehicle);
    }

    bool VehicleRenderingServer::_is_preloaded(const RID &p_vehicle, const Visual &p_visual) const {
        for (const Variant &request: _get_preload_requests(p_vehicle, p_visual)) {
            if (!p_visual.preloaded.has(Array(request)[0])) {
                return false;
            }
        }
        return true;
    }

    Ref<Resource> VehicleRenderingServer::_get_model(const Visual &p_visual, const RID &p_resource) {
        const Ref<Resource> *loaded = p_visual.preloaded.getptr(p_resource);
        return loaded != nullptr ? *loaded : Ref<Resource>();
    }

    int VehicleRenderingServer::builds_get_pending_count() const {
        return static_cast<int>(pending_builds.size());
    }

    /* The models this server built from model files are built again from them; the ones handed
     * over are their owner's to build again. Building a model emits signals scripts answer, so the
     * vehicles are taken first. */
    void VehicleRenderingServer::_on_data_reload_requested() {
        Vector<RID> reloaded;
        for (const KeyValue<RID, Visual> &item: vehicles) {
            reloaded.push_back(item.key);
        }
        for (const RID &vehicle: reloaded) {
            const Visual *visual = vehicles.getptr(vehicle);
            if (visual == nullptr) {
                continue;
            }
            if (visual->own_models) {
                vehicle_set_appearance(vehicle, visual->appearance);
            }
        }
    }

    void VehicleRenderingServer::_detach_all() {
        preloads_drain();
        while (!vehicles.is_empty()) {
            vehicle_detach(vehicles.begin()->key);
        }
    }

    VehicleRenderingServer::Visual *VehicleRenderingServer::_get_visual(const RID &p_vehicle) {
        return vehicles.getptr(p_vehicle);
    }

    const VehicleRenderingServer::Visual *VehicleRenderingServer::_get_visual(const RID &p_vehicle) const {
        return vehicles.getptr(p_vehicle);
    }

    void VehicleRenderingServer::_vehicle_preload(const RID &p_vehicle) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->preload_task = -1;
        _preload(p_vehicle, *visual);
    }

    void VehicleRenderingServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("vehicle_attach", "vehicle", "node_id"), &VehicleRenderingServer::vehicle_attach);
        ClassDB::bind_method(D_METHOD("vehicle_detach", "vehicle"), &VehicleRenderingServer::vehicle_detach);
        ClassDB::bind_method(D_METHOD("preloads_drain"), &VehicleRenderingServer::preloads_drain);
        ClassDB::bind_method(D_METHOD("vehicle_is_attached", "vehicle"), &VehicleRenderingServer::vehicle_is_attached);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_transform", "vehicle", "transform"),
                &VehicleRenderingServer::vehicle_set_transform);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_transform", "vehicle"), &VehicleRenderingServer::vehicle_get_transform);
        ClassDB::bind_method(
                D_METHOD("vehicle_mount_node", "vehicle", "node_id"), &VehicleRenderingServer::vehicle_mount_node);
        ClassDB::bind_method(
                D_METHOD("vehicle_unmount_node", "vehicle", "node_id"), &VehicleRenderingServer::vehicle_unmount_node);
        ClassDB::bind_method(
                D_METHOD("detection_area_get_vehicle", "area"), &VehicleRenderingServer::detection_area_get_vehicle);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_scenario", "vehicle", "scenario"), &VehicleRenderingServer::vehicle_set_scenario);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_appearance", "vehicle", "appearance"),
                &VehicleRenderingServer::vehicle_set_appearance);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_appearance", "vehicle"), &VehicleRenderingServer::vehicle_get_appearance);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_models", "vehicle", "model", "low_poly"),
                &VehicleRenderingServer::vehicle_set_models);
        ClassDB::bind_method(D_METHOD("vehicle_get_model", "vehicle"), &VehicleRenderingServer::vehicle_get_model);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_head_display_material", "vehicle", "material"),
                &VehicleRenderingServer::vehicle_set_head_display_material);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_snow_material", "vehicle", "material"),
                &VehicleRenderingServer::vehicle_set_snow_material);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_snow", "vehicle", "snow"), &VehicleRenderingServer::vehicle_set_snow);
        ClassDB::bind_method(D_METHOD("vehicle_get_snow", "vehicle"), &VehicleRenderingServer::vehicle_get_snow);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_shown_cabin", "vehicle", "cabin"),
                &VehicleRenderingServer::vehicle_set_shown_cabin);
        ClassDB::bind_method(D_METHOD("vehicle_is_detailed", "vehicle"), &VehicleRenderingServer::vehicle_is_detailed);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_editable", "vehicle", "editable"), &VehicleRenderingServer::vehicle_set_editable);
        ClassDB::bind_method(D_METHOD("vehicle_is_editable", "vehicle"), &VehicleRenderingServer::vehicle_is_editable);
        ClassDB::bind_method(D_METHOD("builds_get_pending_count"), &VehicleRenderingServer::builds_get_pending_count);
        ADD_SIGNAL(MethodInfo(vehicle_model_built_signal, PropertyInfo(Variant::RID, "vehicle")));
        ADD_SIGNAL(MethodInfo(builds_finished_signal));
    }

    Node3D *VehicleRenderingServer::_node(const Visual &p_visual) {
        return Object::cast_to<Node3D>(ObjectDB::get_instance(p_visual.node));
    }

    void VehicleRenderingServer::vehicle_attach(const RID &p_vehicle, const uint64_t p_node_id) {
        ERR_FAIL_COND(!p_vehicle.is_valid());
        if (vehicles.has(p_vehicle)) {
            vehicle_detach(p_vehicle);
        }
        Visual &visual = vehicles[p_vehicle];
        visual.node = ObjectID(p_node_id);
        if (const Node3D *node = _node(visual); node != nullptr && node->is_inside_tree()) {
            visual.scenario = node->get_world_3d()->get_scenario();
        }
        visit_order.push_back(p_vehicle);
        _set_processing(true);
        _vehicle_attached(p_vehicle);
        // a vehicle already standing on its track is drawn there at once
        _place(p_vehicle, visual);
    }

    void VehicleRenderingServer::vehicle_detach(const RID &p_vehicle) {
        Visual *visual = vehicles.getptr(p_vehicle);
        if (visual == nullptr) {
            return;
        }
        _cancel_build(p_vehicle, *visual);
        _free_models(p_vehicle, *visual);
        _free_model_resources(*visual);
        _vehicle_detaching(p_vehicle);
        if (SceneryHUDMouseServer *mouse = SceneryHUDMouseServer::get_instance(); mouse != nullptr) {
            mouse->pickable_free(visual->pickable);
        }
        if (Node *holder = Object::cast_to<Node>(ObjectDB::get_instance(visual->holder)); holder != nullptr) {
            holder->queue_free();
        }
        area_vehicles.erase(visual->detection_area);
        if (PhysicsServer3D *physics = PhysicsServer3D::get_singleton(); physics != nullptr) {
            if (visual->detection_area.is_valid()) {
                physics->free_rid(visual->detection_area);
            }
            if (visual->detection_shape.is_valid()) {
                physics->free_rid(visual->detection_shape);
            }
        }
        vehicles.erase(p_vehicle);
        visit_order.erase(p_vehicle);
        _set_processing(!vehicles.is_empty());
    }

    bool VehicleRenderingServer::vehicle_is_attached(const RID &p_vehicle) const {
        return vehicles.has(p_vehicle);
    }

    void VehicleRenderingServer::vehicle_set_transform(const RID &p_vehicle, const Transform3D &p_transform) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->transform = p_transform;
        _move(p_vehicle, *visual);
    }

    Transform3D VehicleRenderingServer::vehicle_get_transform(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        return visual != nullptr ? visual->transform : Transform3D();
    }

    void VehicleRenderingServer::vehicle_mount_node(const RID &p_vehicle, const uint64_t p_node_id) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->mounts.push_back(ObjectID(p_node_id));
        if (Node3D *mount = Object::cast_to<Node3D>(ObjectDB::get_instance(ObjectID(p_node_id)));
            visual->placed && mount != nullptr && mount->is_inside_tree()) {
            mount->set_global_transform(visual->transform);
        }
    }

    /* A vehicle freed takes its mounts' places with it: nothing to take off then */
    void VehicleRenderingServer::vehicle_unmount_node(const RID &p_vehicle, const uint64_t p_node_id) {
        if (Visual *visual = vehicles.getptr(p_vehicle); visual != nullptr) {
            visual->mounts.erase(ObjectID(p_node_id));
        }
    }

    RID VehicleRenderingServer::detection_area_get_vehicle(const RID &p_area) const {
        const RID *vehicle = area_vehicles.getptr(p_area);
        return vehicle != nullptr ? *vehicle : RID();
    }

    /* The models handed over are their owner's, which moves them itself */
    void VehicleRenderingServer::vehicle_set_scenario(const RID &p_vehicle, const RID &p_scenario) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->scenario = p_scenario;
        if (visual->placed) {
            _show_models(p_vehicle, *visual, p_scenario);
        }
    }

    /* The models this server built drawn in p_scenario - in none for an empty one */
    void VehicleRenderingServer::_show_models(const RID &p_vehicle, const Visual &p_visual, const RID &p_scenario) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        ERR_FAIL_NULL(models);
        if (p_visual.own_models) {
            for (const RID &instance: {p_visual.model, p_visual.low_poly, p_visual.passengers}) {
                if (instance.is_valid()) {
                    models->instance_set_scenario(instance, p_scenario);
                }
            }
            for (const RID &instance: p_visual.attachments) {
                models->instance_set_scenario(instance, p_scenario);
            }
        }
        _vehicle_scenario_changed(p_vehicle, p_scenario);
    }

    void
    VehicleRenderingServer::vehicle_set_appearance(const RID &p_vehicle, const Ref<VehicleAppearance> &p_appearance) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->appearance = p_appearance;
        visual->model_missing = false;
        // models of its own replace the ones built from the last appearance, built once the
        // vehicle is within the draw distance (_update_detail()); handed-over ones stay. Its files
        // are registered at once and loaded on a worker (_preload())
        const bool own_models = p_appearance.is_valid() && !p_appearance->get_model_filename().is_empty();
        if (own_models) {
            _free_models(p_vehicle, *visual);
            _free_model_resources(*visual);
            ModelRenderingServer *models = ModelRenderingServer::get_instance();
            ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
            ERR_FAIL_NULL(models);
            ERR_FAIL_NULL(lazy_loader);
            const String data_path = p_appearance->get_data_path();
            PackedStringArray filenames = p_appearance->get_attachment_model_filenames();
            filenames.push_back(p_appearance->get_model_filename());
            filenames.push_back(p_appearance->get_low_poly_model_filename());
            filenames.push_back(p_appearance->get_passengers_model_filename());
            for (const String &filename: filenames) {
                if (filename.is_empty() || visual->model_resources.has(filename)) {
                    continue;
                }
                // the key a scenery placement of the same file has
                visual->model_resources[filename] = lazy_loader->resource_register(
                        data_path.path_join(filename), callable_mp(models, &ModelRenderingServer::model_load)
                                                               .bind(StringName(), data_path.path_join(filename)));
            }
        }
        _vehicle_appearance_changed(p_vehicle);
        if (own_models) {
            visual->preloaded.clear();
            visual->preload_task = -1;
            _preload(p_vehicle, *visual);
        }
        _bind_parts(p_vehicle, *visual);
        _update_detail(p_vehicle, *visual);
    }

    Ref<VehicleAppearance> VehicleRenderingServer::vehicle_get_appearance(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(visual, Ref<VehicleAppearance>());
        return visual->appearance;
    }

    void VehicleRenderingServer::vehicle_set_models(const RID &p_vehicle, const RID &p_model, const RID &p_low_poly) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        _free_models(p_vehicle, *visual);
        visual->model = p_model;
        visual->low_poly = p_low_poly;
        visual->own_models = false;
        // handed over, the models are their owner's nodes already - posed from the start, and
        // drawn as instances only once the camera is known to be far (_update_detail())
        visual->detailed = true;
        model_vehicles[p_model] = p_vehicle;
        _bind_parts(p_vehicle, *visual);
        _send_snow(p_vehicle, *visual);
        // a model handed over is one built - by whoever owns it, before this server could hear it
        emit_signal(vehicle_model_built_signal, p_vehicle);
    }

    RID VehicleRenderingServer::vehicle_get_model(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(visual, RID());
        return visual->model;
    }

    /* The snow drawn now, if any, is drawn with it from now on */
    void
    VehicleRenderingServer::vehicle_set_snow_material(const RID &p_vehicle, const Ref<ShaderMaterial> &p_material) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->snow_overlay = p_material;
        if (visual->snow_material.is_valid()) {
            visual->snow_material.unref();
            _lay_snow(p_vehicle, *visual);
        }
        _send_snow(p_vehicle, *visual);
    }

    void VehicleRenderingServer::vehicle_set_snow(const RID &p_vehicle, const float p_snow) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->snow = CLAMP(p_snow, 0.0F, 1.0F);
        if (Math::abs(visual->snow - visual->snow_sent) > SNOW_SEND_THRESHOLD ||
            (visual->snow == 0.0F && visual->snow_sent != 0.0F)) {
            _send_snow(p_vehicle, *visual);
        }
    }

    float VehicleRenderingServer::vehicle_get_snow(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(visual, 0.0F);
        return visual->snow;
    }

    void
    VehicleRenderingServer::vehicle_set_head_display_material(const RID &p_vehicle, const Ref<Material> &p_material) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->head_display_material = p_material;
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        if (models == nullptr || !visual->model.is_valid() || visual->appearance.is_null()) {
            return;
        }
        const String submodel = visual->appearance->get_head_display_submodel();
        if (models->instance_has_part(visual->model, submodel)) {
            models->instance_set_part_material_override(visual->model, submodel, p_material);
        }
    }

    void VehicleRenderingServer::vehicle_set_shown_cabin(const RID &p_vehicle, const RID &p_cabin) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->shown_cabin = p_cabin;
        _vehicle_shown_cabin_changed(p_vehicle);
    }

    bool VehicleRenderingServer::vehicle_is_detailed(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(visual, false);
        return visual->detailed;
    }

    /* The nodes are built anew: the holder goes with the detail, and comes back shown in the Scene
     * dock or hidden, with the instancer that gives its nodes owners or none */
    void VehicleRenderingServer::vehicle_set_editable(const RID &p_vehicle, const bool p_editable) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        if (visual->editable == p_editable) {
            return;
        }
        visual->editable = p_editable;
        if (visual->detailed) {
            _set_detailed(p_vehicle, *visual, false);
        }
        _update_detail(p_vehicle, *visual);
    }

    bool VehicleRenderingServer::vehicle_is_editable(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(visual, false);
        return visual->editable;
    }

    void VehicleRenderingServer::_free_models(const RID &p_vehicle, Visual &p_visual) {
        // the snow's overlay goes with the models - taken off the ones handed over, which stay their
        // owner's; the models built next get one anew (_send_snow())
        if (p_visual.snow_material.is_valid()) {
            p_visual.snow_material.unref();
            _lay_snow(p_vehicle, p_visual);
        }
        _vehicle_models_freed(p_vehicle);
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        model_vehicles.erase(p_visual.model);
        if (p_visual.own_models && models != nullptr) {
            for (const RID &instance: {p_visual.model, p_visual.low_poly, p_visual.passengers}) {
                if (instance.is_valid()) {
                    models->instance_free(instance);
                }
            }
            for (const RID &instance: p_visual.attachments) {
                models->instance_free(instance);
            }
        }
        p_visual.model = RID();
        p_visual.low_poly = RID();
        p_visual.passengers = RID();
        p_visual.attachments.clear();
        p_visual.own_models = false;
        if (ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance(); lazy_loader != nullptr) {
            for (const RID &resource: p_visual.held_models) {
                lazy_loader->resource_release(resource);
            }
        }
        p_visual.held_models.clear();
    }

    void VehicleRenderingServer::_free_model_resources(Visual &p_visual) {
        if (ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance(); lazy_loader != nullptr) {
            for (const KeyValue<String, RID> &resource: p_visual.model_resources) {
                lazy_loader->resource_free(resource.value);
            }
        }
        p_visual.model_resources.clear();
    }

    /* Every model of a vehicle lives in one vehicle-local frame; the appearance says
     * where that frame sits in the node. The exterior and the low-poly interior are nodes near the
     * camera - their submodels are posed and hidden - the passengers and the attachments never are. */
    void VehicleRenderingServer::_create_models(const RID &p_vehicle, Visual &p_visual) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        ERR_FAIL_NULL(models);
        ERR_FAIL_NULL(lazy_loader);
        const Ref<VehicleAppearance> &appearance = p_visual.appearance;
        const String data_path = appearance->get_data_path();
        // models built again while the vehicle is drawn in detail go under the holder it has
        Node3D *holder = Object::cast_to<Node3D>(ObjectDB::get_instance(p_visual.holder));
        const auto create = [&](const String &p_filename, const PackedStringArray &p_skins,
                                const ModelRenderingServer::Instancer p_instancer) {
            // held while built, as a streamed scenery placement holds its model
            const RID *resource = p_visual.model_resources.getptr(p_filename);
            const Ref<Resource> loaded = resource != nullptr ? _get_model(p_visual, *resource) : Ref<Resource>();
            if (loaded.is_null()) {
                return RID();
            }
            const Ref<Resource> model = lazy_loader->resource_hold(*resource, loaded);
            p_visual.held_models.push_back(*resource);
            if (model.is_null()) {
                return RID();
            }
            const RID instance = models->instance_create(
                    StringName(), model, p_instancer, ModelRenderingServer::INSTANCE_KIND_DYNAMIC);
            // a vehicle's translucent submodels - its windows - are drawn blended near the camera,
            // opaque by the optimized instancer far away; a cutout leaves holes in the glass
            constexpr bool FORCE_ALPHA = true;
            models->instance_set_options(instance, data_path, p_skins, Array(), FORCE_ALPHA, {}, 0);
            if (p_instancer != ModelRenderingServer::INSTANCER_OPTIMIZED) {
                models->instance_attach_object_instance_id(instance, holder->get_instance_id());
            }
            models->instance_set_node_transform(instance, appearance->get_model_transform());
            models->instance_set_scenario(instance, p_visual.placed ? p_visual.scenario : RID());
            models->instance_set_transform(instance, p_visual.transform * appearance->get_model_transform());
            models->instance_build(instance);
            return instance;
        };
        const ModelRenderingServer::Instancer detail = detail_instancer(p_visual.detailed, p_visual.editable);
        p_visual.own_models = true;
        p_visual.model = create(appearance->get_model_filename(), appearance->get_skins(), detail);
        p_visual.model_missing = !p_visual.model.is_valid();
        model_vehicles[p_visual.model] = p_vehicle;
        p_visual.low_poly = create(appearance->get_low_poly_model_filename(), appearance->get_skins(), detail);
        // the passengers carry no skin of their own
        p_visual.passengers =
                create(appearance->get_passengers_model_filename(), PackedStringArray(),
                       ModelRenderingServer::INSTANCER_OPTIMIZED);
        // drawn as the exterior is, never posed
        for (const String &filename: appearance->get_attachment_model_filenames()) {
            const RID attachment = create(filename, appearance->get_skins(), ModelRenderingServer::INSTANCER_OPTIMIZED);
            if (attachment.is_valid()) {
                p_visual.attachments.push_back(attachment);
            }
        }
        _vehicle_models_created(p_vehicle);
        // the new instances carry the snow the vehicle has, if it is drawn in detail
        _send_snow(p_vehicle, p_visual);
        _register_pickable(p_vehicle, p_visual);
    }

    VehicleRenderingServer::Part VehicleRenderingServer::_part(const Visual &p_visual, const String &p_submodel) const {
        const ModelRenderingServer *models = ModelRenderingServer::get_instance();
        if (p_submodel.is_empty() || models == nullptr || !p_visual.model.is_valid() ||
            !models->instance_has_part(p_visual.model, p_submodel)) {
            return Part();
        }
        return Part{
                p_submodel.to_lower(),
                p_visual.model_transform * models->instance_get_part_transform(p_visual.model, p_submodel)};
    }

    Vector<VehicleRenderingServer::Part>
    VehicleRenderingServer::_parts(const Visual &p_visual, const PackedStringArray &p_submodels) const {
        Vector<Part> parts;
        for (const String &submodel: p_submodels) {
            parts.push_back(_part(p_visual, submodel));
        }
        return parts;
    }

    /* The submodels of the exterior that move, found in the model the vehicle is drawn with. */
    void VehicleRenderingServer::_bind_parts(const RID &p_vehicle, Visual &p_visual) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        const Ref<VehicleAppearance> &appearance = p_visual.appearance;
        p_visual.poses.clear();
        if (appearance.is_null() || models == nullptr || !p_visual.model.is_valid()) {
            _vehicle_parts_bound(p_vehicle);
            return;
        }
        p_visual.model_transform = appearance->get_model_transform();
        _bind_running_gear(p_vehicle, p_visual);
        _vehicle_parts_bound(p_vehicle);
        vehicle_set_head_display_material(p_vehicle, p_visual.head_display_material);
        _update_detection_area(p_vehicle, p_visual);
        _place(p_vehicle, p_visual);
    }

    /* The vehicle is drawn where its kind's server placed it - its answer and nothing else */
    void VehicleRenderingServer::_place(const RID &p_vehicle, Visual &p_visual) {
        if (_vehicle_get_placement(p_vehicle, p_visual.transform)) {
            _move(p_vehicle, p_visual);
        }
    }

    /* Everything of the vehicle goes where it stands: the nodes mounted on it, its models - built
     * and in their world from its first place on - and its detection area; the running gear
     * follows where the vehicle is drawn in detail. */
    void VehicleRenderingServer::_move(const RID &p_vehicle, Visual &p_visual) {
        if (!p_visual.placed) {
            p_visual.placed = true;
            _show_models(p_vehicle, p_visual, p_visual.scenario);
            _update_detection_area(p_vehicle, p_visual);
            // its models are built where it stands
            _update_detail(p_vehicle, p_visual);
        }
        for (const ObjectID &mount_id: p_visual.mounts) {
            if (Node3D *mount = Object::cast_to<Node3D>(ObjectDB::get_instance(mount_id));
                mount != nullptr && mount->is_inside_tree()) {
                mount->set_global_transform(p_visual.transform);
            }
        }
        if (Node3D *holder = Object::cast_to<Node3D>(ObjectDB::get_instance(p_visual.holder)); holder != nullptr) {
            holder->set_global_transform(p_visual.transform);
        }
        const Transform3D model_transform = p_visual.transform * p_visual.model_transform;
        if (ModelRenderingServer *models = ModelRenderingServer::get_instance();
            models != nullptr && p_visual.own_models) {
            for (const RID &instance: {p_visual.model, p_visual.low_poly, p_visual.passengers}) {
                if (instance.is_valid()) {
                    models->instance_set_transform(instance, model_transform);
                }
            }
            for (const RID &instance: p_visual.attachments) {
                models->instance_set_transform(instance, model_transform);
            }
        }
        _vehicle_moved(p_vehicle);
        if (PhysicsServer3D *physics = PhysicsServer3D::get_singleton();
            physics != nullptr && p_visual.detection_area.is_valid()) {
            physics->area_set_transform(p_visual.detection_area, model_transform);
        }
        if (p_visual.detailed) {
            _pose_running_gear(p_vehicle, p_visual);
            _send_poses(p_visual);
        }
    }

    void VehicleRenderingServer::_pose(Visual &p_visual, const Part &p_part, const Basis &p_pose) {
        if (!p_part.submodel.is_empty()) {
            p_visual.poses[p_part.submodel] = Transform3D(p_pose);
        }
    }

    void VehicleRenderingServer::_pose(Visual &p_visual, const Part &p_part, const Transform3D &p_pose) {
        if (!p_part.submodel.is_empty()) {
            p_visual.poses[p_part.submodel] = p_pose;
        }
    }

    void VehicleRenderingServer::_send_poses(Visual &p_visual) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        if (models != nullptr && p_visual.model.is_valid() && !p_visual.poses.is_empty()) {
            models->instance_set_part_poses(p_visual.model, p_visual.poses);
        }
    }

    /* The snow drawn while the vehicle is drawn in detail: an overlay of its own that draws how much
     * of it there is, laid over its models when it comes and taken off when it goes - the vehicles
     * far away take no material for it. More or less of it changes the overlay alone. */
    void VehicleRenderingServer::_send_snow(const RID &p_vehicle, Visual &p_visual) {
        p_visual.snow_sent = p_visual.snow;
        const bool drawn = p_visual.detailed && p_visual.snow > SNOW_DRAWN_MIN && p_visual.snow_overlay.is_valid();
        if (drawn == p_visual.snow_material.is_valid()) {
            if (drawn) {
                p_visual.snow_material->set_shader_parameter(VEHICLE_SNOW_PARAMETER, p_visual.snow);
            }
            return;
        }
        if (drawn) {
            p_visual.snow_material = p_visual.snow_overlay->duplicate();
            p_visual.snow_material->set_shader_parameter(VEHICLE_SNOW_PARAMETER, p_visual.snow);
        } else {
            p_visual.snow_material.unref();
        }
        _lay_snow(p_vehicle, p_visual);
    }

    /* The snow's overlay, or none, over the exterior and its attachments - never over the low-poly
     * interior or the passengers - and over what a plugin's renderer draws of its own */
    void VehicleRenderingServer::_lay_snow(const RID &p_vehicle, const Visual &p_visual) {
        if (ModelRenderingServer *models = ModelRenderingServer::get_instance(); models != nullptr) {
            if (p_visual.model.is_valid()) {
                models->instance_set_material_overlay(p_visual.model, p_visual.snow_material);
            }
            for (const RID &instance: p_visual.attachments) {
                models->instance_set_material_overlay(instance, p_visual.snow_material);
            }
        }
        _vehicle_snow_changed(p_vehicle, p_visual.snow_material);
    }

    /// A vehicle far from the camera is rendered from RenderingServer instances instead of a node
    /// hierarchy: nothing animates at that distance, and the hierarchy is what costs - hundreds of
    /// Node3Ds per vehicle to walk, notify and propagate a transform through, times the hundreds of
    /// vehicles a scenery runs. Its simulation is untouched. Note the OPTIMIZED backend does not
    /// render SUBMODEL_FREE_SPOTLIGHT (see TODO.md), so a distant vehicle loses its lights.
    ///
    /// Beyond the streaming's draw distance a vehicle is not drawn at all: the models built from its
    /// appearance are built when it comes within it - a scenery's hundreds of vehicles all built at
    /// load filled it with the materials and textures of vehicles nobody saw - and freed when it
    /// leaves. Without the streaming's camera - while a scenery loads - nothing is decided; the
    /// editor builds them at once.
    void VehicleRenderingServer::_update_detail(const RID &p_vehicle, Visual &p_visual) {
        const SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        const Node3D *node = _node(p_visual);
        // the nodes of a vehicle drawn in detail are built under its scene node
        if (node == nullptr || !node->is_inside_tree() || !p_visual.placed) {
            return;
        }
        const bool has_camera = streaming != nullptr && streaming->streaming_has_camera();
        const double distance =
                has_camera ? p_visual.transform.origin.distance_to(streaming->streaming_get_camera_position()) : 0.0;
        if (p_visual.appearance.is_valid() && !p_visual.appearance->get_model_filename().is_empty()) {
            const bool at_once = p_visual.editable || Engine::get_singleton()->is_editor_hint();
            bool drawn = at_once || p_visual.model.is_valid();
            if (!at_once && has_camera) {
                const float draw_distance = streaming->streaming_get_draw_distance();
                const float hysteresis = MAX(DETAIL_HYSTERESIS_MIN, draw_distance * DETAIL_HYSTERESIS);
                drawn = p_visual.model.is_valid() ? distance <= draw_distance + hysteresis : distance <= draw_distance;
            }
            // a model that could not be loaded is not loaded again - it would stay a pending build,
            // and the loading screen waits for the builds within the draw distance
            if (drawn && !p_visual.model.is_valid() && !p_visual.model_missing) {
                if (at_once) {
                    // the editor's or an editable vehicle's models are wanted at once: loaded here
                    if (!_is_preloaded(p_vehicle, p_visual)) {
                        _apply_preloaded(
                                p_vehicle, p_visual, _load_resources(_get_preload_requests(p_vehicle, p_visual)));
                    }
                    _build_models(p_vehicle, p_visual);
                } else if (!p_visual.build_pending) {
                    int index = 0;
                    while (index < pending_builds.size() && pending_builds[index].distance <= distance) {
                        ++index;
                    }
                    pending_builds.insert(index, PendingBuild{p_vehicle, distance});
                    p_visual.build_pending = true;
                }
                return;
            }
            if (!drawn) {
                _cancel_build(p_vehicle, p_visual);
            }
            if (!drawn && p_visual.model.is_valid()) {
                if (p_visual.detailed) {
                    _set_detailed(p_vehicle, p_visual, false);
                }
                _free_models(p_vehicle, p_visual);
                _vehicle_models_changed(p_vehicle);
                _bind_parts(p_vehicle, p_visual);
                _register_pickable(p_vehicle, p_visual);
                return;
            }
        }
        if (!p_visual.model.is_valid()) {
            return;
        }
        bool detailed = p_visual.editable;
        if (!detailed) {
            if (!has_camera) {
                return;
            }
            const float hysteresis = MAX(DETAIL_HYSTERESIS_MIN, detail_distance * DETAIL_HYSTERESIS);
            detailed = p_visual.detailed ? distance <= detail_distance : distance <= detail_distance - hysteresis;
        }
        if (detailed != p_visual.detailed) {
            _set_detailed(p_vehicle, p_visual, detailed);
        }
    }

    /* Built, the vehicle takes its detail at once: one near the camera is drawn as nodes before its
     * build counts as done (builds_get_pending_count()), not at the sweep's next turn */
    void VehicleRenderingServer::_build_models(const RID &p_vehicle, Visual &p_visual) {
        // in the game's log before and after: a crash with no message is found by the last line
        const String model_path =
                p_visual.appearance->get_data_path().path_join(p_visual.appearance->get_model_filename());
        UtilityFunctions::print("[VehicleRendering] building ", model_path);
        _create_models(p_vehicle, p_visual);
        _bind_parts(p_vehicle, p_visual);
        _vehicle_models_changed(p_vehicle);
        // the built models hold what they were built from (ResourceLazyLoader), and their materials
        // the textures
        p_visual.preloaded.clear();
        p_visual.preloaded_materials.clear();
        _update_detail(p_vehicle, p_visual);
        UtilityFunctions::print("[VehicleRendering] built ", model_path);
    }

    void VehicleRenderingServer::_cancel_build(const RID &p_vehicle, Visual &p_visual) {
        if (!p_visual.build_pending) {
            return;
        }
        p_visual.build_pending = false;
        for (int index = 0; index < pending_builds.size(); ++index) {
            if (pending_builds[index].vehicle == p_vehicle) {
                pending_builds.remove_at(index);
                if (pending_builds.is_empty()) {
                    emit_signal(builds_finished_signal);
                }
                return;
            }
        }
    }

    void VehicleRenderingServer::_set_detailed(const RID &p_vehicle, Visual &p_visual, const bool p_detailed) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        Node3D *node = _node(p_visual);
        ERR_FAIL_NULL(models);
        ERR_FAIL_NULL(node);
        p_visual.detailed = p_detailed;
        // the models this server built get a node of its own to be built under, there only while
        // the vehicle is drawn in detail; the ones handed over have their owner's. An editable
        // vehicle's holder is shown in the Scene dock - a node's internal mode is chosen only when
        // it is added - and owned as the scene node is, which its nodes take (the model backend's)
        if (p_detailed && p_visual.own_models) {
            Node3D *holder = memnew(Node3D);
            holder->set_name(p_visual.appearance->get_model_filename().get_file().get_basename());
            node->add_child(holder, false, p_visual.editable ? Node::INTERNAL_MODE_DISABLED : Node::INTERNAL_MODE_BACK);
            if (p_visual.editable) {
                holder->set_owner(node->get_owner() != nullptr ? node->get_owner() : node);
            }
            holder->set_global_transform(p_visual.transform);
            p_visual.holder = ObjectID(holder->get_instance_id());
            models->instance_attach_object_instance_id(p_visual.model, p_visual.holder);
            if (p_visual.low_poly.is_valid()) {
                models->instance_attach_object_instance_id(p_visual.low_poly, p_visual.holder);
            }
        }
        const ModelRenderingServer::Instancer instancer = detail_instancer(p_visual.detailed, p_visual.editable);
        models->instance_set_instancer(p_visual.model, instancer);
        if (p_visual.low_poly.is_valid()) {
            models->instance_set_instancer(p_visual.low_poly, instancer);
        }
        if (Node *holder = Object::cast_to<Node>(ObjectDB::get_instance(p_visual.holder));
            !p_detailed && holder != nullptr) {
            // the nodes built under it went with the instancer that built them; out of the tree at
            // once, so that the holder made next does not take its name
            node->remove_child(holder);
            holder->queue_free();
            p_visual.holder = ObjectID();
        }
        _register_pickable(p_vehicle, p_visual);
        // the snow is drawn in detail only - after the pickable, whose outline gave the model back
        // the overlay it had
        _send_snow(p_vehicle, p_visual);
        if (p_detailed) {
            _place(p_vehicle, p_visual);
        }
    }

    /* The space the player finds the vehicle in - a box the size of the model, in the space of its
     * scene node's world; whose it is, detection_area_get_vehicle() says */
    void VehicleRenderingServer::_update_detection_area(const RID &p_vehicle, Visual &p_visual) {
        PhysicsServer3D *physics = PhysicsServer3D::get_singleton();
        const ModelRenderingServer *models = ModelRenderingServer::get_instance();
        const Node3D *node = _node(p_visual);
        if (Engine::get_singleton()->is_editor_hint() || physics == nullptr || models == nullptr || node == nullptr ||
            !node->is_inside_tree() || !p_visual.model.is_valid() || !p_visual.placed) {
            return;
        }
        const AABB aabb = models->instance_get_aabb(p_visual.model);
        if (aabb.size == Vector3()) {
            return;
        }
        if (!p_visual.detection_area.is_valid()) {
            p_visual.detection_area = physics->area_create();
            p_visual.detection_shape = physics->box_shape_create();
            physics->area_add_shape(p_visual.detection_area, p_visual.detection_shape);
            physics->area_set_monitorable(p_visual.detection_area, true);
            area_vehicles[p_visual.detection_area] = p_vehicle;
            physics->area_set_space(p_visual.detection_area, node->get_world_3d()->get_space());
        }
        physics->shape_set_data(p_visual.detection_shape, aabb.size * 0.5);
        physics->area_set_shape_transform(p_visual.detection_area, 0, Transform3D(Basis(), aabb.get_center()));
        physics->area_set_transform(p_visual.detection_area, p_visual.transform * p_visual.model_transform);
    }

    /// The model is clicked in free camera (SceneryHUDMouseServer) while it is detailed, so only
    /// the vehicles near the camera are tested under the cursor
    void VehicleRenderingServer::_register_pickable(const RID &p_vehicle, Visual &p_visual) {
        SceneryHUDMouseServer *mouse = SceneryHUDMouseServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        if (Engine::get_singleton()->is_editor_hint() || mouse == nullptr || vehicle_server == nullptr) {
            return;
        }
        mouse->pickable_free(p_visual.pickable);
        p_visual.pickable = RID();
        if (p_visual.detailed && p_visual.model.is_valid()) {
            p_visual.pickable = mouse->vehicle_pickable_create(
                    p_visual.model, vehicle_server->vehicle_get_id(p_vehicle), p_vehicle);
        }
    }

    void VehicleRenderingServer::_follow_placements(Object *p_server, const char *p_placed, const char *p_changed) {
        p_server->connect(p_placed, callable_mp(this, &VehicleRenderingServer::_on_vehicle_placed));
        p_server->connect(p_changed, callable_mp(this, &VehicleRenderingServer::_on_vehicle_placed));
    }

    /// Placed - on loading, its first place - its detail is decided where it stands
    void VehicleRenderingServer::_on_vehicle_placed(const RID &p_vehicle) {
        if (Visual *visual = vehicles.getptr(p_vehicle); visual != nullptr) {
            _place(p_vehicle, *visual);
            _update_detail(p_vehicle, *visual);
        }
    }

    void VehicleRenderingServer::_on_vehicle_config_changed(const RID &p_vehicle) {
        if (Visual *visual = vehicles.getptr(p_vehicle); visual != nullptr) {
            _place(p_vehicle, *visual);
        }
    }

    void VehicleRenderingServer::_on_vehicle_freed(const RID &p_vehicle) {
        vehicle_detach(p_vehicle);
    }

    /* A model built again - another instancer, a stream - is found and drawn anew: its size for
     * the detection area, and what a plugin's renderer finds in it */
    void VehicleRenderingServer::_on_instance_built(const RID &p_instance) {
        const RID *vehicle = model_vehicles.getptr(p_instance);
        Visual *visual = vehicle != nullptr ? vehicles.getptr(*vehicle) : nullptr;
        if (visual == nullptr) {
            return;
        }
        _bind_parts(*vehicle, *visual);
        emit_signal(vehicle_model_built_signal, *vehicle);
    }

    void VehicleRenderingServer::_set_processing(const bool p_processing) {
        if (processing == p_processing) {
            return;
        }
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree == nullptr) {
            return;
        }
        processing = p_processing;
        if (p_processing) {
            tree->connect("process_frame", callable_mp(this, &VehicleRenderingServer::_process_frame));
            return;
        }
        tree->disconnect("process_frame", callable_mp(this, &VehicleRenderingServer::_process_frame));
    }

    /* Every vehicle's detail a few times a second, a budget of them a frame; the poses of the
     * vehicles drawn in detail every frame, a budget of them a frame - with what a plugin's renderer
     * does at each. */
    void VehicleRenderingServer::_process_frame() {
        if (Engine::get_singleton()->is_editor_hint() || visit_order.is_empty()) {
            return;
        }
        const SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        const double delta = tree != nullptr ? tree->get_root()->get_process_delta_time() : 0.0;
        const int count = static_cast<int>(visit_order.size());

        // the models of the vehicles that came within the draw distance, the nearest first
        const uint64_t deadline = Time::get_singleton()->get_ticks_msec() + BUILD_BUDGET_MSEC;
        const bool had_builds = !pending_builds.is_empty();
        // one whose files are still loading on a worker waits in its place (_preload())
        int index = 0;
        while (index < pending_builds.size() && Time::get_singleton()->get_ticks_msec() < deadline) {
            const RID vehicle = pending_builds[index].vehicle;
            Visual &visual = vehicles[vehicle];
            if (!_is_preloaded(vehicle, visual)) {
                _preload(vehicle, visual);
                ++index;
                continue;
            }
            pending_builds.remove_at(index);
            visual.build_pending = false;
            _build_models(vehicle, visual);
        }
        if (had_builds && pending_builds.is_empty()) {
            emit_signal(builds_finished_signal);
        }

        slow_elapsed += delta;
        const int slow_budget =
                MIN(count, MAX(1, MIN(MAX_SLOW_UPDATES_PER_FRAME,
                                      static_cast<int>(Math::ceil(count * delta / SLOW_UPDATE_PERIOD)))));
        for (int visited = 0; visited < slow_budget; ++visited) {
            slow_cursor = (slow_cursor + 1) % count;
            const RID vehicle = visit_order[slow_cursor];
            Visual &visual = vehicles[vehicle];
            _update_detail(vehicle, visual);
            _vehicle_update(vehicle);
        }

        for (int visited = 0, looked = 0; visited < MAX_DETAILED_UPDATES_PER_FRAME && looked < count; ++looked) {
            detailed_cursor = (detailed_cursor + 1) % count;
            const RID vehicle = visit_order[detailed_cursor];
            Visual &visual = vehicles[vehicle];
            if (!visual.detailed || !visual.model.is_valid()) {
                continue;
            }
            ++visited;
            _vehicle_pose(vehicle);
            _send_poses(visual);
        }
        _frame_processed(delta);
    }
} // namespace godot
