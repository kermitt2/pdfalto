"""Console entry point: hand over to the bundled pdfalto executable.

Implemented with ``exec`` rather than a subprocess so that `pdfalto` installed
from the wheel is indistinguishable from the native binary — same pid, same
signal handling, same exit status, no second process in the tree.
"""

from __future__ import annotations

import os
import subprocess
import sys

from ._binary import binary_path


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    try:
        binary = str(binary_path())
    except FileNotFoundError as exc:
        print(f"pdfalto: {exc}", file=sys.stderr)
        return 127

    # os.execv exists on Windows but does not replace the process there:
    # the parent exits immediately and the exit status is lost.
    if sys.platform != "win32":
        try:
            os.execv(binary, [binary, *argv])
        except OSError as exc:
            print(f"pdfalto: cannot execute {binary}: {exc}", file=sys.stderr)
            return 126
    # Windows: run as a child and forward the exit status.
    return subprocess.run([binary, *argv]).returncode


if __name__ == "__main__":  # pragma: no cover
    raise SystemExit(main())
