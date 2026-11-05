---
name: testing
description: Write, change, run and judge tests of Reloaded Core (GUT, demo/tests, ReloadedGutTest) and headless probes. Use before adding or editing a test, before "fixing" a test that went red, before running tests ahead of a commit, when a change touches physics (vehicle components, the cab logic that commands them), and when a headless probe has to prove a hypothesis. Answers "what may a test assert", "is this red test a regression", "how do I run just my test" and "why does my probe read frozen values".
---

# Testing

`AGENTS.md` ("Checks", "Before every commit") and `CODE_STYLE.md` ("Tests") bind as written; this
skill gathers them with what they cost us when they were broken.

## A red test is a suspected regression - ABSOLUTE

A test that turns red after a change is **distrusted, not fixed**. Before touching it:

1. Read what it guarded - its assertions and its setup - find the change that turned it red, and
   report it to the operator: the commit, the line, what it does to the real data. "On purpose"
   is the **operator's** word, never yours: a commit message, a comment citing the original or
   your own reading of the intent does not make a behaviour change wanted. Until the operator
   confirms that exact behaviour, the change is the regression and the test stays as it is - the
   regression is looked for first, the test is changed last.
2. Never make it pass by removing its setup or its assertion, by loosening a tolerance, or by
   giving it a value the code now happens to produce.
3. When the change moved a responsibility (a field to another component, a command to another
   owner), the test follows it to the new owner with the **same** assertion.

2026-10-04: the battery moved from the controller to `RailVehiclePowerSupply`; five tests lost
`battery_voltage = 110` with nothing in its place and stayed green, because none of them asserted
the low voltage. The tester could not start the vehicles. Breaking this rule breaks the project's
rules.

## The test steps the simulation - no timeouts - PROHIBITED, ABSOLUTE

A test's result never depends on how fast the machine is. On 2026-10-07 a vehicle's start-up test
went red on CI and green locally: the simulation then moved with the frames (SimulationRuntime,
real frame time x the speed, up to 25 simulated seconds a frame at x100), the test checked between
frames, and it waited 120 simulated seconds for the effect of a key that had not been taken at
all. The rules that came out of it:

