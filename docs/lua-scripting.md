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

### In a node

A scene can run a script with a node. The script runs while the node is in the scene tree, in a
context of its own. Everything the script created is removed when the node leaves the tree. A
`LuaNode`'s script runs only in the game; the scripts of the nodes that draw a texture run in the
editor too, so what they draw can be seen while the scene is made.

* `LuaNode` runs `script_path`, or the code in its own `source` - never both. The path may be
  `res://`, `user://` or a path on disk. `require` loads files from the script's directory - for
  code of the node's own, from the directory of its scene. Fonts and images are read from
  `data_dir`, or from that directory when it is empty. A changed `script_path`, `source` or
  `data_dir` runs the script again at once.
* `LuaTextureNode` also owns a drawing (see `reloaded.draw`) and shows it in its `texture`, a
  `DrawingTexture`. A material in the scene uses the same texture to show what the script draws.
  The script gets the drawing as the first argument of its file: `local drawing = ...`.
  With a `material` (a `canvas_item` shader), the drawing is drawn through the material into the
  texture, at the texture's size. The script then draws at `drawing_size`. A texture with `flip_y`
  holds the drawing upside down, for models whose texture coordinates count up from the bottom;
  the script still draws from the top.
* `LuaVehicleTextureNode` is a `LuaTextureNode` that draws for a vehicle, for example its
  destination sign. Its script also gets the vehicle it is given (`set_vehicle()`), or `nil`:
  `local drawing, vehicle = ...`.
* `LuaCabinTextureNode` is a `LuaVehicleTextureNode` that sits under a cab's `Cabin3D`. Its script
  gets the cab's vehicle and cabin: `local drawing, vehicle, cabin = ...`. With a
  `cabin_implementation` the script can also use the cab's controls through `reloaded.cabin`.

A script can declare variables set in the node's inspector:

```lua
local TEXT = reloaded.export_var("text", "Pociąg osobowy")
local COLOUR = reloaded.export_var("colour", {255, 170, 0})
```

`export_var(name, default)` returns what the node sets the variable to, or the default. A variable
is a boolean, an integer, a number, a string or a colour `{r, g, b[, a]}`, of its default's type.
The node shows it as the property `script_variables/<name>`, saves it with the scene and runs the
script again when it changes.

A node written in GDScript that extends one of these can add its own arguments after them by
defining `_get_script_arguments()`.

### In the editor

The editor plugin *Lua scripts* (`addons/libreloaded/editor/lua_scripts`) opens `.lua` files in the
script editor as text and colours them as Lua. When a script is saved - in the editor or in another
program - every script context that loaded it, through `require` as well, runs its code again in a
fresh Lua state, so a texture node shows the saved script at once, with no reload of the scene.

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

The events, timers, subscriptions, drawings, fonts and images a script creates are freed together
with the scenario, or when the editor applies new code. A script can free its own event with
`reloaded.event.free()`, stop a timer or subscription with `reloaded.cancel()`, and free a drawing,
a font or an image with `reloaded.draw.free()`.

### The sandbox

* Available libraries: `base`, `string`, `table`, `math`, `coroutine` and `utf8`.
* Not available: `io`, `os`, `debug`, `package`, `dofile` and `loadfile`. `load` accepts only
  text.
* `require("lib.util")` loads `lib/util.lua` from the script's directory. Nothing outside that
  directory can be loaded. Fonts and images are read only from the data directory, in the same
  way.
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

### `reloaded.time` - the clock on the wall

| Function | Returns / does |
|---|---|
| `after(seconds, fn)` | runs `fn()` once, after the seconds; returns a subscription |
| `every(seconds, fn)` | runs `fn()` every so many seconds; returns a subscription |

These timers follow real time, not the simulation's: they run while the simulation is paused or
runs faster, and in the editor. A board or a screen that only animates uses them; anything that
follows the simulation uses `reloaded.sim`. `reloaded.cancel()` stops them too.

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

### `reloaded.station` - stations, timetables and the trains running them

A timetable is a plan: the train and its entries, each a station with the times planned there. A
run is a timetable run by a trainset: it knows which entry the train has got to, and when the train
arrived at a station, left it or passed it. Entries are counted from 1. Times are hours since
midnight (`13.5` is half past one); the planned ones are the timetable's, the run's are those
shifted by its `offset`.

