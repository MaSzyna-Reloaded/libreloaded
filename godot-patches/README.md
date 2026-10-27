# Godot patches

Fixes of Godot the engine is built with before Godot releases them - each a pull request of
godotengine/godot, as its `gh pr diff`, named `<pull request>-<what it fixes>.patch`. The
`Makefile` applies them in name order to the `GODOT_VERSION`-stable clone and names the engine's
build after them (`GODOT_BUILD`), so a patch added is a new release `godot-<build>-double`, built by
the `godot-engine` workflow - called first by the pull request's CI when the release is missing -
and fetched by `ci/fetch-godot.sh`. A game's `make
release-linux`/`release-windows` builds the template it misses itself.

A patch goes when the Godot version it is merged into is the one built.

A fix of our own that is no pull request is named `local-<what it fixes>.patch`, applied after
them.

* `118329-sdfgi-ambient-light.patch` - godotengine/godot#118329: SDFGI takes the environment's
  ambient light as the forward pass does (godotengine/godot#55104). Without it the ground within
  SDFGI's cascades, a cube around the camera, is darker than beyond them (MaSzyna-Reloaded/libreloaded#9)
* `local-resource-loader-gdscript-deadlock.patch` - a load waiting on a resource another thread
  loads (`ResourceLoader::_run_load_task()`, its sleeping wait) releases the mutexes the thread
  allows unlocking, as a wait within the WorkerThreadPool does. Without it a scene loaded with
  `load_threaded_request()` hung at random, forever: a script compiled in it held
  `GDScriptCache`'s mutex while it waited on the scene it preloads, and the worker loading that
  scene waited on the same mutex for the scene's own script (MaSzyna-Reloaded/libreloaded#11).
  The game's main scene hung behind its loading screen every few launches; a project of 256 such
  preloads hung 11 runs in 30 without it, none in 100 with it
