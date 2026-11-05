#pragma once
#include "utils/RIDHash.hpp"
#include "utils/WorkerTaskQueue.hpp"
#include "vehicles/base/VehicleAppearance.hpp"

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* What a vehicle looks like, drawn: its models, its running gear, its head display, its
     * low-poly interior, the detail it is drawn in and where the player finds it. Keyed by the
     * vehicle's VehicleServer handle. A kind of vehicle's renderer derives from it - where its
     * server places the vehicle, and what its running gear is (RailVehicleRenderingServer,
     * RoadVehicleRenderingServer) - and stands as that kind's singleton.
     *
     * The vehicle is drawn at a node (vehicle_attach()) - the one its cab, sounds and anything
     * else of the vehicle ride on; this server moves that node wherever the vehicle is placed, and
     * draws the models there, its running gear turned as the vehicle runs. A plugin's renderer
     * derives from a kind's and animates the rest of its own vehicles natively through the
     * protected hooks below - once a frame for the vehicles near the camera and a few times a
     * second for the rest, no script per vehicle - and stands in its place under the same
     * singleton; without a plugin the core draws the models only. */
    class VehicleRenderingServer : public Object {
            GDCLASS(VehicleRenderingServer, Object)

        public:
            /* The vehicle's exterior model was built - as nodes near the camera, as
             * RenderingServer instances far from it; what dresses its nodes does it again */
            static const char *vehicle_model_built_signal;
            /* No vehicle within the draw distance waits for its models any more
             * (builds_get_pending_count() came down to 0) */
            static const char *builds_finished_signal;

            /* reloaded/vehicles/detail_distance [m]: nearer than this a vehicle is drawn as nodes and
             * animated, further it is drawn as RenderingServer instances and holds still */
            static constexpr const char *DETAIL_DISTANCE_SETTING = "reloaded/vehicles/detail_distance";
            static constexpr float DEFAULT_DETAIL_DISTANCE = 350.0;
            /* A vehicle drawn in detail keeps it until this much nearer than the detail distance,
             * so one on the boundary is not rebuilt over and over - proportional, as a fixed margin
             * is nothing at a long detail distance and more than the distance at a short one */
            static constexpr float DETAIL_HYSTERESIS = 0.25;
            static constexpr float DETAIL_HYSTERESIS_MIN = 25.0;
            /* How often every vehicle's detail is looked at [s] - and what a plugin's renderer looks at
             * with it - and how many vehicles a frame may look at doing it */
            static constexpr double SLOW_UPDATE_PERIOD = 0.25;
            static constexpr int MAX_SLOW_UPDATES_PER_FRAME = 16;
            /* How many vehicles drawn in detail a frame animates */
            static constexpr int MAX_DETAILED_UPDATES_PER_FRAME = 32;
            /* Time the models of the vehicles coming within the draw distance may take to build a
             * frame [ms]; a build that started is finished, so a frame builds at least one - the
             * streaming's own budget (SceneryStreamingServer::BUDGET_MSEC) */
            static constexpr uint64_t BUILD_BUDGET_MSEC = 4;
            /* A vehicle drawn in detail with snow on it draws it with a copy of its own of its snow
             * material (vehicle_set_snow_material()), how much snow lies on it, 0-1, in the copy's
             * VEHICLE_SNOW_PARAMETER - near the camera only, within the detail distance */
            static constexpr const char *VEHICLE_SNOW_PARAMETER = "vehicle_snow";
            /* Less snow than this is not drawn: the vehicle's overlay is taken off */
            static constexpr float SNOW_DRAWN_MIN = 0.001F;
            /* A change of the snow smaller than this is not sent to the models */
            static constexpr float SNOW_SEND_THRESHOLD = 0.01F;

        protected:
            /* A submodel of the exterior model that moves, and where it rests in the vehicle's own
             * frame - what its pose turns it from */
            struct Part {
                    String submodel;
                    Transform3D rest;
            };

            struct Visual {
                    /* The scene node whose world the vehicle is drawn in - its space, and the parent
                     * of the holder; this server never moves it */
                    ObjectID node;
                    /* The world it is drawn in, none while its scene node is out of one */
                    RID scenario;
                    /* Where the vehicle stands, once it has a place: its kind's server placed it
                     * (_vehicle_get_placement()), or it was told (vehicle_set_transform()). Until
                     * then its models are in no world - a scenery's vehicles are built before their
                     * trainset stands them on its track, and were drawn at the origin meanwhile */
                    Transform3D transform;
                    bool placed = false;
                    /* The node the models of a vehicle drawn in detail are built under - this
                     * server's own, there only while the vehicle is detailed */
                    ObjectID holder;
                    /* Nodes of other layers riding on the vehicle (vehicle_mount_node()) */
                    Vector<ObjectID> mounts;
                    Ref<VehicleAppearance> appearance;
                    /* The exterior and the low-poly interior - this server's own when it built them
                     * from the appearance, else whoever handed them over owns them */
                    RID model;
                    RID low_poly;
                    RID passengers;
                    Vector<RID> attachments;
                    bool own_models = false;
                    /* The appearance's model files registered with ResourceLazyLoader, by file
                     * name - loaded at once without lazy loading - and what the models built from
                     * them hold, one per model */
                    HashMap<String, RID> model_resources;
                    Vector<RID> held_models;
                    /* What the preload loaded on a worker, by resource - taken by the next build,
                     * which holds them (_get_model()) */
                    RIDMap<Ref<Resource>> preloaded;
                    /* What holds the files of those models' materials the preload read
                     * (ModelRenderingServer::material_preload()), let go with them after the build */
                    Array preloaded_materials;
                    /* The preload running for it in preload_queue, -1 for none */
                    int preload_task = -1;
                    /* Waiting in pending_builds for its models to be built */
                    bool build_pending = false;
                    /* The appearance's model could not be loaded - not built again until another
                     * appearance is set */
                    bool model_missing = false;
                    Transform3D model_transform;
                    /* The running gear its kind's renderer binds and turns (_bind_running_gear(),
                     * _pose_running_gear()): bogies, and wheels by group - a rail vehicle's front
                     * rolling, powered and rear rolling ones, a road vehicle's all in the first */
                    Part bogies[2];
                    Vector<Part> wheels[3];
                    /* Every pose of the exterior, sent to it in one call */
                    Dictionary poses;
                    Ref<Material> head_display_material;
                    /* What its snow is drawn with (vehicle_set_snow_material()); none - no snow drawn */
                    Ref<ShaderMaterial> snow_overlay;
                    /* Born optimized: a scenery's vehicles built as node hierarchies (cars,
                     * interiors) filled the load; _update_detail() details the near ones */
                    bool detailed = false;
                    /* vehicle_set_editable(): drawn in detail wherever the camera is, its nodes
                     * shown in the editor's Scene dock */
                    bool editable = false;
                    RID pickable;
                    RID detection_area;
                    RID detection_shape;
                    /* vehicle_set_shown_cabin() */
                    RID shown_cabin;
                    /* The snow on it, 0-1, as whoever simulates the weather tells it
                     * (vehicle_set_snow()) - drawn or not; snow_sent is what was sent last */
                    float snow = 0.0F;
                    float snow_sent = 0.0F;
                    /* The overlay its snow is drawn with (snow_overlay, its own copy), there only
                     * while it is drawn in detail with snow on it */
                    Ref<ShaderMaterial> snow_material;
                    /* Per-instance uniforms of its exterior's shaders (vehicle_set_shader_parameter()),
                     * laid on its models as they are built */
                    Dictionary shader_parameters;
            };

            /* Where the kind's server placed the vehicle; false while it stands nowhere */
            virtual bool _vehicle_get_placement(const RID &p_vehicle, Transform3D &p_r_transform) const = 0;
            /* The kind's running gear found in the exterior model, and turned as the vehicle runs -
             * before the poses are sent, every frame the vehicle is drawn in detail */
            virtual void _bind_running_gear(const RID &p_vehicle, Visual &p_visual) {}
            virtual void _pose_running_gear(const RID &p_vehicle, Visual &p_visual) {}
            /* The kind's server announces by p_placed that it placed a vehicle and by p_changed that
             * it moved one: the vehicle is drawn there from then on */
            void _follow_placements(Object *p_server, const char *p_placed, const char *p_changed);

            /* What a plugin's renderer adds to the drawing of a vehicle, called by this server at the
             * moments the base has none of its own for - each with the vehicle's handle, the base's
             * Visual reached by _get_visual(). The base does nothing in them. */
            /* Attached, and before it is detached: what the renderer keeps of the vehicle */
            virtual void _vehicle_attached(const RID &p_vehicle) {}
            virtual void _vehicle_detaching(const RID &p_vehicle) {}
            /* Another appearance was set, its model files registered and not yet loaded */
            virtual void _vehicle_appearance_changed(const RID &p_vehicle) {}
            /* More model files the vehicle's build takes, as [resource, data path, skins] */
            virtual void _vehicle_add_preload_requests(const RID &p_vehicle, Array &p_r_requests) const {}
            /* The model files were loaded (_get_model() has them) */
            virtual void _vehicle_preloaded(const RID &p_vehicle) {}
            /* The models were created from the appearance, freed, or built or dropped as a whole */
            virtual void _vehicle_models_created(const RID &p_vehicle) {}
            virtual void _vehicle_models_freed(const RID &p_vehicle) {}
            virtual void _vehicle_models_changed(const RID &p_vehicle) {}
            /* The exterior's moving submodels were found again - in a model built, or none */
            virtual void _vehicle_parts_bound(const RID &p_vehicle) {}
            /* The vehicle moved, and its models went into another world */
            virtual void _vehicle_moved(const RID &p_vehicle) {}
            virtual void _vehicle_scenario_changed(const RID &p_vehicle, const RID &p_scenario) {}
            /* The few times a second every vehicle is looked at */
            virtual void _vehicle_update(const RID &p_vehicle) {}
            /* Every frame for the vehicles drawn in detail, before their poses are sent */
            virtual void _vehicle_pose(const RID &p_vehicle) {}
            /* vehicle_set_shown_cabin() changed */
            virtual void _vehicle_shown_cabin_changed(const RID &p_vehicle) {}
            /* The overlay the vehicle's snow is drawn with (Visual::snow_material) was laid over its
             * exterior and attachments, or taken off them - null: what the renderer draws of its own
             * takes it too. Only the overlay: how much snow it draws changes in it */
            virtual void _vehicle_snow_changed(const RID &p_vehicle, const Ref<Material> &p_overlay) {}
            /* The wipers of the vehicle's exterior model, as vehicle_get_wiper_sweeps() gives them;
             * the base knows of none */
            virtual PackedVector4Array _vehicle_get_wiper_sweeps(const RID &p_vehicle) const {
                return {};
            }
            /* Once a frame, after every vehicle's own work */
            virtual void _frame_processed(double p_delta) {}

            Visual *_get_visual(const RID &p_vehicle);
            const Visual *_get_visual(const RID &p_vehicle) const;
            Part _part(const Visual &p_visual, const String &p_submodel) const;
            Vector<Part> _parts(const Visual &p_visual, const PackedStringArray &p_submodels) const;
            static void _pose(Visual &p_visual, const Part &p_part, const Basis &p_pose);
            static void _pose(Visual &p_visual, const Part &p_part, const Transform3D &p_pose);
            /* The model of a registered file as the preload left it - never loaded here, on the
             * main thread */
            static Ref<Resource> _get_model(const Visual &p_visual, const RID &p_resource);
            /* The vehicle's model files loaded again - one more was registered */
            void _vehicle_preload(const RID &p_vehicle);
            /* Every vehicle detached; a renderer that derives calls it from its own destructor,
             * while its hooks still answer */
            void _detach_all();

        private:
            RIDMap<Visual> vehicles;
            /* The vehicle each exterior model draws */
            RIDMap<RID> model_vehicles;
            /* The vehicle each detection area finds */
            RIDMap<RID> area_vehicles;
            /* The vehicles in the order the frame visits them, and where each visit left off */
            Vector<RID> visit_order;
            int slow_cursor = 0;
            int detailed_cursor = 0;
            double slow_elapsed = 0.0;
            /* A vehicle within the draw distance whose models wait for their build, and how far
             * from the camera it was when it came there - the nearest are built first */
            struct PendingBuild {
                    RID vehicle;
                    double distance = 0.0;
            };
            Vector<PendingBuild> pending_builds;
            /* The vehicles' model files are loaded here, off the main thread (_preload()) - the
             * loader is GDScript, so it is drained before the scripts go (preloads_drain()) */
            Ref<WorkerTaskQueue> preload_queue;
            HashMap<int, RID> preload_vehicles;
            bool processing = false;
            /* DETAIL_DISTANCE_SETTING, read when the settings change - not per vehicle visited */
            float detail_distance = DEFAULT_DETAIL_DISTANCE;

            static Node3D *_node(const Visual &p_visual);
            void _set_processing(bool p_processing);
            void _process_frame();
            void _on_data_reload_requested();
            void _on_streaming_camera_changed();
            void _on_project_settings_changed();
            void _free_models(const RID &p_vehicle, Visual &p_visual);
            static void _free_model_resources(Visual &p_visual);
            Array _get_preload_requests(const RID &p_vehicle, const Visual &p_visual) const;
            static Array _load_resources(const Array &p_requests);
            void _preload(const RID &p_vehicle, Visual &p_visual);
            void _on_preload_finished(int p_task);
            void _apply_preloaded(const RID &p_vehicle, Visual &p_visual, const Array &p_loaded);
            bool _is_preloaded(const RID &p_vehicle, const Visual &p_visual) const;
            void _create_models(const RID &p_vehicle, Visual &p_visual);
            void _bind_parts(const RID &p_vehicle, Visual &p_visual);
            void _place(const RID &p_vehicle, Visual &p_visual);
            void _move(const RID &p_vehicle, Visual &p_visual);
            void _show_models(const RID &p_vehicle, const Visual &p_visual, const RID &p_scenario);
            void _send_poses(Visual &p_visual);
            void _send_snow(const RID &p_vehicle, Visual &p_visual);
            void _lay_snow(const RID &p_vehicle, const Visual &p_visual);
            void _lay_shader_parameters(const Visual &p_visual);
            void _update_detail(const RID &p_vehicle, Visual &p_visual);
            void _set_detailed(const RID &p_vehicle, Visual &p_visual, bool p_detailed);
            /* The models built from the appearance - created, the parts found in them */
            void _build_models(const RID &p_vehicle, Visual &p_visual);
            void _cancel_build(const RID &p_vehicle, Visual &p_visual);
            void _update_detection_area(const RID &p_vehicle, Visual &p_visual);
            void _register_pickable(const RID &p_vehicle, Visual &p_visual);
            void _on_vehicle_placed(const RID &p_vehicle);
            void _on_vehicle_config_changed(const RID &p_vehicle);
            void _on_vehicle_freed(const RID &p_vehicle);
            void _on_instance_built(const RID &p_instance);

        protected:
            static void _bind_methods();

        public:
            VehicleRenderingServer();

            /* The vehicle is drawn in the world of the scene node p_node_id, once it has a place:
             * where its kind's server places it, or where vehicle_set_transform() says.
             * The node itself is never moved (a node that is to follow the vehicle is mounted,
             * vehicle_mount_node()). Freed with the vehicle (VehicleServer.vehicle_freed). */
            void vehicle_attach(const RID &p_vehicle, uint64_t p_node_id);
            void vehicle_detach(const RID &p_vehicle);
            bool vehicle_is_attached(const RID &p_vehicle) const;
            /* Where a vehicle its server has not placed is drawn - one assembled by hand, where its
             * node stands. The server's placement takes over once it places the vehicle. */
            void vehicle_set_transform(const RID &p_vehicle, const Transform3D &p_transform);
            /* Where the vehicle is drawn; the identity for a vehicle not drawn, or without a place */
            Transform3D vehicle_get_transform(const RID &p_vehicle) const;
            /* A Node3D of another layer rides on the vehicle - a cab, a sound emitter, the node of
             * a vehicle assembled by hand: it is put where the vehicle stands, now and wherever
             * the vehicle is placed, until it is unmounted or the vehicle is freed */
            void vehicle_mount_node(const RID &p_vehicle, uint64_t p_node_id);
            void vehicle_unmount_node(const RID &p_vehicle, uint64_t p_node_id);
            /* The vehicle a detection area (PhysicsServer3D) finds - what the player's shape cast
             * hits; an empty RID for an area that is no vehicle's */
            RID detection_area_get_vehicle(const RID &p_area) const;
            /* The world the scene node is in, an empty RID out of it: the models this server built
             * are drawn there - the node's NOTIFICATION_ENTER_WORLD/EXIT_WORLD, as
             * VisualInstance3D's */
            void vehicle_set_scenario(const RID &p_vehicle, const RID &p_scenario);
            /* What the vehicle looks like. With model files, this server builds the models itself -
             * once the vehicle stands within the draw distance of the streaming's camera
             * (SceneryStreamingServer), and frees them beyond it; without, it draws the ones
             * vehicle_set_models() hands it. */
            void vehicle_set_appearance(const RID &p_vehicle, const Ref<VehicleAppearance> &p_appearance);
            Ref<VehicleAppearance> vehicle_get_appearance(const RID &p_vehicle) const;
            /* The exterior and the low-poly interior, as ModelRenderingServer instances somebody else
             * owns - a vehicle assembled by hand */
            void vehicle_set_models(const RID &p_vehicle, const RID &p_model, const RID &p_low_poly);
            /* The exterior model as ModelRenderingServer draws it */
            RID vehicle_get_model(const RID &p_vehicle) const;
            void vehicle_set_head_display_material(const RID &p_vehicle, const Ref<Material> &p_material);
            /* What the vehicle's snow is drawn with, given by whoever builds it (its scenery's,
             * SceneryContentNode.vehicle_snow_material) once it is attached - the snow drawn is a copy
             * of it; none, no snow is drawn. Nothing here loads one of its own */
            void vehicle_set_snow_material(const RID &p_vehicle, const Ref<ShaderMaterial> &p_material);
            /* How much snow lies on the vehicle, 0-1, by whoever simulates the weather - nothing here
             * gathers or melts it; drawn while the vehicle is drawn in detail */
            void vehicle_set_snow(const RID &p_vehicle, float p_snow);
            float vehicle_get_snow(const RID &p_vehicle) const;
            /* A per-instance uniform of the shaders of the vehicle's exterior and its attachments
             * ("instance uniform"), by whoever draws more on it - a vehicle's own of a material its
             * kind shares; kept while its models are built again, null takes it off */
            void vehicle_set_shader_parameter(const RID &p_vehicle, const StringName &p_name, const Variant &p_value);
            /* The wipers of the vehicle's exterior model, three vectors each in the model's frame:
             * where its arm turns about and how long the arm is (x, y, z; w), which way it points at
             * rest and the most it turns away from there (x, y, z; w, radians, signed about the
             * axis), and the axis it turns about (x, y, z); none while its model is not built */
            PackedVector4Array vehicle_get_wiper_sweeps(const RID &p_vehicle) const;
            /* The cabin whose interior is drawn, by whoever shows it, in the place of the
             * low-poly interior's cab it stands for - which a plugin's renderer hides, whoever
             * drives (the player's cab, DynObj.cpp:1389-1397); RID() while none is: every
             * low-poly cab is drawn */
            void vehicle_set_shown_cabin(const RID &p_vehicle, const RID &p_cabin);
            /* Whether the vehicle is drawn in detail - as nodes, animated */
            bool vehicle_is_detailed(const RID &p_vehicle) const;
            /* Whether the vehicle's models stand as nodes the editor shows and edits ("Edit FIZ"):
             * in detail wherever the camera is, under a holder of the scene, owned by the scene's
             * owner - so whoever saves the scene takes them off first */
            void vehicle_set_editable(const RID &p_vehicle, bool p_editable);
            bool vehicle_is_editable(const RID &p_vehicle) const;
            /* The vehicles within the draw distance whose models still wait for their build */
            int builds_get_pending_count() const;
            /* Drops the queued preloads and waits for the running ones. Called while the scene tree
             * is still alive: a preload may run a plugin's model loader written in GDScript */
            void preloads_drain();
    };
} // namespace godot
