# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [0.5] -TBD 


## [0.4]

New in version 0.4 (apart various bug fixes):

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