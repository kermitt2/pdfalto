# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [0.6.3] - unreleased
- Update the bundled xpdf from 4.05 to 4.06 (2025-11-14): about twenty security fixes (object-reference loops in xref/object streams, CMaps, patterns and page labels; bounds checks in the DCT, JBIG2, JPX, Type 1 and PostScript-function decoders), a more robust font ascent heuristic for broken FontDescriptors, and the `LocalParams` argument now threaded through `displayPages`/`GfxState`. The submodule moves to `xpdf-4.06/`; its previous state is tagged `xpdf-4.05-pdfalto`. Extracted text, order and horizontal positions are unchanged on the regression set; a few Strings get a different VPOS/HEIGHT where xpdf now rejects a bogus Ascent
- Keep an accent drawn as a separate glyph in the word of its letter. Fonts without precomposed accented letters (TeX OT1, many Type 1 subsets) draw "ü" as the dieresis glyph then "u", the accent centred over the letter and raised on capitals; other producers draw the accent after the letter, back over it. The word-break tests judged the accent glyph on its own and split "Müller" into "M" + "üller" and "François" into "Franc" + "¸ois" (author names on 79 of 7,500 ICML, arXiv and bioRxiv papers). An accent drawn to the right of the current word is now held until the next glyph: if that glyph is the letter under it, the break is decided on the letter and the accent composed onto it; an accent drawn back over the last letter is composed, never a break. The composed word now starts at its letter rather than at the accent, and spans both glyphs. When no precomposed character exists (a hat over a Greek letter, a bar over a math symbol) the accent is kept as a character after its letter instead of being dropped. A glyph that merely follows a standalone accent-like symbol ("~", "`") with a gap is no longer glued to it. Subscripts after an accented math symbol are now separate tokens, as after any other symbol. The breve (U+02D8) was mapped to the combining inverted breve and so never composed ("ğ")
- Fix a crash (use-after-free) introduced by the held accent above: an accent held at the end of a form XObject was placed after the form had ended, through the GfxFont freed with the form's resources, and pdfalto segfaulted or wrote garbage font styles (4 of 2,000 bioRxiv papers). `TextFontInfo` now also keeps whether the font is Type 3 and its font matrix, and the word constructors read the font name, flags and matrix from it rather than from the `GfxState`, as #248 did for ascent and descent
- Add `-discardClippedText` to drop glyphs whose midpoint falls outside the current clip path. Figures embedded as PDF Form XObjects often carry the text of the page they were exported from, clipped to the figure rectangle: it never renders, but was emitted as ordinary words, duplicating whole paragraphs and headings two or three times (60% of a 2,595-article MDPI corpus was affected). Off by default, so existing output is unchanged
- Add Python bindings and PyPI packaging: `pip install pdfalto` installs wheels bundling the compiled executable, exposing `pdfalto.convert()`/`convert_to_string()` with every command line option as a keyword argument, and putting the `pdfalto` executable itself on PATH. Wheels are built for Linux x86-64/aarch64 and macOS arm64/x86-64 by the new `ci-python.yml` workflow and published on a `v*` tag
- Look for `xpdfrc` and `languages/` in `../share/pdfalto` when they are not beside the executable, and honour `PDFALTO_DATA_DIR`, so pdfalto can be installed into a `bin/` directory without dragging an 11 MB tree of encoding tables along. Installs that keep the resources beside the binary are unaffected
- Update the bundled static libraries: libxml2 2.15.2 → 2.15.4, freetype 2.14.2 → 2.15.0, libpng 1.6.56 → 1.6.59, zlib 1.3.2 → 1.4, ICU 78.3 → 79.1. The Linux aarch64 archives are now built on Ubuntu 22.04 (glibc 2.35) like the x86_64 ones, instead of 24.04, and the macOS archives target macOS 11.0, the floor the wheels claim, instead of the runner's macOS 15. ICU stays on the `Linux` configure preset on macOS: the `MacOSX` one compiles with `-flto` and produces LLVM bitcode archives that link only with a matching or newer Apple toolchain. ALTO output, metadata, annotations, outlines and extracted images are byte-identical to the previous libraries on a 305-PDF sample (Linux x86_64)
- Fix out-of-source CMake builds: `aconf.h` is generated into the xpdf subproject's binary directory, which was not on pdfalto's include path, so only an in-source `cmake ./` could compile
- Fix the macOS library architecture selection: `libs/*/mac/{64,arm64}` was chosen from `check_c_compiler_flag("-arch arm64")`, which reports whether the compiler *can* emit arm64 rather than what is being built for, so an x86_64 build on a current SDK linked the arm64 archives. It now follows `CMAKE_OSX_ARCHITECTURES`, falling back to the host architecture

## [0.6.2]
- Separate superscript citation/affiliation callouts from adjacent words #251
- Drive superscript token boundary from pdfalto's own detection, not a font-ratio heuristic #253

