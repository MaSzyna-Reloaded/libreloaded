# libreloaded

The core of Rails Reloaded. It is a C++ GDExtension for the Godot Engine with a GDScript addon. It
contains what every railway simulation needs: servers for vehicles, cabins, the player and
cameras; tracks, traction and signalling; sceneries, scenarios and Lua scripts; and a test harness.

Everything specific to one simulation is a **plugin**: its vehicles and their physics, its file
formats, sceneries, cabins and rendering. A plugin is a separate repository, compiled into the same
library. A game is built from the core and the plugins it needs. An example of a plugin is
[libmaszyna](https://github.com/MaSzyna-Reloaded/libmaszyna).

## Layout

| Path | Contents |
|------|----------|
| `src/` | C++ sources of the core |
| `addons/libreloaded/` | the Godot addon: GDScript, scenes, shaders, editor plugins |
| `doc_classes/` | API reference of the C++ classes, also shown in the editor's help (F1) |
| `demo/` | the test project, with the tests in `demo/tests/` |
| `docs/` | pages of the documentation site |
| `vendor/` | gut, gnd-sfx and Lua, as submodules |
| `godot-cpp` | Godot's C++ bindings, as a submodule |
| `ci/` | the Linux SDK Docker image and `fetch-godot.sh` |

## Requirements

- Godot 4.7, **double precision**. The binary is expected as `godot-double`; set `GODOT=<binary>`
  to use another name. A single precision Godot cannot load the library.
- CMake 3.30 or newer, Python 3 and a C++17 compiler.

## Getting started

```bash
git clone --recursive https://github.com/MaSzyna-Reloaded/libreloaded
cd libreloaded
make compile-debug        # or just: make
make run-tests            # runs the GUT tests of demo/ headless
```

`make compile-release` builds the release library. The library is written to
`demo/bin/libreloaded/`. To enable the `DEBUG` macro, add `LIBRELOADED_DEBUG=ON`. The build uses
all CPU cores but two.

### Other platforms

| Target | Builds |
|--------|--------|
| `make compile-release-linux` | the Linux release library in the Linux SDK container, with an old glibc so it runs on older distributions |
| `make compile-windows-debug`, `compile-windows-release` | Windows, cross-compiled with MinGW |
| `make cross-compile-debug`, `cross-compile-release` | Linux and Windows together |
| `make compile-android-debug`, `compile-android-release` | Android, using the NDK in `ANDROID_NDK_ROOT` |
| `make compile-profiling` | `template_debug` with `RelWithDebInfo`, for profiling |

For Android, see also
[Godot's documentation](https://docs.godotengine.org/en/stable/tutorials/export/exporting_for_android.html).

## Plugins

A plugin is a directory with a `plugin.cmake` file. Pass it to the build in `LIBRELOADED_PLUGINS`.
Separate several plugins with `;`. Paths are relative to this repository or absolute.

```bash
make compile-debug LIBRELOADED_PLUGINS=<plugin directory> GODOT_PROJECT_DIR=<Godot project>
```

The `plugin.cmake` file adds the plugin's sources and `doc_classes` to the library, links its addon
into the project and names its main class, a `ReloadedPlugin`. Usually you build a plugin from its
own repository: its `Makefile` calls this repository's targets with itself as
`LIBRELOADED_PLUGINS`. The extension points are described in the documentation, under
"Architecture".

## Simulation timing

The simulation is stepped before anything reads it. `SimulationServer` advances its clock on
`SceneTree`'s `process_frame` signal, which is emitted before any node's `_process`. Then
`VehicleServer::stepping_advance()` steps every registered vehicle implementation. So rendering,
the cabin, the HUD and the cameras always see the vehicles' positions for the current frame. How an
implementation steps its vehicles internally is up to the implementation.

## Documentation

The documentation is a Jekyll site. It contains the pages in `docs/` and an API reference generated
from `doc_classes/` and from the `##` comments in the scripts. When the site is built with plugins,
it also contains each plugin's pages and classes.

```bash
make docs-site [LIBRELOADED_PLUGINS=...]   # builds the site in build-docs/site
make docs-server                           # serves it locally (Jekyll, or Ruby in Docker)
make docs-pdf                              # builds one PDF, build-docs/reloaded-docs.pdf
make docs                                  # updates doc_classes/ from the built library
```

`make docs` updates the XML of every class in the library and puts each file into the repository
that declares the class. The editor's help (F1) shows the same reference, for the core and for
every plugin built into the library.

## Code quality

```bash
make style-check [STYLE_FILE=<file>]   # clang-format and clang-tidy
make style-fix [STYLE_FILE=<file>]     # clang-format only; fix clang-tidy findings by hand
make install-git-hooks                 # once per clone: a pre-commit hook that checks .uid files
```

Both style targets use this repository's `.clang-format` and `.clang-tidy`. They skip `src/gen` and
vendored code in plugins. With `LIBRELOADED_PLUGINS` set, they also check the plugins' sources. The
coding rules are in `AGENTS.md` and `CODE_STYLE.md`.

## CI

- `build`: tests and the library for each platform, on pull requests.
- `style-check`: clang-format and clang-tidy, on pull requests.
- `docs`: builds the site, and publishes it on GitHub Pages from `main`.
- `godot-engine`: Godot does not publish double precision builds, so this workflow, run by hand,
  builds the editor and the export templates once per engine build (`GODOT_BUILD` in the `Makefile`: the
  version and the fixes of Godot in `godot-patches/`). They are published as the release
  `godot-<build>-double` of this repository. `ci/fetch-godot.sh`
  installs them, in this repository's CI and in the CI of plugins and games.

The Linux release library is built in the Docker image `ghcr.io/maszyna-reloaded/linux-sdk`. The
image is tagged with a hash of `ci/docker/linux-sdk/Dockerfile`. When the Dockerfile changes, CI
builds and publishes the new image once (`make linux-sdk-image-push`). Nobody has to publish it by
hand.

If you found a bug, have a suggestion or want to join us, open an
[issue](https://github.com/MaSzyna-Reloaded/libreloaded/issues) or start a
[discussion](https://github.com/MaSzyna-Reloaded/libreloaded/discussions).
