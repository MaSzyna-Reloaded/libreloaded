#include "CabinImplementation.hpp"

namespace godot {
    void CabinImplementation::_bind_methods() {
        GDVIRTUAL_BIND(_component_list);
        GDVIRTUAL_BIND(_component_create, "name");
    }

    TypedArray<CabinComponentInfo> CabinImplementation::component_list() const {
        TypedArray<CabinComponentInfo> result;
        GDVIRTUAL_CALL(_component_list, result);
        return result;
    }

    Node3D *CabinImplementation::component_create(const StringName &p_name) const {
        Node3D *result = nullptr;
        GDVIRTUAL_CALL(_component_create, p_name, result);
        return result;
    }
} // namespace godot
