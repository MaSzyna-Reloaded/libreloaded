#pragma once

#include "utils/RIDHash.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    /// Meshes a scenery places as they are - a plugin's world's buildings, platforms and bridges, made
    /// as ArrayMesh resources of its sectors - drawn by a RenderingServer
    /// instance only while the camera is within their range (SceneryStreamingServer). The mesh is
    /// given with its surfaces' materials; the server keeps it and draws it, nothing more.
    class SceneryMeshRenderingServer : public Object {
            GDCLASS(SceneryMeshRenderingServer, Object)

        protected:
            static void _bind_methods();

        private:
            struct MeshPlacement {
                    Ref<Mesh> mesh;
                    Transform3D transform;
                    RID scenario;
                    /// SceneryStreamingServer's registration
                    RID stream_rid;
                    /// The RenderingServer instance while it is built, invalid otherwise
                    RID instance;
            };
            RIDMap<MeshPlacement> placements;
            int stream_owner = 0;

            void _stream_build(const RID &p_mesh, const Variant &p_preloaded);
            void _stream_clear(const RID &p_mesh);

        public:
            SceneryMeshRenderingServer();
            ~SceneryMeshRenderingServer() override;

            static SceneryMeshRenderingServer *get_instance() {
                return singleton;
            }

        private:
            /// The one instance, as the engine's servers keep theirs (RenderingServer::singleton):
            /// set by the constructor, cleared by the destructor
            static inline SceneryMeshRenderingServer *singleton = nullptr;

        public:
            /* `p_mesh` placed by `p_transform` in `p_scenario`, drawn while the camera is within
             * `p_range_end` of its centre (0: the draw distance) */
            RID mesh_create(
                    const Ref<Mesh> &p_mesh, const Transform3D &p_transform, float p_range_end, const RID &p_scenario);
            /* The world it is drawn in; an empty RID draws it nowhere (its node out of the world) */
            void mesh_set_scenario(const RID &p_mesh, const RID &p_scenario);
            void mesh_free(const RID &p_mesh);
    };
} // namespace godot
