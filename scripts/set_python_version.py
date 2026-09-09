#!/usr/bin/env python3
"""Pin the Python package version in pyproject.toml.

The wheel version normally comes from `project(pdfalto VERSION ...)` in
CMakeLists.txt, which .bumpversion.toml keeps up to date. CMake only accepts
numeric components there, so a PEP 440 pre-release such as 0.6.3.dev1 cannot be
expressed that way -- and a rehearsal upload needs one, because an index refuses
a version it already holds and never lets it be reused.

This rewrites the dynamic version into a literal one. It is used only by the
manually dispatched TestPyPI rehearsal; a tag push never runs it, so a real
release always takes its version from CMakeLists.txt.

    python3 scripts/set_python_version.py 0.6.3.dev1
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

PYPROJECT = Path(__file__).resolve().parent.parent / "pyproject.toml"

# PEP 440, restricted to the forms a rehearsal actually needs.
VERSION_RE = re.compile(r"^\d+\.\d+\.\d+(?:(?:a|b|rc|\.dev|\.post)\d+)*$")


def main(argv: list[str]) -> int:
    if len(argv) != 2:
        print(f"usage: {Path(argv[0]).name} <version>", file=sys.stderr)
        return 2

    version = argv[1]
    if not VERSION_RE.match(version):
        print(
            f"error: {version!r} is not a version this script accepts "
            "(expected e.g. 0.6.3, 0.6.3.dev1, 0.6.3rc1)",
            file=sys.stderr,
        )
        return 2

    text = PYPROJECT.read_text()

    if 'dynamic = ["version"]' not in text:
        print(
            "error: pyproject.toml no longer declares a dynamic version; "
            "this script is out of date with it",
            file=sys.stderr,
        )
        return 1

    text = text.replace('dynamic = ["version"]', f'version = "{version}"', 1)

    # Drop the provider that would otherwise fight the literal version. The
    # table runs to the next top-level table header.
    text, count = re.subn(
        r"\[tool\.scikit-build\.metadata\.version\]\n(?:(?!^\[).*\n)*",
        "",
        text,
        count=1,
        flags=re.MULTILINE,
    )
    if count != 1:
        print(
            "error: could not find [tool.scikit-build.metadata.version] "
            "to remove",
            file=sys.stderr,
        )
        return 1

    PYPROJECT.write_text(text)
    print(f"pyproject.toml version pinned to {version}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
