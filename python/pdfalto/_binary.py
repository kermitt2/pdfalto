"""Locating the pdfalto executable."""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

__all__ = ["binary_path", "binary_version"]

#: Name of the executable on this platform.
_EXE = "pdfalto.exe" if sys.platform == "win32" else "pdfalto"

#: Set this to an absolute path to run a pdfalto built elsewhere (a local CMake
#: build, a distribution package) instead of the installed one.
_ENV_OVERRIDE = "PDFALTO_BINARY"


def _script_dirs():
    """Directories where this environment's installed executables live.

    The wheel installs pdfalto as a real executable in the environment's
    scripts directory rather than as a console-script wrapper, so it is on PATH
    and costs nothing to start. sysconfig knows where that directory is for the
    interpreter running us; the user scheme covers ``pip install --user``.
    """
    import sysconfig

    schemes = [sysconfig.get_default_scheme()]
    user_scheme = "nt_user" if os.name == "nt" else "posix_user"
    if user_scheme in sysconfig.get_scheme_names():
        schemes.append(user_scheme)

    seen = set()
    for scheme in schemes:
        try:
            path = sysconfig.get_path("scripts", scheme=scheme)
        except (KeyError, ValueError):  # pragma: no cover - exotic schemes
            continue
        if path and path not in seen:
            seen.add(path)
            yield Path(path)


def _source_tree_candidates():
    """Where a binary built from a checkout of this repository would be.

    Only reached when nothing is installed, which is the case for an editable
    install: scikit-build-core keeps the CMake output in its build directory
    rather than next to the Python sources. Both candidates sit beside the
    repository's own xpdfrc and languages/, which is what pdfalto needs to find
    its runtime resources.
    """
    # <repo>/python/pdfalto/_binary.py -> <repo>
    repo = Path(__file__).resolve().parents[2]
    yield repo / _EXE                                  # cmake ./ && make
    yield from sorted(repo.glob(f"build/*/{_EXE}"))    # scikit-build-core


def _candidates():
    for directory in _script_dirs():
        yield directory / _EXE
    # Covers layouts sysconfig does not describe, e.g. a relocated environment.
    from shutil import which

    found = which(_EXE)
    if found:
        yield Path(found)
    yield from _source_tree_candidates()


def _usable(path: Path) -> bool:
    return path.is_file() and os.access(path, os.X_OK)


def binary_path() -> Path:
    """Return the path of the pdfalto executable that this package will run.

    Honours the ``PDFALTO_BINARY`` environment variable, which lets you point
    the Python API at a locally built binary without reinstalling.

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

    for candidate in _candidates():
        if _usable(candidate):
            return candidate

    searched = ", ".join(str(d) for d in _script_dirs())
    raise FileNotFoundError(
        f"the pdfalto executable was not found (looked in {searched}, on PATH, "
        "and in the surrounding source tree). Install from a wheel "
        "(pip install pdfalto), build the tool in a checkout (cmake ./ && "
        f"make), or point {_ENV_OVERRIDE} at an existing pdfalto binary."
    )


def binary_version() -> str:
    """Return the version reported by the executable itself, e.g. ``0.6.2``.

    This is the version of the C++ tool actually being run, which differs from
    :data:`pdfalto.__version__` when ``PDFALTO_BINARY`` points elsewhere.
    """
    # `pdfalto -v` prints "pdfalto version X.Y.Z" on stderr and exits non-zero
    # (it shares the usage-error path), so neither the stream nor the exit code
    # can be taken at face value here.
    import re

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
