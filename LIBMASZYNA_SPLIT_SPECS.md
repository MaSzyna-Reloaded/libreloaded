# Small ports - behaviour specs for LS-022

Scope: the small pieces of the core that were translated from MaSzyna (MPL 2.0) and that the core
keeps (`LIBMASZYNA_SPLIT.md`, LS-022). Each section describes **what the code must do**: its
inputs, outputs, values and order. It says nothing about how the original writes it. After the
operator accepts a section, its code is written again from that section only. The new comments cite
this file (or the public source named in it), never a MaSzyna file and line.

Measured on `dev/separate-libmaszyna` at 0c41850, 2026-10-19.

**Accepted by the operator, 2026-10-19:** every constant stays as written; the Lua message is
written in our own words; section 5 is not rewritten - our sun is gnd_skybox's, so the Schlyter and
SOLPOS calculation leaves `SimulationServer` and the light level comes from the sky's sun.

Each section ends with **To accept:** the points where the current code does something the operator
may not want carried over into the rewrite.

Contents:

1. [Pantograph arm and contact](#1-pantograph-arm-and-contact)
2. [Collector voltage hold](#2-collector-voltage-hold)
3. [Cab shake](#3-cab-shake)
4. [Track endpoints, switch blades, friction, isolated sections](#4-track-endpoints-switch-blades-friction-isolated-sections)
5. [Sun position and light level](#5-sun-position-and-light-level)
6. [Lua without a runtime](#6-lua-without-a-runtime)

## 1. Pantograph arm and contact

**Today:** `RailVehicleServer::Pantograph::raise` (`RailVehicleServer.cpp:400`), its rise rate in
`vehicle_collect_current` (`:2041-2054`), constants `RailVehicleServer.hpp:142-153`.

A pantograph is two arms on a pivot. The lower arm stands at an angle `a` from its rest angle
`a0`. The upper arm closes the linkage. The slider sits on top. The model gives the geometry
(`vehicle_set_pantograph_geometry()`): lower and upper arm lengths `L1`, `L2`, horizontal offset
`h`, slider height `s`, rest angles.

**Rise rate.** It is computed once per vehicle per step, for both pantographs:

* the arms can rise only when the collector tank pressure `p` [bar] is above **3.45**, or above
  **2.45** for an EMU (`TRAIN_TYPE_EZT`), and the vehicle has 24 V or 110 V power available;
* then the rise rate is `r = max(0, 0.015 * p * dt)` [rad per step]; otherwise `r = 0`.

**Gap.** `g` is the wire height over the slider [m]: the wire found over the pantograph minus the
slider's current height. A lowered pantograph has `g = +inf` and nothing is searched.

**One step of the arm:**

* raised and `r > 0`:
  * `g > 0.001`: `a` grows by `min(r, 0.55 * g)`;
  * `g < -0.001`: the wire pushes the arm down, `a` grows by `0.4 * g`, which is negative;
  * otherwise `a` stays.
* lowered, or `r = 0`: `a` falls at **0.15 rad/s** (`a -= 0.15 * dt`), never below `a0`.
* When `a` changed, the upper arm angle is `b = acos((L1 * cos(a) + h) / L2)`. The new pose is
  taken only if `a + b < pi`. Otherwise the arm keeps its previous pose. The pose gives
  `height = L1 * sin(a) + L2 * sin(b) + s`.
* **Contact:** the slider reaches the wire when the pantograph is raised and `g < 0.01` m.

**To accept:**

* the values 3.45 / 2.45 bar, 0.015, 0.55, 0.4, 0.15 rad/s, 0.001 m and 0.01 m stay as they are;
* the "no 24 V and no 110 V means no rise" condition stays.

## 2. Collector voltage hold

**Today:** `RailVehicleServer::vehicle_collect_current` (`RailVehicleServer.cpp:2135-2144`),
`NO_VOLTAGE_HOLD` (`RailVehicleServer.hpp:157`).

* Each step, `fed` is the largest absolute voltage that any of the vehicle's pantographs reads.
* `fed > 0` resets the vehicle's no-voltage time to 0.
* `fed = 0` adds `dt` to the no-voltage time. While that time is at most **0.2 s**, the vehicle
  keeps the collector voltage it had (`get_collector_voltage()`). Only after 0.2 s does it drop
  to 0.
* The result is set with `set_collector_voltage()`.

This keeps a short loss from tripping the line breaker: a slider bouncing, an insulator, a span
joint.

**To accept:** 0.2 s.

## 3. Cab shake

**Today:** `Cabin3D::_compute_spring_force` (`Cabin3D.cpp:196`), `_process_engine_shake`
(`:207`), the fixed step (`:158-161`), constants and defaults `Cabin3D.hpp:30-75`.

**When it runs.**

* Only while the cab's vehicle has an engine component that is a `RailVehicleDieselEngine`.
* Otherwise the cab stands still: offset, velocity and the step remainder are 0, and the node does
  not process.
* This is checked again on every `vehicle_changed` of the cab's vehicle.

**Fixed step.** The frame time is accumulated and integrated in steps of `h = 1/50` s. The
remainder is carried to the next frame.

**State:** offset `o` (Vector3), velocity `v` (Vector3), engine phase `phi`, which starts at
`pi/2`.

**Spring force towards a target `t`:**

* `d = t - o`.
* If `|d| <= 0.01`, the force is zero.
* Otherwise its size is `(|d| - 0.01) * k + |d| * c` (stiffness `k`, damping `c`), along `-d/|d|`.

**One step:**

1. Engine excitation `e = (x, 0, 0)`, where `n = |engine rpm count|` (revolutions per second):
   * `phi = (phi + n * h) mod 2*pi`;
   * `fade_in = clamp((n - fade_in_rpm / 60) * fade_in_factor, 0, 1)`;
   * `fade_out = 1 - clamp((n - fade_out_rpm / 60) * fade_out_factor, 0, 1)`;
   * `x = sin(4 * phi) * h * engine_shake_scale * fade_in * fade_out`; `x = 0` when `n = 0`.
2. `S = force(e) * 1.25`.
3. Jolts on the move:
   * `V = min(|vehicle speed| [km/h], 150)`;
   * when a random number in `[0, V]` exceeds **25**, three random components are drawn, each
     `(random in [0, 2V] - V) / (2V * 4)`;
   * they are multiplied per axis by `jolt_scale` and by 1.25, and `S += force(that vector)`.
4. `S *= 0.85`.
5. `a = (jolt_scale.x + jolt_scale.y + jolt_scale.z) / 200`; `v -= (S + 100 * v) * a`;
   `o += v * h`.
6. If `|o.y| > |jolt_limit|`, then `v.y = -v.y`.

Random numbers come from Godot's `randf_range`.

**Outputs:**

* `get_camera_shake_offset() = o`;
* `get_camera_shake_roll() = atan(v.x * angle_scale.x)`.

**Defaults** (exported; a MaSzyna cab sets them from its MMD, `maszyna_dynamic_train_cabin.gd:119`):

| Property | Default |
|---|---|
| `shake_spring_stiffness` | 125 |
| `shake_spring_damping` | 0.002 |
| `shake_jolt_scale` | (0.2, 0.2, 0.1) |
| `shake_jolt_limit` | 2.0 |
| `shake_angle_scale` | (0.05, 0.1) |
| `engine_shake_scale` | 2.0 |
| `engine_shake_fade_in_rpm` | 90 |
| `engine_shake_fade_in_factor` | 0.3 |
| `engine_shake_fade_out_rpm` | 600 |
| `engine_shake_fade_out_factor` | 0.5 |

**To accept:**

* only a diesel shakes the cab; an electric vehicle's cab stands still, even when it moves fast and
  the jolts of step 3 would apply. This is a limitation of the current code, not of the
  behaviour;
* the fixed constants 1/50 s, 0.01, 1.25, 25 km/h, 150 km/h, 4, 0.85, 100 and 200 stay;
* `shake_angle_scale.y` is set from the cab's data (`mmd_cabin_instancer.gd:277`), but nothing in
  `Cabin3D` reads it.

## 4. Track endpoints, switch blades, friction, isolated sections

**Today:** `TrackServer.cpp` - `_endpoints_equal` (`:254`), `_set_switch_f_offset` (`:475`),
`track_get_friction` (`:813`), `_count_isolated` (`:916`); constants `TrackServer.hpp:60,119,130`.

**Endpoints equal.**

* Two track endpoints are the same point when each of the three axes differs by at most **0.02 m**.
  This is a cube, not a sphere.
* It is used wherever two tracks are tested for joining.

**Switch blade offsets.** Setting a switch's offset to `x`:

* stores `f_offset = x`;
* the first blade gets `min(x, 0.1)` [m];
* the second blade gets `max(x, 0)`;
* then `switch_offset_changed(switch, x)` is emitted.

**Friction.**

* An unknown track reads **0.15**.
* A track whose friction modifier was never set reads its own friction `f`.
* Once `track_set_friction_modifier(track, m, b)` is called, it reads `clamp(f * m + b, 0, 1)`.

**Isolated sections.** An axle entering (+1) or leaving (-1) a track changes the axle count of each
isolated section the track belongs to. For each such section:

* count 0 → above 0 emits `isolated_occupied(section, vehicle)`;
* above 0 → 0 emits `isolated_freed(section, vehicle)`;
* then always `isolated_axle_entered` or `isolated_axle_left` (section, vehicle);
* then the same change is applied to the section's parent, if it has one, recursively.

**To accept:**

* 0.02 m, 0.1 m and 0.15 stay;
* the endpoint test stays a cube.

## 5. Sun position and light level

**Today:** `SimulationServer::_update_light_level` (`SimulationServer.cpp:468`), constants
`:13-59`, settings and factors `SimulationServer.hpp:118-129`.

**Inputs:** date (year, month, day), local time of day [h], timezone offset [h], latitude and
longitude [decimal degrees], air temperature [°C], cloud cover [0-1].

**Sun position.** It follows Paul Schlyter's published method ("How to compute planetary
positions") for the Sun:

* day number `d = 367y - 7(y + (m + 9) / 12) / 4 + 275m / 9 + D - 730530`, in integer arithmetic,
  plus the day fraction;
* orbital elements of the Sun at `d`;
* eccentric anomaly in its first-order form;
* ecliptic longitude;
* declination and right ascension at the obliquity of `d`;
* sidereal time `6.697375 + 0.0657098242 d + UT` [h];
* local hour angle, wrapped to ±180°;
* elevation from latitude, declination and hour angle.

All coefficients are Schlyter's own.

**Refraction.** It follows NREL SOLPOS (public domain):

* none above an elevation of 85°;
* three fits below it (from 5°, from -0.575°, and lower);
* scaled by `(P * 283) / (1013 * (273 + T))`, with surface pressure `P = 1013` mbar and `T` the
  air temperature;
* given in arcseconds, converted to degrees.

**Light level:**

* `daylight = smoothstep(night_altitude, day_altitude, elevation + refraction)`. The settings
  `reloaded/lights/night_altitude` (-6°) and `reloaded/lights/day_altitude` (6°) are this project's
  own tuning.
* `level = daylight * (1 - clamp(cloud_cover, 0, 1) * 0.65)`.
* `light_level_changed(level)` is emitted only when the value differs from the last one.

**To accept:**

* **the day fraction is taken from local time, and only the sidereal time uses UT.** This is the
  current code's behaviour. Schlyter adds `UT/24`, so a timezone shifts the day number by up to an
  hour of the Sun's motion. Should the rewrite keep this, or use UT for both?
* the overcast factor **0.65** is MaSzyna's value. Should it stay, or become a setting?
* the pressure stays a constant 1013 mbar.

## 6. Lua without a runtime

**Today:** `ScenarioScriptServer.hpp:77`, `NO_RUNTIME_MESSAGE`.

A build without the Lua runtime still creates script contexts. Running code in one reports a
script error (`script_error`, and the log), and checking a source (`context_check_source()`)
returns the same text: Lua scripts are not supported in this build.

**To accept:** the wording. Today it is the original's sentence, word for word: "lua scripts not
supported in this build." The alternative is our own sentence.
