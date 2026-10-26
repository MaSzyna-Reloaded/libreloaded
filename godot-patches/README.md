# Godot patches

Fixes of Godot the engine is built with before Godot releases them - each a pull request of
godotengine/godot, as its `gh pr diff`, named `<pull request>-<what it fixes>.patch`. The
`Makefile` applies them in name order to the `GODOT_VERSION`-stable clone and names the engine's
build after them (`GODOT_BUILD`), so a patch added is a new release `godot-<build>-double`, built by
the `godot-engine` workflow run by hand and fetched by `ci/fetch-godot.sh`. A game's `make
release-linux`/`release-windows` builds the template it misses itself.

A patch goes when the Godot version it is merged into is the one built.

* `118329-sdfgi-ambient-light.patch` - godotengine/godot#118329: SDFGI takes the environment's
  ambient light as the forward pass does (godotengine/godot#55104). Without it the ground within
  SDFGI's cascades, a cube around the camera, is darker than beyond them (MaSzyna-Reloaded/libreloaded#9)