| Function | Returns / does |
|---|---|
| `find(name)`, `name(station)` | the station of the name, `nil` when there is none; its name |
| `run(v)` | the run of the vehicle's trainset, `nil` when it runs no timetable |
| `timetable(run)`, `offset(run)` | the timetable it runs; the hours it shifts its times by |
| `current_entry(run)` | the entry the train drives to or stands at; the entry count + 1 past the last |
| `arrived(run, i)`, `departed(run, i)` | when the train arrived at the entry, and left or passed it; `nil` until it did |
| `delay(run)` | how late the train is now, in minutes; early when negative |
| `train(timetable)` | the train's name (its number), category and label - three values |
| `origin(timetable)`, `destination(timetable)` | the station the train runs from and to, `nil` for none |
| `entry_count(timetable)` | how many entries it has |
| `entry(timetable, i)` | `{station =, arrival =, departure =, stop =, kilometre =, speed_limit =, track_count =}`; a time or a speed limit not given is `nil` |

```lua
local run = reloaded.station.run(vehicle)
if run then
    local timetable = reloaded.station.timetable(run)
    local number = reloaded.station.train(timetable)
    local destination = reloaded.station.destination(timetable)
    if destination then
        reloaded.log.info(number .. " to " .. reloaded.station.name(destination))
    end
end
```

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

### `reloaded.draw` - drawing on a texture

A drawing is a picture, for example a cab's screen or a station's board. It is drawn on the GPU. A
`LuaTextureNode` shows its drawing in a texture. A drawing created with `create()` can be pasted
into another drawing.

Coordinates are pixels, and `y` points down. Colours are `{r, g, b}` or `{r, g, b, a}`, each from
0 to 255. A colour with an alpha below 255 is mixed with what is under it. Points are a flat list
`{x1, y1, x2, y2, ...}`. Lines, outlines and pixels go through the pixels their coordinates name.
Rectangles and text start at their top left corner. Paths of fonts and images are relative to the
data directory.

| Function | Returns / does |
|---|---|
| `create(w, h[, transparent])` | a drawing, opaque black or transparent |
| `size(d)` | its width and height |
| `clear(d, colour)` | removes everything drawn and fills it with the colour |
| `rect(d, x, y, w, h, colour)` | a filled rectangle |
| `rect_outline(d, x, y, w, h, colour[, width])` | the edge of the rectangle, inside it |
| `line(d, points, colour[, width])` | a line through the points |
| `ellipse(d, x, y, w, h, colour)`, `ellipse_outline(...)` | the ellipse the rectangle bounds |
| `polygon(d, points, colour)`, `polygon_outline(d, points, colour[, width])` | |
| `pieslice(d, x, y, w, h, start, end, colour)` | a slice of the ellipse; angles in degrees, clockwise from three o'clock |
| `pixel(d, x, y, colour)` | one pixel |
| `load_font(path[, antialiasing])` | a TrueType or OpenType font; `"gray"` smooths its edges (the default), `"none"` draws it in whole pixels, as a LED board shows it |
| `text(d, x, y, text, font, size, colour)` | the text at a size in pixels |
| `text_size(font, text, size)` | the width and height the text takes |
| `load_image(path)` | an image (png, bmp, tga, dds, ...) |
| `load_mask(path)` | an image used as a mask: pasted with a `tint`, it is drawn in that colour. It is as opaque as the image's alpha, or as bright as the image when it has no alpha |
| `image_size(img)` | its width and height |
| `image_pixel(img, x, y)` | the colour of a pixel, `{r, g, b, a}` |
| `paste(d, source, x, y[, options])` | pastes an image or another drawing, mixed by its alpha. Options: `w`, `h` (scales it), `source = {x, y, w, h}` (pastes a part of it), `filter = "nearest"` or `"linear"`, `flip_x`, `flip_y`, `tint = colour` |
| `paste_rotated(d, source, cx, cy, angle[, filter])` | pastes it turned counter-clockwise by the angle in degrees, centred at `cx, cy` |
| `free(handle)` | frees a drawing, a font or an image the script created; `false` if the script did not create it |

A drawing is drawn again in the next frame after it changes, so a script redraws it only when its
content changes, for example from `reloaded.time.every()` or `reloaded.sim.every()`.

### `reloaded.log`

`debug(text)`, `info(text)`, `warning(text)` and `error(text)` write to the game log.

## Examples

A board that scrolls its text, run by a `LuaTextureNode` with a texture of 256x32:

```lua
local board = ...
local font = reloaded.draw.load_font("fonts/board.ttf")
local text = "The train to the station will depart from track 1"
local width = reloaded.draw.text_size(font, text, 20)
local x = 256

reloaded.time.every(0.05, function()
    reloaded.draw.clear(board, {0, 0, 0})
    reloaded.draw.text(board, x, 4, text, font, 20, {255, 180, 0})
    x = x - 2
    if x < -width then
        x = 256
    end
end)
```

For examples, see the documentation of a plugin. It shows scripts for its own sceneries and the
modules it adds.
