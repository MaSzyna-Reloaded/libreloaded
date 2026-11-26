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
* `0006-local-sdfgi-sharp-reflections.patch` - on `0005-local-sdfgi-transparent-reflections.patch`,
  a surface within SDFGI's cascades takes its reflection from the probes only as its lobe grows as
  wide as their texels (some 30 degrees, `SDFGI_OCT_SIZE` 6): from a roughness of 0.35, all of it
  from 0.6 (`SDFGI_PROBE_ROUGHNESS_FROM`, `SDFGI_PROBE_ROUGHNESS_FULL`), the opaque surfaces'
  (`gi.glsl`) as the transparent ones'. Below, the reflection is traced through the distance field
  and the sky's is taken where the ray leaves it. Without it a smooth surface of a roughness of 0.2
  and more - water raised by a wind - took a blur of everything around it from the probes, darker
  and duller than the sky's reflection beyond the cascades: a hard square on every lake. A traced
  ray takes as a hit only what it comes nearer to than it starts from the surface it leaves
  (`start_clearance`): the hit's softness grows to some 4 cells with the roughness, farther than
  the ray's start is pushed off its own surface, and water of a roughness of 0.15 and more traced
  its reflection into itself - dark within the cascades, the same square
* `0007-119867-tighter-directional-shadowmap-draw-rect.patch` - godotengine/godot#119867 (on
  godotengine/godot#119705, numerical stability of directional shadows far from the world origin):
  a directional light's shadow pass only draws the texel-snapped bounding rectangle of the camera
  frustum it actually covers within the stabilized, sphere-sized shadow map, instead of the whole
  square - this is GPU time, not resolution: the sampled texel density, `shadow_texel_size` and
  the cascades' world-space footprint are unchanged, so it does not sharpen shadows at any
  distance, only the overdraw cost of casters that fall in the (already wasted) square but outside
  the frustum's rectangle. Hand-adapted to 4.7.2: master already calls
  `light_culler->prepare_directional_light_cascade()` from within the per-cascade loop, an API
  4.7.2 does not have (only the per-light `prepare_directional_light()` at the top of the function,
  kept as is) - both PRs' diffs carry that call, dropped here along with the `receiver_frustum_planes`
  it alone fed
* `0008-local-directional-shadow-split-stability.patch` - on `0007-...`, for every split: drops
  its tighter draw rect (`USE_TIGHTER_DRAW_RECT`, `normalized_render_bounds` now always the full
  rect) and the far plane's snap to steps of `frustum_circumscribing_radius`
  (godotengine/godot#119705). Both are safe at the cascade distances Godot itself ships with, but
  a split given a short enough distance to sharpen a scene's own close-up content (the cab, here)
  exposed both as real, measured flicker - not from camera motion, a motionless camera flickered
  too (see `FINDINGS.md`, "a directional shadow split's sampled rect is Godot's own stabilized
  bounding sphere", 2026-11-26, for what was tried first and ruled out before this). The square
  the tighter draw rect would have saved GPU time on is small enough at a short split distance
  that the trade is not worth it; the far plane snap bought nothing a texel-grid-snapped x/y did
  not already give

  The flicker the player kept seeing after both turned out to not be on the first split at all - a
  capture of it (2026-11-26) pointed at the cab's own side wall, a couple of metres and more from
  the camera: with a cab-tight first split (a couple of metres), everything of the cab beyond it -
  the far wall, the ceiling - falls in the *second* split, which at the defaults still reaches from
  there to 125 m and was still in density mode. A density split that wide, this close, resizes by a
  visible amount every frame, on a flat wall large enough to show it plainly. The second split now
  keeps the sphere as well - `cascade_prefers_density` is `i > 1`, not `i > 0` - leaving density
  mode to the splits actually far enough that a resize is never seen (125 m and out, at the
  defaults). Still not confirmed fixed in the game; a cab-tight second split distance
  (`reloaded/scenery/shadows/split_2`, a project setting, no engine patch needed) is also expected
  to matter here, the same way the first split's did for the cab itself
