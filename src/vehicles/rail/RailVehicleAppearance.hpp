#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    /* What a kind of rail vehicle looks like: the models it is drawn with and the submodels of its
     * exterior model the running gear turns - bogies and wheels. Shared by every vehicle of the kind
     * and handed to RailVehicleRenderingServer, which builds the vehicle from it; a plugin's
     * appearance derives from it with what its own renderer animates. Submodels are named as the
     * model names them; a missing one is an empty name. */
    class RailVehicleAppearance : public Resource {
            GDCLASS(RailVehicleAppearance, Resource)

        public:
            /* The submodel a head display material is drawn on, for a vehicle put together by hand -
             * the wrapper's own name; a vehicle of the game data takes the submodel with replaceable
             * skin 4, as the original (MaszynaRailVehicle3DInstancer, DynObj.cpp:2539-2545) */
            static constexpr const char *DEFAULT_HEAD_DISPLAY_SUBMODEL = "tablice_relacyjne";

        private:
            String data_path;
            String model_filename;
            String low_poly_model_filename;
            String passengers_model_filename;
            PackedStringArray attachment_model_filenames;
            PackedStringArray skins;
            Transform3D model_transform;
            String front_bogie;
            String rear_bogie;
            PackedStringArray front_rolling_wheels;
            PackedStringArray powered_wheels;
            PackedStringArray rear_rolling_wheels;
            String head_display_submodel = DEFAULT_HEAD_DISPLAY_SUBMODEL;

        protected:
            static void _bind_methods();

        public:
            /* Where the model files are, and the exterior, the low-poly interior seen through the
             * windows and the passengers; skins by number of the model's replaceable materials */
            void set_data_path(const String &p_value);
            String get_data_path() const;
            void set_model_filename(const String &p_value);
            String get_model_filename() const;
            void set_low_poly_model_filename(const String &p_value);
            String get_low_poly_model_filename() const;
            void set_passengers_model_filename(const String &p_value);
            String get_passengers_model_filename() const;
            /* Models drawn with the exterior, in its frame and with its skins (attachments:,
             * DynObj.cpp:5384) */
            void set_attachment_model_filenames(const PackedStringArray &p_value);
            PackedStringArray get_attachment_model_filenames() const;
            void set_skins(const PackedStringArray &p_value);
            PackedStringArray get_skins() const;
            /* Where every model of the vehicle sits in the vehicle's own frame */
            void set_model_transform(const Transform3D &p_value);
            Transform3D get_model_transform() const;
            void set_front_bogie(const String &p_value);
            String get_front_bogie() const;
            void set_rear_bogie(const String &p_value);
            String get_rear_bogie() const;
            void set_front_rolling_wheels(const PackedStringArray &p_value);
            PackedStringArray get_front_rolling_wheels() const;
            void set_powered_wheels(const PackedStringArray &p_value);
            PackedStringArray get_powered_wheels() const;
            void set_rear_rolling_wheels(const PackedStringArray &p_value);
            PackedStringArray get_rear_rolling_wheels() const;
            void set_head_display_submodel(const String &p_value);
            String get_head_display_submodel() const;
    };
} // namespace godot
