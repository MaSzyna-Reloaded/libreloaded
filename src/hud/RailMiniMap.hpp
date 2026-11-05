#pragma once

#include "utils/RIDHash.hpp"

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>

namespace godot {
    class InputEvent;

    /* A map of the rails around the camera, seen from above: the tracks, the switches (a click
     * throws one) and the rail vehicles with a crew or a cabin to drive from, each named - a
     * Control, so it stands in a HUD window or anywhere in a HUD. The tracks' lines are taken from
     * TrackServer once, in world space, and drawn in one batch through the view's transform; only
     * the tracks around the view are asked for, again when the view leaves them or the tracks
     * change. It draws nothing and processes nothing while hidden. */
    class RailMiniMap : public Control {
            GDCLASS(RailMiniMap, Control)

        private:
            /* A track's lines in world space (x, z), its curves sampled once */
            struct TrackLines {
                    PackedVector2Array common;
                    PackedVector2Array diverging;
                    bool is_switch = false;
                    RID switch_rid;
                    // where both curves of a switch part - its handle
                    Vector2 switch_point;
                    bool has_switch_point = false;
                    String name;
                    Vector2 label_point;
            };
            /* A switch handle of the tracks around the view */
            struct SwitchHandle {
                    RID switch_rid;
                    Vector2 position;
            };
            /* A track name of the tracks around the view */
            struct TrackLabel {
                    String name;
                    Vector2 position;
            };

            static void _bind_methods();

            Color track_color = Color(1.0, 1.0, 1.0);
            Color switch_active_color = Color(1.0, 1.0, 0.0);
            Color label_color = Color(1.0, 1.0, 0.0);
            Color vehicle_color = Color(0.0, 1.0, 0.0);
            Color background_color = Color(0.0, 0.0, 0.0, 0.5);
            double zoom = 1.0;
            bool follow_camera_rotation = true;

            // the tracks around the view, their lines, and the area (world x, z) they were asked for
            RIDMap<TrackLines> track_lines;
            Rect2 query_rect;
            // what is drawn of them: line segments by colour, the switch handles and the names
            PackedVector2Array track_segments;
            PackedVector2Array active_segments;
            LocalVector<SwitchHandle> switch_handles;
            LocalVector<TrackLabel> track_labels;
            // the camera the view was last drawn from, and how long ago the vehicles were drawn
            Vector2 camera_position;
            real_t camera_rotation = 0.0;
            double vehicles_drawn_ago = 0.0;
            // the switch handle under the mouse, -1 for none
            int hovered_switch = -1;

            Transform2D _get_view_transform() const;
            int _find_switch_handle(const Vector2 &p_view_position) const;
            void _build_batches();
            void _on_tracks_changed();
            void _on_switch_active_track_changed(const RID &p_switch, int p_active_track);

        protected:
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _notification(int p_what);

        public:
            void _gui_input(const Ref<InputEvent> &p_event) override;
            String _get_tooltip(const Vector2 &p_at_position) const override;

            void zoom_in();
            void zoom_out();
            void zoom_reset();

            void set_track_color(const Color &p_value);
            Color get_track_color() const;
            void set_switch_active_color(const Color &p_value);
            Color get_switch_active_color() const;
            void set_label_color(const Color &p_value);
            Color get_label_color() const;
            void set_vehicle_color(const Color &p_value);
            Color get_vehicle_color() const;
            void set_background_color(const Color &p_value);
            Color get_background_color() const;
            void set_zoom(double p_value);
            double get_zoom() const;
            void set_follow_camera_rotation(bool p_value);
            bool get_follow_camera_rotation() const;
    };
} // namespace godot
