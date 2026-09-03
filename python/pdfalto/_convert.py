"""The Python API around the pdfalto executable."""

from __future__ import annotations

import os
import subprocess
import tempfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Sequence, Union

from ._binary import binary_path

__all__ = [
    "PdfAltoError",
    "ConversionResult",
    "convert",
    "convert_to_string",
    "run",
]

StrPath = Union[str, "os.PathLike[str]"]

#: pdfalto wrote the ALTO file and every sidecar successfully.
EXIT_OK = 0

#: pdfalto wrote a correct file, but had to turn off page streaming partway
#: through (typically because TMPDIR was not writable), so its peak memory was
#: no longer bounded. The output is usable; only the memory guarantee was lost.
EXIT_STREAMING_DISABLED = 5

_EXIT_MESSAGES = {
    1: "the PDF could not be opened (missing, corrupt, or wrong password)",
    2: "pdfalto could not initialise the ALTO output device",
    4: "writing the ALTO file failed; the output is missing or truncated",
    98: "pdfalto ran out of memory",
    99: "pdfalto rejected its arguments",
}

# Boolean options, in the order pdfalto documents them. Keeping the mapping in
# one table rather than inline in convert() makes it obvious what the Python
# API does and does not cover; anything absent here is still reachable through
# `extra_args`.
_FLAGS = {
    "verbose": "-verbose",
    "only_graphics_coordinates": "-onlyGraphsCoord",
    "skip_graphics": "-skipGraphs",
    "vector_coordinates_only": "-vectorCoordsOnly",
    "vector_boxes": "-vectorBoxes",
    "outline": "-outline",
    "annotations": "-annotation",
    "no_line_numbers": "-noLineNumbers",
    "reading_order": "-readingOrder",
    "no_text": "-noText",
    "char_reading_order_attr": "-charReadingOrderAttr",
    "full_font_name": "-fullFontName",
    "quiet": "-q",
}

# Options taking a value.
_VALUES = {
    "first_page": "-f",
    "last_page": "-l",
    "vector_limit": "-vectorLimit",
    "files_limit": "-filesLimit",
    "namespace_uri": "-nsURI",
    "owner_password": "-opw",
    "user_password": "-upw",
}


class PdfAltoError(RuntimeError):
    """Raised when pdfalto exits with a failure code.

    Attributes:
        returncode: the process exit status.
        stdout: captured standard output, if it was captured.
        stderr: captured standard error, if it was captured.
        cmd: the full argument list that was executed.
    """

    def __init__(
        self,
        returncode: int,
        cmd: Sequence[str],
        stdout: Optional[str] = None,
        stderr: Optional[str] = None,
    ) -> None:
        self.returncode = returncode
        self.cmd = list(cmd)
        self.stdout = stdout
        self.stderr = stderr
        reason = _EXIT_MESSAGES.get(returncode, "pdfalto failed")
        message = f"pdfalto exited with status {returncode}: {reason}"
        if stderr:
            message = f"{message}\n{stderr.strip()}"
        super().__init__(message)


@dataclass
class ConversionResult:
    """What a successful :func:`convert` produced.

    The sidecar attributes are ``None`` unless the corresponding option was
    requested and pdfalto actually wrote the file.
    """

    #: The ALTO XML file.
    alto: Path
    #: ``<name>_metadata.xml``, the document metadata pdfalto always emits.
    metadata: Optional[Path] = None
    #: ``<name>_outline.xml``, written when ``outline=True``.
    outline: Optional[Path] = None
    #: ``<name>_annot.xml``, written when ``annotations=True``.
    annotations: Optional[Path] = None
    #: ``<name>.xml_data/``, holding extracted images and vector graphics.
    data_dir: Optional[Path] = None
    #: True when pdfalto exited with :data:`EXIT_STREAMING_DISABLED`: the
    #: output is correct, but peak memory was not bounded during the run.
    streaming_disabled: bool = False
    returncode: int = 0
    stdout: str = ""
    stderr: str = ""
    cmd: list = field(default_factory=list)

    def read_text(self, encoding: str = "utf-8") -> str:
        """Return the contents of the ALTO file."""
        return self.alto.read_text(encoding=encoding)


def _build_args(options: dict) -> list:
    args = []
    for name, flag in _FLAGS.items():
        if options.get(name):
            args.append(flag)
    for name, flag in _VALUES.items():
        value = options.get(name)
        if value is not None:
            args.extend([flag, str(value)])
    return args


def _default_output(pdf: Path) -> Path:
    # Mirrors pdfalto's own default: strip a .pdf/.PDF suffix, append .xml.
    if pdf.suffix in (".pdf", ".PDF"):
        return pdf.with_suffix(".xml")
    return pdf.with_name(pdf.name + ".xml")


def run(
    args: Sequence[str],
    *,
    capture_output: bool = True,
    timeout: Optional[float] = None,
    cwd: Optional[StrPath] = None,
) -> subprocess.CompletedProcess:
    """Run the pdfalto executable with ``args`` verbatim and return the result.

    The escape hatch for anything :func:`convert` does not model. No checking
    of the exit status is done here.
    """
    cmd = [str(binary_path()), *(str(a) for a in args)]
    return subprocess.run(
        cmd,
        stdout=subprocess.PIPE if capture_output else None,
        stderr=subprocess.PIPE if capture_output else None,
        text=True,
        timeout=timeout,
        cwd=os.fspath(cwd) if cwd is not None else None,
    )


