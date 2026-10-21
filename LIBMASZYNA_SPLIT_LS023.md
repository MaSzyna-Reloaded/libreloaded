# LS-023 - every `RailVehicle*` property classified: fact or Mover model

Scope: `LIBMASZYNA_SPLIT.md`, LS-023 (#316, "Cutting the layers: configuration"). The table lists
every property bound by the core's `RailVehicle<X>` component and list-item classes
(`src/vehicles/rail/`). Each row gives its readers outside its own class and a proposal:

* `stays` - a fact about the vehicle, kept in the core interface;
* `down` - a description of the Mover's model, which goes to the `MaszynaMover<X>` adapter;
* `plugin-data` - MaSzyna data that only a MaSzyna layer needs;
* `unclear` - the code alone does not decide it;
* `ALARM` - Mover model description that the core reads.

**Accepted by the operator, 2026-10-19:**

* the 203 `down` rows go down;
* of the 19 `unclear` rows, the 10 read only by the Mover's adapter go down (`brake_method`,
  `mechanical_inertia`, `mechanical_min_rpm_hydro_drive`, `mechanical_min_rpm_retarder`,
  `torque_converter_table`, `retarder_placement`, `throttle_table_resistance_torque`,
  `motor_blowers_speed`, `cntrl_converter_overload_relay_off_when_main_is_off`,
  `override_manual_power`);
* `pantograph_preset_default`, `platform_max_speed`, `load_list` and its item's `max_load`,
  `load_type`, `load_offset` stay, unused, in `TODO.md`;
* `instrument_type` and `selector_position` stay untouched, in `TODO.md`;
* `handle_step` leaves the brake for the player's setting `brakestep` in `UserSettings` (the
  original's `eu07.ini`, `Globals.cpp:656-658`, used by `Train.cpp:1965,1973`);
* done 2026-10-19: the `down` rows are in the `MaszynaMover<X>` adapters, the engines' shared ones in
  `MaszynaMoverEngineConfiguration`, `MaszynaMoverElectricEngineConfiguration` and
  `MaszynaMoverDieselConfiguration` (resources the engine adapters hold); the FIZ tables are
  `MaszynaMover*Item`s; the selectors `BrakeMethod`, `BrakeHandleType`, `TrainBrakeValve`,
  `EimControlType` and `RetarderPlacement` went with their rows. `StartMode`, `CompressorPower`,
  `BrakeDelaySetting`, `BrakeOperationMode`, `DynamicBrakeType`, `AutoRelayMode`, `PantographType`,
  `HandlePosition` and `EngineType` stay - their properties stay;
* the ALARM `train_type` goes down to `MaszynaMoverRailVehicleController`; the core gets two facts
  the FIZ factory sets from `Type=`: the coupling that bounds a unit (on the controller) and the
  pantographs' lowest rising pressure (on the power source).

Measured on `dev/separate-libmaszyna` at 81b6d25, 2026-10-19. Readers were found by grep and each
one was checked by hand. "adapter" is the property's `MaszynaMover*` class (or, for
`RailVehicleEngine`/`RailVehicleElectricEngine`, the Mover drive and circuit units). "fiz" is the
FIZ parser in `plugins/libmaszyna/addons/libmaszyna/fiz/`, "mmd" is the MMD code.

## Summary

| Proposal | Properties |
|---|---|
| stays | 331 |
| down | 203 |
| unclear | 19 |
| plugin-data | 1 |
| ALARM | 1 |
| **total** | **555** |

Points for the operator:

* **ALARM `RailVehicleController.train_type`.** This is the Mover's `TrainType`. The core reads it
  in `RailVehicleServer.cpp:867-869`, to search the powered car over permanent couplings, and in
  `:2045`, for the EMU pantograph pressure. Both places need only the fact "a multiple unit,
  electric or diesel". Option: that fact in the core, the selector in the adapter.
* **No class to move to yet.** `RailVehicleEngine` and `RailVehicleElectricEngine` have no
  `MaszynaMover*` adapter. Their `down` rows need a home designed first: the drive and circuit
  units, or a Mover engine base.
* **Enums of the Mover's model in the core headers** (LS-024):
  * used only by the plugin: `TrainBrakeValve`, `BrakeHandleType`, `BrakeMethod`,
    `RetarderPlacement`, `ThrottlePositionItem::ClutchBehavior`, `EimControlType`;
  * reaching core API or defaults: `StartMode` (the declared type in several core headers),
    `EngineType` (`MAIN` is a default in the core's lighting and heating), `ValveOperation` (a
    command argument).
* **Judgement calls** (marked in the rows):
  * buffer and coupler stiffness, stroke and maximum force are classed `stays`, although `SpringKB`
    also serves as a damping percentage;
  * the master controller's step delays are classed `stays`.
* **Found on the way, outside LS-023** (in `TODO.md`):
  * the FIZ `AirLeakRate` is scaled by 0.01 twice (`fiz_train_brake_parser.gd:78`,
    `MaszynaMoverRailVehicleBrake.cpp:688`);
  * config overwrites live Mover state on every reapply (`torque_converter_lockup_rate`,
    `mechanical_shunt_mode_ratio`);
  * properties that nothing reads, or that the Mover ignores;
  * `*_rpm` properties that hold revolutions per second.

## Configuration split: brake, spring brake, EP/ED brake, power supply, heating

Scope: every property bound in `_bind_methods()` of `src/vehicles/rail/RailVehicle{Brake,BrakePressureTableItem,CompressorListItem,SpringBrake,ElectroPneumaticDynamicBrake,PowerSupply,Heating}.cpp`. Readers were checked by `get_/set_<name>`, `"<name>"`, `.<name>` and by the adapter's config-dump keys.

"adapter" = `MaszynaMoverRailVehicle<X>.cpp` (`_apply_configuration` / `_fill_config_dictionary`). "fiz" = the FIZ parser under `plugins/libmaszyna/addons/libmaszyna/fiz/` that writes the property. A dump key is listed when the adapter publishes the value in `_fill_config_dictionary`. "(unread)" means nothing reads that key.

