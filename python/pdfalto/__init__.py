"""Python bindings for pdfalto, the PDF to ALTO XML converter.

The package ships the compiled ``pdfalto`` executable and drives it as a
subprocess, so ``pip install pdfalto`` is all that is needed — there is no
system dependency to install and no build step at import time.

    >>> import pdfalto
    >>> result = pdfalto.convert("paper.pdf", "paper.xml", outline=True)
    >>> result.alto
    PosixPath('paper.xml')

The ``pdfalto`` command line tool is installed alongside the package and
behaves exactly like the upstream binary.
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

try:  # pragma: no cover - trivial
    from importlib.metadata import PackageNotFoundError, version as _version

    __version__ = _version("pdfalto")
except PackageNotFoundError:  # pragma: no cover - running from a source tree
    __version__ = "0.0.0.dev0"
