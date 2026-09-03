"""Python bindings for pdfalto, the PDF to ALTO XML converter.

The package ships the compiled ``pdfalto`` executable and drives it as a
subprocess, so ``pip install pdfalto`` is all that is needed — there is no
system dependency to install and no build step at import time.

    >>> import pdfalto
    >>> result = pdfalto.convert("paper.pdf", "paper.xml", outline=True)
    >>> result.alto
    PosixPath('paper.xml')

Installing the package also puts the ``pdfalto`` executable itself on PATH --
the real binary, not a Python wrapper, so invoking it costs nothing extra.
"""

from __future__ import annotations

from ._binary import binary_path, binary_version
from ._convert import (
    EXIT_OK,
    EXIT_STREAMING_DISABLED,
    ConversionResult,
    PdfAltoError,
    convert,
    convert_to_string,
    run,
)

__all__ = [
    "__version__",
    "binary_path",
    "binary_version",
    "convert",
    "convert_to_string",
    "run",
    "ConversionResult",
    "PdfAltoError",
    "EXIT_OK",
    "EXIT_STREAMING_DISABLED",
]


def __getattr__(name: str) -> str:
    # Resolved on first access rather than at import: importlib.metadata pulls
    # in email and inspect, which is a third of the cost of importing this
    # package and is wasted on everyone who never reads __version__.
    if name == "__version__":
        from importlib.metadata import PackageNotFoundError
        from importlib.metadata import version as _version

        try:
            value = _version("pdfalto")
        except PackageNotFoundError:  # running from a source tree
            value = "0.0.0.dev0"
        globals()["__version__"] = value
        return value
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