## [0.6.1]
- Fix nondeterministic output: `TextRawWord::spaceAfter` was never initialised and font ascent/descent were read through an already-freed `GfxFont`, so line/block grouping and `<SP>` emission depended on heap contents. The same PDF could produce different `<TextLine>`/`<TextBlock>` structure across runs (extracted text was unaffected), which showed up as a ~0.3-0.5% noise floor in downstream A/B comparisons #248
- Bound peak memory on vector-heavy PDFs: when the `.svg` is not written (`-onlyGraphsCoord`/`-noImage`) the full vector geometry is no longer built in memory, only the per-page bounding box that consumers read (fixes >6GB OOM on pathological figures; ALTO output unchanged)
- Add `-vectorCoordsOnly` to dump each vector path's bounding-box rectangle instead of full curve geometry (smaller `.svg`, same coordinates)
- Add `-vectorLimit <N>` to cap the number of vector paths emitted per page, guarding against pathological/corrupt files
- Add `-vectorBoxes` to emit one bounding box per vector group in the ALTO instead of a single per-page union box, so vector coordinates can be read without parsing the `.svg` files
- **Changes `.svg` output**: fix fill groups losing their colour. `fill()`/`eoFill()` built the style string with `GString(tmp, sizeof(tmp))`, embedding NUL bytes that truncated it to `fill: `, so colour and fill-opacity were silently dropped from every fill group ever written. They are now emitted (e.g. `fill: #000000;fill-rule: evenodd;fill-opacity: 1;`). The ALTO is unaffected, but tools diffing `.svg` files against previous output will see changes
- Guard bitmap extraction against implausible image dimensions before allocation
- Catch `GMemException` in `main()` so allocation failures exit with code 98 and an error message, as every other xpdf front-end does, instead of terminating the process with SIGABRT

## [0.6.0]
- Change release schema following the major.minor.patch convention
- update to xpdf-4.05
- add more font mappings
- full automation for multi-platform building, all platforms are now in sync #178 #177 #180 #181 #186 #188 #189 #193 #194 #195 
- support for Linux ARM 64 
- Improve -noImage parameters #213
- Fixes some potential memory Leaks #30
- Improve removal of line numbers in case of footnotes #211
- Fixed inconsistent missing of text #201 #202 #203
- **Breaking change**: deprecate support to old glibc the earliest version is now ubuntu 20 #197

## [0.5]
- skipping this version, as the 0.5 version is currently shipped in Grobid

## [0.4]

- support for xpdf language support package for language-specific fonts like Arabic, Chinese-simplified, Japanese, etc. they are pre-installed locally and portable

- refined line number detection and fixing a bug which could result in random missing numbers in the ALTO output

- update to xpdf-4.03

- fix issue with character spacing due to invalid rotation condition

- update dependencies and dependency install script

## [0.3]


- line number detection: line numbers (typically added for review in manuscripts/preprints) are specifically identified and not anymore mixed with the rest of text content, they will be grouped in a separate block or, optionally, not outputted in the ALTO file (`noLineNumbers` option)

- removal of `-blocks` option, the block information are always returned for ensuring ALTO validation (`<TextBlock>` element)

- bug fixing on reading order

- fix possible incorrect XMax and YMax values at 0 on block coordinates having only one line

## [0.2]


- support Unicode composition of characters

- generalize reading order to all blocks (it was limited to the blocks of the first page)

- detect subscript/superscript text font style attribute

- use SVG as a format for vectorial images

- propagate unsolved character Unicode value (free Unicode range for embedded fonts) as encoded special character in ALTO (so-called "placeholder" approach)

- generate metadata information in a separate XML file (as ALTO schema does not support that)

- use the latest version of xpdf, version 4.00

- add cmake

- [ALTO](https://github.com/altoxml/documentation/wiki) output is replacing custom Xerox XML format

- Note: this released version was used for Grobid release 0.5.6

## [0.1]

- encode URI (using `xmlURIEscape` from libxml2) for the @href attribute content to avoid blocking XML wellformedness issues. From our experiments, this problem happens in average for 2-3 scholar PDF out of one thousand.
- output coordinates attributes for the BLOCK elements when the `-block` option is selected,
- add a parameter `-readingOrder` which re-order the blocks following the reading order when the -block option is selected. By default in pdf2xml, the elements followed the PDF content stream (the so-called _raw order_). In xpdf, several text flow orders are available including the raw order and the reading order. Note that, with this modification and this new option, only the blocks are re-ordered.
  From our experiments, the raw order can diverge quite significantly from the order of elements according to the visual/reading layout in 2-4% of scholar PDF (e.g. title element is introduced at the end of the page element, while visually present at the top of the page), and minor changes can be present in up to 100% of PDF for some scientific publishers (e.g. headnote introduced at the end of the page content). This additional mode can be thus quite useful for information/structure extraction applications exploiting pdfalto output.

- use the latest version of xpdf, version 3.04.

<!-- markdownlint-disable-file MD024 MD033 -->
