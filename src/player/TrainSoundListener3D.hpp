#pragma once
#include <godot_cpp/classes/audio_listener3d.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    class Cabin3D;
    class Camera3D;

    /* Where the vehicles are heard from: the current camera, and the cab it is in. Whatever plays
     * the vehicles' sounds finds it as it enters the tree. */
    class TrainSoundListener3D : public AudioListener3D {
            GDCLASS(TrainSoundListener3D, AudioListener3D)

        public:
            /* The context outside every cab - the last column of a soundproofing profile */
            static constexpr int EXTERIOR_CONTEXT = 5;
            static const char *context_changed_signal;

        private:
            /* The vehicle whose cab the listener is in (VehicleServer's), none outside */
            RID listener_vehicle;
            ObjectID listener_cabin;
            int listener_context = EXTERIOR_CONTEXT;

            void _refresh_context(Camera3D *p_camera);

        protected:
            static void _bind_methods();
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _notification(int p_what);

        public:
            RID get_listener_vehicle() const;
            Cabin3D *get_listener_cabin() const;
            int get_listener_context() const;
            bool is_inside_vehicle(const RID &p_vehicle) const;
    };
} // namespace godot