False positives in the starting JSON that were dropped: `brake_force_max` -> `macros.hpp` (a doc comment), `cntrl_battery_start_mode` -> `VehicleController.hpp:177` (a comment), `allow` -> `RailVehicleInverter`/`RailVehicleController.hpp` (the inverter's own `allow`), `speed_factor` -> `RailVehicleServer.cpp` (a pantograph function parameter), `pipe_pressure`/`handle_position` -> the brake's live getters `get_pipe_pressure()`/`get_handle_position(HandlePosition)`, not the table item fields.

### RailVehicleBrake

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `valve_type` | - | adapter (dump `brake_valve_type`, unread), fiz: fiz_train_brake_parser.gd | down | Picks the Mover's distributor model (TBrakeValve) and, through it, its BrakeSubsystem |
| `valve_parameters` | - | adapter, fiz: fiz_train_brake_parser.gd | down | Raw FIZ `BrakeValve=` string; the Mover's TNESt3::SetSize() reads relay options out of it |
| `friction_elements_per_axle` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | NBpA: brake blocks per axle, a count of equipment |
| `brake_force_max` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MBF: the brake's maximum force, a rating |
| `est_valve_size` | - | adapter, fiz: fiz_train_brake_parser.gd | down | `Size=` of the Mover's ESt valve model (BrakeValveSize) |
| `brake_force_traction` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | TBF: magnetic track brake force [kN]; a rating, and it shows the track brake exists |
| `max_cylinder_pressure` | - | adapter (dump `brake_max_cylinder_pressure`, unread), fiz, sound: brake_sound_model.gd | stays | MaxBP: maximum cylinder pressure, a limit; sound scales the brake hiss by it |
| `max_aux_pressure` | - | adapter (dump `brake_max_control_pressure`, unread), fiz, sound: brake_sound_model.gd | stays | MaxLBP: maximum independent-brake pressure, a limit; sound uses it as the control pressure range |
| `max_tare_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | TareMaxBP: maximum cylinder pressure of the empty vehicle, a load-brake limit |
| `max_medium_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MedMaxBP: maximum cylinder pressure at medium load, a load-brake limit |
| `max_antislip_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MaxASBP: maximum anti-skid brake pressure, a limit |
| `cylinder_count` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | BCN: number of brake cylinders, a count |
| `cylinder_radius` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | BCR: cylinder radius, a dimension (gives the brake volume and the force per bar) |
| `cylinder_distance` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | BCD: piston stroke, a dimension |
| `cylinder_spring_force` | - | adapter, fiz: fiz_train_brake_parser.gd | down | BCS: return-spring term of the Mover's cylinder-force equation; the core already exposes the resulting force through `get_force_at()` |
| `piston_stroke_adjuster_resistance` | - | adapter, fiz: fiz_train_brake_parser.gd | down | BSA: slack-adjuster term of the Mover's cylinder-force equation |
| `cylinder_gear_ratio` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | BCM: brake rigging lever ratio, a dimension of the rig |
| `cylinder_gear_ratio_low` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | BCMlo: lever ratio of the empty setting (also decides the Mover's LoadFlag) |
| `cylinder_gear_ratio_high` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | BCMHi: lever ratio of the loaded setting |
| `pipe_pressure_min` | - | adapter (only as derived `brake_pipe_pressure_delta`, unread), fiz | stays | LoPP: lowest brake-pipe pressure of the working range (full service) |
| `pipe_pressure_max` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | HiPP: nominal brake-pipe pressure, the running pressure a driver and the AI know |
| `tank_volume_main` | - | adapter (dump `brake_main_reservoir_volume` comes from the Mover's VeselVolume, unread), fiz, ai: maszyna_auto_rewident.gd | stays | Vv: main reservoir volume; the AI skips the feed-pipe wait when it is 0 (no reservoir) |
| `tank_volume_aux` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | BVV: auxiliary reservoir volume, a dimension |
| `compressor_cab_a_min_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MinCP: compressor governor cut-in pressure, a limit |
| `compressor_cab_a_max_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MaxCP: compressor governor cut-out pressure, a limit |
| `compressor_cab_b_min_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MinCP_B: cut-in pressure when driven from cab B (cab-dependent governor) |
| `compressor_cab_b_max_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MaxCP_B: cut-out pressure when driven from cab B |
| `compressor_speed` | - | adapter, fiz, cabin: python_screen_state.gd | stays | Compressor capacity rating; the cab screen counts the vehicle as having a compressor when it is above a threshold |
| `compressor_power` | - | adapter, fiz: fiz_train_brake_parser.gd, fiz: fiz_vehicle_builder.gd | stays | What feeds the compressor (main circuit, converter, engine, couplers), which equipment is wired to what |
| `rig_effectiveness` | - | adapter, fiz: fiz_train_brake_parser.gd | down | BRE: efficiency coefficient of the Mover's brake-force equation |
| `brake_method` | - | adapter, fiz: fiz_train_brake_parser.gd | unclear | BM: the friction material (block/composite/disc) is a fact, but the enum's values (P10-Bg/Bgu, P10y, FR510/513, Cosid, Disk1+Mg) are the Mover's friction-curve ids. Needs a decision: either a coarse material fact stays in the core and the curve id goes down, or the whole enum goes down |
| `rapid_transfer` | - | adapter, fiz: fiz_train_brake_parser.gd | down | RM: RapidMult, a multiplier in the Mover's force equation for the R setting |
| `rapid_switching_speed` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | RV: speed at which the R-setting speed relay switches, a limit |
| `air_leak_multiplier` | - | adapter, fiz: fiz_train_brake_parser.gd | down | AirLeakRate: tuning for the Mover's "fake air leaks" (Mover.cpp:3833, 3869, 3999) |
| `compressor_tank_valve_active` | - | adapter, fiz: fiz_train_brake_parser.gd | down | CompressorTankValve: how the Mover's compressor routine vents overpressure (MOVER.h:1533, Mover.cpp:3966) |
| `compressor_lower_emergency_closing_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MinEVP: closing pressure of the main reservoir safety valve, a limit |
| `compressor_higher_emergency_closing_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MaxEVP: opening pressure of the main reservoir safety valve, a limit |
| `main_pipe_blocking_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | LPOn: main reservoir pressure below which the brake pipe is locked off, a limit (the cab lamp shows it) |
| `main_pipe_unblocking_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | LPOff: pressure at which the lock releases, a limit |
| `main_pipe_minimum_unblocking_handle_position` | - | adapter, fiz: fiz_train_brake_parser.gd | down | HandlePipeUnlockPos: a raw Mover handle position (BrakeCtrlPos units, -3 sentinel) whose meaning depends on the Mover handle model |
| `main_pipe_emergency_cuts_off_handle` | - | adapter (stores the value itself), fiz: fiz_train_brake_parser.gd | stays | EmergencyCutsOffHandle: emergency braking cuts the driver's valve off, an interlock of the vehicle |
| `brake_pressure_table` | - | adapter, fiz: fiz_train_brake_parser.gd | down | BPT: the FIZ-shaped handle-position-to-pressure table for the Mover's EP/pneumatic handle model |
| `compressor_list` | - | adapter, fiz: fiz_train_brake_parser.gd | down | CompressorList: the FIZ-shaped compressor programmer table (factors per position), maximum of 8 Mover rows |
| `compressor_emergency_valve_area` | - | adapter, fiz: fiz_train_brake_parser.gd | down | EVArea: safety valve flow area, an input to the Mover's flow equation |
| `releaser_enabled_only_at_no_power_pos` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | ReleaserPowerPosLock: the releaser works only with the controller at zero, an interlock of the cab |
| `universal_brake_button_1` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | UBB1: which functions the cab's universal button 1 has (releaser, emergency valve, ...) |
| `universal_brake_button_2` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | UBB2: the same for button 2 |
| `universal_brake_button_3` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | UBB3: the same for button 3 |
| `cntrl_brake_system` | - | adapter (dump `brake_system`, unread), fiz, sound: brake_sound_model.gd | stays | Individual/pneumatic/electro-pneumatic train brake, which equipment exists; sound reads it for the EP hiss |
| `cntrl_brake_ctrl_position_count` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | BCPN: number of positions of the train brake handle (0 = no handle; the adapter also gates ASB with it) |
| `cntrl_brake_delays` | RailVehicleServer.cpp:1441 | adapter (dump `brake_delays` from the Mover, unread), fiz, ai: maszyna_auto_rewident.gd | stays | Which brake settings (G/P/R/Mg) the distributor has; the core's trainset type and the AI's brake setting choice depend on it |
| `cntrl_brake_delay_1` | - | adapter (dump `brake_delay_times`, unread), fiz | stays | BDelay1: application time of the first delay setting [s] |
| `cntrl_brake_delay_2` | - | adapter (dump `brake_delay_times`, unread), fiz | stays | BDelay2: application time of the second delay setting [s] |
| `cntrl_brake_delay_3` | - | adapter (dump `brake_delay_times`, unread), fiz | stays | BDelay3: application time of the third delay setting [s] |
| `cntrl_brake_delay_4` | - | adapter (dump `brake_delay_times`, unread), fiz | stays | BDelay4: application time of the fourth delay setting [s] |
| `cntrl_max_brake_pressure_mass` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | MaxBPMass: mass at which the load brake reaches MaxBP [t] |
| `handle_movement` | - | adapter (dump `brake_handle_movement`), mmd: mmd_semantic_catalog.gd, fiz (derived from the handle type) | stays | A key moves the handle one position per press or while held; the cab's key binding reads it |
| `handle_step` | - | adapter | unclear | Positions per key press for `brake_level_increase/decrease` (the original's global `fBrakeStep`). This is neither a vehicle fact nor the Mover's model; it is input tuning. Needs a decision whether it belongs to the cab/input layer |
| `cntrl_brake_op_modes` | - | adapter (dump `brake_operation_mode_position_max` comes from the Mover, read by mmd: mmd_semantic_catalog.gd), fiz | stays | BrakeOpModes: which operating modes (PN/EP/MED) the brake has, which equipment exists |
| `cntrl_brake_handle_type` | - | adapter (dump `brake_handle_type`, unread), fiz, sound: brake_sound_model.gd | down | Picks the Mover's TDriverHandle model (FV4a, M394, MHZ_*...). Positions are already abstracted by `get_handle_position()`. Sound uses it for the FV4a/FVel6 hiss (plugin) |
| `cntrl_anti_skid_brake_type` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | ASB: whether an anti-skid brake exists and whether it is manual or automatic |
| `cntrl_local_brake_type` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | Kind of independent brake: none/manual/pneumatic/hydraulic, which equipment exists |
| `cntrl_local_brake_handle_type` | - | adapter (dumps `brake_local_handle_type` (unread) and `brake_local_handle_available` (unread)), fiz | down | Picks the Mover's handle model for the independent brake |
| `cntrl_manual_brake_present` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | The vehicle has a hand brake, which equipment exists |
| `cntrl_dynamic_brake_type` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | How the dynamic brake is engaged (passive/switch/reversal/automatic), which equipment exists and how it works |
| `cntrl_local_brake_traxx` | - | adapter, fiz: fiz_train_brake_parser.gd | down | LocalBrakeTraxx: time-based behaviour variant of the Mover's independent handle model (LocHandleTimeTraxx) |
| `cntrl_release_parking_by_spring_brake` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | Releasing the spring brake also releases the parking brake, vehicle logic |
| `cntrl_release_parking_by_spring_brake_when_door_open` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | The same, but with the doors open, vehicle logic |
| `cntrl_spring_brake_cuts_off_drive` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | An applied spring brake cuts traction, an interlock |
| `cntrl_spring_brake_drive_emergency_velocity` | - | adapter, fiz: fiz_train_brake_parser.gd | stays | Speed limit for driving against the spring brake, a limit |

### RailVehicleBrakePressureTableItem

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `handle_position` | - | adapter, fiz: fiz_train_brake_parser.gd | down | BPT row key: a raw Mover handle position |
| `pipe_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | down | BPT column: brake-pipe pressure at that position (Mover's TBrakePressure.PipePressureVal) |
| `brake_cylinder_pressure` | - | adapter, fiz: fiz_train_brake_parser.gd | down | BPT column: cylinder pressure (BrakePressureVal) |
| `fill_speed` | - | adapter, fiz: fiz_train_brake_parser.gd | down | BPT column: flow speed (FlowSpeedVal), a coefficient of the Mover's handle model |
| `brake_type` | - | adapter, fiz: fiz_train_brake_parser.gd | down | BPT column: row kind P/EP/individual (the Mover's TBrakePressure.BrakeType) |

### RailVehicleCompressorListItem

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `allow` | - | adapter, fiz: fiz_train_brake_parser.gd | down | CompressorList cl_Allow row: the compressor programmer's per-position enable, a FIZ-shaped table |
| `speed_factor` | - | adapter, fiz: fiz_train_brake_parser.gd | down | cl_SpeedFactor row: a Mover compressor-speed factor per position |
| `min_pressure_factor` | - | adapter, fiz: fiz_train_brake_parser.gd | down | cl_MinFactor row: a cut-in pressure factor per position |
| `max_pressure_factor` | - | adapter, fiz: fiz_train_brake_parser.gd | down | cl_MaxFactor row: a cut-out pressure factor per position |

### RailVehicleSpringBrake

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `spring_actuator_chamber_volume` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | `Volume`: the capacity of the Mover reservoir behind SpringBrake.Cylinder (CreateCap) |
| `spring_actuator_max_filling_force` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | `ResetP`: SpringBrake.ResetPressure of the Mover's spring model (the name says force but the value is a pressure) |
| `pressure_force_coefficient` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | `MBF`: SpringBrake.MaxBrakeForce, a coefficient of the Mover's spring force curve |
| `spring_actuator_preload_pressure` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | `MaxSP`: SpringBrake.MaxSetPressure, a point of the Mover's spring curve |
| `spring_full_balance_pressure` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | `MinFP`: SpringBrake.MinForcePressure, a point of the Mover's spring curve |
| `brake_signal_released_state_pressure` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | `PressOff`: threshold the Mover uses to derive "released" (the core exposes the result as `get_braking()`) |
| `brake_signal_braked_state_pressure` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | `PressOn`: threshold the Mover uses to derive "braking" |
| `valve_cross_section_actuator_discharge` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | FIZ `ValveOnArea` -> the Mover's ValveOffArea, a flow area of its valve equation |
| `valve_cross_section_actuator_charge` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | FIZ `ValveOffArea` -> the Mover's ValveOnArea, a flow area |
| `valve_cross_section_pneumatic_brake` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | `ValvePNBArea`: a flow area. Above 0 it also switches on the Mover's PNBrakeConnection |
| `required_coupler_connection_method` | - | adapter, fiz: fiz_train_spring_brake_parser.gd | down | `MTC`: a bitmask in the Mover's coupling-flag encoding (default 127) for which couplings carry the command |

### RailVehicleElectroPneumaticDynamicBrake

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `coupler_check` | - | adapter (also state `dcemued_coupler_check`, unread), fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `CouplerCheck`: DCEMUED_CC, which neighbour's ED brake the Mover's DC-EMU ED logic checks |
| `electro_pneumatic_min_regenerative_braking` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `EP_min_Im`: DCEMUED_EP_min_Im, the ED current threshold of the Mover's EP/ED hand-over |
| `electro_pneumatic_max_ep_brake_engagement_speed` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `EP_max_Vel`: DCEMUED_EP_max_Vel, the speed threshold of the same Mover logic |
| `electro_pneumatic_brake_delay` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `EP_delay`: DCEMUED_EP_delay, a timer of the same Mover logic |
| `ep_brake_fuse` | - | adapter | stays | Initial position of the EP brake fuse switch (applied through EpFuseSwitch), a cab state with no backend in it. No FIZ key |
| `blending_max_velocity` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `MED_Vmax`: tuning of the Mover's EP+ED blending algorithm |
| `blending_min_velocity` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `MED_Vmin`: blending tuning |
| `blending_reference_velocity` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `MED_Vref`: blending tuning |
| `blending_max_deceleration` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `MED_amax`: blending tuning (the deceleration the blending aims at) |
| `blending_velocity_correction` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `MED_EPVC`: a switch inside the blending algorithm |
| `blending_load_correction` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `MED_Ncor`: a switch inside the blending algorithm |
| `blending_min_ed_brake_request` | - | adapter, fiz: fiz_train_electro_pneumatic_dynamic_brake_parser.gd | down | `MED_MinBrakeReqED`: blending tuning |

### RailVehiclePowerSupply

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `battery_voltage` | - | adapter, fiz: fiz_train_power_supply_parser.gd (through fiz_import_context.gd from fiz_train_lighting_parser.gd) | stays | LMaxVoltage: nominal battery voltage, a rating (0 = no battery, so the adapter disables BatteryStart). The cab screen reads the live state key, not this property |
| `cntrl_battery_start_mode` | - | adapter, fiz: fiz_train_power_supply_parser.gd | stays | Whether the battery is switched on by hand or automatically, the vehicle's start-up behaviour (the enum is the Mover's start_t, see Notes) |
| `cntrl_converter_start_mode` | - | adapter, fiz: fiz_train_power_supply_parser.gd | stays | How the converter starts (manual/automatic/with the battery...), start-up behaviour |
| `cntrl_converter_start_delay` | - | adapter, fiz: fiz_train_power_supply_parser.gd | stays | Converter start time [s], a fact a driver waits for (also exposed live as `converter_time_to_start`) |

### RailVehicleHeating

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `heating_source` | - | adapter, fiz: fiz_train_heating_parser.gd | stays | `Heating=`: what feeds the train heating (generator, main circuit, power cable...), which equipment exists |
| `heating_generator_engine` | - | fiz: fiz_train_heating_parser.gd only | down | `HGeneratorEngine`: a FIZ key the adapter never applies (the generator always follows the Mover's own `enrot`), so it is dead config |
| `heating_generator_min_rpm` | - | adapter, fiz: fiz_train_heating_parser.gd | down | `HGeneratorMinRPM`: a point of the Mover's linear rpm-to-voltage generator (EngineGenerator) |
| `heating_generator_min_voltage` | - | adapter, fiz: fiz_train_heating_parser.gd | down | `HGeneratorMinVoltage`: a point of the same linear curve |
| `heating_generator_max_rpm` | - | adapter, fiz: fiz_train_heating_parser.gd | down | `HGeneratorMaxRPM`: a point of the same linear curve |
| `heating_generator_max_voltage` | - | adapter, fiz: fiz_train_heating_parser.gd | down | `HGeneratorMaxVoltage`: a point of the same linear curve |
| `heating_power_cable_type` | - | adapter, fiz: fiz_train_heating_parser.gd | down | `HPowerTrans`: the Mover's TPowerType for its power-cable model (No/Bio/Mech/Electric/Steam power) |
| `heating_max_voltage` | - | adapter, fiz: fiz_train_heating_parser.gd | stays | `HMaxVoltage`: the heating's rated voltage |

#### Notes

- **No ALARM.** The only core reader of any property here is `RailVehicleServer.cpp:1441` (`trainset_determine_type`), which reads `cntrl_brake_delays`. That is a fact (the G/P/R settings the distributor has), so it stays. No file under `src/` or `addons/libreloaded/` reads an adapter config-dump key of these components.
- **Mover-model enums still defined in the core `RailVehicleBrake.hpp`, with no core use outside the class:**
  - `TrainBrakeValve`: Mover TBrakeValve. The adapter casts it by int value ("assuming same int values"). Goes down with `valve_type`.
  - `BrakeHandleType`: Mover TBrakeHandle, shared by the main and local handle. Goes down with both handle-type properties. The sound plugin compares it with FV4A/FVEL6.
  - `BrakeMethod`: Mover friction-curve ids (unclear, see the table).
  - `HandlePosition`: bh_MIN..bh_EPB. It belongs to the core *interface* (`get_handle_position()`), not to configuration. It is backend-neutral as a set of named positions, but the values it returns are in Mover handle units.
- **Encoding enums that mirror the Mover but describe facts (kept "stays"):**
  - `CompressorPower`: `COMPRESSOR_POWER_CONVERTER_MANUAL` is a Mover default with no FIZ spelling.
  - `BrakeDelaySetting` and `BrakeOperationMode`: Mover bitmasks.
  - `DynamicBrakeType`: dbrake_* flags.
  - `AntiSkidBrakeType::ANTI_SKID_BRAKE_YES`: the Mover's ASBType 128.
  - `LocalBrakeType`.
  - `RailVehicleController::StartMode`: start_t, used by the PowerSupply start modes.
  - Heating: `RailVehicleController::TrainPowerSource` (TPowerSource) and `TrainPowerType` (TPowerType), plus `RailVehicleEngine::EngineType` for the dead `heating_generator_engine`.

  If these move into the core as facts, their Mover-specific values (`..._CONVERTER_MANUAL`, `ANTI_SKID_BRAKE_YES`, `START_MODE_MANUAL_WITH_AUTO_FALLBACK`, `START_MODE_DIRECTION`) should be checked.
- **Core interface getters that publish Mover coefficients as "live state":** `RailVehicleElectroPneumaticDynamicBrake::get_ed_braking_ep_delay()`, `get_ep_max_brake_engagement_speed()` and `get_ep_min_regenerative_braking()` only echo DCEMUED_EP_delay/max_Vel/min_Im. Nothing outside the class files calls them. If the matching properties go down, these pure virtuals should go down too.
- **The config dump is published by the adapter, not the core.** `MaszynaMoverRailVehicleBrake::_fill_config_dictionary` puts facts (`brake_system`, `brake_delays`, `brake_delay_times`, `brake_max_cylinder_pressure`, `brake_handle_movement`, `brake_operation_mode_position_max`, `brakes_controller_position_*`) next to Mover selectors (`brake_valve_type`, `brake_handle_type`, `brake_local_handle_type`).
  - Read: only `brake_handle_movement`, `brake_operation_mode_position_max` and `brakes_controller_position_*`, all by mmd.
  - Never read anywhere: `brake_ep_enabled`, `brake_handle_type`, `brake_local_handle_type`, `brake_valve_type`, `brake_delays`, `brake_main_reservoir_volume`, `brake_system`, `brake_delay_times`, `brake_local_handle_available`, `brake_max_cylinder_pressure`, `brake_max_control_pressure`, `brake_pipe_pressure_high`, `brake_pipe_pressure_delta`.
- **Surprising:** `main_pipe_emergency_cuts_off_handle` is bound in the core, but its getter and setter are pure virtual there. The value is stored in the adapter (`MaszynaMoverRailVehicleBrake.hpp:135-143`). It is the only brake property stored like that, and it is a fact, so the storage should move up to the core.
- **Surprising, likely bug (not checked against the original's LoadFIZ):** `air_leak_multiplier` is scaled twice.
  - `fiz_train_brake_parser.gd:80` stores `AirLeakRate * 0.01`.
  - The adapter (`MaszynaMoverRailVehicleBrake.cpp:688`) multiplies by 0.01 again; its comment says the property is a multiplier where 1.0 means 0.01.
  - So `AirLeakRate=1` in a FIZ gives 0.0001 instead of 0.01. A vehicle without the key keeps the default 1.0, which gives 0.01.
- **Spring brake valve areas are cross-wired on purpose:** FIZ `ValveOnArea` -> `valve_cross_section_actuator_discharge` -> Mover `ValveOffArea`, and the other way round. The adapter comment cites Mover.cpp:11025 as the original's quirk. The property names describe what the Mover does with the values, not the FIZ key names.
- **Fields the FIZ parser derives instead of reading directly:**
  - `handle_movement`: CONTINUOUS only for an FV4a.
  - `releaser_enabled_only_at_no_power_pos`: defaults to true for diesel and diesel-electric vehicles.
  - `compressor_power`: MAIN becomes ENGINE for diesels, in `fiz_vehicle_builder.gd:141`.

  These are Mover-load rules applied in the plugin, which is the right side, and nothing in the core repeats them.
- **Borderline group:** in the brake rig, geometry and counts (cylinder count/radius/stroke, lever ratios, pressures, reservoir volumes) are marked "stays". The pure equation terms (BCS spring, BSA slack adjuster, BRE efficiency, RM rapid multiplier) are marked "down", because the core already exposes the computed force through `get_force_at()`. Moving the whole rig down would also be defensible, if the core keeps only `get_force_at()` and the limits.

## Diesel engine and engine table rows: configuration classification

Abbreviations. `unit` = adapter `MaszynaMoverDieselEngineUnit.cpp` (the diesel's Mover implementation, installed by `MaszynaMoverRailVehicleDieselEngine`/`...DieselElectricEngine`). `de-adapter` = `MaszynaMoverRailVehicleDieselElectricEngine.cpp`. `fiz-d` = `fiz_train_diesel_engine_parser.gd`, `fiz-de` = `fiz_train_diesel_electric_engine_parser.gd`, `fiz-common` = `fiz_train_engine_common.gd`, `fiz-es` = `fiz_train_electric_series_engine_parser.gd`, `fiz-ei` = `fiz_train_electric_induction_engine_parser.gd`. Mover line numbers refer to the vendored `plugins/libmaszyna/src/maszyna/maszyna-mover/McZapkie/Mover.cpp`. None of these properties is published in the config dump: the diesel's only dump key is `engine_idle_rpm_count` (`unit:299`, derived), and no code reads it by key.

### RailVehicleDieselEngine

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `turbo_position` | `running_sound_model.gd:125,131` | adapter `unit:136` (TurboTest); fiz: `fiz_train_turbo_parser.gd:13` -> `fiz_import_context.gd:34` -> `fiz_vehicle_builder.gd:148` | stays | TurboPos: the master controller position from which the turbocharger is heard. Core sound uses it to decide when the turbo is heard. The Mover never reads TurboTest. |
| `oil_pump_pressure_minimum` | - | adapter `unit:125`; fiz: `fiz-d:90` | stays | OilMinPressure: the oil pressure the engine needs to run (Mover.cpp:7072). A start-up condition; `test_sm42_startup_sequence.gd:38,81,108` waits on it. |
| `oil_pump_pressure_maximum` | - | adapter `unit:126`; fiz: `fiz-d:92` | stays | OilMaxPressure: the pressure the pump builds at top revs (Mover.cpp:2053). A pump rating and the gauge's range. |
| `fuel_pump_start_mode` | - | adapter `unit:127-128` (start_mode_to_mover); fiz: `fiz-common:81` | stays | FuelStart: how the fuel pump is switched on (manual, automatic, battery...). A wiring fact, typed with the core `RailVehicleController::StartMode` that ~10 other core properties share. Read by `maszyna_startup_test.gd:199`. |
| `oil_pump_start_mode` | - | adapter `unit:129-130`; fiz: `fiz-common:84` | stays | OilStart: same as the fuel pump, for the oil pump. Read by `maszyna_startup_test.gd:196`. |
| `water_pump_start_mode` | - | adapter `unit:131-132`; fiz: `fiz-common:87` | stays | WaterStart: same as the fuel pump, for the water pump. |
| `mechanical_min_rpm` | - | adapter `unit:134` (dizel_nmin), `unit:184` (dizel_nreg_min = nmin x share), `unit:291` (idle rpm count); fiz: `fiz-d:44` | stays | nmin: the engine's idle speed [1/s]. A basic engine fact. The core gets the derived idle speed through `get_idle_rpm_count()`. |
| `mechanical_max_rpm` | `running_sound_model.gd:128-129` | adapter `unit:135` (dizel_nmax); fiz: `fiz-d:210` (DList `nmax`) | stays | The governor's top speed [1/s], from the DList header. Core sound divides by it to get the turbo's pitch. A fact. |
| `mechanical_fuel_cutoff_rpm` | - | adapter `unit:137`; fiz: `fiz-d:52` | stays | nmax_cutoff: the speed at which the overspeed limiter cuts the fuel (Mover.cpp:7179). An engine limit. |
| `mechanical_inertia` | - | adapter `unit:138` (dizel_AIM); fiz: `fiz-d:56`, `fiz-de:69` (own default 1.25) | unclear | AIM: a true shaft moment of inertia in the mechanical branch (Mover.cpp:7361, 7375), but only a ramp divisor in the diesel-electric branch (Mover.cpp:4953). Needs a decision: does the core want a neutral inertia [kg m2], and does the diesel-electric value mean that at all? |
| `mechanical_clutch_engage_speed` | - | adapter `unit:139` (engageupspeed); fiz: `fiz-d:60` | down | EUS: how fast the Mover's clutch engages (Mover.cpp:6931). A model rate. |
| `mechanical_clutch_disengage_speed` | - | adapter `unit:140` (engagedownspeed); fiz: `fiz-d:62` | down | EDS: how fast the Mover's clutch disengages (Mover.cpp:6929). A model rate. |
| `mechanical_min_rpm_hydro_drive` | - | adapter `unit:141`; fiz: `fiz-d:47` (falls back to nmin) | unclear | nmin_hdrive: the idle setpoint while driving on the converter. A real governor setpoint, but it only makes sense inside the Mover's hydro governor law (Mover.cpp:7161, together with `_factor`). Needs a decision on whether the core describes governor setpoints per mode. |
| `mechanical_min_rpm_hydro_drive_factor` | - | adapter `unit:142`; fiz: `fiz-d:48` | down | nmin_hdrive_factor: the slope of the Mover's governor law, `nmin_hdrive + eimic * factor` (Mover.cpp:7161). |
| `mechanical_min_rpm_retarder` | - | adapter `unit:143`; fiz: `fiz-d:50` (falls back to nmin) | unclear | nmin_retarder: the speed the governor ramps to while the retarder brakes (Mover.cpp:7164, rate `5.0 * dt`). Same question as `mechanical_min_rpm_hydro_drive`. |
| `mechanical_nominal_max_rpm` | - | adapter `unit:144` (Mover `nmax`); fiz: `fiz-d:51` (Engine `nmax` / 60) | stays | The highest allowed engine speed [1/s] (MOVER.h:1277; used for overspeed and the rpm ratio, Mover.cpp:415, 1086). A limit, but it overlaps `mechanical_max_rpm` (see Notes). |
| `mechanical_regulator_acceleration` | - | adapter `unit:145`; fiz: `fiz-d:54` | down | nreg_acc: how fast the Mover's governor raises its setpoint (Mover.cpp:7169). Default 999. |
| `mechanical_rpm_decrease_rate` | - | adapter `unit:146`; fiz: `fiz-d:58`, `fiz-de:71` | down | RPMDecRate: a multiplier in the Mover's revolution ramp (Mover.cpp:4956). |
| `mechanical_shunt_mode_ratio` | - | adapter `unit:147` (AnPos), `unit:188` (ShuntModeAllow = ratio > 0, plain diesel only); fiz: `fiz-d:66` (inverts ratios under 1) | stays | ShuntMode: the ratio of an extra shunting gear (2Ls150), 0 when there is none. A gearbox fact. It is stored in the Mover's AnPos (see Notes). |
| `clutch_min_velocity_full_engage` | - | adapter `unit:148`; fiz: `fiz-d:77` | down | minVelfullengage: a speed threshold in the Mover's clutch and gear logic (Mover.cpp:7022-7209). |
| `clutch_diameter` | - | adapter `unit:149`; fiz: `fiz-d:79` | down | engageDia: one factor of the Mover's clutch torque `MaxForce*engage*Dia*friction` (Mover.cpp:7305), with no radius or pressure model. A neutral fact would be the maximum clutch torque. |
| `clutch_max_force` | - | adapter `unit:150`; fiz: `fiz-d:81` | down | engageMaxForce: a factor of the same product (Mover.cpp:7305). |
| `clutch_friction` | - | adapter `unit:151`; fiz: `fiz-d:83` | down | engagefriction: a factor of the same product (Mover.cpp:7242, 7305). |
| `torque_converter_unlock_velocity` | - | adapter `unit:152` (dizel_maxVelANS); fiz: `fiz-d:166` | down | MaxVelANS: below this speed, when not powered, the Mover drains the converter (Mover.cpp:7280). A model threshold. |
| `retarder_engage_velocity` | - | adapter `unit:153`; fiz: `fiz-d:185` | down | R_EngageVel: a speed threshold in the Mover's retarder and governor logic (Mover.cpp:7007, 7163). |
| `retarder_clutch` | - | adapter `unit:154`; fiz: `fiz-d:188` | stays | R_IsClutch: the retarder has its own clutch. Says which equipment exists (Mover.cpp:7404). |
| `retarder_clutch_speed` | - | adapter `unit:155`; fiz: `fiz-d:187` | down | R_ClutchSpeed: how fast the Mover spins that clutch up (Mover.cpp:7408). |
| `retarder_with_individual` | - | adapter `unit:156`; fiz: `fiz-d:189` | stays | R_WithIndividual: the independent brake handle commands the retarder (Mover.cpp:7396). A control-wiring fact. |
| `torque_converter_present` | - | adapter `unit:191` (hydro_TC); fiz: `fiz-d:67-68` | stays | IsTC: the vehicle has a torque converter. |
| `torque_converter_max_torque_ratio` | - | adapter `unit:192`; fiz: `fiz-d:142` | down | TC_TMMax: a parameter of the Mover's analytic converter curve, used only when there is no HTCList (Mover.cpp:7319-7321). |
| `torque_converter_coupling_point` | - | adapter `unit:193`; fiz: `fiz-d:144` | down | TC_CP: the same analytic curve (Mover.cpp:7319-7325). |
| `torque_converter_lockup_torque` | - | adapter `unit:194`; fiz: `fiz-d:146` | down | TC_LT: `TorqueL = LT * LockupRate` in the Mover's lock-up model (Mover.cpp:7331). |
| `torque_converter_lockup_rate` | - | adapter `unit:195`; fiz: `fiz-d:148` | down | TC_LR. It writes `hydro_TC_LockupRate`, which the Mover integrates as the live lock-up state (Mover.cpp:7288-7300). See Notes. |
| `torque_converter_unlock_rate` | - | adapter `unit:196`; fiz: `fiz-d:150` | down | TC_ULR: a Mover rate that the vendored Mover never reads (`hydro_TC_UnlockRate` has 0 uses). Dead config. |
| `torque_converter_fill_rate_increase` | - | adapter `unit:197`; fiz: `fiz-d:152` | down | TC_FRI: the Mover's fill and lock-up rate (Mover.cpp:7278, 7288). |
| `torque_converter_fill_rate_decrease` | - | adapter `unit:198`; fiz: `fiz-d:154` | down | TC_FRD: the Mover's drain rate (Mover.cpp:7281, 7294). |
| `torque_converter_torque_in_in` | - | adapter `unit:199`; fiz: `fiz-d:156` | down | TC_TII: a coefficient of the Mover's quadratic converter torque (Mover.cpp:7310). |
| `torque_converter_torque_in_out` | - | adapter `unit:200`; fiz: `fiz-d:158` | down | TC_TIO: a coefficient of the same (Mover.cpp:7311). |
| `torque_converter_torque_out_out` | - | adapter `unit:201`; fiz: `fiz-d:160` | down | TC_TOO: a coefficient of the same (Mover.cpp:7312). |
| `torque_converter_lockup_speed` | - | adapter `unit:202`; fiz: `fiz-d:162` | down | TC_LS: a threshold of the Mover's lock-up control (Mover.cpp:7286, 7291). |
| `torque_converter_unlock_speed` | - | adapter `unit:203`; fiz: `fiz-d:164` | down | TC_ULS: a threshold of the Mover's unlock control (Mover.cpp:7291). |
| `torque_converter_table` | - | adapter `unit:205-215` (hydro_TC_Table); fiz: `fiz-d:273` (HTCList) | unclear | A generic speed-ratio to torque-ratio curve of the converter (Mover.cpp:7314-7317), so in itself a hardware fact. But every other converter parameter is a Mover coefficient. Needs a decision on whether the core keeps any description of the converter beyond "present". |
| `vel2nmax_table` | - | adapter `unit:217-228` (dizel_vel2nmax_Table, y / 60); fiz: `fiz-d:276` (V2NList) | down | A FIZ V2NList: a speed to max-revolutions limit of the Mover's governor (Mover.cpp:7168-7170). No vehicle in the corpus uses it. |
| `retarder_present` | - | adapter `unit:230` (hydro_R); fiz: `fiz-d:167-168` | stays | IsRetarder: the vehicle has a hydrodynamic retarder. |
| `retarder_placement` | - | adapter `unit:231`; fiz: `fiz-d:171` (cast to the enum) | unclear | R_Place: where the retarder sits in the driveline. Physically a topology fact, but it is the Mover's 0/1/2 selector that switches its branches (Mover.cpp:7259, 7274, 7380, 7400). Needs a decision on whether the core models driveline topology. |
| `retarder_torque_in_in` | - | adapter `unit:232`; fiz: `fiz-d:173` | down | R_TII: the coefficient of `n*n*k` in the Mover (Mover.cpp:7425). |
| `retarder_max_torque` | - | adapter `unit:233`; fiz: `fiz-d:175` | stays | R_MT: the retarder's maximum braking torque. A rating (Mover.cpp:7421). |
| `retarder_max_power` | - | adapter `unit:234`; fiz: `fiz-d:177` | stays | R_MP: the retarder's maximum power. A rating (Mover.cpp:7423-7424). |
| `retarder_fill_rate_increase` | - | adapter `unit:235`; fiz: `fiz-d:179` | down | R_FRI: the Mover's fill rate (Mover.cpp:7416). |
| `retarder_fill_rate_decrease` | - | adapter `unit:236`; fiz: `fiz-d:181` | down | R_FRD: the Mover's drain rate (Mover.cpp:7418). |
| `retarder_min_velocity` | - | adapter `unit:237`; fiz: `fiz-d:183` | down | R_MinVel: below this speed the Mover drops the retarder request (Mover.cpp:7398). A model threshold. |
| `throttle_table_max_torque` | - | adapter `unit:240` (dizel_Mmax); fiz: `fiz-d:206` | stays | DList Mmax: the engine's maximum torque. A rating. The Mover uses it for its parabola when there is no DMList (Mover.cpp:7251). |
| `throttle_table_max_torque_rpm` | - | adapter `unit:241` (dizel_nMmax); fiz: `fiz-d:208` | stays | DList nMmax: the speed of maximum torque [1/s]. A rating (Mover.cpp:7252). |
| `throttle_table_max_rpm_torque` | - | adapter `unit:242` (dizel_Mnmax); fiz: `fiz-d:212` | stays | DList Mnmax: the torque at top speed. A rating (Mover.cpp:7251). |
| `throttle_table_nominal_fuel_dose` | - | adapter `unit:243` (dizel_nominalfill); fiz: `fiz-d:214` | down | DList nominalfill: a parameter of the Mover's fill model. The vendored Mover never reads `dizel_nominalfill` (0 uses). Dead config. |
| `throttle_table_resistance_torque` | - | adapter `unit:244` (dizel_Mstand); fiz: `fiz-d:216` | unclear | Mstand: the engine's own drag torque (Mover.cpp:7249-7266, 5089), which would be a fact. But the Mover also uses it as the starter's torque (`Mstand/(0.3+n/nmin)`, Mover.cpp:7269). Needs a decision: rating or tuning knob? |
| `throttle_table_nominal_fuel_consumption_rate` | - | adapter `unit:245`; fiz: `fiz-d:218` | stays | NomFuelConsRate: the specific fuel consumption [g/kWh]. An engine rating. The vendored Mover never reads it (0 uses). |
| `throttle_table_positions` | - | adapter `unit:247-267` (into `RList[i].Relay/R/Mn`); fiz: `fiz-d:267` (DList rows) | down | A FIZ DList table written straight into the Mover's RList, with Mover clutch codes (`Mn`). |
| `torque_table` | - | adapter `unit:270-280` (dizel_Momentum_Table, x / 60); fiz: `fiz-d:270` (DMList) | stays | A generic rpm to torque [Nm] curve of `VehicleCurvePointItem` (Mover.cpp:7249). The engine's torque characteristic, meaningful to any backend. |
| `cooling_heat_kw` | - | adapter `unit:160`; fiz: `fiz-d:94` | down | HeatKW: a heat-exchange factor of the Mover's dizel_heat model (Mover.cpp:7548). |
| `cooling_heat_kv` | - | adapter `unit:161`; fiz: `fiz-d:96` | down | HeatKV: a dizel_heat factor. |
| `cooling_heat_kfe` | - | adapter `unit:162`; fiz: `fiz-d:98` | down | HeatKFE: a dizel_heat factor. |
| `cooling_heat_kfs` | - | adapter `unit:163`; fiz: `fiz-d:100` | down | HeatKFS: a dizel_heat factor. |
| `cooling_heat_kfo` | - | adapter `unit:164`; fiz: `fiz-d:102` | down | HeatKFO: a dizel_heat factor. |
| `cooling_heat_kfo2` | - | adapter `unit:165`; fiz: `fiz-d:104` | down | HeatKFO2: a dizel_heat factor (Mover.cpp:7509, 7551). |
| `cooling_water_min_temperature` | - | adapter `unit:166`; fiz: `fiz-d:106` | stays | WaterMinTemperature [C]: a thermal limit of the cooling water (-1 = not set). The core already shows water temperatures as state, for warm-up. |
| `cooling_water_max_temperature` | - | adapter `unit:167`; fiz: `fiz-d:108` | stays | WaterMaxTemperature [C]: the overheat limit behind the heat-malfunction lamp (`get_heat_malfunction`). |
| `cooling_water_flow_temperature` | - | adapter `unit:168`; fiz: `fiz-d:110` | stays | WaterFlowTemperature [C]: the thermostat's setpoint for opening the flow (Mover.cpp:7567). A cooling-system fact. |
| `cooling_water_cooling_temperature` | - | adapter `unit:169`; fiz: `fiz-d:112` | stays | WaterCoolingTemperature [C]: the temperature at which the fan or radiator cuts in. A setpoint fact. |
| `cooling_water_shutters` | - | adapter `unit:170`; fiz: `fiz-d:114` | stays | WaterShutters: the radiator has shutters. Says which equipment exists. |
| `cooling_water_aux_circuit` | - | adapter `unit:171`; fiz: `fiz-d:116` | stays | WaterAuxCircuit: the engine has a second water circuit. Says which equipment exists. The core already has state and commands for it (`water_circuits_link`). |
| `cooling_water_aux_min_temperature` | - | adapter `unit:172`; fiz: `fiz-d:118` | stays | The auxiliary circuit's limit [C]. |
| `cooling_water_aux_max_temperature` | - | adapter `unit:173`; fiz: `fiz-d:120` | stays | The auxiliary circuit's limit [C]. |
| `cooling_water_aux_cooling_temperature` | - | adapter `unit:174`; fiz: `fiz-d:122` | stays | The auxiliary circuit's setpoint [C] (Mover.cpp:7537-7539). |
| `cooling_water_aux_shutters` | - | adapter `unit:175`; fiz: `fiz-d:124` | stays | The auxiliary radiator has shutters. Says which equipment exists. |
| `cooling_oil_min_temperature` | - | adapter `unit:176`; fiz: `fiz-d:126` | stays | OilMinTemperature [C]: a lubrication limit. The core shows `oil_temperature` as state. |
| `cooling_oil_max_temperature` | - | adapter `unit:177`; fiz: `fiz-d:128` | stays | OilMaxTemperature [C]: a lubrication limit. |
| `cooling_fan_speed` | - | adapter `unit:178`; fiz: `fiz-d:130` | down | WaterCoolingFanSpeed: the Mover's encoding, a share of the engine's revs or an absolute value when negative (Mover.cpp:7542). |
| `cooling_heater_min_temperature` | - | adapter `unit:179`; fiz: `fiz-d:132` | stays | HeaterMinTemperature [C]: the water heater's thermostat. The heater exists in the core's state and commands. |
| `cooling_heater_max_temperature` | - | adapter `unit:180`; fiz: `fiz-d:134` | stays | HeaterMaxTemperature [C]: the water heater's thermostat. |
| `cooling_nominal_power` | - | adapter `unit:181-182` (`heat.powerfactor = 1235 / it`); fiz: `fiz-d:136` | down | NominalCoolingPower: rescales the Mover's heat model, which was calibrated on su45 (Mover.cpp:7493-7496). Pure model scaling. It carries the core constant `NOMINAL_COOLING_POWER` with it. |

### RailVehicleDieselElectricEngine

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `wwlist` | - | adapter `de-adapter:24-47` (DElist[] and SST[]); fiz: `fiz-de:151` (WWList) | down | A FIZ WWList table, mapped one to one onto the Mover's TDEScheme and TShuntScheme arrays, with the shunt power computed by a Mover formula (`de-adapter:43-45`). The facts inside it (revs and generator power per notch) already reach the core as `get_max_rpm()`/`get_idle_rpm_count()`. |
| `generator_voltage_flat` | - | adapter `de-adapter:15` (Flat); fiz: `fiz-de:55` | down | Flat: a Mover switch that selects a flat generator characteristic (Mover.cpp:5413, 5737, 5827). |
| `hyperbolic_speed` | - | adapter `de-adapter:16` (Vhyp); fiz: `fiz-de:57` | down | Vhyp: a parameter of the Mover's diesel-electric traction hyperbola (Mover.cpp:5301-5305). The Mover also reuses the field as a timer (Mover.cpp:5251-5254). |
| `additional_speed` | - | adapter `de-adapter:17` (Vadd); fiz: `fiz-de:59` | down | Vadd: the same hyperbola's offset (Mover.cpp:5304, 5311). The Mover also uses Vadd as live state in other branches (Mover.cpp:5232-5242). |
| `power_correction_ratio` | - | adapter `de-adapter:18` (PowerCorRatio); fiz: `fiz-de:61` | down | Cr: the Mover's correction coefficient (Mover.cpp:5305, 5312). |
| `shunt_relay_type` | - | adapter `de-adapter:19` (RelayType); fiz: `fiz-de:63` | down | RelayType: the Mover's selector for field-shunting logic (`switch (RelayType)`, Mover.cpp:5463-5610). A plain int with no enum. |
| `shunt_mode_allowed` | - | adapter `de-adapter:20` (ShuntModeAllow); fiz: `fiz-de:66` | stays | ShuntMode: the locomotive has a shunting mode (Mover.cpp:769, 5695). Says which equipment exists; matters to a driver and the cab. |
| `heating_rpm` | - | adapter `de-adapter:21` (EngineHeatingRPM); fiz: `fiz-de:68` | stays | HeatingRPM: the engine speed guaranteed while train heating is on (MOVER.h:1119, Mover.cpp:4937-4939). A vehicle fact. Stored in rpm (the Mover divides by 60). |

### RailVehicleDieselEngineUnit

Binds no properties. It is a plain C++ interface, not a Godot class (`RailVehicleDieselEngineUnit.hpp:7-13`), with only pure virtual getters, commands, `apply_configuration` and `fill_config`. Nothing to classify.

### RailVehicleWWListItem

The row of `RailVehicleDieselElectricEngine.wwlist` and of `RailVehicleElectricInductionEngine.wwlist` (ffList). No core code reads any item field. The JSON's core hits are false positives: `rpm` is the engine's own `get_rpm()`/state, `max_power` belongs to SpeedControl and ElectricInduction, `max_current` is the traction wire's.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `rpm` | - | adapter `de-adapter:35`, `MaszynaMoverRailVehicleElectricInductionEngine.cpp:114` (DElist.RPM); fiz: `fiz-de:116`, `fiz-ei:134` | down | The TDEScheme RPM column. For a diesel-electric it is the engine revs per notch. For ffList the same field holds a speed, which the Mover compares with Vel (Mover.cpp:5883-5890). Mover struct reuse, not a fact with one meaning. |
| `max_power` | - | adapter `de-adapter:36`, `...ElectricInductionEngine.cpp:115` (GenPower); fiz: `fiz-de:117`, `fiz-ei:135` | down | The TDEScheme GenPower column: the generator power per notch (diesel-electric) or the power per speed (ffList). It goes down with the table. |
| `max_voltage` | - | adapter `de-adapter:37` (Umax); fiz: `fiz-de:118` | down | The TDEScheme Umax column: the generator voltage limit per notch (Mover.cpp:5296, 5337). |
| `max_current` | - | adapter `de-adapter:38` (Imax); fiz: `fiz-de:119` | down | The TDEScheme Imax column: the current limit per notch (Mover.cpp:5295, 5336). |
| `has_shunting` | - | adapter `de-adapter:39`; fiz: `fiz-de:121` | down | Only marks that the FIZ row had the 3 extra SST columns. FIZ-shaped. |
| `min_wakeup_voltage` | - | adapter `de-adapter:40,43` (SST.Umin, and Pmin derived from it); fiz: `fiz-de:122` | down | A column of the Mover's TShuntScheme (Mover.cpp:5320-5326). |
| `max_wakeup_voltage` | - | adapter `de-adapter:41,45` (SST.Umax); fiz: `fiz-de:123` | down | A column of the Mover's TShuntScheme. |
| `max_wakeup_power` | - | adapter `de-adapter:42,44-45` (SST.Pmax, clamped by a Mover formula); fiz: `fiz-de:124` | down | A column of the Mover's TShuntScheme. |

### RailVehicleMotorParameter

The row of `RailVehicleEngine.motor_param_table`, which the Mover reads as `TMotorParameters`/`MPTRelay`. Core code only stores the array (`RailVehicleEngine.hpp:83-105`) and never reads a row.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `shunting_up` | - | adapter `MaszynaMoverDriveUnit.cpp:188` (MPTRelay.Iup); fiz: `fiz-common:129` | down | The current threshold of the Mover's field-shunt relay. Only the diesel-electric MotorParamTable column fills it. |
| `shunting_down` | - | adapter `MaszynaMoverDriveUnit.cpp:189` (MPTRelay.Idown); fiz: `fiz-common:130` | down | The shunt relay's dropout threshold. |
| `voltage_constant_multiplier` | - | adapter `MaszynaMoverDriveUnit.cpp:176` (mfi); fiz: `fiz-common:125,132,153,171`, `fiz-es:164` | down | mfi, a coefficient of the Mover's series-motor current equation. For a plain diesel's gearbox it holds a shift speed (`fiz-common:146-155`). |
| `saturation_current_multiplier` | - | adapter `MaszynaMoverDriveUnit.cpp:174` (mIsat); fiz: `fiz-common:126,133,151,167`, `fiz-es:165` | down | mIsat. For a plain diesel it holds the gear ratio (`fiz-common:146-151`; Mover.cpp:4971). |
| `initial_voltage_constant_multiplier` | - | adapter `MaszynaMoverDriveUnit.cpp:186` (mfi0); fiz: `fiz-common:134,170` | down | mfi0, a column of MotorParamTable0 (readMPT0). |
| `voltage_constant` | - | adapter `MaszynaMoverDriveUnit.cpp:175` (fi); fiz: `fiz-common:127,135,152,169`, `fiz-es:166` | down | fi, the back-EMF constant of the Mover's `Current()`. For a diesel's gearbox it holds a shift speed. |
| `saturation_current` | - | adapter `MaszynaMoverDriveUnit.cpp:177` (Isat); fiz: `fiz-common:128,136,172`, `fiz-es:167` | down | Isat, a coefficient of the Mover's motor equation. |
| `initial_voltage_constant` | - | adapter `MaszynaMoverDriveUnit.cpp:187` (fi0); fiz: `fiz-common:137,168` | down | fi0, the back-EMF term of the Mover (Mover.cpp:333, 389). |
| `auto_switch` | - | adapter `MaszynaMoverDriveUnit.cpp:179` (MotorParam.AutoSwitch); fiz: `fiz-common:139,154,173`, `fiz-es:169` | down | An optional FIZ column: the controller passes this gear or shunt by itself. A flag of the Mover's table. The JSON's core hit is the same-named field of RailVehicleRelayListItem. |

### RailVehicleRelayListItem

The row of `RailVehicleElectricSeriesEngine.relay_list` (FIZ RList), written into the Mover's `RList[]`. No core code reads a row. The JSON's core hits for `resistance` are false positives: `RailVehicleServer.cpp:2109` is a comment, and `TractionServer.cpp:328` and nearby lines are about the traction wire's resistance.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `relay_position` | - | adapter `MaszynaMoverRailVehicleElectricSeriesEngine.cpp:45` (RList.Relay); fiz: `fiz-es:141` | down | The Mover's relay code for the notch. |
| `resistance` | - | adapter `MaszynaMoverRailVehicleElectricSeriesEngine.cpp:46` (RList.R); fiz: `fiz-es:142` | down | The starting resistance per notch [ohm]. A fact, but it is a column of the FIZ RList row and the Mover's `R > 0` notch logic (Mover.cpp:2273, 2430). It goes down with the table. |
| `branch_count` | - | adapter `...ElectricSeriesEngine.cpp:47` (RList.Bn); fiz: `fiz-es:143` | down | The number of parallel motor branches per notch (Mover.cpp:322). An RList column. |
| `motors_per_branch` | - | adapter `...ElectricSeriesEngine.cpp:48` (RList.Mn); fiz: `fiz-es:144` | down | The number of motors in series per notch (Mover.cpp:320, 409). An RList column. |
| `auto_switch` | - | adapter `...ElectricSeriesEngine.cpp:49` (RList.AutoSwitch); fiz: `fiz-es:145` | down | A FIZ flag: the notch is passed automatically. A flag of the Mover's table. |
| `shunt_index` | - | adapter `...ElectricSeriesEngine.cpp:50` (RList.ScndAct); fiz: `fiz-es:147` | down | ScndAct: the field-shunt position set by the notch. 255 means none (`...ElectricSeriesEngine.cpp:85`). Mover encoding. |

### RailVehicleThrottlePositionItem

The row of `RailVehicleDieselEngine.throttle_table_positions` (FIZ DList rows `Relay R Mn`).

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `throttle_position` | - | adapter `unit:264` (RList.Relay); fiz: `fiz-d:245` | down | The DList Relay column, written into the Mover's RList relay slot. |
| `fuel_dose` | - | adapter `unit:265` (RList.R); fiz: `fiz-d:246` | down | The fuel fill per notch, stored in the Mover's RList resistance slot. Exposed to the core only as live state (`get_fill_desired`). |
| `clutch_behavior` | - | adapter `unit:266` (RList.Mn); fiz: `fiz-d:247` | down | The Mover's per-notch clutch and governor code, stored in the motors-per-branch slot. The enum is a Mover model selector (see Notes). |

#### Notes

- **Mover model selector enums.**
  - `RailVehicleDieselEngine::RetarderPlacement` (`RailVehicleDieselEngine.hpp:67-72`, bound at `.cpp:243-245`) is the Mover's `hydro_R_Placement` 0/1/2 (FIZ R_Place). It switches Mover branches at Mover.cpp:7259, 7274, 7380 and 7400. No core code uses it; only `fiz-d:171` and `test_train_diesel_engine_mechanical.gd` do.
  - `RailVehicleThrottlePositionItem::ClutchBehavior` (`RailVehicleThrottlePositionItem.hpp:11-17`) holds the DList `Mn` codes. It is a Mover model selector. No core code uses it. The only other user is `maszyna_startup_test.gd:224`, which compares the double state `get_clutch_desired()` with `CLUTCH_BEHAVIOR_NONE`.
  - `shunt_relay_type` is a Mover selector kept as a bare int.
  - The pumps' `StartMode` is the core `RailVehicleController` enum (MaSzyna's start_t, mapped by `MaszynaMoverVehicleServer::start_mode_to_mover`). It is shared by many core components and is not decided here.
- **Model code in the core interface: none.** `RailVehicleDieselEngine.cpp` (422 lines) is pure forwarding:
  - state getters to `diesel_engine_unit` (lines 10-108);
  - `_apply_configuration` and `_fill_config_dictionary` to the unit (109-120);
  - the `turbo_position` accessor (122-128);
  - bindings (130-289);
  - the state dump (296-333);
  - commands forwarded to the unit (335-393) and their registration (395-421).

  `RailVehicleDieselElectricEngine.cpp` is the same, forwarding to `traction_motors_unit` (lines 49-91). What does belong to the Mover in the core files:
  - (a) the constant `NOMINAL_COOLING_POWER = 1235.0` (`RailVehicleDieselEngine.hpp:63-65`, su45's default from the original's Mover.cpp:11371). It is used only by the adapter (`unit:181-182`) and should go down with `cooling_nominal_power`.
  - (b) every property default in `RailVehicleDieselEngine.hpp:85-197`, `RailVehicleDieselElectricEngine.hpp:31-37` and the item headers. These are the Mover's own defaults (for example nreg_acc 999, clutch 0.5/6000/0.5, converter 2.0/0.85/3000/4.5, heat 0.35/0.6/1/80/25/25, fan 0.075).
  - (c) header comments that describe Mover fields: `RailVehicleDieselEngineUnit.hpp:32-43, 53-60` (`RList[MainCtrlPos].R/.Mn`, `dizel_engage`, `dizel_heat.Twy/Ts/To`, `hydro_R_Fill`). So the unit's live getters `get_fill_desired`/`get_clutch_desired` are DList-row readouts.
- **Two "max rpm" properties.** `mechanical_max_rpm` comes from the DList header `nmax`, stored raw because it is already in 1/s (`fiz-d:210`, into `dizel_nmax`). `mechanical_nominal_max_rpm` comes from Engine `nmax` divided by 60 (`fiz-d:51`, into the Mover's `nmax`). The core sound reads only the first. They should probably become one fact.
- **Units do not match the names.** Every `mechanical_*_rpm` property holds revolutions per second. `heating_rpm`, the x of `torque_table` and the y of `vel2nmax_table` hold revolutions per minute. The test sets `mechanical_min_rpm = 600.0` (`test_train_diesel_engine_mechanical.gd:39`), an rpm-sized value in a 1/s property.
- **Configuration overwrites live Mover state on every reapply.**
  - `torque_converter_lockup_rate` writes `hydro_TC_LockupRate`, which the Mover integrates as the live lock-up state (Mover.cpp:7286-7300).
  - `mechanical_shunt_mode_ratio` writes the Mover's `AnPos`, "the analog control position" (MOVER.h:1377). It does so unconditionally (`unit:147`), also for a diesel-electric, where AnPos is the shunt-field interpolation (Mover.cpp:5320-5326). There it is always 0, because the diesel-electric parser never sets the ratio.
- **Dead configuration.** The adapter writes these fields but the vendored Mover never reads them: `TurboTest` (`turbo_position`; the core sound reads the property itself), `dizel_nominalfill` (`throttle_table_nominal_fuel_dose`), `dizel_NominalFuelConsumptionRate` and `hydro_TC_UnlockRate`.
- **Row classes reused with different meanings.**
  - `RailVehicleWWListItem` serves both the diesel-electric WWList and the induction engine's ffList, where `rpm` holds a speed.
  - `RailVehicleMotorParameter` holds a plain diesel's gear ratio in `saturation_current_multiplier` and its shift speeds in `voltage_constant`/`voltage_constant_multiplier` (`fiz-common:146-178`). Gear ratios, which are facts, sit in Mover-named fields.
  - `RailVehicleThrottlePositionItem` rows go into the same `RList[]` as `RailVehicleRelayListItem` rows: `fuel_dose` in the resistance slot `R`, `clutch_behavior` in the motors slot `Mn`.

  All four row classes are Mover struct mirrors (TDEScheme+TShuntScheme, TMotorParameters+MPTRelay, TScheme). The few facts in them (revs, power and limits per notch, gear ratios) are best given to the core as derived facts. `get_max_rpm()` and `get_idle_rpm_count()` already do this for the diesel-electric's revs (`unit:117-120, 285-293`, Mover.cpp:1099-1105).
- **No ALARM.** The only core readers of these classes' properties are `running_sound_model.gd` (`turbo_position`, `mechanical_max_rpm`), and both properties are facts.

## Configuration split: engine, electric engines, power source, speed control, universal controller, switches

Scope: `RailVehicleEngine`, `RailVehicleElectricEngine`, `RailVehicleElectricInductionEngine`, `RailVehicleElectricSeriesEngine`, `RailVehicleEnginePowerSource`, `RailVehicleSpeedControl`, `RailVehicleUniversalController`, `RailVehicleUniversalControllerListItem`, `RailVehicleSwitches`. Every property bound in each `_bind_methods()` is listed (the JSON missed `current_collector_pantograph_type`, which `RailVehicleEnginePowerSource.cpp` binds with a plain `ADD_PROPERTY`).

Where the Mover side lives: `RailVehicleEngine` and `RailVehicleElectricEngine` have no `MaszynaMover...` adapter of their own. Their configuration is read by the composed units `MaszynaMoverDriveUnit` (behind `RailVehicleDriveUnit`) and `MaszynaMoverCircuitUnit` (behind `RailVehicleCircuitUnit`). The concrete adapters `MaszynaMoverRailVehicleElectricSeriesEngine`, `...ElectricInductionEngine`, `...DieselEngine` and `...DieselElectricEngine` install those units (`drive_unit = &drive_unit_impl`, etc., in their `.hpp`). In the tables below, "adapter" for these two classes means those units. "down" for an Engine/ElectricEngine property therefore means moving it to the drive or circuit unit's side, or to a shared Mover engine base, because no `MaszynaMoverRailVehicleEngine` class exists to receive it.

Reader lines were checked by grepping `get_/set_<name>`, `"<name>"`, `&"<name>"`, `.<name>` and the dump keys across `src/`, `addons/libreloaded/`, `plugins/libmaszyna/{src,addons}` and `demo/tests`. Tests and saved data are not listed as readers.

### RailVehicleEngine

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `motor_param_table` | - | adapter: MaszynaMoverDriveUnit.cpp:165-189; fiz: fiz_train_electric_series_engine_parser.gd:182, fiz_train_diesel_electric_engine_parser.gd:153 | down | FIZ `MotorParamTable0:` rows (Isat, fi, mfi, fi0, mfi0, Iup/Idown) are Mover's series-motor model, consumed only by Mover's equations |
| `transmission_gear_teeth_motor` | `running_sound_model.gd:157` (via `get_transmission_ratio()`) | adapter: MaszynaMoverDriveUnit.cpp:122, :129 (ratio); fiz: fiz_train_engine_common.gd:35 | stays | gear tooth count. The core's `get_transmission_ratio()` derives the motor/wheel ratio from it, and the core sound uses that ratio for motor revolutions |
| `transmission_gear_teeth_wheel` | `running_sound_model.gd:157` (via `get_transmission_ratio()`) | adapter: MaszynaMoverDriveUnit.cpp:123, :129; fiz: fiz_train_engine_common.gd:36 | stays | same as above, the wheel side of the gear |
| `transmission_efficiency` | - | adapter: MaszynaMoverDriveUnit.cpp:130; fiz: fiz_train_engine_common.gd:38 | stays | the gearbox's efficiency, a drivetrain rating that has meaning in any traction model; today only Mover's Fw uses it |
| `maximum_traction_force` | - | adapter: MaszynaMoverDriveUnit.cpp:131; other adapter: MaszynaMoverRailVehicleBuffCoupl.cpp:151 (bare-coupler spring/limit sizing); fiz: fiz_train_engine_common.gd:40 | stays | the vehicle's maximum tractive force (Ftmax), a rating; a second adapter already sizes the couplers from it |
| `motor_blowers_speed` | - | adapter: MaszynaMoverDriveUnit.cpp:136; fiz: fiz_train_engine_common.gd:42 | unclear | blower rpm stored in Mover's encoding: a positive value is a fraction of the engine rpm, a negative value is absolute rpm (MOVER.h:897). Needs a decision on normalising it into a backend-neutral rpm before it can stay |
| `motor_blowers_sustain_time` | - | adapter: MaszynaMoverDriveUnit.cpp:137; fiz: fiz_train_engine_common.gd:44 | stays | how long the traction-motor blowers keep running after stop: the vehicle's blower automation, not a Mover equation coefficient |
| `motor_blowers_start_velocity` | - | adapter: MaszynaMoverDriveUnit.cpp:138; fiz: fiz_train_engine_common.gd:46 | stays | the speed above which the blowers start automatically: blower automation, a vehicle fact |
| `pressure_switch_present` | - | adapter: MaszynaMoverDriveUnit.cpp:132; fiz: fiz_train_engine_common.gd:53 | stays | whether the vehicle has a control pressure switch (equipment presence) |
| `main_init_time` | - (own dump: RailVehicleEngine.cpp:106 key `main_init_time`) | adapter: MaszynaMoverDriveUnit.cpp:133; cab: python_screen_state.gd:138 (dump key); fiz: fiz_train_engine_common.gd:73 | stays | the time the main circuit needs to get ready; the cab screen shows a countdown against it |
| `inverters_count` | - | adapter: MaszynaMoverDriveUnit.cpp:134; fiz: fiz_train_engine_common.gd:48 | stays | the number of traction inverters, a count (the python screen counts the live `inverters` array instead) |
| `motor_blowers_start_mode` | - | adapter: MaszynaMoverDriveUnit.cpp:139; fiz: fiz_train_engine_common.gd:77 | stays | how the blowers are switched on (manual/automatic...), cab-visible behaviour expressed in the core `StartMode` enum |
| `cntrl_eim_control_additional_zeros` | - | adapter: MaszynaMoverDriveUnit.cpp:142; fiz: fiz_train_engine_common.gd:68 | down | `EIMCtrlAddZeros`, a lever-mapping detail that only Mover's `CheckEIMIC` interprets |
| `cntrl_eim_control_emergency` | - | adapter: MaszynaMoverDriveUnit.cpp:143; fiz: fiz_train_engine_common.gd:70 | down | `EIMCtrlEmergency`, a variant flag of Mover's EIM lever model |
| `cntrl_eim_control_type` | - | adapter: MaszynaMoverDriveUnit.cpp:144 (and read back as `p_mover->EIMCtrlType` in `process()`, :258); fiz: fiz_train_engine_common.gd:75 | down | `EIMCtrlType` 0..3, a numeric selector of Mover's lever model (`EimControlType` has no names, only numbers) |
| `cntrl_auto_relay_mode` | - | adapter: MaszynaMoverDriveUnit.cpp:145; fiz: fiz_train_engine_common.gd:91-92 | stays | whether the vehicle has an automatic starting relay (no / yes / optional, i.e. switchable from the cab): equipment |
| `cntrl_has_camshaft` | - | adapter: MaszynaMoverDriveUnit.cpp:146; fiz: fiz_train_engine_common.gd:62 | stays | whether the controller is a camshaft one: equipment |
| `cntrl_series_shunt_on_series_position` | - | adapter: MaszynaMoverDriveUnit.cpp:147; fiz: fiz_train_engine_common.gd:64 | down | `ScndS`, a flag in Mover's controller-stepping logic, not visible to the driver |
| `cntrl_fast_series_circuit` | - | adapter: MaszynaMoverDriveUnit.cpp:148; fiz: fiz_train_engine_common.gd:66 | down | `FSCircuit`, a flag in Mover's relay-stepping logic (stored as an int in the Mover) |

### RailVehicleElectricEngine

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `circuit_resistance` | - | adapter: MaszynaMoverCircuitUnit.cpp:91; fiz: fiz_train_electric_series_engine_parser.gd:77 | down | `CircuitRes`, a resistance term in Mover's current equation (also used by the adapter's ESMVelocity port) |
| `circuit_imax_low` | - | adapter: MaszynaMoverCircuitUnit.cpp:92; fiz: fiz_train_electric_series_engine_parser.gd:79 | stays | the overload relay's low current threshold, a rating; the live `circuit_imax` is published to the cab |
| `circuit_imax_high` | - | adapter: MaszynaMoverCircuitUnit.cpp:93; fiz: fiz_train_electric_series_engine_parser.gd:81 | stays | the overload relay's high threshold (high setting), a rating |
| `circuit_imin_low` | - | adapter: MaszynaMoverCircuitUnit.cpp:94; fiz: fiz_train_electric_series_engine_parser.gd:83, fiz_train_engine_parser.gd:73 (EZT default) | stays | the current the automatic start relay steps at (low setting): a relay setting of the vehicle |
| `circuit_imin_high` | - | adapter: MaszynaMoverCircuitUnit.cpp:95; fiz: fiz_train_electric_series_engine_parser.gd:85, fiz_train_engine_parser.gd:74 | stays | the start relay's high setting, which the reverser can switch to (see `direction_switches_circuit_imin_high`) |
| `circuit_tuhex_sum` | - | adapter: MaszynaMoverCircuitUnit.cpp:100; fiz: fiz_train_electric_series_engine_parser.gd:87 | down | TUHEX regulator coefficient, used only by Mover's TUHEX model |
| `circuit_tuhex_diff` | - | adapter: MaszynaMoverCircuitUnit.cpp:101; fiz: ...series_engine_parser.gd:89 | down | TUHEX regulator coefficient |
| `circuit_tuhex_min_current` | - | adapter: MaszynaMoverCircuitUnit.cpp:102; fiz: ...series_engine_parser.gd:93 | down | TUHEX `MinIw` |
| `circuit_tuhex_max_current` | - | adapter: MaszynaMoverCircuitUnit.cpp:103; fiz: ...series_engine_parser.gd:91 | down | TUHEX `MaxIw` |
| `circuit_tuhex_stages` | - | adapter: MaszynaMoverCircuitUnit.cpp:104; fiz: ...series_engine_parser.gd:95 | down | TUHEX stage count, a selector of Mover's TUHEX model |
| `circuit_tuhex_sum_1` | - | adapter: MaszynaMoverCircuitUnit.cpp:105; fiz: ...series_engine_parser.gd:97 | down | TUHEX per-stage coefficient |
| `circuit_tuhex_sum_2` | - | adapter: MaszynaMoverCircuitUnit.cpp:106; fiz: ...series_engine_parser.gd:99 | down | TUHEX per-stage coefficient |
| `circuit_tuhex_sum_3` | - | adapter: MaszynaMoverCircuitUnit.cpp:107; fiz: ...series_engine_parser.gd:101 | down | TUHEX per-stage coefficient |
| `cntrl_converter_overload_relay_start_mode` | - | adapter: MaszynaMoverCircuitUnit.cpp:109-110 | stays | how the converter overload relay is reset (manual / with converter...), cab-visible operation. No FIZ parser sets it: the default is always used |
| `cntrl_converter_overload_relay_off_when_main_is_off` | - | adapter: MaszynaMoverCircuitUnit.cpp:111-112 | unclear | a behaviour switch of Mover's relay logic (Mover.cpp:1874, 3361). Needs a domain call on whether this is real vehicle wiring or a Mover quirk. No FIZ parser sets it |
| `cntrl_main_switch_start_mode` | - | adapter: MaszynaMoverCircuitUnit.cpp:113-114 | stays | how the main switch is closed (manual/automatic); a driver and the AI need it. No FIZ parser sets it |

### RailVehicleElectricInductionEngine

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `slip_current_ratio` | - | adapter: MaszynaMoverRailVehicleElectricInductionEngine.cpp:51; fiz: fiz_train_electric_induction_engine_parser.gd:40 | down | `dfic`, an `eimc[]` coefficient of Mover's induction-motor model |
| `max_slip` | - | adapter: ...InductionEngine.cpp:52; fiz: ...induction_engine_parser.gd:42 | down | `dfmax`, an eimc coefficient |
| `pole_pairs` | - | adapter: ...InductionEngine.cpp:53; fiz: ...induction_engine_parser.gd:44 | stays | the motor's number of pole pairs, a physical fact of an induction motor (and what an inverter sound's frequency would need); today only eimc uses it |
| `nominal_uf_ratio` | - | adapter: ...InductionEngine.cpp:54; fiz: ...parser.gd:46 | down | `cfu`, an eimc coefficient |
| `current_torque_ratio` | - | adapter: ...InductionEngine.cpp:55; fiz: ...parser.gd:48 | down | `cim`, an eimc coefficient |
| `current_three_phase_ratio` | - | adapter: ...InductionEngine.cpp:56; fiz: ...parser.gd:50 | down | `icif`, an eimc coefficient |
| `max_supply_voltage` | - | adapter: ...InductionEngine.cpp:57; fiz: ...parser.gd:52 | down | `Uzmax`, the inverter-model voltage for full output (not the line rating, which is `current_collector_max_voltage`) |
| `max_supply_voltage_braking` | - | adapter: ...InductionEngine.cpp:58; fiz: ...parser.gd:54 | down | `Uzh`, its braking counterpart in eimc |
| `inverter_voltage_drop` | - | adapter: ...InductionEngine.cpp:59; fiz: ...parser.gd:56 | down | `DU`, an eimc coefficient |
| `no_load_current` | - | adapter: ...InductionEngine.cpp:60; fiz: ...parser.gd:58 | down | `I0`, an eimc coefficient |
| `inverter_uf_setpoint` | - | adapter: ...InductionEngine.cpp:61; fiz: ...parser.gd:60 | down | `fcfu`, an eimc coefficient |
| `inverter_uf_setpoint_braking` | - | adapter: ...InductionEngine.cpp:62; fiz: ...parser.gd:63-65 (falls back to fcfu) | down | `fcfuH`, an eimc coefficient |
| `initial_force` | - | adapter: ...InductionEngine.cpp:63; fiz: ...parser.gd:67 | stays | `F0`, the starting tractive effort, a rating of the vehicle |
| `force_drop_rate` | - | adapter: ...InductionEngine.cpp:64; fiz: ...parser.gd:69 | down | `a1`, the slope of eimc's force curve |
| `max_power` | - (the JSON's core hits are `RailVehicleSpeedControl`/`RailVehicleWWListItem`'s own `max_power`) | adapter: ...InductionEngine.cpp:65; fiz: ...parser.gd:71 | stays | `Pmax`, the maximum traction power, a rating |
| `max_braking_force` | - | adapter: ...InductionEngine.cpp:66; fiz: ...parser.gd:73 | stays | `Fh`, the maximum ED braking force, a rating |
| `max_braking_power` | - | adapter: ...InductionEngine.cpp:67; fiz: ...parser.gd:75 | stays | `Ph`, the maximum ED braking power, a rating |
| `braking_decay_velocity` | - | adapter: ...InductionEngine.cpp:68; fiz: ...parser.gd:81 | down | `Vh0`, a knot of eimc's ED fade-out curve (the parser notes the names read as swapped, :77-78) |
| `braking_decay_start_velocity` | - | adapter: ...InductionEngine.cpp:69; fiz: ...parser.gd:83 | down | `Vh1`, the other knot of the same curve |
| `motor_max_current` | - | adapter: ...InductionEngine.cpp:70; fiz: ...parser.gd:85 | stays | `Imax`, the motors' current limit, a rating |
| `nominal_voltage` | - (the JSON's core hits are TractionServer / scenery wire `nominal_voltage`) | adapter: ...InductionEngine.cpp:75; fiz: ...parser.gd:89 | stays | `Volt=`, the vehicle's nominal supply voltage |
| `electrodynamic_brake_cylinder_ratio` | - | adapter: ...InductionEngine.cpp:73; fiz: ...parser.gd:91 | down | `abed`, Mover's ED/pneumatic blending coefficient |
| `electrodynamic_ep_ratio` | - | adapter: ...InductionEngine.cpp:74; fiz: ...parser.gd:93 | down | `edep`, the blending coefficient; its Mover default 1.5 lives in the core header |
| `logarithmic_force_control` | - | adapter: ...InductionEngine.cpp:76; fiz: ...parser.gd:95 | down | `eimclf`, a selector of Mover's force-control law |
| `inverter_control_coupler_flag` | - | adapter: ...InductionEngine.cpp:77; fiz: ...parser.gd:97 | down | `InvCtrCplFlag`, a Mover coupler-flag mask (default 4, in the core header) |
| `flat_force_characteristic` | - | adapter: ...InductionEngine.cpp:78; fiz: ...parser.gd:99 | down | `Flat`, a selector of Mover's force characteristic |
| `max_power_table` | - | adapter: ...InductionEngine.cpp:88-96 (`EIM_Pmax_Table`); fiz: ...parser.gd:168 | down | a FIZ-shaped table feeding Mover's `EIM_Pmax_Table` |
| `wwlist` | - (the JSON's core hits are `RailVehicleDieselElectricEngine`'s own `wwlist`) | adapter: ...InductionEngine.cpp:104-115 (`DElist`); fiz: ...parser.gd:158-165, fiz_vehicle_builder.gd:110 | down | the `ffList:`/`ffBrakeList:` FIZ table, pushed into Mover's `DElist` |

### RailVehicleElectricSeriesEngine

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `nominal_voltage` | - (the JSON's core hits are TractionServer / scenery) | adapter: MaszynaMoverRailVehicleElectricSeriesEngine.cpp:15; fiz: fiz_train_electric_series_engine_parser.gd:35 | stays | `Volt=`, the vehicle's nominal supply voltage |
| `winding_resistance` | - | adapter: ...SeriesEngine.cpp:16 (and :115 in the ESMVelocity port); fiz: ...series_engine_parser.gd:38 (clamped to at least 0.01) | down | `WindingRes`, a resistance term in Mover's motor equation |
| `max_rpm` | `running_sound_model.gd:152, :169` (via `get_circuit_nmax_rpm()`, which returns Mover `nmax`*60 that this property sets) | adapter: ...SeriesEngine.cpp:17; fiz: ...series_engine_parser.gd:47 (the JSON's core hits in RailVehicleDieselEngine/RenderingServer are the diesel's own `get_max_rpm()`) | stays | the motor's maximum rpm; the core motor sound normalises its amplitude by it |
| `resistor_fan_type` | - | adapter: ...SeriesEngine.cpp:19; fiz: ...series_engine_parser.gd:110-111 | stays | whether the resistor fan exists and whether it is automatic (equipment) |
| `resistor_fan_max_rpm` | `running_sound_model.gd:189` | adapter: ...SeriesEngine.cpp:20, :75 (re-published to the dump from `RVentnmax`, no reader of that key); fiz: ...series_engine_parser.gd:114 | stays | the resistor fan's top speed; the core ventilator sound scales frequency and volume by it. Holds rev/s despite its name (the parser divides by 60) |
| `resistor_fan_cutoff_resistance` | - | adapter: ...SeriesEngine.cpp:21; fiz: ...parser.gd:116 | down | `RVentCutOff`, a term of Mover's fan-drive model |
| `resistor_fan_min_current` | - | adapter: ...SeriesEngine.cpp:22; fiz: ...parser.gd:118 | down | `RVentMinI`, a term of Mover's fan-drive model |
| `resistor_fan_speed` | - | adapter: ...SeriesEngine.cpp:23; fiz: ...parser.gd:120 | down | `RVentSpeed`, Mover's fan spin-up rate |
| `dynamic_brake_resistance` | - | adapter: ...SeriesEngine.cpp:24; fiz: ...parser.gd:122 | down | `DynBrakeRes`, a resistance term in Mover's ED-brake equation |
| `dynamic_brake_resistance_1` | - | adapter: ...SeriesEngine.cpp:25; fiz: ...parser.gd:124 | down | `DynBrakeRes1`, the same |
| `dynamic_brake_resistance_2` | - | adapter: ...SeriesEngine.cpp:26; fiz: ...parser.gd:126 | down | `DynBrakeRes2`, the same |
| `direction_switches_circuit_imin_high` | - (own dump: RailVehicleElectricSeriesEngine.cpp:60, :62 keys `direction_switches_circuit_imin_high`, `direction_position_max`) | cab: cabin_logic.gd:140, direction_key.gd (dump key); mmd: mmd_semantic_catalog.gd:117 (`direction_position_max`); fiz: fiz_vehicle_builder.gd:152 (EZT). The adapter does not read it | stays | the reverser has an extra "high start" position past forward: a cab fact the key's position count is built from |
| `relay_list` | - | adapter: ...SeriesEngine.cpp:30-51 (`RList`); fiz: ...parser.gd:180 | down | the FIZ `RList:` table (resistance, branches, motors per branch, shunt per position) in Mover's `TScheme` shape |

### RailVehicleEnginePowerSource

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `source_type` | `RailVehicleServer.cpp:888`, `mover_switches_power_source.gd:18, :22` | adapter: MaszynaMoverRailVehicleEnginePowerSource.cpp:53-54; fiz: fiz_train_power_parser.gd:35, :39 | stays | what feeds the traction (catenary, accumulator, ...). The server finds the trainset's current-collecting vehicle by it |
| `current_collector_number_of_collectors` | `RailVehicleServer.cpp:889` | adapter: ...EnginePowerSource.cpp:80; fiz: fiz_train_power_parser.gd:46, :66 | stays | the number of pantographs (equipment count) |
| `current_collector_max_voltage` | - (only the doc-comment example in `VehicleComponent.hpp:103`) | adapter: ...EnginePowerSource.cpp:86, :94; fiz: fiz_train_power_parser.gd:51 | stays | `MaxV`, the collector's maximum accepted voltage, a rating |
| `current_collector_max_current` | - | adapter: ...EnginePowerSource.cpp:95; fiz: fiz_train_power_parser.gd:52 | stays | `MaxCurrent`, a rating |
| `current_collector_min_collector_lifting` | - | adapter: ...EnginePowerSource.cpp:81; fiz: fiz_train_power_parser.gd:47 | stays | `MinH`, the bottom of the pantograph's working range (geometry). The Mover never reads it (MOVER.h:523) |
| `current_collector_max_collector_lifting` | - | adapter: ...EnginePowerSource.cpp:82; fiz: fiz_train_power_parser.gd:48 | stays | `MaxH`, the top of that range; same remark |
| `current_collector_sliding_width` | `RailVehicleServer.cpp:2044`, `track_traction_panel.gd:99` | adapter: ...EnginePowerSource.cpp:83; fiz: fiz_train_power_parser.gd:49 | stays | `CSW`, the slider's width; the core contact search uses half of it |
| `current_collector_pantograph_type` (missing from JSON) | `RailVehicleRenderingServer.cpp:1135` | adapter: not read; fiz: fiz_train_power_parser.gd:50 | stays | `PantType`, the pantograph model whose arm dimensions the renderer draws |
| `current_collector_min_main_switch_voltage` | - | adapter: ...EnginePowerSource.cpp:87; fiz: fiz_train_power_parser.gd:54 | stays | `MinV`, the no-voltage threshold for the main switch, a relay setting |
| `current_collector_min_pantograph_tank_pressure` | - | adapter: ...EnginePowerSource.cpp:89; fiz: fiz_train_power_parser.gd:58 | stays | `MinPress`, the pressure the pantograph pressure switch needs, a rating (the startup test reads the live getter) |
| `current_collector_max_pantograph_tank_pressure` | - | adapter: ...EnginePowerSource.cpp:90; fiz: fiz_train_power_parser.gd:60 | stays | `MaxPress`, the pressure behind the reducer |
| `current_collector_overvoltage_relay` | - | adapter: ...EnginePowerSource.cpp:91; fiz: fiz_train_power_parser.gd:53 | stays | `OverVoltProt`: whether an overvoltage relay is fitted (equipment) |
| `current_collector_required_main_switch_voltage` | - | adapter: ...EnginePowerSource.cpp:88; fiz: fiz_train_power_parser.gd:56 | stays | `InsetV`, the voltage needed to close the main switch. Nothing in the vendored Mover reads it |
| `current_collector_fake_power` | - | adapter: ...EnginePowerSource.cpp:92; fiz: fiz_train_power_parser.gd:62 | down | `FakePower`, a MaSzyna simulator switch (power without a real contact), not a vehicle fact. Nothing in the vendored Mover reads it |
| `current_collector_physical_layout` | - | adapter: ...EnginePowerSource.cpp:93; fiz: fiz_train_power_parser.gd:63 | stays | which pantographs physically exist (front/rear flags, used at Mover.cpp:2167) |
| `transducer_input_voltage` | - (own state dump: RailVehicleEnginePowerSource.cpp:299) | adapter: ...EnginePowerSource.cpp:60; fiz: fiz_train_power_parser.gd:40 | stays | the transducer's input voltage, a rating |
| `accumulator_recharge_source` | - (own state dump: RailVehicleEnginePowerSource.cpp:265) | adapter: ...EnginePowerSource.cpp:71. No FIZ parser sets it | stays | what recharges the accumulator, a vehicle fact |
| `power_cable_source` | - (own state dump: RailVehicleEnginePowerSource.cpp:301) | adapter: ...EnginePowerSource.cpp:56, :100; fiz: fiz_train_power_parser.gd:41 | stays | `PowerTrans`, what the power cable carries |
| `power_cable_steam_pressure` | - (own state dump: RailVehicleEnginePowerSource.cpp:302) | adapter: ...EnginePowerSource.cpp:102; fiz: fiz_train_power_parser.gd:42 | stays | the steam pressure the cable/pipe brings |
| `cntrl_pantograph_compressor_start_mode` | - | adapter: ...EnginePowerSource.cpp:111-112; fiz: fiz_train_power_parser.gd:74 | stays | how the pantograph compressor is started, a cab operation fact |
| `cntrl_pantograph_auto_valve` | - | adapter: ...EnginePowerSource.cpp:113; fiz: fiz_train_power_parser.gd:77 | stays | `PantAutoValve`: whether an automatic pantograph valve is fitted (equipment) |
| `cntrl_pantographs_valve_start_mode` | - | adapter: ...EnginePowerSource.cpp:114-115; fiz: fiz_train_power_parser.gd:79 | stays | how the master pantograph valve is operated |
| `cntrl_pantographs_valve_spring` | - | adapter: ...EnginePowerSource.cpp:116; fiz: fiz_train_power_parser.gd:82 | stays | whether the master valve springs back (impulse vs latching) |
| `cntrl_pantograph_valve_start_mode` | - | adapter: ...EnginePowerSource.cpp:118-119; fiz: fiz_train_power_parser.gd:84 | stays | how each pantograph's own valve is operated |
| `cntrl_pantograph_valve_spring` | - | adapter: ...EnginePowerSource.cpp:120; fiz: fiz_train_power_parser.gd:87 | stays | whether a pantograph valve springs back |
| `cntrl_pantograph_valve_solenoid` | - | adapter: ...EnginePowerSource.cpp:121; fiz: fiz_train_power_parser.gd:89 | stays | whether a pantograph valve is solenoid-driven |

### RailVehicleSpeedControl

No core file other than the class's own reads any of these; the JSON's core hits for `delay`, `offset`, `min_velocity`, `max_velocity`, `max_power` are unrelated members of the same name. The plugin cab reads only the live state keys (`speed_control_*`, python_screen_state.gd:33-37, mmd_semantic_catalog.gd:995ff).

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `speed_control_enabled` | - | adapter: MaszynaMoverRailVehicleSpeedControl.cpp:22; fiz: fiz_train_speed_control_parser.gd:21 | stays | whether the vehicle has speed control (equipment) |
| `delay` | - | adapter: ...SpeedControl.cpp:32; fiz: ...parser.gd:23 | stays | `SpeedCtrlDelay`, how long the impulse lever must be held before the set speed changes (Mover.cpp:5641): cab lever timing |
| `impulse_lever` | - | adapter: ...SpeedControl.cpp:33; fiz: ...parser.gd:25 | stays | `SpeedCtrlType=Time`: the speed is set with an impulse lever (cab fact) |
| `disables_on` | - | adapter: ...SpeedControl.cpp:34; fiz: ...parser.gd:27 | stays | what switches speed control off (master controller movement, braking): driver-facing behaviour |
| `preset_speeds` | - | adapter: ...SpeedControl.cpp:36-45; fiz: ...parser.gd:34 | stays | the speeds the cab's preset buttons select |
| `override_manual_power` | - | adapter: ...SpeedControl.cpp:48; fiz: ...parser.gd:36 | unclear | `ManualStateOverride`: decides whether speed control or the manual controller wins (Mover.cpp:2615, 6797). Real behaviour or a quirk of Mover's combination rule: needs a domain call |
| `initial_power` | - | adapter: ...SpeedControl.cpp:49; fiz: ...parser.gd:38 | down | `InitPwr`, the power cap below `full_power_velocity` in Mover's regulator law (Mover.cpp:6830) |
| `full_power_velocity` | - | adapter: ...SpeedControl.cpp:50; fiz: ...parser.gd:40 | down | `MaxPwrVel`, a knot of the regulator law (Mover.cpp:6829) |
| `start_velocity` | - | adapter: ...SpeedControl.cpp:51; fiz: ...parser.gd:42 | down | `StartVel`, a knot of the regulator law (Mover.cpp:6832) |
| `velocity_step` | - | adapter: ...SpeedControl.cpp:52; fiz: ...parser.gd:44 | stays | how much one press changes the set speed (cab fact, Mover.cpp:6870) |
| `power_step` | - | adapter: ...SpeedControl.cpp:53; fiz: ...parser.gd:46 | stays | how much one press changes the requested power (cab fact) |
| `min_power` | - | adapter: ...SpeedControl.cpp:54; fiz: ...parser.gd:48 | stays | the lower end of the power the driver can request |
| `max_power` | - | adapter: ...SpeedControl.cpp:55; fiz: ...parser.gd:50 | stays | the upper end of that range |
| `min_velocity` | - | adapter: ...SpeedControl.cpp:56; fiz: ...parser.gd:52 | stays | the lowest speed that can be set |
| `max_velocity` | - | adapter: ...SpeedControl.cpp:57; fiz: ...parser.gd:54 | stays | the highest speed that can be set |
| `offset` | - | adapter: ...SpeedControl.cpp:58; fiz: ...parser.gd:56 | down | the regulator's error offset (Mover.cpp:6801) |
| `proportional_gain_positive` | - | adapter: ...SpeedControl.cpp:59; fiz: ...parser.gd:58 | down | `kPpos`, a PI gain |
| `proportional_gain_negative` | - | adapter: ...SpeedControl.cpp:60; fiz: ...parser.gd:60 | down | `kPneg`, a PI gain |
| `integral_gain_positive` | - | adapter: ...SpeedControl.cpp:61; fiz: ...parser.gd:62 | down | `kIpos`, a PI gain |
| `integral_gain_negative` | - | adapter: ...SpeedControl.cpp:62; fiz: ...parser.gd:64 | down | `kIneg`, a PI gain |
| `brake_intervention` | - | adapter: ...SpeedControl.cpp:63; fiz: ...parser.gd:66 | stays | whether speed control may brake (a feature of the vehicle) |
| `brake_intervention_max_velocity` | - | adapter: ...SpeedControl.cpp:64; fiz: ...parser.gd:68 | down | `BrakeIntMaxVel`, a threshold inside the regulator law (Mover.cpp:6805) |
| `power_up_speed` | - | adapter: ...SpeedControl.cpp:65; fiz: ...parser.gd:70 | down | the regulator output's ramp rate (Mover.cpp:6828) |
| `power_down_speed` | - | adapter: ...SpeedControl.cpp:66; fiz: ...parser.gd:72 | down | the regulator output's ramp rate |

### RailVehicleUniversalController

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `integrated_brake_pn` | - | adapter: MaszynaMoverRailVehicleUniversalController.cpp:15; fiz: fiz_train_universal_controller_parser.gd:29 | stays | the combined lever also works the pneumatic brake (equipment) |
| `integrated_brake` | - | adapter: ...UniversalController.cpp:16; fiz: ...parser.gd:31 | stays | the combined lever also works the train brake |
| `integrated_local_brake` | - | adapter: ...UniversalController.cpp:17; fiz: ...parser.gd:33 | stays | the combined lever's braking positions work the local brake |
| `selector_position` | `mover_switches_controller.gd:16` | adapter: ...UniversalController.cpp:50 (only republishes it as the state key `selector_position`) | unclear | runtime state stored as a configuration property. No FIZ parser sets it and nothing writes it to the Mover, despite the test name "forwarded to mover". Needs a decision to make it live state or remove it, not a config split |
| `positions` | - (the JSON's core hits are other classes' own `positions`) | adapter: ...UniversalController.cpp:19-41 (`UniCtrlList`); fiz: ...parser.gd:60 | down | the FIZ `UCList:` table in Mover's `UniCtrlList` shape |

### RailVehicleUniversalControllerListItem

All are columns of the `UCList:` row, read only by the adapter (MaszynaMoverRailVehicleUniversalController.cpp:31-39) and written only by fiz_train_universal_controller_parser.gd:42-50. They go with `positions`.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `pneumatic_brake_position` | - | adapter: ...UniversalController.cpp:31; fiz: ...parser.gd:42 | down | `UniCtrlList[].mode`, a column of the UCList row |
| `min_percentage` | - | adapter: :32; fiz: :43 | down | `MinCtrlVal`, a UCList column |
| `max_percentage` | - | adapter: :33; fiz: :44 | down | `MaxCtrlVal`, a UCList column |
| `target_value` | - | adapter: :34; fiz: :45 | down | `SetCtrlVal`, a UCList column |
| `increase_speed` | - | adapter: :35; fiz: :46 | down | `SpeedUp`, a UCList column (ramp rate) |
| `decrease_speed` | - | adapter: :36; fiz: :47 | down | `SpeedDown`, a UCList column |
| `bounce_back_position` | - | adapter: :37; fiz: :48 | down | `ReturnPosition`, a UCList column |
| `nearest_stable_down` | - | adapter: :39; fiz: :50 | down | `PrevPosFastDec`, a UCList column |
| `nearest_stable_up` | - | adapter: :38; fiz: :49 | down | `NextPosFastInc`, a UCList column |

### RailVehicleSwitches

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `pantograph_impulse` | - (own dump: RailVehicleSwitches.cpp:66 key `pantograph_switch_impulse`) | adapter: MaszynaMoverRailVehicleSwitches.cpp:14; cab: pantographs.gd:68 (dump key); mmd: mmd_semantic_catalog.gd:1392, :1403; fiz: fiz_train_switches_parser.gd:47 | stays | whether the cab's pantograph switches are impulse (monostable) ones |
| `converter_impulse` | - (own dump: RailVehicleSwitches.cpp:68 key `converter_switch_impulse`) | adapter: ...Switches.cpp:15; cab: cabin_logic.gd:119; mmd: mmd_semantic_catalog.gd:1232; fiz: ...parser.gd:49 | stays | whether the converter switch is an impulse one |
| `motor_connectors_impulse` | - (own dump: RailVehicleSwitches.cpp:70 key `motor_connectors_switch_impulse`) | adapter: ...Switches.cpp:16; mmd: mmd_semantic_catalog.gd:290; fiz: ...parser.gd:52 | stays | whether the line contactor switch is an impulse or a toggle one |
| `relay_reset_button_1` | - | adapter: ...Switches.cpp:18; fiz: ...parser.gd:54 (mmd uses only the `RELAY_RESET_BUTTON_1` enum as a command argument) | stays | which relays the cab's reset button 1 resets (cab wiring) |
| `relay_reset_button_2` | - | adapter: ...Switches.cpp:19; fiz: ...parser.gd:56 | stays | same, button 2 |
| `relay_reset_button_3` | - | adapter: ...Switches.cpp:20; fiz: ...parser.gd:58 | stays | same, button 3 |
| `pantograph_presets` | - | adapter: ...Switches.cpp:22-25, :35 (dump key `pantograph_preset_max`); mmd: mmd_semantic_catalog.gd:2138 (that key); fiz: ...parser.gd:65. The JSON's `cabin_logic.gd` hit is a local variable | stays | the positions of the cab's pantograph selector |
| `pantograph_preset_default` | - | fiz: ...parser.gd:67 only | unclear | no reader at all, and the header calls it "no key of the original". Dead: decide between wiring it to the selector's start position and removing it |
| `modern_dimmer` | - | fiz: ...parser.gd:69 only | stays | the cab's headlight dimmer type; not ported yet (TODO.md), no consumer |
| `dimmer_list_cycle` | - | fiz: ...parser.gd:36 only | stays | whether the dimmer selector wraps around; no consumer |
| `dimmer_list_default_position` | - | fiz: ...parser.gd:38 only | stays | the dimmer's start position; no consumer |
| `dimmer_list_positions` | - | fiz: ...parser.gd:94 only | stays | what each dimmer position lights (high beam / dimmed / off, `RailVehicleDimmerListItem`), a cab fact; no consumer |

#### Notes

* **Enums that are Mover model selectors**
  * `RailVehicleEngine::EimControlType` (`EIM_CONTROL_TYPE_0..3`): Mover's `EIMCtrlType` under bare numbers. It has no core use (only the adapter, the FIZ parser and one test). It should go down with `cntrl_eim_control_type`. The adapter also branches on it in `MaszynaMoverDriveUnit::process()` (`p_mover->EIMCtrlType > 0`).
  * `RailVehicleEngine::EngineType` mirrors Mover's `TEngineType`, including `DUMB`, `WHEELS_DRIVEN` and `MAIN`, which have no engine class (the FIZ engine parser warns "no engine class for it yet"). Its core uses are `RailVehicleRenderingServer.cpp:1661-1662`, `running_sound_model.gd:154-165`, and `RailVehicleLighting.hpp:73` / `RailVehicleHeating.hpp:47`, where `MAIN` is the generator-engine default. The engine kinds themselves are a fact, but `MAIN` and `DUMB` are Mover vocabulary sitting in the core enum. The header says it is "what the simulation keys its own configuration off" (RailVehicleEngine.hpp:171-172).
  * `RailVehicleEngine::AutoRelayMode`, `RailVehicleElectricSeriesEngine::FanType` (`RVent=`) and `RailVehicleEnginePowerSource::PantographType` describe equipment, not models. `PantographType` is read by the core renderer.
  * `RailVehicleEnginePowerSource::ValveOperation` is Mover's `operation_t` (MOVER.h:177) used as a command argument of the core API (`pantographs_valve_operate`, `pantograph_valve_operate`). It is not configuration, but it is Mover command vocabulary in the core interface. Only the plugin cab uses it (pantographs.gd, pantograph_selected.gd, pantograph_presets.gd).
  * The `StartMode` used by `motor_blowers_start_mode` and all `cntrl_*_start_mode` properties belongs to `RailVehicleController`, not to these classes. Its `Converter`, `Battery` and `Direction` values are Mover `start_t` values (for the owner of that class to judge).
* **Model code in `RailVehicleEngine.cpp` (312 lines)**: there is no Mover computation in it. Every getter from line 8 to 89 is a null-guarded delegation to `drive_unit`, and so are `main_switch`/`motor_*` (251-283) and `_apply_configuration` (91-96). The only logic:
  * `get_transmission_ratio()` (41-45): wheel teeth over motor teeth, 1.0 without a gear. This is the derivation from `LoadFIZ_Engine`; it is a fact derivation and both the core sound and the adapter use it.
  * `_do_process_component()` (201-215): calls `drive_unit->process()` (where the adapter runs the DynObj EIMIC step) and emits `engine_start`/`engine_stop` on main-switch edges. This is the original's meaning of "engine started", per the comment at 201.
  * `_fill_config_dictionary()` (104-110): `main_init_time` plus the drive unit's `transmission_ratio`.
  * The core API surface is Mover-shaped live state, not model code: `get_circuit_nmax_rpm`, `get_circuit_rlist_size` (RlistSize), `get_eimic_real`, `get_main_no_power_pos`, `get_current0/1/2` (ShowCurrent's ammeter indices), `get_relay_novolt/overvoltage/ground`. The state keys `engine_rpm_count`, `circuit_rlist_size` and so on are published from 218-249.
* **Model code in `RailVehicleElectricEngine.cpp` (209 lines)**: none. Every getter and command (13-51, 139-191) delegates to `circuit_unit` or `traction_motors_unit`; `_fill_state_dictionary` is at 111-136. The header carries Mover defaults ("defaults of MOVER.h:1740-1744, 1762-1769", RailVehicleElectricEngine.hpp:56-69: TUHEX 750/10/60/400). The indicator state keys `indicators_*` (contactors, diff relay, resistors, vent overload, high current, main breaker) are named after the original cab's indicator lamps.
* **Other ports or Mover facts in the core files**
  * `RailVehicleElectricSeriesEngine.cpp:62`: `direction_position_max = 1 + direction_switches_circuit_imin_high`, a cab-key rule ported from Train.cpp:9451-9458.
  * `RailVehicleSwitches.cpp:65-71`: publishes the impulse flags under different keys (`*_switch_impulse`), which is why a property-name grep misses the cab and MMD readers.
  * `RailVehicleEnginePowerSource.cpp:11-25`: pantograph up/down sound signals ported from DynObj.cpp:3881-4036.
  * `RailVehicleElectricInductionEngine.hpp:15, 17`: Mover defaults `DEFAULT_ELECTRODYNAMIC_EP_RATIO = 1.5` and `DEFAULT_INVERTER_CONTROL_COUPLER_FLAG = 4`, which go down with their properties.
  * `RailVehicleSpeedControl.hpp:45-69`: all defaults copied from `LoadFIZ_SpeedControl`/MOVER.h.
* **Surprises**
  * The adapters of `RailVehicleEngine` and `RailVehicleElectricEngine` are composition units, not subclasses. "Down" for their properties needs a target to be designed first: either move the properties onto each concrete Mover engine adapter, or give the units their own configuration objects.
  * `RailVehicleEnginePowerSource` keeps every collector value twice: once as a config property (`current_collector_max_voltage`) and once as an abstract live getter (`get_collector_max_voltage`, which the adapter answers from Mover's `CollectorParameters`). The state dump publishes the live copy under the property's own name (`current_collector_max_voltage`, RailVehicleEnginePowerSource.cpp:267).
  * `current_collector_min/max_collector_lifting`, `required_main_switch_voltage` (InsetV) and `fake_power` reach the Mover, but nothing in the vendored Mover reads them. MOVER.h:523 says of MaxH "nigdzie nie używany" ("not used anywhere").
  * No FIZ parser sets `cntrl_converter_overload_relay_start_mode`, `cntrl_converter_overload_relay_off_when_main_is_off`, `cntrl_main_switch_start_mode` or `accumulator_recharge_source`: they are always at their compiled defaults.
  * `resistor_fan_max_rpm` holds rev/s: the parser divides `RVentnmax` by 60 at fiz_train_electric_series_engine_parser.gd:114. The adapter also re-dumps it from the Mover under the same key, which no one reads.
  * `selector_position` (universal controller) is state disguised as configuration and never reaches the Mover. The core debug HUD reads it.
  * `maximum_traction_force` is the one engine property read by a second adapter (`MaszynaMoverRailVehicleBuffCoupl.cpp:151`, bare-coupler sizing).
* **No ALARM rows.** Every core reader in these classes reads a fact: the teeth via the transmission ratio, the series `max_rpm` via `get_circuit_nmax_rpm`, `resistor_fan_max_rpm`, `source_type`, `current_collector_number_of_collectors`, `current_collector_sliding_width` and `current_collector_pantograph_type`. The one exception is `selector_position`, which is runtime state rather than Mover model and is marked unclear.

## Body, controls and equipment components

Readers were checked by grepping `get_/set_/is_<name>`, `"<name>"`/`&"<name>"`, the dump keys, and `.name` in GDScript over `src/`, `addons/libreloaded/`, `plugins/` (not counting the vendored `maszyna-mover`) and `demo/tests`. False positives from the starting table are left out: `type`, `voltage`, `angle`, `positions`, `off`, `active`, `error`, `allow`, `data_path`, `model_filename`, `skins`, `doors`, `mirrors` all match other classes. "(own)" marks core logic inside the property's own class file. It does not count as a core reader under the rule, but it is listed because it is core behaviour that uses the value. "writer" marks code that only sets the value.

### RailVehicleController

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `coupler_adapter_model` | `RailVehicleRenderingServer.cpp:361` (through the fitted copy, `RailVehicleServer.cpp:919`) | adapter (copies it to the neighbour, `MaszynaMoverRailVehicleController.cpp:394`); mmd writer: `maszyna_vehicle_system.gd:285` (from MMD) | stays | the model of the coupler adapter this vehicle lends, drawn by the core renderer; it is drawing data and belongs next to `RailVehicleAppearance` |
| `coupler_adapter_length` | `RailVehicleServer.cpp:651` | adapter (`TCoupling::adapter_length`); mmd writer: `maszyna_vehicle_system.gd:286` | stays | the adapter's length, used by the core trainset layout to space the vehicles |
| `coupler_adapter_height` | - | adapter; mmd writer: `maszyna_vehicle_system.gd:287` | stays | the adapter's height, a dimension |
| `train_type` | `RailVehicleServer.cpp:867`, `RailVehicleServer.cpp:2045` | adapter (`mover->TrainType`, dump `train_type`); fiz: `fiz_train_controller_parser.gd`, plus defaults in `fiz_vehicle_builder.gd`, `fiz_train_engine_parser.gd`, `fiz_train_engine_common.gd` | down (done: `unit_coupling`, `current_collector_pantograph_rising_pressure`) | Mover's `TrainType` selector (EZT, ET41, ET42, ET22, SN61, EP05, ET40, 181, DMU picks hard-coded branches in Mover.cpp). The core uses it for the "is this a multiple unit" checks: the powered-car search at :867 and the EMU pantograph at :2045 |
| `type_name` | - (core writer: `RailVehicleServer.cpp:455`, `:982`) | adapter (passed to the Mover as `TypeName`) | stays | the vehicle type's name, set by the core server from the placement |
| `load_name` | `RailVehicleRenderingServer.cpp:1894` (core writer: `RailVehicleServer.cpp:471`, `:983`) | adapter (`AssignLoad`) | stays | what the scenery put on the vehicle; the core renderer matches it against `accepted_loads` to draw the load |
| `load_amount` | `RailVehicleRenderingServer.cpp:1898` (core writer: `RailVehicleServer.cpp:472`, `:984`) | adapter (`AssignLoad`) | stays | how much is loaded; the renderer uses it for the load model's fill height |
| `reduced_mass` | - | adapter (`Mred`); fiz: `fiz_train_controller_parser.gd` (`Mred`) | stays | the rotating parts' equivalent mass, a mass fact any longitudinal model needs |
| `sand_capacity` | - | adapter (`SandCapacity`); fiz (`SandCap`) | stays | the sand box's capacity |
| `heating_power` | - | adapter (`HeatingPower`); fiz (`HeatingP`) | stays | the power the train heating draws, a rating. The state key `heating_power` is a different value (`RailVehicleHeating` live power) |
| `light_power` | - | adapter (`LightPower`); fiz (`LightP`) | stays | the power the lighting draws, a rating. The state key `light_power` is Lighting's live power |
| `cntrl_ground_relay_start_mode` | - | adapter (`GroundRelayStart` via `start_mode_to_mover`); fiz: `fiz_train_controller_parser.gd` (`GroundRelayStart`) | down | `StartMode` selector of Mover's relay logic, a FIZ `...Start=` key |
| `cntrl_compartment_lights_start_mode` | - | adapter (`CompartmentLightsStart`); fiz (`CompartmentLightsStart`) | down | `StartMode` selector, a FIZ `...Start=` key; the absent-key default is the Mover's "legacy behaviour" |
| `cntrl_automatic_cab_activation` | - | adapter (`AutomaticCabActivation`); fiz | down | a Mover flag: whether the bridged control follows a cab change by itself (Mover.cpp:2673, 2714) |
| `cntrl_inactive_cab_flag` | - | adapter (`InactiveCabFlag`); fiz (`InactiveCabFlag`) | down | a Mover bitmask of what `CabDeactivisation` does (emergency brake, mirrors, pantograph, spring brake, direction) |

### RailVehicleDoors

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `type` | `RailVehicleRenderingServer.cpp:1427` | adapter (`Doors.type`); fiz: `fiz_train_doors_parser.gd` | stays | the door kind (shift, rotate, fold, plug), drawn by the renderer |
| `open_time` | - | adapter (`auto_duration`); fiz (`DoorStayOpen`) | stays | how long automatic doors stay open |
| `open_speed` | - | adapter (`open_rate`); fiz (`OpenSpeed`) | stays | the door leaf's opening speed (the tests derive dwell times from it) |
| `close_speed` | - | adapter (`close_rate`); fiz (`CloseSpeed`) | stays | the door leaf's closing speed |
| `max_shift` | - | adapter (`Doors.range`, normalises the positions); fiz (`DoorMaxShiftL/R`) | stays | how far a door leaf travels |
| `open_method` | - | adapter (`open_control`); cabin: `doors.gd`; fiz (`OpenCtrl`) | stays | who opens the doors (passenger, automatic, driver, conductor, mixed) |
| `close_method` | `RailVehicleServer.cpp:1403`, `:1412` | adapter (`close_control`); cabin: `doors.gd`; fiz (`CloseCtrl`) | stays | who closes the doors; the core trainset doorway query uses it |
| `voltage` | - | adapter (`Doors.voltage`); fiz (`DoorVoltage`) | stays | the door circuit's voltage |
| `close_warning` | - | adapter (`has_warning`); fiz (`DoorClosureWarning`) | stays | the doors have a closing warning |
| `close_auto_close_warning` | - | adapter (`has_autowarning`); cabin: `doors.gd`; fiz (`DoorClosureWarningAuto`) | stays | the warning sounds when the doors close automatically too |
| `open_delay` | - | adapter (`Doors.open_delay`); fiz (`DoorOpenDelay`) | stays | the delay before the doors start to open |
| `close_delay` | - | adapter (`Doors.close_delay`); fiz (`DoorCloseDelay`) | stays | the delay before the doors start to close |
| `open_with_permit` | - | adapter (`DoorsOpenWithPermitAfter`); fiz (`DoorOpenWithPermit`) | stays | the doors open remotely after the permit button is held this long |
| `has_lock` | - | adapter (`Doors.has_lock`); fiz (`DoorBlocked`) | stays | the vehicle has a door lock |
| `max_shift_plug` | `RailVehicleRenderingServer.cpp:1422` | adapter (`range_out`); fiz (`DoorMaxShiftPlug`) | stays | how far a plug door moves out, drawn by the renderer |
| `permit_list` | - | adapter (`permit_presets`, dump `doors_permit_preset_max`); mmd: `mmd_semantic_catalog.gd` (by dump key); fiz (`DoorPermitList`) | stays | the positions of the cab's permit preset switch. The values are FIZ side codes |
| `permit_default` | - | adapter (`permit_preset`); fiz (`DoorPermitListDefault`) | stays | the preset switch's starting position |
| `close_auto_close_remote` | - | adapter (`auto_include_remote`); fiz (`DoorAutoCloseRemote`) | stays | automatic closing also covers remotely opened doors |
| `close_auto_close_velocity` | - | adapter (`auto_velocity`); fiz (`DoorAutoCloseVel`) | stays | the speed above which the doors close by themselves |
| `platform_max_speed` | - | fiz (`PlatformMaxSpeed`) only | unclear | parsed but never applied or read, and the vendored MOVER.h has no such field. Needs the original key's meaning, or removal |
| `platform_type` | `RailVehicleRenderingServer.cpp:1453` | adapter (`step_type`); fiz | stays | the door step slides or turns, drawn by the renderer |
| `platform_max_shift` | `RailVehicleRenderingServer.cpp:1449` | adapter (`step_range`); fiz (`PlatformMaxShift`) | stays | how far the door step travels, drawn by the renderer |
| `platform_speed` | - | adapter (`step_rate`); fiz (`PlatformSpeed`) | stays | the door step's speed |
| `mirror_max_shift` | `RailVehicleRenderingServer.cpp:1388` | fiz (`MirrorMaxShift`) | stays | how far a mirror swings [deg], drawn by the renderer |
| `mirror_close_velocity` | `RailVehicleDoors.cpp:163` (own) | fiz (`MirrorVelClose`) | stays | the speed above which the mirrors fold; core doors logic uses it |
| `permit_required` | `RailVehicleServer.cpp:1421` | adapter (`permit_needed`); fiz (`DoorNeedPermit`) | stays | the doors need the driver's permit; the core trainset permit query uses it |
| `permit_light_blinking` | - | adapter (`DoorsPermitLightBlinking`); cabin: `door_permits.gd`; fiz (`DoorsPermitLightBlinking`) | stays | how the cab's permit lamp behaves, read by the cab lamp code |

### RailVehicleAppearance

A Resource, not a component, so it has no dump. Every value is written by the plugin's `maszyna_rail_vehicle_3d_instancer.gd` from MMD/E3D. `RailVehicle3D.cpp:229-241` writes the bogie, wheel, pantograph, wiper and mirror names and the transform for a vehicle assembled by hand, which shows the core uses these names without MaSzyna data.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `data_path` | `RailVehicleRenderingServer.cpp:239`, `:627`, `:854`, `:1785` | writer: vehicle instancer | stays | the directory the vehicle's model files are in, loaded by the core renderer |
| `model_filename` | `RailVehicleRenderingServer.cpp:619`, `:629`, `:888`, `:1056`, `:1723`, `:1785`, `:1826` | vehicle instancer (also reads it to load the E3D) | stays | the exterior model |
| `low_poly_model_filename` | `RailVehicleRenderingServer.cpp:630`, `:891` | writer: vehicle instancer | stays | the low-poly interior seen through the windows |
| `passengers_model_filename` | `RailVehicleRenderingServer.cpp:631`, `:894` | writer: vehicle instancer | stays | the passengers model |
| `attachment_model_filenames` | `RailVehicleRenderingServer.cpp:628`, `:897` | writer: vehicle instancer (MMD `attachments:`) | stays | extra models drawn with the exterior |
| `skins` | `RailVehicleRenderingServer.cpp:241`, `:888`, `:891`, `:898`, `:944` | writer: vehicle instancer | stays | the textures for the model's replaceable materials (numbered the MaSzyna way) |
| `model_transform` | `RailVehicleRenderingServer.cpp:880`, `:882`, `:986`, `:1113` | writer: vehicle instancer | stays | where the models sit in the vehicle frame |
| `front_bogie` | `RailVehicleRenderingServer.cpp:987` | writer: vehicle instancer | stays | the submodel that turns as the front bogie |
| `rear_bogie` | `RailVehicleRenderingServer.cpp:988` | writer: vehicle instancer | stays | the rear bogie's submodel |
| `front_rolling_wheels` | `RailVehicleRenderingServer.cpp:989` | writer: vehicle instancer | stays | the front rolling wheels' submodels |
| `powered_wheels` | `RailVehicleRenderingServer.cpp:990` | writer: vehicle instancer | stays | the powered wheels' submodels |
| `rear_rolling_wheels` | `RailVehicleRenderingServer.cpp:991` | writer: vehicle instancer | stays | the rear rolling wheels' submodels |
| `pantograph_front_arms` | `RailVehicleRenderingServer.cpp:993`, `:1107` | writer: vehicle instancer | stays | the first pantograph's arm submodels (5 slots, TAnimPant layout) |
| `pantograph_rear_arms` | `RailVehicleRenderingServer.cpp:995`, `:1108` | writer: vehicle instancer | stays | the second pantograph's arm submodels |
| `wiper_arms` | `RailVehicleRenderingServer.cpp:996` | vehicle instancer (also sets `RailVehicleWipers.wiper_count` from its size) | stays | the wiper arm and blade submodels (3 per wiper) |
| `mirrors` | `RailVehicleRenderingServer.cpp:997` | writer: vehicle instancer | stays | the mirror submodels |
| `doors` | `RailVehicleRenderingServer.cpp:998` | writer: vehicle instancer | stays | the door submodels (3 per door, odd left, even right) |
| `pantograph_factors` | `RailVehicleRenderingServer.cpp:1181` | writer: vehicle instancer (MMD `pantfactors:`) | plugin-data | the raw 4-number MMD `pantfactors:` fallback for an E3D the renderer cannot measure. The fact behind it (pantograph place and slider height) already goes to the core as `vehicle_set_pantograph_geometry`, so the MaSzyna instancer could work it out itself |
| `pendulums` | `RailVehicleRenderingServer.cpp:1000` | writer: vehicle instancer | stays | submodels that swing with the engine |
| `pendulum_amplitude` | `RailVehicleRenderingServer.cpp:1001` | writer: vehicle instancer | stays | how far those submodels swing [deg] |
| `door_steps` | `RailVehicleRenderingServer.cpp:999` | writer: vehicle instancer | stays | the door step submodels |
| `head_display_submodel` | `RailVehicleRenderingServer.cpp:738` | writer: vehicle instancer | stays | the submodel the head display material is drawn on |
| `low_poly_emission_energy` | `RailVehicleRenderingServer.cpp:2108` | - | stays | how bright the low-poly interior glows (the wrapper's own value; nobody writes it) |
| `low_poly_emission_fade_time` | `RailVehicleRenderingServer.cpp:2109` | - | stays | how fast that glow follows the cab light (the wrapper's own) |
| `joint_cabs` | `RailVehicleRenderingServer.cpp:1868`, `:2111` | writer: vehicle instancer (MMD `jointcabs:`) | stays | one low-poly cab serves both ends |

### RailVehicleLighting

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `head_light_color` | - | fiz: `fiz_train_lighting_parser.gd` | stays | the headlight colour, for a renderer (nothing uses it yet) |
| `head_light_dimmed_multiplier` | `RailVehicleRenderingServer.cpp:1637` | fiz (`DimmedMultiplier`) | stays | how bright dimmed headlights are, used by the renderer |
| `head_light_normal_multiplier` | - | fiz (`NormalMultiplier`) | stays | normal headlight brightness, for a renderer (unused) |
| `head_light_high_beam_dimmed_multiplier` | - | fiz (`HighbeamDimmedMultiplier`) | stays | dimmed high-beam brightness (unused) |
| `head_light_high_beam_normal_multiplier` | - | fiz (`HighBeamMultiplier`) | stays | high-beam brightness (unused) |
| `lights_default_selector_position` | - | adapter (`LightsDefPos`); fiz (`Default`) | stays | where the cab's light selector starts |
| `lights_wrap_selector` | - | adapter (`LightsWrap`); fiz (`Wrap`) | stays | the light selector wraps around |
| `lights_list` | - | adapter (stored as `light_position_list`, gives `Lights[][]`, dump `light_position_max`); mmd: `mmd_semantic_catalog.gd` (by dump key); fiz | stays | the light presets of the cab's selector |
| `light_source` | - | adapter (`LightPowerSource.SourceType`); fiz (`Light`) | down | Mover's `TPowerSource` selector for its lighting power model |
| `source_generator_engine` | - | fiz (`LGeneratorEngine`) only | down | a `TPowerParameters` generator engine type; the adapter does not even apply it |
| `source_accumulator_max_voltage` | - | - | down | a `TPowerParameters` accumulator field; nothing reads or writes it |
| `light_alternative_source` | - | adapter (`AlterLightPowerSource.SourceType`); fiz (`AlterLight`) | down | Mover's `TPowerSource` for the alternative lighting supply |
| `light_alternative_max_voltage` | - | fiz (`AlterLMaxVoltage`) only | down | a `TPowerParameters` field, never applied |
| `light_alternative_capacity` | - | - | down | a `TPowerParameters` accumulator capacity, never written or read |
| `source_accumulator_recharge_source` | - | - | down | a `TPowerParameters` recharge source, never written or read |
| `instrument_type` | - | - | unclear | no writer, no reader, no comment. Needs its origin (some FIZ key?) or removal |

### RailVehicleLightListItem

Every field is read only by the adapter, which packs it into `Lights[end][preset]` bits (`MaszynaMoverRailVehicleLighting.cpp:25-41`), and written by `fiz_train_lighting_parser.gd`.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `cabin_a_head_light` | - | adapter; fiz | stays | the preset lights cab A's upper headlight |
| `cabin_a_left_white_signal` | - | adapter; fiz | stays | cab A's left white lamp |
| `cabin_a_left_red_signal` | - | adapter; fiz | stays | cab A's left red marker |
| `cabin_a_right_white_signal` | - | adapter; fiz | stays | cab A's right white lamp |
| `cabin_a_right_red_signal` | - | adapter; fiz | stays | cab A's right red marker |
| `cabin_a_end_signals` | - | adapter; fiz | stays | cab A's end-of-train signals |
| `cabin_a_left_auxiliary_light` | - | adapter; fiz | stays | cab A's left auxiliary lamp |
| `cabin_a_right_auxiliary_light` | - | adapter; fiz | stays | cab A's right auxiliary lamp |
| `cabin_b_head_light` | - | adapter; fiz | stays | cab B's upper headlight |
| `cabin_b_left_white_signal` | - | adapter; fiz | stays | cab B's left white lamp |
| `cabin_b_left_red_signal` | - | adapter; fiz | stays | cab B's left red marker |
| `cabin_b_right_white_signal` | - | adapter; fiz | stays | cab B's right white lamp |
| `cabin_b_right_red_signal` | - | adapter; fiz | stays | cab B's right red marker |
| `cabin_b_end_signals` | - | adapter; fiz | stays | cab B's end-of-train signals |
| `cabin_b_left_auxiliary_light` | - | adapter; fiz | stays | cab B's left auxiliary lamp |
| `cabin_b_right_auxiliary_light` | - | adapter; fiz | stays | cab B's right auxiliary lamp |

### RailVehicleDimmerListItem

The items are stored in `RailVehicleSwitches.dimmer_list_positions`. Nothing in the core or the adapter reads that list; only `fiz_train_switches_parser.gd` fills it.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `high_beam` | - | fiz: `fiz_train_switches_parser.gd` | stays | this dimmer switch position gives high beam (unread) |
| `dimmed` | - | fiz | stays | this position dims the headlights (unread) |
| `off` | - | fiz | stays | this position switches the headlights off (unread) |

### RailVehicleLoad

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `load_list` | - | adapter (only stores it, `MaszynaMoverRailVehicleLoad.hpp:63-68`, never applies it) | unclear | no FIZ parser fills it and nothing uses it; it overlaps `accepted_loads`/`minimum_load_offsets`/`max_load`. Needs a decision: replace those with it, or drop it |
| `load_unit` | - | adapter (`LoadQuantity`); fiz: `fiz_train_load_parser.gd` | stays | whether the load is counted in tons or pieces |
| `overload_factor` | - | adapter (`OverLoadFactor`); fiz (`OverLoadFactor`) | stays | how far past `max_load` the vehicle may be loaded, a limit |
| `load_speed` | - | adapter (`LoadSpeed`); fiz (`LoadSpeed`) | stays | how fast loading goes (boarding rate) |
| `unload_speed` | - | adapter (`UnLoadSpeed`); fiz (`UnLoadSpeed`) | stays | how fast unloading goes |
| `max_load` | `RailVehicleRenderingServer.cpp:1897`, `mover_switches_load.gd:30` | adapter (`MaxLoad`); fiz (`MaxLoad`) | stays | the capacity; the renderer uses it for the fill height |
| `minimum_load_offsets` | `RailVehicleRenderingServer.cpp:1893` | adapter (`LoadAttributes` offsets); fiz | stays | how low each accepted load's model sits when nearly empty, drawn by the renderer. The array runs parallel to `accepted_loads` (FIZ shape) |
| `accepted_loads` | `RailVehicleRenderingServer.cpp:1892` | adapter (`LoadAttributes`); fiz | stays | the kinds of cargo the vehicle accepts; the renderer matches them against `load_name` |

### RailVehicleLoadListItem

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `max_load` | - | - | unclear | only held inside `RailVehicleLoad.load_list`, which nothing fills or reads (the core/debug readers of `max_load` are RailVehicleLoad's own). See `load_list` |
| `load_type` | - | - | unclear | never written or read |
| `load_offset` | - | - | unclear | never written or read |

### RailVehicleWheels

The adapter's dump publishes each value under its own name, plus `axles_count` and `axles_powered_count`. No reader of those keys was found outside tests.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `powered_wheel_diameter` | `RailVehicleWheels.cpp:33` (own, wheel turning) | adapter (dump); vehicle instancer (`maszyna_rail_vehicle_3d_instancer.gd:217`, wheel groups); fiz: `fiz_train_wheels_parser.gd` | stays | the powered wheels' diameter |
| `front_rolling_wheel_diameter` | `RailVehicleWheels.cpp:37` (own) | adapter (dump); vehicle instancer; fiz (`Dl`) | stays | the front rolling wheels' diameter |
| `rear_rolling_wheel_diameter` | `RailVehicleWheels.cpp:42` (own) | adapter (dump); vehicle instancer; fiz (`Dt`) | stays | the rear rolling wheels' diameter |
| `axle_inertial_moment` | - | adapter (`AxleInertialMoment`, dump); fiz (`AIM`) | stays | the axle's moment of inertia, a physical fact |
| `track_width` | `running_sound_model.gd:205` | adapter (`TrackW`, dump); fiz (`Tw`) | stays | the gauge; core sound uses it |
| `axle_arrangement` | - | adapter (`AxleArangement`, dump); vehicle instancer (`:219`); fiz (`Axle`) | stays | the axle arrangement (e.g. Bo'Bo') |
| `bogie_axle_spacing` | `RailVehicleServer.cpp:1208` | adapter (`ADist`, dump); fiz (`Ad`) | stays | the axle spacing in a bogie; the core uses it for the axle distance |
| `bogie_pivot_spacing` | `RailVehicleServer.cpp:1208`, `:1716` | adapter (`BDist`, dump); fiz (`Bd`) | stays | the bogie pivot spacing; the core uses it for axle placement and curve radius |
| `minimum_curve_radius` | - | adapter (dump only); fiz (`Rmin`) | stays | the tightest curve the vehicle can take |
| `bearing_type` | - | adapter (`BearingType`, dump); fiz | stays | slide or roller bearings |

### RailVehicleWipers

The whole wiper behaviour is in the core (`RailVehicleWipers.cpp`). The adapter only answers whether the battery is on.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `angle` | `RailVehicleRenderingServer.cpp:1353`; `RailVehicleWipers.cpp:165` (own, dump `wipers_angle`) | fiz: `fiz_train_wipers_parser.gd` (`Angle`) | stays | how far a wiper sweeps [deg], drawn by the renderer |
| `default_position` | `RailVehicleWipers.cpp:53-63` (own) | fiz (`Default`) | stays | where the wiper switch starts |
| `wiper_count` | `RailVehicleWipers.cpp:29` (own) | writer: vehicle instancer (from `appearance.wiper_arms`) | stays | how many wipers there are |
| `positions` | `RailVehicleWipers.cpp:32-131` (own, dump `wipers_switch_position_max`) | mmd: `mmd_semantic_catalog.gd` (by dump key); fiz | stays | the wiper switch's positions |

### RailVehicleWiperListItem

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `wiper_mask` | `RailVehicleWipers.cpp:35`, `:92`, `:104`, `:106` | fiz | stays | which wipers this switch position runs |
| `transit_time` | `RailVehicleWipers.cpp:117`, `:120` | fiz | stays | how long one sweep takes |
| `period` | `RailVehicleWipers.cpp:127` | fiz | stays | the time between sweeps at this position |
| `return_delay` | `RailVehicleWipers.cpp:131` | fiz | stays | how long a wiper pauses at the far end |

### RailVehicleBuffCoupl

The dump key `coupler_max_force` (the Mover's `FmaxC` per end) has no reader.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `coupler_type` | - | adapter (`TCouplerType`); fiz: `fiz_train_buff_coupl_parser.gd` | stays | automatic, screw, chain, bare or articulated coupler |
| `buffer_stiffness_k` | - | adapter (`SpringKB`); fiz (`kB`) | stays | the buffer's spring stiffness, physical. MOVER.h:763 says the same key doubles as a damping percentage |
| `buffer_max_compression_tolerance` | - | adapter (`DmaxB`); fiz (`DmaxB`) | stays | the buffer's stroke, a limit |
| `buffer_max_tension_tolerance` | - | adapter (`FmaxB`); fiz (`FmaxB`) | stays | the buffer's maximum force, a rating (the name says "tension" for a buffer) |
| `coupler_stiffness_k` | - | adapter (`SpringKC`); fiz (`kC`) | stays | the coupler's spring stiffness |
| `coupler_max_compression_tolerance` | - | adapter (`DmaxC`); fiz (`DmaxC`) | stays | the coupler's play, a limit |
| `coupler_max_tension_tolerance` | - | adapter (`FmaxC`, dump `coupler_max_force`); fiz (`FmaxC`) | stays | the coupler's breaking force |
| `damping_beta` | - | adapter (`beta`); fiz (`beta`) | down | the energy-loss factor of Mover's `ComputeCollision` formula (Mover.cpp:50-62), a Mover model coefficient |
| `allowed_flag` | - | adapter (`AllowedFlag`); fiz | stays | which couplings this coupler can make (the core `CouplingFlags`) |
| `automatic_flag` | - | adapter (`AutomaticCouplingFlag`); fiz (`AutomaticFlag`) | stays | which couplings it makes by itself |
| `power_flag` | - | adapter (`PowerFlag`); fiz (`PowerFlag`) | stays | which power lines (24V/110V/3x400V) it carries |
| `power_coupling` | - | adapter (`PowerCoupling`); fiz (`PowerCoupling`) | stays | which couplings carry power |
| `control_type` | - | adapter (`control_type`); fiz (`ControlType`) | stays | the multiple-unit control standard; couplers of different standards do not control each other |
| `buffer_location` | - | adapter (which ends); fiz | stays | front, back or both ends |

### RailVehicleSecuritySystem

All of these are settings of the vigilance and cab-signal device (SHP/CA/SIFA). Only the adapter, writing `TSecuritySystem`, and `fiz_train_security_system_parser.gd` use them.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `aware_system_active` | - | adapter (`vigilance_enabled`); fiz (flag `active`) | stays | the vehicle has the vigilance device |
| `aware_system_cabsignal` | - | adapter (`cabsignal_enabled`); fiz (flag `cabsignal`) | stays | the vehicle has cab signalling (SHP) |
| `aware_system_separate_acknowledge` | - | adapter (`separate_acknowledge`); fiz | stays | SHP and CA have separate acknowledge buttons |
| `aware_system_sifa` | - | adapter (`is_sifa`); fiz (flag `sifa`) | stays | the device works as a SIFA (hold-type) |
| `aware_delay` | - | adapter (`AwareDelay`); fiz (`AwareDelay`) | stays | the vigilance period |
| `aware_min_speed` | - | adapter (`AwareMinSpeed`; `-1` means 10% of Vmax); fiz (`AwareMinSpeed`) | stays | the speed below which vigilance is idle (the `-1` sentinel is the FIZ absent-key default) |
| `cab_dependent` | - | adapter (`CabDependent`); fiz (`CabDependent`) | stays | the device follows the active cab |
| `emergency_brake_delay` | - | adapter (`EmergencyBrakeDelay`); fiz | stays | the time from alarm to emergency braking |
| `emergency_signal` | - | adapter (`EmergencyBrakeWarningSignal`) | stays | which horn sounds on an emergency brake (no FIZ parser sets it) |
| `radio_stop_enabled` | - | adapter (`radiostop_enabled`); fiz (`RadioStop`) | stays | the vehicle has radio-stop |
| `sound_signal_delay` | - | adapter (`SoundSignalDelay`); fiz | stays | the time from the lamp to the buzzer |
| `shp_magnet_distance` | - | adapter (`MagnetLocation`); fiz (`MagnetLocation`) | stays | where the SHP receiver sits on the vehicle |
| `ca_max_hold_time` | - | adapter (`MaxHoldTime`); fiz (`MaxHoldTime`) | stays | how long the acknowledge button may be held |

### RailVehicleRadio

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `channel_min` | `RailVehicleRadio.cpp:50` (own, clamps the channel) | - | stays | the lowest radio channel |
| `channel_max` | `RailVehicleRadio.cpp:50` (own) | - | stays | the highest radio channel |

### RailVehicleAIHints

The adapter writes all three into `AIHint*` in the Mover, and nothing reads them there (the vendored Mover has no consumer, and the core `DriverServer`/`DriverImplementation` does not read them).

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `pantograph_state` | - | adapter (`AIHintPantstate`); fiz: `fiz_train_ai_hints_parser.gd` (`Pantstate`) | stays | which pantographs a driver should raise, a hint for any driver |
| `raise_pantographs_when_idle` | - | adapter (`AIHintPantUpIfIdle`); fiz (`IdlePantUp`) | stays | raise both pantographs when standing idle, a hint for any driver |
| `local_brake_acceleration_factor` | - | adapter (`AIHintLocalBrakeAccFactor`); fiz (`LocalBrakeAccFactor`) | down | a weight in the original AI's local-brake equation, not a fact about the vehicle |

### RailVehicleMasterController

The adapter's dump publishes `main_controller_position_max`, `second_controller_position_max` and `master_controller_position_max`, all derived from the counts and `coupled_controllers`. `mmd_semantic_catalog.gd` (`config_max_property`) and `python_screen_state.gd:130` read them by key.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `main_position_count` | - | adapter (`MainCtrlPosNo`, dump); mmd/cabin by dump key; fiz: `fiz_train_cntrl_parser.gd` (`MCPN`) | stays | the main controller's number of positions |
| `second_position_count` | - | adapter (`ScndCtrlPosNo`, dump; also `MaszynaMoverRailVehicleSpeedControl.cpp:24`); mmd by dump key; fiz (`SCPN`) | stays | the second (shunt) controller's number of positions |
| `direction_change_max_position` | - | adapter (`MainCtrlMaxDirChangePos`); fiz (`DirChangeMaxPos`) | stays | the highest main position at which the reverser may be moved (an interlock) |
| `coupled_controllers` | - | adapter (`CoupledCtrl`, dump `master_controller_position_max`); mmd by dump key; fiz (`CoupledCtrl`) | stays | the second controller continues the main one on a single handle |
| `initial_delay` | - | adapter (`InitialCtrlDelay`); fiz (`IniCDelay`) | stays | how long the first step takes |
| `step_delay` | - | adapter (`CtrlDelay`); fiz (`SCDelay`) | stays | the time per step up |
| `step_down_delay` | - | adapter (`CtrlDownDelay`); fiz (`SCDDelay`) | stays | the time per step down |
| `tachometer_max_speed` | `RailVehicleMasterController.cpp:184` (own, Hasler recorder) | fiz (`MaxTachoSpeed`) | stays | the top of the speed recorder's dial [km/h]; core cab instrument logic uses it |

### RailVehicleInverter

This is not configuration. It is a value object the adapter fills fresh on each `get_inverters()` call from `mover->Inverters` (`MaszynaMoverRailVehicleElectricInductionEngine.cpp:16-20`). It is not a component and has no dump.

| Property | Core readers | Plugin readers | Proposal | Why |
|---|---|---|---|---|
| `active` | - | adapter (writer); cabin: `python_screen_state.gd:265` | stays | the inverter is working, a state readout any backend can give |
| `error` | - | adapter (writer); cabin: `python_screen_state.gd:266` | stays | the inverter has a fault, a state readout |
| `allow` | - | adapter (writer); cabin: `python_screen_state.gd:267` | stays | the inverter is allowed to run, a state readout |

### RailVehicleHorns, RailVehicleCabinKind, RailVehicleComponentType

These bind no properties: Horns binds only methods, the other two only enums.

#### Notes

- **`RailVehicleController.StartMode`** (`START_MODE_DISABLED/MANUAL/AUTOMATIC/MANUAL_WITH_AUTO_FALLBACK/CONVERTER/BATTERY/DIRECTION`) is the Mover's start-mode selector of the FIZ `...Start=` keys (`GroundRelayStart=`, `CompartmentLightsStart=`, and in other classes the battery, converter, main switch, pantograph compressor/valves, motor blowers, fuel/oil/water pumps). The adapter maps it with `MaszynaMoverVehicleServer::start_mode_to_mover`. Core uses: it is only the declared type of properties in `RailVehicleEngine.hpp`, `RailVehicleDieselEngine.hpp`, `RailVehicleElectricEngine.hpp`, `RailVehicleEnginePowerSource.hpp`, `RailVehiclePowerSupply.hpp` and `RailVehicleController.hpp`. No core code branches on its values. Moving the enum down means every core header that names it has to change.
- **`RailVehicleController.TrainType`** is the Mover's `TrainType` (EZT, ET41, ET42, PseudoDiesel, ET22, SN61, EP05, ET40, 181, DMU), which picks hard-coded branches all over Mover.cpp. It is the only ALARM here. The core reads it in `RailVehicleServer.cpp:867-870` (EZT or DMU, so the powered car is searched over permanent couplings) and `RailVehicleServer.cpp:2045` (the EMU pantograph). Both core uses only need the fact "multiple unit (electric or diesel)". The FIZ parsers also derive defaults from it (`fiz_train_controller_parser.gd:106`, `fiz_train_engine_parser.gd:72`, `fiz_train_engine_common.gd:52`, `fiz_vehicle_builder.gd:152`). A possible split: a core fact such as `multiple_unit` (none/electric/diesel), with the loco-specific selector moved down to the adapter.
- **`RailVehicleController.TrainPowerSource`** is the Mover's `TPowerSource`. In my classes it types Lighting's `light_source`, `light_alternative_source` and `source_accumulator_recharge_source`, and `RailVehicleEngine.EngineType` types `source_generator_engine`. Core uses outside my classes: `RailVehicleEnginePowerSource.cpp:28/32`, `RailVehicleServer.cpp:888` (current collector) and `mover_switches_power_source.gd`. Those belong to other classes' rows.
- **Dead or unused configuration**: `RailVehicleDoors.platform_max_speed`, `RailVehicleLighting.instrument_type`, `source_accumulator_max_voltage`, `light_alternative_capacity`, `source_accumulator_recharge_source` and `source_generator_engine` (the FIZ sets the last one but the adapter does not apply it), `light_alternative_max_voltage` (same), all of `RailVehicleLoadListItem` plus `RailVehicleLoad.load_list`, all of `RailVehicleDimmerListItem` (`RailVehicleSwitches.dimmer_list_positions` is never read), and three of the four headlight multipliers plus `head_light_color` (only `head_light_dimmed_multiplier` reaches the renderer).
- **The core does real work with some values, not just storing them**: Wipers (`RailVehicleWipers.cpp` holds the whole behaviour and reads WiperListItem), Doors mirrors (`RailVehicleDoors.cpp:163`), the wheel angles (`RailVehicleWheels.cpp`), the Hasler recorder (`RailVehicleMasterController.cpp:184`) and the radio channel clamp (`RailVehicleRadio.cpp:50`). These values cannot go down without moving that logic too.
- **`RailVehicleAppearance`**: the core renderer reads every field. The bogie, wheel, pantograph, wiper and mirror names are also written by the core `RailVehicle3D.cpp` for vehicles assembled by hand, so they are not MaSzyna-only. The slot layouts the renderer expects are still the original's (5 pantograph slots per TAnimPant, 3 elements per wiper and per door, odd left and even right). Only `pantograph_factors`, the raw MMD fallback, is flagged plugin-data.
- **`coupler_adapter_model/length/height`** sit on RailVehicleController but come from the MMD (`MmdCabinInstancer.parse_coupler_adapter`) and are drawing data; the model name fits better in `RailVehicleAppearance`.
- **Naming quirks**: `RailVehicleBuffCoupl.buffer_max_tension_tolerance` is `FmaxB`, a buffer's maximum force, not a tension. The `SpringKB` comment in MOVER.h:763 says the same key doubles as a damping percentage, so `buffer_stiffness_k` and `coupler_stiffness_k` hold the Mover's convention, not a clean stiffness.
- **The dump keys differ from the property names**: `wipers_angle` (from `angle`), `wipers_switch_position_max` (from `positions`), `doors_permit_preset_max` (from `permit_list`), `light_position_max` (from `lights_list`), `main/second/master_controller_position_max` (from the counts and `coupled_controllers`) and `coupler_max_force` (from `FmaxC`). The MMD catalog and the python screen state read them by key; `coupler_max_force` and `wipers_angle` have no reader.
