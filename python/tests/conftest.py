"""Shared fixtures.

The repository's .gitignore excludes *.pdf, and a checked-in binary fixture
would need an exception to that rule, so the tests build the PDF they need at
run time. It is a minimal but complete one-page document with a line of text
in a standard font, which is enough to exercise the full pdfalto pipeline.
"""

from __future__ import annotations

import pytest

TEXT = "Hello pdfalto"


def _minimal_pdf(text: str = TEXT) -> bytes:
    content = f"BT /F1 24 Tf 72 700 Td ({text}) Tj ET\n".encode("ascii")
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>",
        b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
        b"<< /Length " + str(len(content)).encode() + b" >>\nstream\n" + content + b"endstream",
    ]

    out = bytearray(b"%PDF-1.4\n")
    offsets = []
    for number, body in enumerate(objects, start=1):
        offsets.append(len(out))
        out += b"%d 0 obj\n" % number + body + b"\nendobj\n"

    xref_offset = len(out)
    out += b"xref\n0 %d\n" % (len(objects) + 1)
    out += b"0000000000 65535 f \n"
    for offset in offsets:
        out += b"%010d 00000 n \n" % offset
    out += b"trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (
        len(objects) + 1,
        xref_offset,
    )
    return bytes(out)


@pytest.fixture
def sample_pdf(tmp_path):
    """Path to a one-page PDF containing :data:`TEXT`."""
    path = tmp_path / "sample.pdf"
    path.write_bytes(_minimal_pdf())
    return path
