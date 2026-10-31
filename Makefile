.PHONY: linux-sdk-image linux-sdk-image-push compile-release-linux compile-android-release compile-android-debug godot-version godot-build godot-bin docs compile watch-and-compile docs-site docs-server docs-install docs-pdf cleanup style-check style-fix compile-release-symbols
.DEFAULT_GOAL = compile-debug

# The Godot project the library is built into (its bin/libreloaded/) and whose addons/ CMake links
# the addon into - relative to this directory or absolute; a game with libreloaded as a submodule
# passes its own: make -C vendor/libreloaded compile-debug GODOT_PROJECT_DIR=$(CURDIR)
GODOT_PROJECT_DIR?=demo
# The plugins built into the library (their directories, each with its plugin.cmake): none for the
# core alone; libmaszyna and a game pass theirs: make compile-debug LIBRELOADED_PLUGINS=<path>
LIBRELOADED_PLUGINS?=
CMAKE_PROJECT_ARGS=-DGODOT_PROJECT_DIR=$(GODOT_PROJECT_DIR) "-DLIBRELOADED_PLUGINS=$(LIBRELOADED_PLUGINS)"
# A build's directories are its plugins' own (build-debug-<tag>, the core alone's build-debug): the
# games built on this checkout - each its plugins - build side by side instead of reconfiguring one
# directory in turn, and one build's objects never land in another's library
BUILD_TAG:=$(if $(LIBRELOADED_PLUGINS),-$(shell printf '%s' "$(LIBRELOADED_PLUGINS)" | sha256sum | cut -c1-8))
# Every step of a build holds this checkout's lock: two builds at once wait for each other rather
# than write the same files (gnd-sfx's, a directory of the same plugins)
BUILD_LOCK=flock $(CURDIR)/.build.lock
CMAKE_BUILD_JOBS=$(shell cores=$$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 1); if [ "$$cores" -gt 2 ]; then echo $$((cores - 2)); else echo 1; fi)
CLANG_TIDY_BUILD_DIR=build-clang-tidy
CLANG_TIDY_COMPILE_COMMANDS_FILE=$(CLANG_TIDY_BUILD_DIR)/compile_commands.json
CLANG_TIDY_BINDINGS_FILE=$(CLANG_TIDY_BUILD_DIR)/godot-cpp/gen/include/godot_cpp/classes/node.hpp
LIBRELOADED_DEBUG:=""
# The extension is built against double precision godot-cpp, so only a Godot built the same way
# can load it - a single precision binary dies with a glibc heap assertion while the module
# initialises. Override when your double precision build is named differently:
#   make compile-debug GODOT=godot-double
GODOT?=$(shell command -v godot-double >/dev/null && echo godot-double || echo godot)
CMAKE_GODOTCPP_API_VERSION=4.7
# f21961238's precision and API file: the double-precision API, dumped from $(GODOT) and never
# versioned; CMakeLists.txt binds it anyway (FORCE)
CMAKE_GODOTCPP_PRECISION=double
CMAKE_GODOTCPP_API_FILE=$(CURDIR)/extension_api.json
# The engine the library is built for - a game exporting with it has to use exactly this one, since
# the export looks its template up by this version
GODOT_VERSION:=4.7.2
# Fixes of Godot's not released yet, which the engine is built with: godot-patches/*.patch, applied
# in name order to the clone (godot-patches/README.md). The engine's build - its directory, its
# release - is named after them as well, so a patch added is a new engine everywhere it is fetched
GODOT_PATCHES:=$(sort $(wildcard $(CURDIR)/godot-patches/*.patch))
GODOT_BUILD:=$(GODOT_VERSION)$(if $(GODOT_PATCHES),-$(shell cat $(GODOT_PATCHES) | sha256sum | cut -c1-8))

# glibc is only forward compatible: a library or template linked against a rolling distribution's
# glibc (2.43-2.44 here) refuses to start on anything older - Ubuntu 22.04/24.04, Debian 12, Mint.
# The Linux release is therefore built in ci/docker/linux-sdk, Godot's own buildroot SDK
# (glibc 2.34). The checkout is mounted at its own path and the build runs as the host user, so
# the cmake cache and every output land exactly where a host build would put them; a game building
# into its own project mounts its checkout (LINUX_SDK_MOUNT), this one too when it is not inside
# it (the game's make libreloaded-local), and every plugin of LIBRELOADED_PLUGINS outside both (a
# plugin is a repository of its own).
# Published on ghcr.io and tagged by its Dockerfile: the CI pulls it rather than building it, and a
# changed Dockerfile is a new tag the CI builds once and publishes itself (make linux-sdk-image-push)
LINUX_SDK_IMAGE:=ghcr.io/maszyna-reloaded/linux-sdk:$(shell sha256sum ci/docker/linux-sdk/Dockerfile | cut -c1-12)
LINUX_SDK_MOUNT?=$(CURDIR)
LINUX_SDK_OWN_MOUNT:=$(if $(filter $(LINUX_SDK_MOUNT)/%,$(CURDIR)/.),,-v $(CURDIR):$(CURDIR))
LINUX_SDK_PLUGIN_DIRS=$(filter-out $(LINUX_SDK_MOUNT)/% $(CURDIR)/%,$(filter /%,$(subst ;, ,$(LIBRELOADED_PLUGINS))))
LINUX_SDK_PLUGIN_MOUNTS=$(foreach dir,$(LINUX_SDK_PLUGIN_DIRS),-v $(dir):$(dir))
# The host's ccache directory, mounted, so the container's builds reuse and fill the same cache
CCACHE_DIR?=$(HOME)/.cache/ccache
LINUX_SDK_RUN=mkdir -p $(CCACHE_DIR) && $(BUILD_LOCK) docker run --rm --user $(shell id -u):$(shell id -g) \
    -v $(LINUX_SDK_MOUNT):$(LINUX_SDK_MOUNT) $(LINUX_SDK_OWN_MOUNT) $(LINUX_SDK_PLUGIN_MOUNTS) $(LINUX_SDK_ENGINE_MOUNT) -w $(CURDIR) -v $(CCACHE_DIR):/ccache -e CCACHE_DIR=/ccache \
    -e CMAKE_C_COMPILER_LAUNCHER=ccache -e CMAKE_CXX_COMPILER_LAUNCHER=ccache $(LINUX_SDK_IMAGE)
# The engines are built outside the checkout, one directory for every checkout of this machine - a
# game's vendor/libreloaded, a plugin's, this one - so each engine build is cloned and built once;
# make -s godot-bin says where its files are. Building one, a checkout takes the engines' lock
# instead of its own, and the Linux SDK mounts them (LINUX_SDK_ENGINE_MOUNT)
GODOT_BUILDS_DIR?=$(HOME)/.cache/godot-builds
GODOT_BUILD_LOCK=mkdir -p $(GODOT_BUILDS_DIR) && flock $(GODOT_BUILDS_DIR)/.build.lock
GODOT_SOURCE_DIR:=$(GODOT_BUILDS_DIR)/build-godot-$(GODOT_BUILD)
GODOT_BIN:=$(GODOT_SOURCE_DIR)/bin
GODOT_SCONS=scons precision=double production=yes -j$(CMAKE_BUILD_JOBS)
LINUX_TEMPLATE:=$(GODOT_BIN)/godot.linuxbsd.template_release.double.x86_64
ANDROID_TEMPLATES:=$(GODOT_BIN)/android_release.apk $(GODOT_BIN)/android_debug.apk

#Helper for CLion so it would see generated bindings
generate-bindings: $(CMAKE_GODOTCPP_API_FILE)
	$(BUILD_LOCK) cmake -B cmake-build-debug -DGODOTCPP_PRECISION=$(CMAKE_GODOTCPP_PRECISION) -DGODOTCPP_CUSTOM_API_FILE=$(CMAKE_GODOTCPP_API_FILE)
	$(BUILD_LOCK) cmake --build cmake-build-debug --target generate_bindings


# Refreshes doc_classes/*.xml of libreloaded and of every plugin of LIBRELOADED_PLUGINS from the
# library built into GODOT_PROJECT_DIR - each class's XML goes to the repository declaring it
# (scripts/update-doc-classes)
docs:
	LIBRELOADED_PLUGINS="$(LIBRELOADED_PLUGINS)" scripts/update-doc-classes $(GODOT) $(GODOT_PROJECT_DIR)


cleanup:
	rm -rf bin
	rm -rf demo/bin
	rm -rf demo/addons/gut


cleanup-build-debug:
	rm -rf build-debug$(BUILD_TAG)


cleanup-build-release:
	rm -rf build-release$(BUILD_TAG)


cleanup-builds: cleanup-build-debug cleanup-build-release
	

# gnd-sfx, the sound addon the project links (CMakeLists.txt, reloaded_link_addon), is a GDExtension of
# its own: its library is built by its own Makefile, in this library's precision, beside every build
# of this one
GND_SFX_MAKE=$(BUILD_LOCK) $(MAKE) -C vendor/gnd-sfx PRECISION=double GODOT=$(GODOT)

compile-debug: $(CLANG_TIDY_COMPILE_COMMANDS_FILE) $(CLANG_TIDY_BINDINGS_FILE)
	$(BUILD_LOCK) cmake -B build-debug$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Debug -DGODOTCPP_TARGET=template_debug -DLIBRELOADED_DEBUG=$(LIBRELOADED_DEBUG) -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	$(BUILD_LOCK) cmake --build build-debug$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)
	$(GND_SFX_MAKE) compile-debug


compile-release:
	$(BUILD_LOCK) cmake -B build-release$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Release -DGODOTCPP_TARGET=template_release -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	$(BUILD_LOCK) cmake --build build-release$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)
	$(GND_SFX_MAKE) compile-release


# Optimized, but still the template_debug library the editor loads - the one to profile on.
# compile-debug builds the vendored Mover at -O0, which makes the physics several times slower
# than it is in a shipped build and sends any frame-time investigation after the wrong subsystem.
# Overwrites the same .so as compile-debug; run that to go back.
compile-profiling:
	$(BUILD_LOCK) cmake -B build-profiling$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=RelWithDebInfo -DGODOTCPP_TARGET=template_debug -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	$(BUILD_LOCK) cmake --build build-profiling$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)


# A shipped build that keeps its symbols, for turning a core dump into names. The switch that
# matters is the build type, not a flag of ours: godot-cpp links with -s unless DEBUG_SYMBOLS is
# on (its cmake/common_compiler_flags.cmake), and DEBUG_SYMBOLS is Debug or RelWithDebInfo. The
# price is -O2 instead of -O3, so this build is for diagnosing a crash and not for measuring frame
# times. It writes the same library as compile-release, so rebuild that one afterwards.
compile-release-symbols:
	$(BUILD_LOCK) cmake -B build-release-symbols$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=RelWithDebInfo -DGODOTCPP_TARGET=template_release -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	$(BUILD_LOCK) cmake --build build-release-symbols$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)


compile-all: compile-debug compile-release


cross-compile-release: compile-release compile-windows-release


cross-compile-debug: $(CLANG_TIDY_COMPILE_COMMANDS_FILE) $(CLANG_TIDY_BINDINGS_FILE)
	$(BUILD_LOCK) cmake -B build-linux64-debug$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) \
          -DCMAKE_BUILD_TYPE=Debug \
          -DGODOTCPP_TARGET="template_debug" \
          -DLIBRELOADED_DEBUG=ON \
          -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION)
	$(BUILD_LOCK) cmake --build build-linux64-debug$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)
	$(BUILD_LOCK) cmake -B build-win64-debug$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) \
          -DCMAKE_BUILD_TYPE=Debug \
          -DGODOTCPP_TARGET="template_debug" \
          -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) \
          -DGODOTCPP_PRECISION=$(CMAKE_GODOTCPP_PRECISION) -DGODOTCPP_CUSTOM_API_FILE=$(CMAKE_GODOTCPP_API_FILE) \
          -DGODOTCPP_PLATFORM=windows \
          -DCMAKE_SYSTEM_NAME=Windows \
          -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
          -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
          -DCMAKE_SIZEOF_VOID_P=8
	$(BUILD_LOCK) cmake --build build-win64-debug$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)


compile-windows-debug:
	$(BUILD_LOCK) cmake -B build-win64-debug$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) \
          -DCMAKE_BUILD_TYPE=Debug \
          -DGODOTCPP_TARGET="template_debug" \
          -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) \
          -DGODOTCPP_PLATFORM=windows \
          -DCMAKE_SYSTEM_NAME=Windows \
          -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
          -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
          -DCMAKE_SIZEOF_VOID_P=8
	$(BUILD_LOCK) cmake --build build-win64-debug$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)


compile-windows-release:
	$(BUILD_LOCK) cmake -B build-win64-release$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) \
          -DCMAKE_BUILD_TYPE=Release \
          -DGODOTCPP_TARGET="template_release" \
          -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) \
          -DGODOTCPP_PLATFORM=windows \
          -DCMAKE_SYSTEM_NAME=Windows \
          -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
          -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
          -DCMAKE_SIZEOF_VOID_P=8
	$(BUILD_LOCK) cmake --build build-win64-release$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)
	$(GND_SFX_MAKE) compile-windows-release


# Any NDK will do for a GDExtension; GitHub's runners carry one in ANDROID_NDK_LATEST_HOME
ANDROID_NDK_ROOT?=$(ANDROID_NDK_LATEST_HOME)

compile-android-debug:
	$(BUILD_LOCK) cmake -B build-android-debug$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Debug -DGODOTCPP_TARGET=template_debug -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) -DGODOTCPP_PLATFORM=android -DANDROID_NDK_ROOT=$(ANDROID_NDK_ROOT)
	$(BUILD_LOCK) cmake --build build-android-debug$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)
	$(GND_SFX_MAKE) compile-android-debug


compile-android-release:
	$(BUILD_LOCK) cmake -B build-android-release$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Release -DGODOTCPP_TARGET=template_release -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) -DGODOTCPP_PLATFORM=android -DANDROID_NDK_ROOT=$(ANDROID_NDK_ROOT)
	$(BUILD_LOCK) cmake --build build-android-release$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)
	$(GND_SFX_MAKE) compile-android-release


linux-sdk-image:
	docker image inspect $(LINUX_SDK_IMAGE) > /dev/null 2>&1 || docker pull -q $(LINUX_SDK_IMAGE) \
	    || docker build -q -t $(LINUX_SDK_IMAGE) ci/docker/linux-sdk


linux-sdk-image-push: linux-sdk-image
	docker push $(LINUX_SDK_IMAGE)



# The SDK container has no Godot, so the API the build binds against is dumped on the host first
$(CMAKE_GODOTCPP_API_FILE):
	cd $(CURDIR) && $(GODOT) --headless --dump-extension-api


compile-release-linux: $(CMAKE_GODOTCPP_API_FILE) linux-sdk-image
	$(LINUX_SDK_RUN) sh -c 'cmake -B build-release-linux$(BUILD_TAG) $(CMAKE_PROJECT_ARGS) -DCMAKE_BUILD_TYPE=Release -DGODOTCPP_TARGET=template_release -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) && cmake --build build-release-linux$(BUILD_TAG) --parallel $(CMAKE_BUILD_JOBS)'
	$(GND_SFX_MAKE) compile-release-linux


# The runtime of the cab Python screens, built in the SDK like the release. PythonScreenServer
# looks for it in the game dir: cp -a build-python-runtime/python2.7 <game_dir>/
PYTHON_RUNTIME_BUILD_DIR:=build-python-runtime
PYTHON_RUNTIME_DIR:=$(PYTHON_RUNTIME_BUILD_DIR)/python2.7

.PHONY: python-runtime
python-runtime: $(PYTHON_RUNTIME_DIR)/lib/libpython2.7.so.1.0

$(PYTHON_RUNTIME_DIR)/lib/libpython2.7.so.1.0: scripts/build-python-runtime | linux-sdk-image
	$(LINUX_SDK_RUN) scripts/build-python-runtime $(PYTHON_RUNTIME_DIR) $(PYTHON_RUNTIME_BUILD_DIR)


# patched before it takes its name: a clone a patch failed on is not taken for the engine's source;
# a checkout that waited for the lock finds the clone another one made
$(GODOT_SOURCE_DIR):
	$(GODOT_BUILD_LOCK) sh -c 'test -d $@ || { rm -rf $@.tmp \
	    && git clone --depth 1 --branch $(GODOT_VERSION)-stable https://github.com/godotengine/godot.git $@.tmp \
	    $(if $(GODOT_PATCHES),&& git -C $@.tmp apply $(GODOT_PATCHES)) && mv $@.tmp $@; }'


# Godot publishes no double precision build, so the engine is built here, once per engine version.
# CI does it in .github/workflows/godot-engine.yml and every other run fetches the result into
# $(GODOT_BIN) with ci/fetch-godot.sh, which is what keeps these rules from firing there.
godot-version:
	@echo $(GODOT_VERSION)


# The engine's build: its version and its patches (GODOT_BUILD) - the release godot-<build>-double
godot-build:
	@echo $(GODOT_BUILD)


# Where the engine's build puts its files (GODOT_BIN) - what a game installs its templates from,
# and ci/fetch-godot.sh fetches into
godot-bin:
	@echo $(GODOT_BIN)


# The engine's files are built under the engines' lock, the Linux ones with the engines mounted into
# the SDK and through ccache (scons' compiler launchers, given the environment ccache needs - scons
# passes none on by itself), its paths taken relative to the engines' directory so that another
# engine build hits the same cache
$(GODOT_BIN)/%: BUILD_LOCK=$(GODOT_BUILD_LOCK)
$(GODOT_BIN)/%: LINUX_SDK_ENGINE_MOUNT=-v $(GODOT_BUILDS_DIR):$(GODOT_BUILDS_DIR) -e CCACHE_BASEDIR=$(GODOT_BUILDS_DIR)


# The editor too is built in the SDK - the host's own is linked against the host's glibc, and this
# one runs on the CI runner as well. The editor is linked without LTO: it is the tool the game is
# made and exported with, and LTO's link of it takes most of its build; the templates keep it
$(GODOT_BIN)/godot.linuxbsd.%.double.x86_64: | $(GODOT_SOURCE_DIR) linux-sdk-image
	$(LINUX_SDK_RUN) sh -c 'cd $(GODOT_SOURCE_DIR) && $(GODOT_SCONS) platform=linuxbsd arch=x86_64 target=$* \
	    $(if $(filter editor,$*),lto=none) \
	    c_compiler_launcher=ccache cpp_compiler_launcher=ccache import_env_vars=HOME,CCACHE_DIR,CCACHE_BASEDIR'


# Direct3D 12 is not supported by the project, so the template is built without it and needs none
# of its SDK
$(GODOT_BIN)/godot.windows.%.double.x86_64.exe: | $(GODOT_SOURCE_DIR)
	$(BUILD_LOCK) sh -c 'cd $(GODOT_SOURCE_DIR) && $(GODOT_SCONS) platform=windows arch=x86_64 target=$* d3d12=no'


# The export presets do not use a gradle build, so the export takes the ready APKs; scons puts the
# native libraries where gradle packs them from. Needs ANDROID_HOME and a JDK 17. Built without
# the Swappy frame pacing library, which scons refuses to build without otherwise.
$(ANDROID_TEMPLATES) &: | $(GODOT_SOURCE_DIR)
	$(BUILD_LOCK) sh -c 'cd $(GODOT_SOURCE_DIR) && $(GODOT_SCONS) platform=android arch=arm64 target=template_release swappy=no \
	    && $(GODOT_SCONS) platform=android arch=arm64 target=template_debug swappy=no \
	    && cd platform/android/java && ./gradlew generateGodotTemplates'


# The site of DOCS_SOURCE - libreloaded or a plugin of LIBRELOADED_PLUGINS: its docs/ and the
# class reference of libreloaded and the plugins, as the library is built with them
# (scripts/make-docs-site, make-api-docs) - assembled into DOCS_BUILD_DIR, not versioned. A plugin
# passes its own: make -C vendor/libreloaded docs-site LIBRELOADED_PLUGINS=<path>
# DOCS_SOURCE=<plugin> DOCS_BUILD_DIR=$(CURDIR)/build-docs
DOCS_BUILD_DIR?=$(CURDIR)/build-docs
DOCS_SITE_DIR=$(DOCS_BUILD_DIR)/site
DOCS_SOURCE?=libreloaded
docs-site:
	LIBRELOADED_PLUGINS="$(LIBRELOADED_PLUGINS)" scripts/make-docs-site $(DOCS_SITE_DIR) $(DOCS_SOURCE)


docs-install:
	cd docs && make install


# Assembles the site and serves it under the source's name, as GitHub Pages serves its repository;
# without Ruby on the host, Jekyll runs in Docker
docs-server: docs-site
	$(MAKE) -C docs runserver SITE=$(DOCS_SITE_DIR) BASEURL=/$(DOCS_SOURCE)


# The site as one PDF: the home page, the guides in the order of its menu and the class reference
# (scripts/make-docs-book). The diagrams are drawn by mermaid-cli and the book typeset by pandoc
# with XeLaTeX, both in Docker, as the host user so that DOCS_BUILD_DIR stays the host's
DOCS_BOOK_DIR=$(DOCS_BUILD_DIR)/book
DOCS_PDF=$(DOCS_BUILD_DIR)/reloaded-docs.pdf
DOCS_PANDOC_IMAGE:=pandoc/extra:3.7
DOCS_MERMAID_IMAGE:=minlag/mermaid-cli:11.4.2
DOCS_DOCKER_RUN=docker run --rm --user "$(shell id -u):$(shell id -g)" -v "$(CURDIR):$(CURDIR)" \
	-v "$(DOCS_BUILD_DIR):$(DOCS_BUILD_DIR)" -w $(DOCS_BUILD_DIR)
docs-pdf: docs-site
	rm -rf $(DOCS_BOOK_DIR)
	mkdir -p $(DOCS_BOOK_DIR)
	scripts/make-docs-book $(DOCS_SITE_DIR) $(DOCS_BOOK_DIR)
	for diagram in $(DOCS_BOOK_DIR)/*.mmd; do \
		$(DOCS_DOCKER_RUN) $(DOCS_MERMAID_IMAGE) -q -b white -s 2 -i "$$diagram" -o "$${diagram%.mmd}.png" || exit 1; \
	done
	$(DOCS_DOCKER_RUN) $(DOCS_PANDOC_IMAGE) $(DOCS_BOOK_DIR)/book.md --metadata-file=$(CURDIR)/scripts/docs-pdf.yaml \
		--resource-path=$(DOCS_BOOK_DIR):$(DOCS_SITE_DIR) --pdf-engine=xelatex --toc --toc-depth=2 \
		--top-level-division=chapter -o $(DOCS_PDF)
	@echo "Documentation: $(DOCS_PDF)"


watch-and-compile:
	sh scripts/autocompile.sh


$(CLANG_TIDY_COMPILE_COMMANDS_FILE): $(CMAKE_GODOTCPP_API_FILE)
	@echo "Style: configuring clang-tidy database..." && \
	$(BUILD_LOCK) cmake --log-level=ERROR -S . -B $(CLANG_TIDY_BUILD_DIR) -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DGODOTCPP_API_VERSION=$(CMAKE_GODOTCPP_API_VERSION) "-DLIBRELOADED_PLUGINS=$(LIBRELOADED_PLUGINS)" "-DGODOT_PROJECT_DIR=$(GODOT_PROJECT_DIR)"

$(CLANG_TIDY_BINDINGS_FILE): $(CLANG_TIDY_BUILD_DIR)/CMakeCache.txt
	@echo "Style: generating clang-tidy bindings..." && \
	$(BUILD_LOCK) cmake --build $(CLANG_TIDY_BUILD_DIR) --target generate_bindings

style-check: $(CLANG_TIDY_COMPILE_COMMANDS_FILE) $(CLANG_TIDY_BINDINGS_FILE)
	@LIBRELOADED_PLUGINS="$(LIBRELOADED_PLUGINS)" scripts/style-check $(STYLE_FILE)


style-fix:
	@LIBRELOADED_PLUGINS="$(LIBRELOADED_PLUGINS)" scripts/style-fix $(STYLE_FILE)

# A staged script or shader goes in with its .uid (scripts/check-staged-uids)
.PHONY: check-staged-uids install-git-hooks
check-staged-uids:
	@scripts/check-staged-uids

# The repository's hooks (scripts/git-hooks): pre-commit runs check-staged-uids
install-git-hooks:
	git config core.hooksPath scripts/git-hooks

docker-build-tests:
	docker build -t godot-tests .


docker-run-tests: docker-build-tests
	docker run --rm godot-tests


run-tests: compile-debug
	godot-double --path demo --headless -s addons/gut/gut_cmdln.gd -gdir=res://tests/ -gexit
