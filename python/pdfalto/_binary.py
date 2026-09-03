"""Locating the bundled pdfalto executable."""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path

__all__ = ["binary_path", "binary_version"]

#: Directory inside the installed package holding the executable, its xpdfrc
#: and the languages/ tree. pdfalto resolves both of the latter relative to
#: its own location, so they must stay together.
_BIN_DIR = Path(__file__).parent / "_bin"

#: Set this to an absolute path to run a pdfalto built elsewhere (a local
#: CMake build, a distribution package) instead of the bundled one.
_ENV_OVERRIDE = "PDFALTO_BINARY"


#: Name of the executable on this platform.
_EXE = "pdfalto.exe" if sys.platform == "win32" else "pdfalto"


def _bundled() -> Path:
    return _BIN_DIR / _EXE


def _source_tree_candidates():
    """Where a binary built from a checkout of this repository would be.

    Only consulted when the wheel's own ``_bin`` directory is absent, which is
    the case for an editable install: scikit-build-core keeps the CMake output
    in its build directory rather than next to the Python sources. Both
    candidates sit beside the repository's xpdfrc and languages/, which is what
    pdfalto needs to find its runtime resources.
    """
    # <repo>/python/pdfalto/_binary.py -> <repo>
    repo = Path(__file__).resolve().parents[2]
    yield repo / _EXE                                  # cmake ./ && make
    yield from sorted(repo.glob(f"build/*/{_EXE}"))    # scikit-build-core


def binary_path() -> Path:
    """Return the path of the pdfalto executable that this package will run.

    Honours the ``PDFALTO_BINARY`` environment variable, which lets you point
    the Python API at a locally built binary without reinstalling the wheel.

    Raises:
        FileNotFoundError: if no usable executable is found.
    """
    override = os.environ.get(_ENV_OVERRIDE)
    if override:
        path = Path(override).expanduser()
        if not path.is_file():
            raise FileNotFoundError(
                f"{_ENV_OVERRIDE} points at {path}, which is not a file"
            )
        return path

    path = _bundled()
    if not path.is_file():
        for candidate in _source_tree_candidates():
            if candidate.is_file() and os.access(candidate, os.X_OK):
                return candidate
        raise FileNotFoundError(
            f"the pdfalto executable is missing from {_BIN_DIR}, and no build "
            "was found in the surrounding source tree. Install from a wheel "
            "(pip install pdfalto), build the tool in a checkout (cmake ./ && "
            f"make), or point {_ENV_OVERRIDE} at an existing pdfalto binary."
        )
    if not os.access(path, os.X_OK):
        raise FileNotFoundError(f"{path} exists but is not executable")
    return path


def binary_version() -> str:
    """Return the version reported by the executable itself, e.g. ``0.6.2``.

    This is the version of the C++ tool actually being run, which differs from
    :data:`pdfalto.__version__` when ``PDFALTO_BINARY`` points elsewhere.
    """
    # `pdfalto -v` prints "pdfalto version X.Y.Z" on stderr and exits non-zero
    # (it shares the usage-error path), so neither the stream nor the exit code
    # can be taken at face value here.
    proc = subprocess.run(
        [str(binary_path()), "-v"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    match = re.search(r"version\s+(\S+)", proc.stdout or "")
    if not match:
        raise RuntimeError(
            f"could not parse a version out of `pdfalto -v`: {proc.stdout!r}"
        )
    return match.group(1)
