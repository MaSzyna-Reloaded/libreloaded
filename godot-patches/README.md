# Godot patches

Fixes of Godot the engine is built with before Godot releases them - each a pull request of
godotengine/godot, as its `gh pr diff`, named `<order>-<pull request>-<what it fixes>.patch`. The
`Makefile` applies them in the order the four digits of `<order>` give - a patch built on another
comes after it - to the `GODOT_VERSION`-stable clone and names the engine's build after them (`GODOT_BUILD`), so a patch added is a new release `godot-<build>-double`, built by
the `godot-engine` workflow - called first by the pull request's CI when the release is missing -
and fetched by `ci/fetch-godot.sh`. A game's `make release-linux`/`release-windows` builds the
template it misses itself.

A patch goes when the Godot version it is merged into is the one built.

A fix of our own that is no pull request is named `<order>-local-<what it fixes>.patch`.

* `0001-117669-auto-exposure-histogram.patch` - godotengine/godot#117669: auto exposure measures the
  picture by a histogram of its luminance (Rec. 709) in log2 steps between the min and max
  sensitivity and takes its log average (godotengine/godot-proposals#3080). Without it the
  picture's plain average of `max(r, g, b)` was measured: a cab's windows or the sky outside
  pulled it up, so no scale and sensitivities kept both a cab and the outside right - the outside
  burnt out with the cab's, a wagon's underframe stayed dark with the outside's
* `0002-118329-sdfgi-ambient-light.patch` - godotengine/godot#118329: SDFGI takes the environment's
  ambient light as the forward pass does (godotengine/godot#55104). Without it the ground within
  SDFGI's cascades, a cube around the camera, is darker than beyond them (MaSzyna-Reloaded/libreloaded#9)
  - with the reflections kept the sky's: the ambient light is diffuse only, as in the forward pass;
  the probes record where their rays reached the sky (the history's alpha) and the store pass
  gives the radiance the sky back there. Without it water within the cascades reflected the ambient
  colour - a grey, warm square on every river
* `0003-local-auto-exposure-metering.patch` - on `0001-117669-auto-exposure-histogram.patch`, auto
  exposure measures as Unreal Engine's histogram does: a pixel darker than the min sensitivity is
  counted at it, not left out (below a luminance of 0.005); the darkest pixels, in percent of the
  picture `RenderingServer.camera_attributes_set_auto_exposure_low_percent()` (10 by default; no
  property of `CameraAttributes`, so a plain Godot opens what sets it), and the brightest 10%
  (`HIGH_PERCENT`) are left out of the log average; the middle of the picture counts four times as
  much as its corners (center-weighted metering, `CENTER_WEIGHT`); and the exposure adapts in log2
  luminance, up and down alike. Without it an evening's dark ground went out of the measure, and
  its sky and lamps alone darkened the picture to black; a cab's window frame at the edges of the
  picture brightened it all
* `0004-local-resource-loader-gdscript-deadlock.patch` - a load waiting on a resource another thread
  loads (`ResourceLoader::_run_load_task()`, its sleeping wait) releases the mutexes the thread
  allows unlocking, as a wait within the WorkerThreadPool does. Without it a scene loaded with
  `load_threaded_request()` hung at random, forever: a script compiled in it held
  `GDScriptCache`'s mutex while it waited on the scene it preloads, and the worker loading that
  scene waited on the same mutex for the scene's own script (MaSzyna-Reloaded/libreloaded#11).
  The game's main scene hung behind its loading screen every few launches; a project of 256 such
  preloads hung 11 runs in 30 without it, none in 100 with it
* `0005-local-sdfgi-transparent-reflections.patch` - a transparent material within SDFGI's cascades
  takes the sky's reflection as smooth as it is (`min(1, roughness * 5)`, as `gi.glsl` does where
  an opaque surface's traced ray reaches the sky), and its reflection goes into the sky's at the
  last cascade, where `scene_forward_clustered.glsl` overwrote the blend. Without it water had a
  hard edge where SDFGI ends: blurred, darker probe reflections within, the sky beyond
