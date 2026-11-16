#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
    /* A component a vehicle's implementation can make (VehicleImplementationServer::
     * component_list()): the name the implementation makes it by (component_create()), its
     * title for a list, and its kind - a VehicleComponentType, or a kind of vehicle's own numbered
     * on from it (RailVehicleComponentType), as the component answers
     * (VehicleComponent::get_component_type()) */
    class VehicleComponentInfo : public RefCounted {
            GDCLASS(VehicleComponentInfo, RefCounted)

        private:
            StringName name;
            String title;
            int component_type = 0;

        protected:
            static void _bind_methods();

        public:
            void set_name(const StringName &p_value);
            StringName get_name() const;
            void set_title(const String &p_value);
            String get_title() const;
            void set_component_type(int p_value);
            int get_component_type() const;
    };
} // namespace godot
