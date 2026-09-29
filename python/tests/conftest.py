"""Shared fixtures.

The repository's .gitignore excludes *.pdf, and a checked-in binary fixture
would need an exception to that rule, so the tests build the PDF they need at
run time. It is a minimal but complete one-page document with a line of text
in a standard font, which is enough to exercise the full pdfalto pipeline.
"""

from __future__ import annotations

import pytest

TEXT = "Hello pdfalto"


def _stream(data: bytes) -> bytes:
    return b"<< /Length " + str(len(data)).encode() + b" >>\nstream\n" + data + b"endstream"


def _minimal_pdf(text: str = TEXT, font: bytes = b"", extra_objects=()) -> bytes:
    """A one-page PDF showing ``text`` in font F1.

    ``font`` is the font dictionary (object 4), a non-embedded Helvetica by
    default; ``extra_objects`` are numbered from 6.
    """
    content = f"BT /F1 24 Tf 72 700 Td ({text}) Tj ET\n".encode("ascii")
    objects = [
        b"<< /Type /Catalog /Pages 2 0 R >>",
        b"<< /Type /Pages /Kids [3 0 R] /Count 1 >>",
        b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] "
        b"/Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >>",
        font or b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>",
        _stream(content),
        *extra_objects,
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


#: First character code used by :func:`glyph_names_pdf`.
FIRST_CODE = ord("A")


@pytest.fixture
def glyph_names_pdf(tmp_path):
    """Build a PDF whose text is made of the given glyph names.

    The font has no ToUnicode map and its encoding names every glyph through a
    Differences array, so the glyph name is all pdfalto has to find the
    character -- the situation of the TeX and publisher symbol fonts. The font
    is not embedded: no glyph needs to exist for the name to be resolved.
    """

    def build(names):
        assert len(names) <= 26
        differences = " ".join("/" + name for name in names)
        font = (
            "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica "
            f"/Encoding << /Type /Encoding /Differences [{FIRST_CODE} {differences}] >> >>"
        )
        # one word per glyph
        text = " ".join(chr(FIRST_CODE + i) for i in range(len(names)))
        path = tmp_path / "glyph-names.pdf"
        path.write_bytes(_minimal_pdf(text, font.encode("ascii")))
        return path

    return build


@pytest.fixture
def type3_pdf(tmp_path):
    """Build a PDF showing ``text`` in a Type 3 font.

    ``names`` maps each character code used by the text to the name of its
    glyph procedure. Every glyph draws the same box: what is under test is the
    text, not the shape.
    """

    def build(text, names):
        codes = sorted(names)
        first, last = codes[0], codes[-1]
        differences = " ".join(f"{code} /{names[code]}" for code in codes)
        procs = " ".join(f"/{name} 6 0 R" for name in sorted(set(names.values())))
        widths = " ".join("500" for _ in range(first, last + 1))
        font = (
            "<< /Type /Font /Subtype /Type3 /FontBBox [0 0 500 700] "
            "/FontMatrix [0.001 0 0 0.001 0 0] "
            f"/CharProcs << {procs} >> "
            f"/Encoding << /Type /Encoding /Differences [{differences}] >> "
            f"/FirstChar {first} /LastChar {last} /Widths [{widths}] >>"
        )
        glyph = _stream(b"500 0 0 0 450 700 d1 50 0 400 700 re f\n")
        path = tmp_path / "type3.pdf"
        path.write_bytes(_minimal_pdf(text, font.encode("ascii"), [glyph]))
        return path

    return build
