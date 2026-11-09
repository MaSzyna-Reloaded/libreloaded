#!/usr/bin/env bash
# Fetch the double precision Godot built by libreloaded's .github/workflows/godot-engine.yml instead
# of building it - from libreloaded's release in any repository, a plugin's or a game's CI included
# (GODOT_RELEASE_REPOSITORY names another repository's release). The files
# land in the Makefile's $(GODOT_BIN), exactly where its engine rules would put them, so nothing is
# rebuilt; the editor is installed as godot-double and the templates where the export looks them up.
#
# Usage: ci/fetch-godot.sh [<file>...]   only these files of the release (default: all of them)

set -euo pipefail

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="$(make -s -C "$REPO_DIR" godot-version)"
# the version and the godot-patches the engine is built with; the templates are looked up by the version
BUILD="$(make -s -C "$REPO_DIR" godot-build)"
TAG="godot-$BUILD-double"
REPOSITORY="${GODOT_RELEASE_REPOSITORY:-MaSzyna-Reloaded/libreloaded}"
GODOT_BIN="$(make -s -C "$REPO_DIR" godot-bin)"
BIN_DIR="${BIN_DIR:-$HOME/.local/bin}"
TEMPLATES_DIR="$HOME/.local/share/godot/export_templates/$VERSION.stable.double"

# release file -> where Godot expects it; the editor goes into BIN_DIR
declare -A INSTALL=(
    [godot.linuxbsd.editor.double.x86_64]="$BIN_DIR/godot-double"
    [godot.linuxbsd.template_release.double.x86_64]="$TEMPLATES_DIR/linux_release.x86_64"
    [godot.linuxbsd.template_debug.double.x86_64]="$TEMPLATES_DIR/linux_debug.x86_64"
    [godot.windows.template_release.double.x86_64.exe]="$TEMPLATES_DIR/windows_release_x86_64.exe"
    [godot.windows.template_debug.double.x86_64.exe]="$TEMPLATES_DIR/windows_debug_x86_64.exe"
    [android_release.apk]="$TEMPLATES_DIR/android_release.apk"
    [android_debug.apk]="$TEMPLATES_DIR/android_debug.apk"
)

FILES=("$@")
[ "${#FILES[@]}" -eq 0 ] && FILES=("${!INSTALL[@]}")

# A release file into $GODOT_BIN, a few times over before giving up: with the job's token (GH_TOKEN)
# through the API, as a signed-in user, or without one by its public URL - GitHub times
# unauthenticated downloads out when it is under load
FETCH_ATTEMPTS=5
fetch_release_file() {
    local attempt
    for attempt in $(seq "$FETCH_ATTEMPTS"); do
        if [ -n "${GH_TOKEN:-}" ]; then
            gh release download "$TAG" --repo "$REPOSITORY" --pattern "$1" --dir "$GODOT_BIN" --clobber \
                && return 0
        else
            curl -fsSL -o "$GODOT_BIN/$1" "https://github.com/$REPOSITORY/releases/download/$TAG/$1" \
                && return 0
        fi
        echo "Fetching $1 failed (attempt $attempt of $FETCH_ATTEMPTS)" >&2
        sleep $((attempt * 10))
    done
    return 1
}

mkdir -p "$GODOT_BIN"
for file in "${FILES[@]}"; do
    target="${INSTALL[$file]:?unknown file: $file}"
    if [ ! -f "$GODOT_BIN/$file" ]; then
        fetch_release_file "$file" \
            || { echo "::error::$TAG has no $file - run the godot-engine workflow of $REPOSITORY by hand" >&2; exit 1; }
    fi
    install -D -m 755 "$GODOT_BIN/$file" "$target"
    echo "$file -> $target"
done