def convert(
    pdf: StrPath,
    output: Optional[StrPath] = None,
    *,
    # page range
    first_page: Optional[int] = None,
    last_page: Optional[int] = None,
    # graphics
    only_graphics_coordinates: bool = False,
    skip_graphics: bool = False,
    vector_coordinates_only: bool = False,
    vector_limit: Optional[int] = None,
    vector_boxes: bool = False,
    files_limit: Optional[int] = None,
    # extra output files
    outline: bool = False,
    annotations: bool = False,
    # text
    no_text: bool = False,
    no_line_numbers: bool = False,
    reading_order: bool = False,
    char_reading_order_attr: bool = False,
    full_font_name: bool = False,
    namespace_uri: Optional[str] = None,
    # encrypted documents
    owner_password: Optional[str] = None,
    user_password: Optional[str] = None,
    # process control
    verbose: bool = False,
    quiet: bool = False,
    extra_args: Sequence[str] = (),
    timeout: Optional[float] = None,
    check: bool = True,
) -> ConversionResult:
    """Convert ``pdf`` to ALTO XML and return the paths that were written.

    Args:
        pdf: the input PDF.
        output: the ALTO file to write. Defaults to the input path with its
            ``.pdf`` suffix replaced by ``.xml``, which is what the command
            line tool does. Sidecar files are named after it.
        first_page: first page to convert (``-f``), 1-based.
        last_page: last page to convert (``-l``); 0 or ``None`` means the end
            of the document.
        only_graphics_coordinates: record image coordinates in the ALTO but do
            not write the image files (``-onlyGraphsCoord``).
        skip_graphics: skip bitmap and vector graphics entirely
            (``-skipGraphs``).
        vector_coordinates_only: for vector graphics, emit each path's bounding
            box instead of its full geometry (``-vectorCoordsOnly``).
        vector_limit: cap on vector paths emitted per page, 0 for no cap
            (``-vectorLimit``).
        vector_boxes: emit one bounding box per vector group into the ALTO, so
            the coordinates can be read without the ``.svg`` sidecars
            (``-vectorBoxes``).
        files_limit: cap on the number of asset files extracted
            (``-filesLimit``).
        outline: also write ``<name>_outline.xml`` (``-outline``).
        annotations: also write ``<name>_annot.xml`` (``-annotation``).
        no_text: do not extract text; the result is not valid ALTO
            (``-noText``).
        no_line_numbers: drop line numbers added in manuscript-style documents
            (``-noLineNumbers``).
        reading_order: order blocks by reading order (``-readingOrder``).
        char_reading_order_attr: add a TYPE attribute marking right-to-left
            reading order; the result is not valid ALTO
            (``-charReadingOrderAttr``).
        full_font_name: do not normalise font names (``-fullFontName``).
        namespace_uri: namespace URI to declare on the ALTO root (``-nsURI``).
        owner_password: owner password for encrypted files (``-opw``).
        user_password: user password for encrypted files (``-upw``).
        verbose: print PDF attributes to stderr (``-verbose``).
        quiet: suppress messages and errors (``-q``).
        extra_args: additional command line arguments, passed through
            untouched and placed before the file names.
        timeout: seconds to wait before killing pdfalto and raising
            :class:`subprocess.TimeoutExpired`.
        check: raise :class:`PdfAltoError` on a failing exit status. Set to
            False to inspect ``returncode`` yourself.

    Returns:
        A :class:`ConversionResult` naming the files that exist on disk.

    Raises:
        FileNotFoundError: if ``pdf`` does not exist.
        PdfAltoError: if pdfalto failed and ``check`` is True.
        subprocess.TimeoutExpired: if ``timeout`` elapsed.
    """
    pdf_path = Path(pdf)
    if not pdf_path.is_file():
        raise FileNotFoundError(f"no such PDF: {pdf_path}")

    out_path = Path(output) if output is not None else _default_output(pdf_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    # _build_args reads the keyword arguments above by name, through the
    # _FLAGS and _VALUES tables.
    args = _build_args(locals())
    args.extend(str(a) for a in extra_args)
    args.extend([os.fspath(pdf_path), os.fspath(out_path)])

    cmd = [str(binary_path()), *args]
    proc = run(args, timeout=timeout)

    if check and proc.returncode not in (EXIT_OK, EXIT_STREAMING_DISABLED):
        raise PdfAltoError(proc.returncode, cmd, proc.stdout, proc.stderr)

    # pdfalto derives the sidecar names from the output path with any .xml
    # suffix stripped, and the asset directory from the output path as given.
    stem = out_path
    if out_path.suffix in (".xml", ".XML"):
        stem = out_path.with_suffix("")

    def existing(path: Path):
        return path if path.exists() else None

    return ConversionResult(
        alto=out_path,
        metadata=existing(stem.with_name(stem.name + "_metadata.xml")),
        outline=existing(stem.with_name(stem.name + "_outline.xml")),
        annotations=existing(stem.with_name(stem.name + "_annot.xml")),
        data_dir=existing(out_path.with_name(out_path.name + "_data")),
        streaming_disabled=proc.returncode == EXIT_STREAMING_DISABLED,
        returncode=proc.returncode,
        stdout=proc.stdout or "",
        stderr=proc.stderr or "",
        cmd=cmd,
    )


def convert_to_string(pdf: StrPath, *, encoding: str = "utf-8", **kwargs) -> str:
    """Convert ``pdf`` and return the ALTO XML as a string.

    Accepts every keyword argument of :func:`convert` except ``output``. The
    conversion still runs through a file: pdfalto has no in-memory output mode,
    so this writes to a temporary directory that is removed on return, taking
    any extracted images and sidecar files with it. Use :func:`convert` when
    you need those.
    """
    if "output" in kwargs:
        raise TypeError("convert_to_string() does not accept 'output'")
    with tempfile.TemporaryDirectory(prefix="pdfalto-") as tmp:
        result = convert(pdf, Path(tmp) / "out.xml", **kwargs)
        return result.alto.read_text(encoding=encoding)
