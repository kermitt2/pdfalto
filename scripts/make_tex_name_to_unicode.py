#!/usr/bin/env python3
"""Generate languages/xpdf-others/tex.nameToUnicode.

The glyph names of the TeX math and symbol fonts are not in the Adobe Glyph
List, so xpdf cannot map them. lcdf-typetools maintains the list of those
names, texglyphlist.txt, which TeX Live ships (`kpsewhich texglyphlist.txt`).
This script turns it into the file format xpdf reads, one `<hex> <name>` per
line:

- names xpdf already knows, from its built-in table or from another
  nameToUnicode file, are left out, so no existing mapping changes;
- a nameToUnicode file holds one code point per name: names standing for a
  sequence are left out, and where the list gives alternatives the first usable
  one is taken, preferring the Basic Multilingual Plane;
- combining marks, spaces, private-use and control code points are not usable:
  on their own they are not the character the glyph shows.

The file format has no comment syntax, which is why this lives here.

usage: make_tex_name_to_unicode.py path/to/texglyphlist.txt > tex.nameToUnicode
"""

from __future__ import annotations

import re
import sys
import unicodedata
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
OUTPUT = REPO / "languages" / "xpdf-others" / "tex.nameToUnicode"


def known_names():
    table = next(REPO.glob("xpdf-*/xpdf/NameToUnicodeTable.h"))
    names = set(re.findall(r'\{0x[0-9a-fA-F]+,\s*"([^"]+)"\}', table.read_text()))
    for path in (REPO / "languages").rglob("*.nameToUnicode"):
        if path == OUTPUT:
            continue
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            fields = line.split()
            if len(fields) >= 2:
                names.add(fields[1])
    return names


def usable(alternative):
    alternative = alternative.strip()
    if " " in alternative:  # a sequence
        return None
    code = int(alternative, 16)
    if 0xD800 <= code <= 0xDFFF:
        return None
    category = unicodedata.category(chr(code))
    if category[0] in "MC" or category == "Zs":
        return None
    return code


def main(argv):
    if len(argv) != 2:
        sys.exit(__doc__)
    known = known_names()
    seen = set()
    for line in Path(argv[1]).read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        name, value = line.split(";")[:2]
        if name in seen or name in known:
            continue
        seen.add(name)
        codes = [code for code in map(usable, value.split(",")) if code is not None]
        if not codes:
            continue
        bmp = [code for code in codes if code <= 0xFFFF]
        print("%04x %s" % ((bmp or codes)[0], name))


if __name__ == "__main__":
    main(sys.argv)
