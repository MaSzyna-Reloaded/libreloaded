"""The sources documented together: libreloaded and the plugins built into its library.

A source is a repository with `src/` (C++), `doc_classes/` (their class reference), one addon in
`addons/<name>/` (the directory holding a plugin.cfg) and optionally `docs/` (its guides) and
`demo/project.godot` (its project, whose autoloads count). libreloaded is the first; the plugins
come from LIBRELOADED_PLUGINS, as the build takes them (;-separated, relative to this checkout or
absolute). Used by scripts/make-api-docs, make-docs-site and update-doc-classes.
"""

import os
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


@dataclass
class Source:
    root: Path
    name: str

    @property
    def src_dir(self) -> Path:
        return self.root / "src"

    @property
    def xml_dir(self) -> Path:
        return self.root / "doc_classes"

    @property
    def addon_dir(self) -> Path:
        return self.root / "addons" / self.name

    @property
    def docs_dir(self) -> Path:
        return self.root / "docs"

    @property
    def project(self) -> Path:
        return self.root / "demo" / "project.godot"


def _addon_name(root: Path) -> str:
    addons = sorted(path.parent.name for path in (root / "addons").glob("*/plugin.cfg"))
    if not len(addons) == 1:
        raise SystemExit(f"{root}: expected one addon with a plugin.cfg in addons/, found {addons}")
    return addons[0]


def sources() -> list:
    """libreloaded, then every plugin of LIBRELOADED_PLUGINS"""
    found = [Source(ROOT, "libreloaded")]
    for entry in os.environ.get("LIBRELOADED_PLUGINS", "").split(";"):
        if entry.strip():
            root = (ROOT / entry.strip()).resolve()
            found.append(Source(root, _addon_name(root)))
    return found
