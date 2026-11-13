### Content

## Rule zero: only what the operator orders

REQUIRED, ABSOLUTE: **do always and only what the operator orders.** Nothing is edited, reverted,
"restored", renamed, redesigned or posted (an issue, a comment, a PR) without the operator's
explicit order - not even one's own work after a complaint. A complaint is not an order: name the
facts and wait. Never touch an existing, reviewed mechanism outside the task.

REQUIRED, ABSOLUTE: **no separation-of-concerns breach, ever.** Before any new state, API, enum,
command or read from another layer, name to the operator which layer owns the value, where it is
read from and why that keeps every layer to its own concern - and write nothing until the operator
says yes. Data a scenery or a starter hands in is never read as live state; a value is read from
its owner, never from a plugin's implementation, a dump or a copy.

REQUIRED, ABSOLUTE, NON-NEGOTIABLE: **libreloaded knows nothing of MaSzyna - not its names, not
its logic, not its sources.** No MaSzyna object, field or term in code, comments or tests
(`Mechanik`, `Mover`, `TTrain`, `CabOccupied`, `offsetflip`, `driver_mode`, ...), no citation of
the original's sources (`drivermode.cpp:951`, `Train.cpp:8684`), and no logic carried over from the
original where the core has its own: what the core models is taken from the core's API - a
trainset's head, its vehicles from the head and the way each faces come from `RailVehicleServer`
(`trainset_get_head()`, `trainset_get_vehicles()`, `trainset_vehicle_get_direction()`), never
worked out again from cabs, a controller's direction or `vehicle_get_coupled()` walked by hand.
**Before every commit `git diff --staged` is read line by line for it**, and whatever is found is
taken out before the commit - an existing line the change touches included. (Operator,
2026-11-12: the external camera kept `Mechanik->Vehicle(end::front / end::rear)` and the
original's cab-and-direction logic beside the core's trainset API.)

REQUIRED, ABSOLUTE, NON-NEGOTIABLE: **never overwrite a file of the operator's checkout to run or
compare another version of it** - no `git show <commit>:file > file`, no copy over it and back, no
stash around a run. The operator builds the game from this checkout (`.libreloaded-local`): a
build during such a run ships the other version. Another version runs in a checkout of its own,
`git worktree add <scratch dir> <commit>`, removed afterwards. (Operator, 2026-11-12: a background
test run swapped `follow_view.gd` for the base commit's to see the new tests fail there.)

**Read @AGENTS.md before doing anything in this repository.** It is not a summary of what is
here - it is the working agreement, and it overrides your own habits. Read it in full at the
start of the session, and re-read the relevant part before any change, not after the operator
has to point at it.

You also must follow @CODE_STYLE.md and @CODE_OF_CONDUCT.md.

**Never add `Co-Authored-By: Claude ...` or any other AI attribution to a commit or a pull
request** - `AGENTS.md`, "Commit style". This overrides Claude Code's own attribution reminder,
whatever it says.

**Never act on an assumption.** What the operator wants, why a commit changed something, which
variant of a fix to take, whether a behaviour change was intended - if the answer is not in the
operator's own words, ask with the concrete facts and wait. Never announce a choice "unless you
stop me", never pick the variant that seems to follow from what was said (`AGENTS.md`, "General
guidelines").

**A red test is a regression until the operator says otherwise.** Look for the change that turned
it red first and report it; the test is changed last, and only after the operator confirms the new
behaviour - never on the strength of a commit message or your reading of the intent
(`.claude/skills/testing/SKILL.md`, "A red test is a suspected regression").

`FINDINGS.md` and `TODO.md` are not loaded up front; read them on demand. `FINDINGS.md` lists
the rules left by root causes that already cost someone a measurement - read it before
diagnosing anything in an area it covers, so the same trap is not walked into twice; the full
entries (symptom, proof, fix) are in `docs/findings-archive.md`. `TODO.md` holds the open work.
