#include "rendering/StandardTrackRenderingBackend.hpp"

#include "tracks/TrackServer.hpp"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/core/class_db.hpp>

#include <initializer_list>

namespace godot {
    namespace {
        /// The rail's section, 60E1 [m]: from the foot's left edge round the head to the foot's right
        /// edge, x from the rail's centre, y from the head's top
        constexpr real_t RAIL_PROFILE[][2] = {
                {-0.075F, -0.172F}, {-0.075F, -0.160F}, {-0.0085F, -0.135F}, {-0.0085F, -0.045F},
                {-0.036F, -0.037F}, {-0.036F, 0.0F},    {0.036F, 0.0F},      {0.036F, -0.037F},
                {0.0085F, -0.045F}, {0.0085F, -0.135F}, {0.075F, -0.160F},   {0.075F, -0.172F},
        };
        /// How far apart the points a curve's tangent is taken from are [m]
        constexpr double TANGENT_STEP = 0.05;
    } // namespace

    void StandardTrackRenderingBackend::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("track_get_mesh", "track_render"), &StandardTrackRenderingBackend::track_get_mesh);
        ClassDB::bind_method(
                D_METHOD("track_get_sleeper_multimeshes", "track_render"),
                &StandardTrackRenderingBackend::track_get_sleeper_multimeshes);
        ClassDB::bind_method(
                D_METHOD("track_get_sleeper_transforms", "track_render"),
                &StandardTrackRenderingBackend::track_get_sleeper_transforms);
    }

    StandardTrackRenderingBackend::StandardTrackRenderingBackend() {
        const String path = ProjectSettings::get_singleton()->get_setting(LOOK_SETTING, DEFAULT_LOOK);
        for (const TrackData::SleeperKind kind:
             {TrackData::SLEEPER_CONCRETE, TrackData::SLEEPER_WOOD, TrackData::SLEEPER_STEEL}) {
            base_sleepers[kind] = _build_base_sleeper(kind);
        }
        look = ResourceLoader::get_singleton()->load(path);
        ERR_FAIL_COND_MSG(look.is_null(), "StandardTrackRenderingBackend: no track look at " + path);
    }

    StandardTrackRenderingBackend::~StandardTrackRenderingBackend() {
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (const KeyValue<RID, Track> &item: tracks) {
            if (item.value.instance.is_valid() && rendering != nullptr) {
                rendering->free_rid(item.value.instance);
            }
        }
    }

    void StandardTrackRenderingBackend::track_create(const RID &p_track_render, const RID &p_track) {
        Track track;
        track.track = p_track;
        tracks.insert(p_track_render, track);
    }

    void StandardTrackRenderingBackend::track_free(const RID &p_track_render) {
        track_clear(p_track_render);
        tracks.erase(p_track_render);
    }

    void StandardTrackRenderingBackend::track_set_description(
            const RID &p_track_render, const Ref<TrackData> &p_description) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        track->description = p_description;
    }

    void StandardTrackRenderingBackend::track_set_visible(const RID &p_track_render, const bool p_visible) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        track->visible = p_visible;
        RenderingServer *rendering = RenderingServer::get_singleton();
        if (track->instance.is_valid()) {
            rendering->instance_set_visible(track->instance, p_visible);
        }
        for (const RID &instance: track->sleeper_instances) {
            rendering->instance_set_visible(instance, p_visible);
        }
    }

    void StandardTrackRenderingBackend::track_set_scenario(const RID &p_track_render, const RID &p_scenario) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        track->scenario = p_scenario;
        RenderingServer *rendering = RenderingServer::get_singleton();
        if (track->instance.is_valid()) {
            rendering->instance_set_scenario(track->instance, p_scenario);
        }
        for (const RID &instance: track->sleeper_instances) {
            rendering->instance_set_scenario(instance, p_scenario);
        }
    }

    /// The blades are not drawn
    void StandardTrackRenderingBackend::track_set_switch_blade_offsets(
            const RID & /* p_track_render */, const double /* p_offset1 */, const double /* p_offset2 */) {}

    double StandardTrackRenderingBackend::get_priority() const {
        return 0.5;
    }

    bool StandardTrackRenderingBackend::track_stream(const RID & /* p_track_render */) {
        return false;
    }

    void StandardTrackRenderingBackend::track_build(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        TrackServer *track_server = TrackServer::get_instance();
        ERR_FAIL_NULL(track_server);
        if (!track_server->track_exists(track->track)) {
            return;
        }
        track_clear(p_track_render);
        // a track drawn before its description is what a TrackData says by default
        Ref<TrackData> description = track->description;
        if (description.is_null()) {
            description.instantiate();
        }
        const bool road = track_server->track_get_group(track->track) == TrackServer::GROUP_ROAD;
        Vector<Surface> surfaces;
        const TrackServer::SwitchTrack branches[] = {TrackServer::TRACK_COMMON, TrackServer::TRACK_DIVERGING};
        for (const TrackServer::SwitchTrack branch: branches) {
            if (branch == TrackServer::TRACK_DIVERGING && !track_server->track_is_switch(track->track)) {
                break;
            }
            const Ref<TrackCurve> track_curve = track_server->track_get_curve(track->track, branch);
            const Ref<Curve3D> domain_curve = track_server->track_get_domain_curve(track->track, branch);
            if (track_curve.is_null() || domain_curve.is_null()) {
                continue;
            }
            const Ref<Curve3D> curve = _build_curve(domain_curve);
            const Vector<Frame> frames = _build_frames(curve, track_curve);
            if (frames.size() < 2) {
                continue;
            }
            if (road) {
                _add_road(frames, **description, surfaces);
                continue;
            }
            _add_rail_track(
                    frames, **description, branch == TrackServer::TRACK_DIVERGING ? DIVERGING_BED_DROP : 0.0, surfaces);
            // a switch's sleepers are the common curve's, the diverging rails lie on them
            if (branch == TrackServer::TRACK_COMMON) {
                _add_sleepers(*track, curve, track_curve, **description);
            }
        }
        if (surfaces.is_empty()) {
            return;
        }
        Ref<ArrayMesh> mesh;
        mesh.instantiate();
        for (const Surface &surface: surfaces) {
            Array arrays;
            arrays.resize(Mesh::ARRAY_MAX);
            arrays[Mesh::ARRAY_VERTEX] = surface.vertices;
            arrays[Mesh::ARRAY_NORMAL] = surface.normals;
            arrays[Mesh::ARRAY_TEX_UV] = surface.uvs;
            arrays[Mesh::ARRAY_INDEX] = surface.indices;
            mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
            mesh->surface_set_material(mesh->get_surface_count() - 1, surface.material);
        }
        RenderingServer *rendering = RenderingServer::get_singleton();
        track->mesh = mesh;
        track->instance = rendering->instance_create2(mesh->get_rid(), track->scenario);
        rendering->instance_set_visible(track->instance, track->visible);
    }

    void StandardTrackRenderingBackend::track_clear(const RID &p_track_render) {
        Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL(track);
        RenderingServer *rendering = RenderingServer::get_singleton();
        if (track->instance.is_valid()) {
            rendering->free_rid(track->instance);
            track->instance = RID();
        }
        track->mesh.unref();
        for (const RID &instance: track->sleeper_instances) {
            rendering->free_rid(instance);
        }
        for (const RID &multimesh: track->sleeper_multimeshes) {
            rendering->free_rid(multimesh);
        }
        track->sleeper_instances.clear();
        track->sleeper_multimeshes.clear();
    }

    TypedArray<RID> StandardTrackRenderingBackend::track_get_sleeper_multimeshes(const RID &p_track_render) const {
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, TypedArray<RID>());
        TypedArray<RID> multimeshes;
        for (const RID &multimesh: track->sleeper_multimeshes) {
            multimeshes.push_back(multimesh);
        }
        return multimeshes;
    }

    /// Placed again as track_build() places them: the common curve's
    Array StandardTrackRenderingBackend::track_get_sleeper_transforms(const RID &p_track_render) const {
        Array transforms;
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, transforms);
        const TrackServer *track_server = TrackServer::get_instance();
        const Ref<TrackCurve> track_curve = track_server->track_get_curve(track->track);
        const Ref<Curve3D> domain_curve = track_server->track_get_domain_curve(track->track);
        if (track_curve.is_null() || domain_curve.is_null() || track->description.is_null()) {
            return transforms;
        }
        for (const Transform3D &sleeper:
             _place_sleepers(_build_curve(domain_curve), track_curve, **track->description)) {
            transforms.push_back(sleeper);
        }
        return transforms;
    }

    Ref<ArrayMesh> StandardTrackRenderingBackend::track_get_mesh(const RID &p_track_render) const {
        const Track *track = tracks.getptr(p_track_render);
        ERR_FAIL_NULL_V(track, Ref<ArrayMesh>());
        return track->mesh;
    }

    /// The tracks are drawn each on its own
    void StandardTrackRenderingBackend::topology_rebuilt() {}

    /// TrackServer's domain curve of the track - where its vehicles run, its ends lifted by the roll -
    /// sampled as the drawing needs it: a straight one sparsely
    Ref<Curve3D> StandardTrackRenderingBackend::_build_curve(const Ref<Curve3D> &p_domain_curve) {
        Ref<Curve3D> curve = p_domain_curve->duplicate();
        const bool straight = curve->get_point_out(0).is_zero_approx() && curve->get_point_in(1).is_zero_approx();
        curve->set_bake_interval(static_cast<real_t>(straight ? STRAIGHT_SEGMENT_LENGTH : SEGMENT_LENGTH));
        return curve;
    }

    /// A frame at each baked point; the side and up turned by the roll, interpolated along the curve,
    /// as MaSzyna's backend turns its sections (maszyna_track_rendering_backend.gd
    /// _build_rail_section(): Track.cpp:2652)
    Vector<StandardTrackRenderingBackend::Frame>
    StandardTrackRenderingBackend::_build_frames(const Ref<Curve3D> &p_curve, const Ref<TrackCurve> &p_track_curve) {
        Vector<Frame> frames;
        const double length = p_curve->get_baked_length();
        if (Math::is_zero_approx(length)) {
            return frames;
        }
        const int steps = MAX(1, static_cast<int>(Math::ceil(length / p_curve->get_bake_interval())));
        for (int step = 0; step <= steps; step++) {
            frames.push_back(_frame_at(p_curve, p_track_curve, length * step / steps));
        }
        return frames;
    }

    /// The frame p_distance along p_curve: the tangent from the points just behind and ahead
    StandardTrackRenderingBackend::Frame StandardTrackRenderingBackend::_frame_at(
            const Ref<Curve3D> &p_curve, const Ref<TrackCurve> &p_track_curve, const double p_distance) {
        const double length = p_curve->get_baked_length();
        const Vector3 behind = p_curve->sample_baked(static_cast<real_t>(MAX(p_distance - TANGENT_STEP, 0.0)));
        const Vector3 ahead = p_curve->sample_baked(static_cast<real_t>(MIN(p_distance + TANGENT_STEP, length)));
        const Vector3 tangent = (ahead - behind).normalized();
        const Vector3 reference_up =
                Math::abs(tangent.dot(Vector3(0, 1, 0))) > 0.999F ? Vector3(1, 0, 0) : Vector3(0, 1, 0);
        const Vector3 side = reference_up.cross(tangent).normalized();
        const Vector3 up = tangent.cross(side).normalized();
        const double roll = -Math::deg_to_rad(
                Math::lerp(p_track_curve->get_roll1(), p_track_curve->get_roll2(), p_distance / length));
        Frame frame;
        frame.origin = p_curve->sample_baked(static_cast<real_t>(p_distance));
        frame.side = (side * Math::cos(roll)) - (up * Math::sin(roll));
        frame.up = (side * Math::sin(roll)) + (up * Math::cos(roll));
        frame.forward = tangent;
        frame.distance = p_distance;
        return frame;
    }

    /// Every edge of the section a strip of its own, its normal across the edge: the section's corners
    /// stay sharp. u runs across the section, v along the curve, both in metres
    void StandardTrackRenderingBackend::_sweep(
            const Vector<Frame> &p_frames, const PackedVector2Array &p_points, const Ref<Material> &p_material,
            Vector<Surface> &p_surfaces) {
        int surface_index = -1;
        for (int index = 0; index < p_surfaces.size(); index++) {
            if (p_surfaces[index].material == p_material) {
                surface_index = index;
                break;
            }
        }
        if (surface_index < 0) {
            Surface surface;
            surface.material = p_material;
            p_surfaces.push_back(surface);
            surface_index = static_cast<int>(p_surfaces.size()) - 1;
        }
        Surface &surface = p_surfaces.write[surface_index];
        double u = 0.0;
        for (int edge = 0; edge + 1 < p_points.size(); edge++) {
            const Vector2 from = p_points[edge];
            const Vector2 to = p_points[edge + 1];
            const double edge_length = from.distance_to(to);
            const Vector2 normal_2d = Vector2(-(to.y - from.y), to.x - from.x).normalized();
            const int first = static_cast<int>(surface.vertices.size());
            for (const Frame &frame: p_frames) {
                const Vector3 normal = frame.side * normal_2d.x + frame.up * normal_2d.y;
                surface.vertices.push_back(frame.origin + frame.side * from.x + frame.up * from.y);
                surface.vertices.push_back(frame.origin + frame.side * to.x + frame.up * to.y);
                surface.normals.push_back(normal);
                surface.normals.push_back(normal);
                surface.uvs.push_back(Vector2(static_cast<real_t>(u), static_cast<real_t>(frame.distance)));
                surface.uvs.push_back(
                        Vector2(static_cast<real_t>(u + edge_length), static_cast<real_t>(frame.distance)));
            }
            for (int step = 0; step + 1 < p_frames.size(); step++) {
                const int a0 = first + (step * 2);
                const int b0 = a0 + 1;
                const int a1 = a0 + 2;
                const int b1 = a0 + 3;
                // Godot's front faces are clockwise: (v0 - v2) x (v0 - v1) faces the viewer
                const Vector3 normal = surface.normals[a0];
                const Vector3 face = (surface.vertices[a0] - surface.vertices[b1])
                                             .cross(surface.vertices[a0] - surface.vertices[b0]);
                const bool flipped = face.dot(normal) < 0.0F;
                surface.indices.push_back(a0);
                surface.indices.push_back(flipped ? b1 : b0);
                surface.indices.push_back(flipped ? b0 : b1);
                surface.indices.push_back(a0);
                surface.indices.push_back(flipped ? a1 : b1);
                surface.indices.push_back(flipped ? b1 : a1);
            }
            u += edge_length;
        }
    }

    /// A sleeper: its section (z across the sleeper, y up) - a closed polygon - swept along X over the
    /// sleeper's length, its ends closed
    Ref<ArrayMesh> StandardTrackRenderingBackend::_build_base_sleeper(const TrackData::SleeperKind p_kind) {
        PackedVector2Array section;
        switch (p_kind) {
            case TrackData::SLEEPER_WOOD:
                section.push_back(Vector2(static_cast<real_t>(-0.5 * WOOD_SLEEPER_WIDTH), 0.0F));
                section.push_back(Vector2(static_cast<real_t>(0.5 * WOOD_SLEEPER_WIDTH), 0.0F));
                section.push_back(Vector2(
                        static_cast<real_t>(0.5 * WOOD_SLEEPER_WIDTH), static_cast<real_t>(-WOOD_SLEEPER_HEIGHT)));
                section.push_back(Vector2(
                        static_cast<real_t>(-0.5 * WOOD_SLEEPER_WIDTH), static_cast<real_t>(-WOOD_SLEEPER_HEIGHT)));
                break;
            case TrackData::SLEEPER_STEEL:
                section.push_back(Vector2(static_cast<real_t>(-0.5 * STEEL_SLEEPER_WIDTH), 0.0F));
                section.push_back(Vector2(static_cast<real_t>(0.5 * STEEL_SLEEPER_WIDTH), 0.0F));
                section.push_back(Vector2(
                        static_cast<real_t>(0.5 * STEEL_SLEEPER_WIDTH), static_cast<real_t>(-STEEL_SLEEPER_HEIGHT)));
                section.push_back(Vector2(
                        static_cast<real_t>(-0.5 * STEEL_SLEEPER_WIDTH), static_cast<real_t>(-STEEL_SLEEPER_HEIGHT)));
                break;
            default:
                section.push_back(Vector2(static_cast<real_t>(-0.5 * CONCRETE_SLEEPER_TOP_WIDTH), 0.0F));
                section.push_back(Vector2(static_cast<real_t>(0.5 * CONCRETE_SLEEPER_TOP_WIDTH), 0.0F));
                section.push_back(
                        Vector2(static_cast<real_t>(0.5 * CONCRETE_SLEEPER_BOTTOM_WIDTH),
                                static_cast<real_t>(-SLEEPER_HEIGHT)));
                section.push_back(
                        Vector2(static_cast<real_t>(-0.5 * CONCRETE_SLEEPER_BOTTOM_WIDTH),
                                static_cast<real_t>(-SLEEPER_HEIGHT)));
                break;
        }
        const real_t half_length = static_cast<real_t>(0.5 * SLEEPER_LENGTH);
        PackedVector3Array vertices;
        PackedVector3Array normals;
        PackedInt32Array indices;
        // a face of the sleeper, its corners in order round it, turned to face along p_normal
        const auto add_face = [&](const Vector<Vector3> &p_corners, const Vector3 &p_normal) {
            const int first = static_cast<int>(vertices.size());
            for (const Vector3 &corner: p_corners) {
                vertices.push_back(corner);
                normals.push_back(p_normal);
            }
            for (int corner = 1; corner + 1 < p_corners.size(); corner++) {
                // Godot's front faces are clockwise: (v0 - v2) x (v0 - v1) faces the viewer
                const bool flipped =
                        (p_corners[0] - p_corners[corner + 1]).cross(p_corners[0] - p_corners[corner]).dot(p_normal) <
                        0.0F;
                indices.push_back(first);
                indices.push_back(first + (flipped ? corner + 1 : corner));
                indices.push_back(first + (flipped ? corner : corner + 1));
            }
        };
        Vector<Vector3> left_end;
        Vector<Vector3> right_end;
        for (int index = 0; index < section.size(); index++) {
            const Vector2 from = section[index];
            const Vector2 to = section[(index + 1) % section.size()];
            // the section runs from the top's left edge round to the bottom's: out is the edge turned
            // towards +y on the top
            const Vector2 outward = Vector2(-(to.y - from.y), to.x - from.x).normalized();
            add_face(
                    {Vector3(-half_length, from.y, from.x), Vector3(half_length, from.y, from.x),
                     Vector3(half_length, to.y, to.x), Vector3(-half_length, to.y, to.x)},
                    Vector3(0.0F, outward.y, outward.x));
            left_end.push_back(Vector3(-half_length, from.y, from.x));
            right_end.push_back(Vector3(half_length, from.y, from.x));
        }
        add_face(left_end, Vector3(-1, 0, 0));
        add_face(right_end, Vector3(1, 0, 0));
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = vertices;
        arrays[Mesh::ARRAY_NORMAL] = normals;
        arrays[Mesh::ARRAY_INDEX] = indices;
        Ref<ArrayMesh> mesh;
        mesh.instantiate();
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
        return mesh;
    }

    /// A sleeper every sleeper_spacing along the curve, laid across it under the rails' feet - DoS's
    /// (libmaszyna 655cdcda track_rendering_server.gd _create_sleeper_chunks()) - half a spacing in from
    /// either end: his from the start to the end put two sleepers at every joint of two tracks
    Vector<Transform3D> StandardTrackRenderingBackend::_place_sleepers(
            const Ref<Curve3D> &p_curve, const Ref<TrackCurve> &p_track_curve, const TrackData &p_description) {
        Vector<Transform3D> sleepers;
        const double spacing = p_description.get_sleeper_spacing();
        const double length = p_curve->get_baked_length();
        if (p_description.get_bed() == TrackData::BED_SLAB || p_description.get_bed() == TrackData::BED_EMBEDDED ||
            spacing <= CMP_EPSILON || length <= CMP_EPSILON) {
            return sleepers;
        }
        const int count = static_cast<int>(Math::floor((length / spacing) + 0.5));
        for (int index = 0; index < count; index++) {
            const Frame frame =
                    _frame_at(p_curve, p_track_curve, MIN((static_cast<double>(index) + 0.5) * spacing, length));
            sleepers.push_back(Transform3D(Basis(frame.side, frame.up, frame.forward), frame.origin));
        }
        return sleepers;
    }

    /// The sleepers in MultiMesh chunks drawn only near the camera, as DoS's (655cdcda)
    void StandardTrackRenderingBackend::_add_sleepers(
            Track &p_track, const Ref<Curve3D> &p_curve, const Ref<TrackCurve> &p_track_curve,
            const TrackData &p_description) const {
        const Vector<Transform3D> sleepers = _place_sleepers(p_curve, p_track_curve, p_description);
        if (sleepers.is_empty()) {
            return;
        }
        const TrackData::SleeperKind kind = p_description.get_sleeper_kind();
        Ref<Mesh> mesh = look.is_valid() ? look->sleeper_get_mesh(kind) : Ref<Mesh>();
        if (mesh.is_null()) {
            mesh = base_sleepers[kind];
        }
        const Ref<Material> material = look.is_valid() ? look->sleeper_get_material(kind) : Ref<Material>();
        const double spacing = p_description.get_sleeper_spacing();
        const int per_chunk = MAX(1, static_cast<int>(Math::floor(MAX(SLEEPER_CHUNK_LENGTH, spacing) / spacing)));
        RenderingServer *rendering = RenderingServer::get_singleton();
        for (int first = 0; first < sleepers.size(); first += per_chunk) {
            const int count = MIN(per_chunk, static_cast<int>(sleepers.size()) - first);
            const RID multimesh = rendering->multimesh_create();
            rendering->multimesh_set_mesh(multimesh, mesh->get_rid());
            rendering->multimesh_allocate_data(multimesh, count, RenderingServer::MULTIMESH_TRANSFORM_3D);
            for (int index = 0; index < count; index++) {
                rendering->multimesh_instance_set_transform(multimesh, index, sleepers[first + index]);
            }
            const RID instance = rendering->instance_create2(multimesh, p_track.scenario);
            rendering->instance_geometry_set_visibility_range(
                    instance, 0.0F, static_cast<float>(SLEEPERS_DISTANCE), static_cast<float>(SLEEPERS_BEGIN_MARGIN),
                    static_cast<float>(SLEEPERS_END_MARGIN), RenderingServer::VISIBILITY_RANGE_FADE_SELF);
            if (material.is_valid()) {
                rendering->instance_geometry_set_material_override(instance, material->get_rid());
            }
            rendering->instance_set_visible(instance, p_track.visible);
            p_track.sleeper_multimeshes.push_back(multimesh);
            p_track.sleeper_instances.push_back(instance);
        }
    }

    /// The two rails on the bed of the track's kind over its formation; p_bed_drop lowers the bed of
    /// a switch's diverging curve under the common one's
    void StandardTrackRenderingBackend::_add_rail_track(
            const Vector<Frame> &p_frames, const TrackData &p_description, const double p_bed_drop,
            Vector<Surface> &p_surfaces) const {
        const double rail_height = TrackServer::get_instance()->get_rail_height();
        const double half_gauge = 0.5 * p_description.get_width();
        const real_t rail_scale = static_cast<real_t>(rail_height / RAIL_PROFILE_HEIGHT);
        const Ref<Material> rail_material = look.is_valid() ? look->get_rail_material() : Ref<Material>();
        for (const double centre: {-half_gauge, half_gauge}) {
            PackedVector2Array rail;
            for (const auto &point: RAIL_PROFILE) {
                // the rails stand on the curve, their heads rail_height over it (Track.cpp:555)
                rail.push_back(
                        Vector2(static_cast<real_t>(centre) + point[0],
                                (point[1] * rail_scale) + static_cast<real_t>(rail_height)));
            }
            _sweep(p_frames, rail, rail_material, p_surfaces);
        }

        // the bed and the formation reach BED_JOINT_OVERLAP past the curve's ends, under the next
        // track's: end to end their sections left a seam between the tracks
        Vector<Frame> bed_frames = p_frames;
        bed_frames.write[0].origin -= bed_frames[0].forward * static_cast<real_t>(BED_JOINT_OVERLAP);
        const int last = static_cast<int>(bed_frames.size()) - 1;
        bed_frames.write[last].origin += bed_frames[last].forward * static_cast<real_t>(BED_JOINT_OVERLAP);

        // the bed: its foot is where the formation begins
        const TrackData::Bed bed = p_description.get_bed();
        const Ref<Material> bed_material = look.is_valid() ? look->bed_get_material(bed) : Ref<Material>();
        // the rails' feet on the sleepers' tops, on the curve
        const double sleeper_top = -p_bed_drop;
        double foot_half_width = 0.0;
        double foot = 0.0;
        PackedVector2Array section;
        switch (bed) {
            case TrackData::BED_SLAB:
                foot_half_width = SLAB_HALF_WIDTH;
                foot = sleeper_top - SLAB_THICKNESS;
                section.push_back(Vector2(static_cast<real_t>(-foot_half_width), static_cast<real_t>(foot)));
                section.push_back(Vector2(static_cast<real_t>(-foot_half_width), static_cast<real_t>(sleeper_top)));
                section.push_back(Vector2(static_cast<real_t>(foot_half_width), static_cast<real_t>(sleeper_top)));
                section.push_back(Vector2(static_cast<real_t>(foot_half_width), static_cast<real_t>(foot)));
                break;
            case TrackData::BED_EMBEDDED:
                // the street is level with the rails' heads, and no formation shows under it
                foot_half_width = half_gauge + EMBEDDED_MARGIN;
                section.push_back(
                        Vector2(static_cast<real_t>(-foot_half_width), static_cast<real_t>(rail_height - p_bed_drop)));
                section.push_back(
                        Vector2(static_cast<real_t>(foot_half_width), static_cast<real_t>(rail_height - p_bed_drop)));
                _sweep(bed_frames, section, bed_material, p_surfaces);
                return;
            default: {
                const double crest = sleeper_top - BALLAST_CREST_DROP;
                const double crest_half_width = (0.5 * SLEEPER_LENGTH) + BALLAST_SHOULDER;
                foot = sleeper_top - SLEEPER_HEIGHT - BALLAST_DEPTH;
                foot_half_width = crest_half_width + ((crest - foot) * BALLAST_SLOPE);
                section.push_back(Vector2(static_cast<real_t>(-foot_half_width), static_cast<real_t>(foot)));
                section.push_back(Vector2(static_cast<real_t>(-crest_half_width), static_cast<real_t>(crest)));
                section.push_back(Vector2(static_cast<real_t>(crest_half_width), static_cast<real_t>(crest)));
                section.push_back(Vector2(static_cast<real_t>(foot_half_width), static_cast<real_t>(foot)));
                break;
            }
        }
        _sweep(bed_frames, section, bed_material, p_surfaces);

        // the formation: its crown out from the bed's foot, then its slopes down (a bank) or up (a
        // cutting) by its height
        const double crown_edge = foot_half_width + p_description.get_formation_width();
        const double slope_edge =
                crown_edge + (Math::abs(p_description.get_formation_height()) * p_description.get_formation_slope());
        const double slope_foot = foot - p_description.get_formation_height();
        if (Math::is_zero_approx(slope_edge - foot_half_width)) {
            return;
        }
        const Ref<Material> formation_material = look.is_valid() ? look->get_formation_material() : Ref<Material>();
        for (const real_t side: {-1.0F, 1.0F}) {
            PackedVector2Array formation;
            // from the bed's foot outward, so the section's normals face up on either side
            const Vector2 points[] = {
                    Vector2(side * static_cast<real_t>(foot_half_width), static_cast<real_t>(foot)),
                    Vector2(side * static_cast<real_t>(crown_edge), static_cast<real_t>(foot)),
                    Vector2(side * static_cast<real_t>(slope_edge), static_cast<real_t>(slope_foot)),
            };
            for (const Vector2 &point: points) {
                if (side > 0.0F) {
                    formation.push_back(point);
                } else {
                    formation.insert(0, point);
                }
            }
            _sweep(bed_frames, formation, formation_material, p_surfaces);
        }
    }

    /// The carriageway of the road's surface, and its shoulders a little under it where it has them
    void StandardTrackRenderingBackend::_add_road(
            const Vector<Frame> &p_frames, const TrackData &p_description, Vector<Surface> &p_surfaces) const {
        const real_t half_width = static_cast<real_t>(0.5 * p_description.get_width());
        PackedVector2Array carriageway;
        carriageway.push_back(Vector2(-half_width, 0.0F));
        carriageway.push_back(Vector2(half_width, 0.0F));
        _sweep(p_frames, carriageway,
               look.is_valid() ? look->surface_get_material(p_description.get_surface()) : Ref<Material>(), p_surfaces);
        if (!p_description.get_shoulder()) {
            return;
        }
        const real_t outer = half_width + static_cast<real_t>(SHOULDER_WIDTH);
        const real_t drop = -static_cast<real_t>(SHOULDER_DROP);
        const Ref<Material> shoulder_material = look.is_valid() ? look->get_shoulder_material() : Ref<Material>();
        PackedVector2Array left;
        left.push_back(Vector2(-outer, drop));
        left.push_back(Vector2(-half_width, 0.0F));
        _sweep(p_frames, left, shoulder_material, p_surfaces);
        PackedVector2Array right;
        right.push_back(Vector2(half_width, 0.0F));
        right.push_back(Vector2(outer, drop));
        _sweep(p_frames, right, shoulder_material, p_surfaces);
    }
} // namespace godot
