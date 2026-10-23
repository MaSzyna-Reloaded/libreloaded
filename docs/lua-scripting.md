---
layout: page
title: "Lua Scenario Scripts"
---

A scenery can include scripts written in Lua. A script can, for example, set the signals like a
dispatcher, throw a switch, or give a train its orders when it stops. Scripts use the same servers
as the rest of the simulation. They can only act on the scenario, not on the engine.

* [Running a script](#running-a-script)
* [How a script works](#how-a-script-works)
* [API reference](#api-reference)
* [Examples](#examples)

## Running a script

### From the scenery

A scenery can declare scripts. The plugin's scenery provider runs them after the scenery is built,
when its tracks, events, vehicles and drivers already exist. The scripts run in the order the
scenery declares them. The provider uses `ScenarioScriptServer.context_create()` and
`context_run_file()`. How a scenery declares a script depends on the plugin's file format. See the
plugin's documentation.

### While playing

*View > Lua scripts* opens an editor for the current scenario:

* **Check** compiles the code without running it. It shows the first error and its line, for
  example `editor:2: unexpected symbol near <eof>`.
* **Apply** runs the code. Before it runs, everything the previous Apply created is removed: events,
  timers and subscriptions. You can change the code and apply it as often as you like. Applying an
  empty editor stops everything.

Errors from a running script, such as a callback that fails later, are shown under the editor and
written to the game log.

### In the build

Lua 5.4 comes from the `vendor/lua` submodule. It is compiled in when the CMake option
`LIBRELOADED_LUA` is on, which is the default. Without it, every script fails with
"lua scripts not supported in this build."

## How a script works

### Handles

Everything a script works with is a **handle**: a vehicle, a track, an event, a signal head. You
find a handle by name. A vehicle is found by its id, which is its scenery name in lower case:

```lua
local v = reloaded.vehicle.find("train1")
```

A lookup that finds nothing returns `nil`. A function that expects a vehicle rejects a track.

Two handles of the same object are equal (`==`), but they are different Lua values. Do not use a
handle as a table key. Use its name instead.

### Callbacks

Functions passed to `on_*`, `event.create`, `sim.after` and `sim.every` do not run inside the
server that reports something. They run as scenario events on the simulation clock:

* right after the report, in the order the reports came
* never while the simulation is paused

All times in the API are simulation time. If the simulation runs faster, timers fire faster.

### What a script owns

The events, timers and subscriptions a script creates are freed together with the scenario, or
when the editor applies new code. A script can free its own event with `reloaded.event.free()` and
stop a timer or subscription with `reloaded.cancel()`.

### The sandbox

* Available libraries: `base`, `string`, `table`, `math`, `coroutine` and `utf8`.
* Not available: `io`, `os`, `debug`, `package`, `dofile` and `loadfile`. `load` accepts only
  text.
* `require("lib.util")` loads `lib/util.lua` from the scenery directory. Nothing outside that
  directory can be loaded.
* `print` writes to the game log.
* A call that runs too long, such as an endless loop, fails with an error. So does a script that
  uses too much memory. The game keeps running.

## API reference

All modules are tables in the global `reloaded`, such as `reloaded.vehicle`. They are also
available through `require`. Command and state names are the same as in the console and the cab.

### `reloaded.sim` - time

| Function | Returns / does |
|---|---|
| `time()` | seconds of simulation time since the scenario started |
| `time_of_day()` | hours since midnight (13.5 at half past one) |
| `is_paused()` | whether the simulation is paused |
| `random(a, b)` | a number between `a` and `b` |
| `after(seconds, fn)` | runs `fn()` once, after the seconds; returns a subscription |
| `every(seconds, fn)` | runs `fn()` every so many seconds; returns a subscription |

`reloaded.cancel(subscription)` stops a timer or a subscription.

### `reloaded.vehicle` - vehicles

| Function | Returns / does |
|---|---|
| `find(id)` | the vehicle of the id - its scenery name in lower case; of several, one |
| `find_all(id)` | every vehicle of the id |
| `all()` | every vehicle |
| `id(v)`, `name(v)` | the id that `find` takes; the name shown to the player (the id if there is no name) |
| `speed(v)`, `velocity(v)` | km/h, never negative; km/h, negative backwards |
| `state(v, key)`, `config(v, key)` | one value of its state or configuration (`"brake/cylinder_pressure"`) |
| `commands(v)` | the commands it takes |
| `send_command(v, command, p1, p2)` | sends a command, returns the answer (`send_command(v, "battery", true)`) |
| `coupled(v, end, element)` | the vehicles joined by `"coupler"`, `"brake_hose"`, `"main_hose"`, `"control"`, `"gangway"`, `"heating"` or `"permanent"` |
| `track_position(v)` | `{track = <track>, along = <metres>}`, the position of the vehicle's primary axle |
| `on_command_received(v, fn)` | `fn(command, p1, p2)` for every command the vehicle gets |

### `reloaded.cabin` - the cab, as the driver's hand

| Function | Returns / does |
|---|---|
| `act(cabin, control_id, action, value)` | manipulates a control; `action` is `"increase"`, `"decrease"`, `"hold"`, `"release"`, `"toggle"` or `"set"` |
| `control(cabin, control_id)` | where the control stands |
| `controls(cabin)` | the ids of the cabin's controls |
| `driver_cabin(v)` | the cabin the vehicle's driver sits in, `nil` when nobody drives it |
| `front_cabin(v)`, `rear_cabin(v)`, `machine_room(v)` | the vehicle's cabin of that kind, `nil` when it has none |
| `on_control_changed(v, fn)` | `fn(cabin, control_id, value)` when a control of the vehicle's cabins changes |

### `reloaded.driver` - the vehicle's driver

| Function | Returns / does |
|---|---|
| `send_command(v, command, value1, value2)` | an order to the driver, for example `"SetVelocity", 40, 40`; `false` when nobody drives |
| `timetable(v)` | the driver's timetable and how far it got; `nil` when nobody drives |

### `reloaded.event` - scenario events

| Function | Returns / does |
|---|---|
| `create{name =, delay =, random_delay =, run =, run_else =}` | creates an event. `run(event, activator)` is called when it runs, `run_else` when its condition fails. The scenery's events can queue it by name |
| `find(name)`, `exists(name)` | the event of the name - the scenery's or a script's |
| `name(e)`, `is_queued(e)` | |
| `queue(e, activator, extra_delay)` | runs it after its delay; `false` when it waits already |
| `free(e)` | frees an event the script created |
| `on_launched(e, fn)` | `fn(activator)` whenever the event runs |

### `reloaded.track` - tracks, switches, isolated sections

| Function | Returns / does |
|---|---|
| `find(name)` | the track of the name |
| `is_occupied(t)`, `vehicles(t)` | |
| `find_switch(name)` | the switch of the switch track of the name |
| `switch_get(s)`, `switch_set(s, which)` | `"common"` or `"diverging"` |
| `isolated_find(name)`, `isolated_is_occupied(i)` | |
| `on_vehicle_entered(t, fn)` | `fn(vehicle, toward)` when a vehicle enters the track; `toward` is `"start"` or `"end"`, or `nil` if the vehicle was placed there |
| `on_vehicle_left(t, fn)` | `fn(vehicle)` when a vehicle leaves the track |
| `on_vehicle_started(t, fn)` | `fn(vehicle, toward)` when a vehicle on the track starts off, or turns the other way |
| `on_vehicle_stopped(t, fn)` | `fn(vehicle)` when a vehicle stops on the track (called for both tracks if it stands on two) |
| `on_isolated_occupied(i, fn)`, `on_isolated_freed(i, fn)` | `fn(vehicle)` when the first axle enters the section, or the last axle leaves it |
| `on_switch_changed(s, fn)` | `fn(which)` when the switch moves |

### `reloaded.signal` - signals

A signalling system decides what its signal heads show. A script asks the system for a signal; it
does not light a head itself.

| Function | Returns / does |
|---|---|
| `find_head(name)`, `aspect(h)`, `aspects(h)` | a signal head, the aspect it shows, the ones it can show |
| `find_system(name)` | a signalling system (the scenery's is named after its file) |
| `send_event(system, event, arguments)` | e.g. `send_event(system, "lights", {signal_head = h, aspect = "S1"})` |
| `on_aspect_changed(h, fn)` | `fn(aspect)` whenever the head shows another aspect |
| `find_source(name)` | a source of a system: a sensor, a section |
| `source_send_event(s, event, arguments)` | sends a report from the source to its system. A train passing a sensor is `source_send_event(s, "impulse", {polarity = p})`, where `p` is a `SignallingServer.SourcePolarity`: 0 none, 1 along the track, 2 against it |
| `find_route(name)`, `route_state(r)` | a route, its state: `"idle"`, `"setting"`, `"closed"`, `"locked"`, `"releasing"` |
| `route_set(r)`, `route_cancel(r)`, `route_release(r)`, `route_emergency_release(r)` | sends the request to the route's system, which decides |
| `on_route_state_changed(r, fn)` | `fn(state)` whenever the route goes to another state |

### `reloaded.player` - what the player drives

| Function | Returns / does |
|---|---|
| `vehicle()` | the vehicle the player drives; `nil` for none |
| `take_over(v)` | the player takes over the vehicle and sits in its cab; meanwhile the AI driver only takes orders |
| `enter(v)` | the player sits in the vehicle's cab; its driver, if it has one, drives on |
| `leave()` | the player lets the trainset go; its drivers drive it again |

### `reloaded.camera` - where the player looks from

| Function | Returns / does |
|---|---|
| `mode()`, `set_mode(mode)` | `"cabin"` (only with a vehicle driven), `"free"` or `"follow"` (only with a target) |
| `target()`, `set_target(v)` | the vehicle the following camera follows |
| `follow_view()`, `set_follow_view(view)` | `"trainset_front"`, `"trainset_rear"`, `"bogie"` or `"driveby"` |
| `cycle_follow_view()` | Shift+F4: following, the next view; else the player's vehicle followed |
| `toggle_cabin()` | like F4: from outside, back into the cab of the driven vehicle; from the cab, out of it. Without a driven vehicle it switches between following and the free camera |
| `show_vehicle(v)` | the free camera beside the vehicle, looking at it |

Looking from outside only changes the view. The player still drives the vehicle, and its controls
still work.

### `reloaded.hud` - what the HUD shows

Panel names are defined by the HUD. The demo's panels are `transcripts`, `driving_aid`,
`timetable`, `scenario`, `controls` (every control window at once), `scripts`, `trainsets`, and
one per control window (`general`, `engine`, `brakes`, ..., `mini_map`, `weather_and_time`, `help`).

| Function | Returns / does |
|---|---|
| `show(panel)`, `hide(panel)`, `toggle(panel)`, `is_visible(panel)` | |
| `open_card(v)`, `close_card()`, `card()` | the vehicle card; `card()` is `nil` while it is closed |

### `reloaded.log`

`debug(text)`, `info(text)`, `warning(text)` and `error(text)` write to the game log.

## Examples

For examples, see the documentation of a plugin. It shows scripts for its own sceneries and the
modules it adds.
