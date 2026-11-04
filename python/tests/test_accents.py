"""Accents drawn as separate glyphs must not split or lose the word they belong to.

Fonts without precomposed accented letters (TeX OT1, many Type 1 subsets) draw
"ü" as a dieresis glyph followed by "u", the accent centred over the letter and
raised on capitals; other producers draw the accent after the letter, back over
it. Judged on its own the accent looks like a word boundary, and "Müller" used
to come out as "M" + "üller", "François" as "Franc" + "¸ois".

The PDF is built at run time like the one in conftest, with Helvetica in
WinAnsiEncoding so that the dieresis (0xA8), cedilla (0xB8) and acute (0xB4)
glyphs are available, and every glyph positioned explicitly.
"""

from __future__ import annotations

import re

import pytest

import pdfalto
from conftest import _minimal_pdf

# Helvetica advance widths, in thousandths of the font size
WIDTHS = {
    "M": 833, "u": 556, "l": 222, "e": 556, "r": 333, "F": 611, "a": 556, "n": 556,
    "c": 500, "o": 556, "i": 222, "s": 500, "E": 667, "m": 833, "d": 556, "1": 556,
    "0": 556, "~": 584, "`": 333, "A": 667, "N": 722, "D": 722, "R": 722, "S": 667,
    " ": 278, "dieresis": 333, "cedilla": 333, "acute": 333,
}
SIZE = 10.0


def _w(glyph: str) -> float:
    return WIDTHS[glyph] * SIZE / 1000.0


def _content() -> bytes:
    ops: list[str] = []
    x = 72.0
    y = 700.0

    def draw(s: str, at: float, dy: float = 0.0) -> None:
        ops.append(f"1 0 0 1 {at:.2f} {y + dy:.2f} Tm ({s}) Tj")

    def run(s: str) -> None:
        nonlocal x
        draw(s, x)
        x += sum(_w(ch) for ch in s)

    def accent_before(code: str, name: str, base: str, raise_pt: float = 0.0) -> None:
        # TeX order: the accent first, centred over the letter, then the letter
        nonlocal x
        draw(code, x + (_w(base) - _w(name)) / 2, raise_pt)
        draw(base, x)
        x += _w(base)

    def accent_after(code: str, name: str, base: str) -> None:
        nonlocal x
        draw(base, x)
        draw(code, x + (_w(base) - _w(name)) / 2)
        x += _w(base)

    def space() -> None:
        nonlocal x
        x += _w(" ")

    run("M"); accent_before("\\250", "dieresis", "u"); run("ller"); space()      # Müller
    run("Fran"); accent_after("\\270", "cedilla", "c"); run("ois"); space()        # François
    accent_before("\\264", "acute", "E", raise_pt=2.0); run("mile"); space()       # Émile
    run("~10"); space()                                                            # standalone tilde
    run("`code`"); space()                                                         # standalone graves
    run("ANDR"); accent_before("\\264", "acute", "E", raise_pt=2.0); run("S")      # ANDRÉS
    return ("BT /F1 10 Tf\n" + "\n".join(ops) + "\nET\n").encode("latin-1")


def _accent_pdf() -> bytes:
    pdf = _minimal_pdf("x")
    # swap the content stream and give the font an encoding with the accent glyphs
    font = b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica >>"
    assert font in pdf
    pdf = pdf.replace(font, b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>")
    old_content = b"BT /F1 24 Tf 72 700 Td (x) Tj ET\n"
    assert old_content in pdf
    new_content = _content()
    pdf = pdf.replace(
        b"<< /Length " + str(len(old_content)).encode() + b" >>\nstream\n" + old_content,
        b"<< /Length " + str(len(new_content)).encode() + b" >>\nstream\n" + new_content,
    )
    return _rebuild_xref(pdf)


def _rebuild_xref(pdf: bytes) -> bytes:
    """Recompute the cross-reference table after the objects changed size."""
    head, _ = pdf.split(b"xref\n", 1)
    offsets = [m.start() for m in re.finditer(rb"\n\d+ 0 obj\n", head)]
    offsets = [o + 1 for o in offsets]
    out = bytearray(head)
    xref_offset = len(out)
    out += b"xref\n0 %d\n" % (len(offsets) + 1)
    out += b"0000000000 65535 f \n"
    for offset in offsets:
        out += b"%010d 00000 n \n" % offset
    out += b"trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%d\n%%%%EOF\n" % (len(offsets) + 1, xref_offset)
    return bytes(out)


@pytest.fixture
def accent_pdf(tmp_path):
    path = tmp_path / "accents.pdf"
    path.write_bytes(_accent_pdf())
    return path


def _strings(xml: str) -> list[tuple[str, float, float]]:
    out = []
    for m in re.finditer(r"<String ([^>]*)/>", xml):
        attrs = dict(re.findall(r'(\w+)="([^"]*)"', m.group(1)))
        out.append((attrs["CONTENT"], float(attrs["HPOS"]), float(attrs["WIDTH"])))
    return out


def test_accents_drawn_as_separate_glyphs_stay_in_their_word(accent_pdf, tmp_path):
    result = pdfalto.convert(accent_pdf, tmp_path / "out.xml")
    assert result.returncode == 0
    strings = _strings(result.read_text())
    assert [s[0] for s in strings] == ["Müller", "François", "Émile", "~10", "`code`", "ANDRÉS"]


def test_composed_word_starts_at_its_letter_not_at_the_accent(accent_pdf, tmp_path):
    result = pdfalto.convert(accent_pdf, tmp_path / "out.xml")
    strings = {s[0]: s for s in _strings(result.read_text())}
    # "Émile" starts at the E (x=143.12), not at the acute drawn 1.67pt to its right,
    # and spans the whole word
    _, hpos, width = strings["Émile"]
    assert hpos == pytest.approx(143.12, abs=0.05)
    assert width == pytest.approx(25.0, abs=0.1)
    _, hpos, width = strings["Müller"]
    assert hpos == pytest.approx(72.0, abs=0.05)
    assert width == pytest.approx(27.22, abs=0.1)
