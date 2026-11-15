#pragma once
#include "cabin/CabinComponentInfo.hpp"
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* What builds a kind of cab - registered with CabinSystem under the name its cabs give
     * (Cabin3D::implementation). Implement it in C++ by overriding the virtual methods, or in
     * GDScript by overriding their script counterparts. */
    class CabinImplementation : public Resource {
            GDCLASS(CabinImplementation, Resource)

        protected:
            static void _bind_methods();

            GDVIRTUAL0RC(TypedArray<CabinComponentInfo>, _component_list)
            GDVIRTUAL1RC(Node3D *, _component_create, StringName)

        public:
            /// The controls it can make, each once
            virtual TypedArray<CabinComponentInfo> component_list() const;
            /// A control of a name component_list() gives, with its defaults; null for another
            virtual Node3D *component_create(const StringName &p_name) const;
    };
} // namespace godot
