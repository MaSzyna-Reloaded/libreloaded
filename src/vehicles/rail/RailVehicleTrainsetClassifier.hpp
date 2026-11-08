#pragma once
#include "vehicles/rail/RailVehicleServer.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot {
    /* What a trainset is, from its vehicles - the rule RailVehicleServer recomputes the type of
     * every trainset with whenever its vehicles change (trainset_get_type()). This one is the
     * core's: a wagon (Power < 1) with R or without G carries passengers, one with G and without R
     * goods, both kinds make a mixed trainset and no wagon at all none. A plugin hands the server
     * its own rule (RailVehicleServer::trainset_set_classifier()): a C++ one overrides classify(),
     * a script one _classify(). */
    class RailVehicleTrainsetClassifier : public RefCounted {
            GDCLASS(RailVehicleTrainsetClassifier, RefCounted)

        protected:
            static void _bind_methods();

            GDVIRTUAL1RC(RailVehicleServer::TrainsetType, _classify, TypedArray<RID>)

        public:
            /* A wagon is a vehicle of less power than this (Power < 1, Driver.cpp:2156) */
            static constexpr double WAGON_POWER_MAX = 1.0;

            /* The type of a trainset of p_vehicles, its head first - the script's rule when it has
             * one, else the core's */
            virtual RailVehicleServer::TrainsetType classify(const TypedArray<RID> &p_vehicles) const;
    };
} // namespace godot
