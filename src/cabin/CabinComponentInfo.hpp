#pragma once
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
    /* A control a cab's implementation can make (CabinImplementation::component_list()): the name
     * the implementation makes it by (component_create()), its title for a list, the class of the
     * cab widget it is, and the vehicle commands it sends */
    class CabinComponentInfo : public RefCounted {
            GDCLASS(CabinComponentInfo, RefCounted)

        private:
            StringName name;
            String title;
            StringName widget_class;
            PackedStringArray commands;

        protected:
            static void _bind_methods();

        public:
            void set_name(const StringName &p_value);
            StringName get_name() const;
            void set_title(const String &p_value);
            String get_title() const;
            void set_widget_class(const StringName &p_value);
            StringName get_widget_class() const;
            void set_commands(const PackedStringArray &p_value);
            PackedStringArray get_commands() const;
    };
} // namespace godot