- **No SimulationRuntime in the tests.** `simulation_runtime_hook.gd` places none: no frame moves
  SimulationServer's clock. The test steps it, tick by tick, as a step debugger does -
  `ReloadedGutTest.step(count)`, `wait_simulated_until(done, seconds, what)`, `ticks(seconds)`,
  `TICK` = 1/30 s (the simulation's tick to come, #301). Never `wait_seconds()`, never
  `wait_idle_frames()` or `Time.get_ticks_msec()` to let the simulation run, never a loop on
  `simulation_get_time()` awaiting frames (with no runtime it never ends).
- **Everything that moves with the simulation moves by simulation time** - the vehicles, the
  drivers, the events, and the cab's view of them: a widget's animation (`BaseCabinTool3D._process`,
  on its `SimulationClock`) and the cab's own timing (`CabinSystem`, on `simulation_advanced`). A
  test step moves them all; no frame rate (`--fixed-fps`) is needed or wanted.
- **A key press lasts at least one tick**: a cab logic that takes keys on the simulation's step
  never sees a press that ends before the next tick.
- **A command or a key takes on the next tick, or it never will.** Check its own effect after one
  tick - the switch moved, the controller's position changed - and fail there. Never fold "did
  the key work" into the wait for what it starts.
- **A physical process is waited for as long as the data says it takes**, in simulated seconds
  turned into ticks: the doors' `open_delay + max_shift / open_speed` (`RailVehicleDoors`), a
  relay's or a brake's delay from the vehicle's configuration, plus one `TICK` for the step that
  shows it. Never a blanket constant ("120 s a step", "20 s for the doors") and never a margin of
  a file's own (0.1, 0.25 - frame-clock numbers): the margin is `TICK`, from `ReloadedGutTest`.
- **A wait that runs out fails the test there**, saying what did not come about
  (`wait_simulated_until()` does). GUT's `wait_until()`/`wait_for_signal()` only return false -
  one whose result is ignored lets the test run on, or pass. An `await` of a signal with no limit
  hangs the run.
- **Loading is the machine's work, not simulated time**: a scenery fixture, a model, a screen.
  Its limit is real time and only a hang guard - a few seconds, named, the result checked
  (`ReloadedGutTest.wait_loaded()`, `LOAD_TIMEOUT`; a fixture loads in under 0.1 s). Tens of
  seconds for it is the same antipattern.

## Physics is tested end to end and blackbox - ABSOLUTE

- A change to a physics component - a vehicle component (`src/vehicles/`), the cab logic that
  commands them (`addons/libreloaded/cabin/`) - is tested through the whole sequence a player goes
  through, not only the field that moved: battery -> low voltage (`power24_available`) ->
  pantographs -> relay reset -> main switch -> converter (`power110_available`) -> controller ->
  speed. A flag that is set (`battery_enabled`) proves nothing about what depends on it. The core
  has no backend of its own: such a sequence runs where a backend is, in a plugin's tests
  extending `ReloadedGutTest`.
- Physics tests are **blackbox**: they assert on what components return (typed getters -
  `RailVehiclePowerSupply.get_power24_available()`, `RailVehicleEngine.get_main_switch_enabled()`)
  and/or on the state built into the dump (`get_state()`, `VehicleServer.vehicle_dump_state()`).
  Never on a backend's internals, private members (`_name`) or the way a component got its value.
- Test the real entry point: commands through `vehicle_send_command`, cab controls through
  `CabinSystem.act` on a `CabinLogic` - a hand-built struct can pass while the real path is
  broken.

## Data reading is tested on the data's variants - ABSOLUTE

A test of anything read from data carries a fixture with the variants the data really has, not
only the commonest one: optional parts left out, entries in another order, names other than the
convention's. A fixture that only repeats the convention proves nothing.

## What a test may use

- Only fixtures: `demo/tests/fixtures/`. Never a game's data directory - CI has none.
- Only the public interface of the tested class (`CODE_STYLE.md`, "Tests"). Needing private access
  means the API is missing something.
- Never a bare `[]`/`{}` to a parameter typed `Array[T]`/`Dictionary[K, V]` - the call is refused
  silently and the test checks nothing.
- A regression test is proven both ways: red on the code before the fix, green after.
- No throwaway diagnostic script in `demo/tests/` - probes live in the scratchpad.

## Running

- Parse-check first: `godot-double --headless --path demo --check-only -s res://tests/<file>.gd`
  (5 s). A script that does not parse is skipped by GUT, `-gselect` then matches nothing and the
  whole directory runs until the timeout - it looks like a hang.
- Only the scripts you wrote or changed, **all in one process** - Godot's and GUT's start-up is
  paid once, not per script:
  `godot-double --headless --path demo --log-file <scratchpad>/godot.log -s addons/gut/gut_cmdln.gd -gconfig= -gpre_run_script=res://tests/simulation_runtime_hook.gd -gtest=res://tests/a.gd,res://tests/b.gd -gexit`.
  `-gconfig=` is required: `.gutconfig.json`'s `dirs` adds the whole directory to `-gtest`, which
  is why it once looked as if `-gtest` did not filter. Many scripts: two or three such processes
  side by side, the list split between them. One script: `-gdir=res://tests/ -gselect=<script>`.
  Never the whole suite.
- Redirect to a file and read the file - `| grep | head` kills the run with SIGPIPE.
- Every headless run takes `--log-file <scratchpad>/godot.log`: it shares the project's user
  directory, and its own `logs/app.log` rotates the operator's (five runs and it is gone).
- **A long timeout hides the early error.** A probe that hangs or fails in its third second, wrapped
  in `timeout 170`, says nothing for 170 s and then only "exit 124". Never one blocking call with a
  generous ceiling: the probe prints a line at every stage (`STAGE loaded`, `STAGE player in`, one
  per step), runs in the background into a file, and the file is watched - no new line for 15 s
  means it is hung or dead: kill it and read the file at once. The ceiling is the time the next
  stage should take, never the whole run "just in case".
- **A probe of a real scenery is capped at 60-120 s, whole run** (`timeout 120`), never more. It
  runs in the background; the first 15 s of its output are read for `Parse Error`/`SCRIPT ERROR`
  before anything waits on it (a probe that does not parse idles silently - 2026-10-10 a turn
  blocked 10 minutes on one). It prints its load progress (`load_progress`), so a long load is not
  silence. A long simulated span is reached by stepping more ticks per frame
  (`SimulationServer.simulation_advance(TICK)` N times between frames), never by a longer
  timeout. Never a tool call that sits in the turn waiting for a probe.
- A probe never `save_*`s a user setting - a killed probe leaves it written in the operator's
  `settings.cfg`. In memory only: `UserSettings.set_setting(section, key, value)`.
- `godot-double`, never `godot` (double-precision extension).
- After every headless run `git status`: Godot rewrites `.tres`/`.tscn`; restore what you did not
  edit.

## Headless probe of a vehicle (scratchpad)

A probe reads frozen values - and "proves" anything - unless the vehicle is really simulated:

- it stands on a track: `RailVehiclePhysicsNode` plus a `RailVehicle3D` whose `controller_path`
  is set **before** it enters the tree (only `RailVehicleServer.vehicle_is_attached()` vehicles are
  stepped);
- the clock runs: a probe steps it itself (`SimulationServer.simulation_advance()`, as
  `ReloadedGutTest.step()` does) - with no `SimulationRuntime` nothing else moves it; check that
  `SimulationServer.simulation_get_time()` grows;
- it is driven: `PlayerServer.player_take_over_vehicle()`, or a driver whose implementation is
  attached through `DriverServer` (`ReloadedGutTest.attach_driver_implementation()`);
- waits are in simulated time the probe steps, never in frames or `create_timer` - a headless
  frame is microseconds and a real second says nothing of the simulation.

A probe using autoloads (`CabinSystem`, `SceneryServer`) runs as a scene
(`godot-double --headless --audio-driver Dummy --path demo <scratch>/probe.tscn`); a `-s` script
does not compile against them. For a regression with no obvious cause, build the commit before it
in a copy in the scratchpad (`git archive <rev>^ | tar -x`, symlink `godot-cpp` and `vendor/*`,
its own `build-debug`) and run the same probe on both: the first step that differs is the
regression.
