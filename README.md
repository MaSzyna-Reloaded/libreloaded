# libreloaded

The core of Rails Reloaded: a C++ GDExtension for Godot Engine and its GDScript addon, holding what
is common to a railway simulation - the servers, the vehicles' layers and their stepping, the cabin
system, the player and the cameras, tracks, traction, signalling, sceneries and their streaming,
scenario events and Lua scripts, and the test harness.

What a particular simulation brings of its own - its vehicles and their physics, its data formats,
sceneries, cabs and drawing - is a **plugin**: a repository of its own compiled into this library.
libmaszyna, the port of the MaSzyna train simulator, is one; a game is built of the core and the
plugins it needs.

## Layout

| Path | What it is |
|------|------------|
| `src/` | the C++ of the core, compiled into the library |
| `addons/libreloaded/` | the Godot addon: GDScript, scenes, shaders, editor plugins |
| `doc_classes/` | the class reference of the C++ classes (also shown by the editor under F1) |
| `demo/` | the test project: the tests (`demo/tests/`), their harness and fixtures |
| `docs/` | the guides of the documentation site |
| `vendor/` | gut, gnd-sfx and Lua, as submodules |
| `godot-cpp` | Godot's C++ bindings, as a submodule |
| `ci/` | the Linux SDK image and `fetch-godot.sh` |

## Plugins

A plugin is a directory with a `plugin.cmake`, given to the build by `LIBRELOADED_PLUGINS`
(`;`-separated, relative to this checkout or absolute):

```bash
make compile-debug LIBRELOADED_PLUGINS=<path to the plugin> GODOT_PROJECT_DIR=<its Godot project>
```

Its `plugin.cmake` adds the plugin's sources and `doc_classes` to the library, links its addon into
the project and names its `ReloadedPlugin` class, whose stages the core runs at the library's start
and end. How a plugin plugs into the core - its implementations, backends and providers - is in the
documentation, "Overall architecture". A plugin is normally built from its own repository, whose
`Makefile` runs this one's rules with itself as `LIBRELOADED_PLUGINS`.

## Requirements

- Godot 4.7, **double precision** (`godot-double`; another name with `GODOT=<binary>`) - the library
  is built against double precision godot-cpp, and a single precision Godot does not load it
- CMake 3.30 or newer, Python 3, a C++17 compiler

## Getting started

```bash
git clone --recursive https://github.com/MaSzyna-Reloaded/libreloaded
cd libreloaded
make compile-debug        # or just make
make run-tests            # the GUT tests of demo/, headless
```

`make compile-release` builds the release library; the library lands in `demo/bin/libreloaded/`.
Add `LIBRELOADED_DEBUG=ON` for the `DEBUG` macro (`make compile-debug LIBRELOADED_DEBUG=ON`).
The number of parallel jobs is the number of cores less two.

### Other platforms

| Target | What it builds |
|--------|----------------|
| `make compile-release-linux` | the Linux release library, in the Linux SDK container (`ci/docker/linux-sdk`): an old glibc, so it starts on older distributions |
| `make compile-windows-debug`, `compile-windows-release` | Windows, cross-compiled with MinGW |
| `make cross-compile-debug`, `cross-compile-release` | Linux and Windows together |
| `make compile-android-debug`, `compile-android-release` | Android, with the NDK of `ANDROID_NDK_ROOT` |
| `make compile-profiling` | `template_debug` with `RelWithDebInfo`, for profiling |

For Android see also [Godot's documentation](https://docs.godotengine.org/en/stable/tutorials/export/exporting_for_android.html).

## Simulation timing

**The simulation is stepped before anything that reads it.** `SimulationServer` advances its clock
on `SceneTree`'s `process_frame`, which is emitted before every node's `_process`, and
`VehicleServer::stepping_advance()` hands the frame to each vehicle implementation registered with
it (`VehicleImplementationServer`). Drawing, the cabin, the HUD and the cameras therefore see the
position of *this* frame. How an implementation steps its vehicles - in which phases, with what
sub-steps - is its own business.

## Documentation

The documentation is a Jekyll site: the guides in `docs/` and the class reference generated from
`doc_classes/` and the scripts' `##` comments. A build of the library is documented with the
plugins it is built with - each plugin's guides go under its name, the class reference covers all
of them:

```bash
make docs-site [LIBRELOADED_PLUGINS=...]   # assembles it into build-docs/site
make docs-server                           # and serves it (Jekyll, or Ruby's image in Docker)
make docs-pdf                              # the same as one PDF, build-docs/reloaded-docs.pdf
make docs                                  # refreshes doc_classes/ from the library
```

`make docs` dumps the class reference of the whole library and hands each class's XML to the
repository declaring it, so the core keeps only its own. The editor shows the same reference under
F1: the library carries the `doc_classes` of the core and of every plugin built into it.

## Code quality

```bash
make style-check [STYLE_FILE=<file>]   # clang-format and clang-tidy
make style-fix [STYLE_FILE=<file>]     # clang-format; clang-tidy findings are fixed by hand
make install-git-hooks                 # once per clone: the pre-commit hook checks staged .uid files
```

Both style targets use `.clang-format` and `.clang-tidy` of this repository and skip `src/gen` and a
plugin's vendored code (`*/maszyna-mover/*`); with `LIBRELOADED_PLUGINS` they check the plugins'
sources as well. The rules of the code are in `AGENTS.md` and `CODE_STYLE.md`.

## CI

- `build` - the tests and the library of each platform, on pull requests
- `style-check` - clang-format and clang-tidy, on pull requests
- `docs` - builds the site, and publishes it on GitHub Pages from `main`
- `godot-engine` - Godot publishes no double precision build, so the editor and the export
  templates are built here, once per engine version (`GODOT_VERSION` in the `Makefile`), and kept
  as this repository's release `godot-<version>-double`. `ci/fetch-godot.sh` installs them from
  there - in this repository's CI, a plugin's or a game's

The Linux release library is built in the image `ghcr.io/maszyna-reloaded/linux-sdk`, tagged by the
hash of `ci/docker/linux-sdk/Dockerfile`; CI pulls it. After changing the Dockerfile, build and
publish the image yourself (a `gh` token with the `write:packages` scope:
`gh auth refresh -h github.com -s write:packages`):

```bash
gh auth token | docker login ghcr.io -u <github user> --password-stdin
make linux-sdk-image-push
```

If you have found a bug, have a suggestion or want to join us - open an
[issue](https://github.com/MaSzyna-Reloaded/libreloaded/issues) or start a
[discussion](https://github.com/MaSzyna-Reloaded/libreloaded/discussions).
