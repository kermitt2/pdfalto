# pdfalto

Python bindings for [pdfalto](https://github.com/kermitt2/pdfalto), a PDF to
[ALTO XML](https://www.loc.gov/standards/alto/) converter built on xpdf and
used as the PDF parsing front end of [GROBID](https://github.com/kermitt2/grobid).

The wheels bundle the compiled `pdfalto` executable, so there is nothing else
to install:

```console
$ pip install pdfalto
```

## Python API

```python
import pdfalto

result = pdfalto.convert("paper.pdf", "paper.xml", outline=True)

print(result.alto)      # paper.xml
print(result.metadata)  # paper_metadata.xml
print(result.outline)   # paper_outline.xml
print(result.data_dir)  # paper.xml_data/  (extracted images and vector graphics)
```

`convert()` returns a `ConversionResult` naming every file that was written.
Sidecar attributes are `None` when the corresponding file was not produced.

To skip the files entirely:

```python
alto_xml = pdfalto.convert_to_string("paper.pdf")
```

Every pdfalto option is a keyword argument, named after the flag it sets:

```python
pdfalto.convert(
    "paper.pdf", "paper.xml",
    first_page=1, last_page=10,   # -f 1 -l 10
    skip_graphics=True,           # -skipGraphs
    no_line_numbers=True,         # -noLineNumbers
    owner_password="secret",      # -opw secret
)
```

See `help(pdfalto.convert)` for the full list. Anything not modelled there can
be passed through verbatim:

```python
pdfalto.convert("paper.pdf", extra_args=["-someNewFlag"])
pdfalto.run(["-v"])   # raw call, no exit-status checking
```

### Errors

A failing conversion raises `PdfAltoError`, carrying the exit status, the
command line and the captured stderr:

```python
try:
    pdfalto.convert("broken.pdf")
except pdfalto.PdfAltoError as exc:
    print(exc.returncode, exc.stderr)
```

Pass `check=False` to get a `ConversionResult` with a non-zero `returncode`
instead. Exit status 5 is not an error: the ALTO file was written correctly,
but pdfalto had to disable page streaming partway through, so its peak memory
was no longer bounded. That case is reported as
`result.streaming_disabled is True`.

## Command line

Installing the package puts `pdfalto` on PATH, with the upstream command line
unchanged:

```console
$ pdfalto -outline paper.pdf paper.xml
```

This is the executable itself, not a Python wrapper around it, so it starts as
fast as a hand-built binary (~7 ms rather than ~100 ms) — which matters when
something invokes it once per PDF.

It is installed the way any Unix program is, into the environment's `bin/` with
its resources in `share/pdfalto/`:

```
<venv>/bin/pdfalto
<venv>/share/pdfalto/xpdfrc
<venv>/share/pdfalto/languages/
```

pdfalto finds them relative to itself. `PDFALTO_DATA_DIR` overrides the lookup
if you ever need to point it somewhere else.

## Notes on the wheels

- Wheels are `py3-none-<platform>`: the package ships an executable rather than
  an extension module, so one wheel covers every supported Python.
- Platforms: Linux x86-64 and aarch64, macOS arm64 and x86-64. On any other
  platform pip falls back to the source distribution, which runs the CMake
  build and needs a C++17 compiler and CMake ≥ 3.15.
- The bundled binaries are built **without fontconfig**, so a conversion does
  not depend on the fonts installed on the machine and gives the same result
  everywhere. The binaries published on the pdfalto GitHub releases page do use
  fontconfig for substituting non-embedded fonts. To get that behaviour:

  ```console
  $ pip install pdfalto --no-binary pdfalto -C cmake.define.NO_FONTCONFIG=OFF
  ```

- `PDFALTO_BINARY=/path/to/pdfalto` makes the Python API run a different
  executable, which is handy for testing a local CMake build. Without it the
  API runs the `pdfalto` installed in the current environment, falling back to
  a binary built in a surrounding checkout, so an editable install works too.

## License

Same as pdfalto itself; see the `LICENSE` file in the repository.
