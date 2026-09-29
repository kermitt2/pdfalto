"""Glyph names to Unicode, for fonts that have nothing but the glyph name.

A font with a builtin or Differences encoding and no ToUnicode map reaches
Unicode through its glyph names only. The names of the Adobe Glyph List are in
xpdf's table; these tests cover the ones that are not.
"""

from __future__ import annotations

import re

import pytest

import pdfalto


def words(pdf):
    xml = pdfalto.convert_to_string(pdf)
    return re.findall(r'<String [^>]*CONTENT="([^"]*)"', xml)


@pytest.mark.parametrize(
    "name, expected",
    [
        # languages/xpdf-others/tex.nameToUnicode
        ("bardbl", "∥"),
        ("prime", "′"),
        ("angbracketleft", "⟨"),
        ("greatermuch", "≫"),
        # Adobe Glyph List Specification: the suffix after a period is dropped
        ("one.pnum", "1"),
        ("a.sc", "a"),
        ("parenleft.s2", "("),
        # ... components are separated by underscores
        ("f_t", "ft"),
        ("T_h", "Th"),
        ("f_t.alt", "ft"),
        # ... 'uni' and 'u' names spell the code points out
        ("uni20AC", "€"),
        ("uni00410042", "AB"),
        ("u1D441", "\U0001d441"),
        ("uni2032.var", "′"),
        # TeX size variants stand for the base character
        ("parenleftbig", "("),
        ("parenrightBigg", ")"),
        ("summationdisplay", "∑"),
        ("integraltext", "∫"),
    ],
)
def test_glyph_name_is_mapped(glyph_names_pdf, name, expected):
    assert words(glyph_names_pdf([name])) == [expected]


def test_names_are_mapped_in_reading_order(glyph_names_pdf):
    pdf = glyph_names_pdf(["A", "bardbl", "one.pnum", "B"])
    assert words(pdf) == ["A", "∥", "1", "B"]


@pytest.mark.parametrize(
    "name",
    [
        "zzunknown",        # not a name anyone knows
        "uniD800",          # a surrogate is not a character
        "u110000",          # beyond the last code point
        "a_zzunknown",      # every component has to map
        "zzunknownbig",     # a size suffix on an unknown base
    ],
)
def test_unmappable_name_is_dropped(glyph_names_pdf, name):
    # The neighbours make sure the glyph is dropped alone and nothing is
    # invented in its place.
    assert words(glyph_names_pdf(["A", name, "B"])) == ["A", "B"]


def test_type3_glyph_names_are_left_alone(type3_pdf):
    # The glyph procedures of a Type 3 font are named freely. Generators
    # commonly call the glyph drawn for a plain hyphen "uni00AD" and the one
    # for a space "uni00A0": reading those names literally would turn a
    # visible hyphen into a soft one.
    pdf = type3_pdf("t-SNE", {ord(c): c for c in "tSNE"} | {ord("-"): "uni00AD"})
    assert words(pdf) == ["t-SNE"]


def test_c_and_a_number_is_a_character_code(glyph_names_pdf):
    # The symbol fonts of several publishers name their glyphs "C" + the
    # character code. It is not an index into the Macintosh glyph ordering,
    # where 176 is the OE ligature.
    assert words(glyph_names_pdf(["C176"])) == ["\u00b0"]
