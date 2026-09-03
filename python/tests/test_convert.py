from __future__ import annotations

import subprocess
import sys

import pytest

import pdfalto
from conftest import TEXT


def test_binary_is_present_and_reports_a_version():
    assert pdfalto.binary_path().is_file()
    version = pdfalto.binary_version()
    assert version.count(".") == 2, version


def test_convert_writes_alto_containing_the_text(sample_pdf, tmp_path):
    out = tmp_path / "out.xml"
    result = pdfalto.convert(sample_pdf, out)

    assert result.returncode == 0
    assert result.alto == out
    xml = result.read_text()
    for word in TEXT.split():
        assert f'CONTENT="{word}"' in xml


def test_default_output_replaces_the_pdf_suffix(sample_pdf):
    result = pdfalto.convert(sample_pdf)
    assert result.alto == sample_pdf.with_suffix(".xml")
    assert result.alto.is_file()


def test_metadata_sidecar_is_always_reported(sample_pdf, tmp_path):
    result = pdfalto.convert(sample_pdf, tmp_path / "out.xml")
    assert result.metadata is not None
    assert result.metadata.name == "out_metadata.xml"
    assert result.outline is None
    assert result.annotations is None


def test_optional_sidecars(sample_pdf, tmp_path):
    result = pdfalto.convert(
        sample_pdf, tmp_path / "out.xml", outline=True, annotations=True
    )
    assert result.outline is not None and result.outline.name == "out_outline.xml"
    assert result.annotations is not None
    assert result.annotations.name == "out_annot.xml"
    assert result.data_dir is not None and result.data_dir.name == "out.xml_data"


def test_boolean_and_valued_options_reach_the_command_line(sample_pdf, tmp_path):
    result = pdfalto.convert(
        sample_pdf,
        tmp_path / "out.xml",
        no_line_numbers=True,
        first_page=1,
        last_page=1,
        vector_limit=10,
    )
    assert "-noLineNumbers" in result.cmd
    assert result.cmd[result.cmd.index("-f") + 1] == "1"
    assert result.cmd[result.cmd.index("-vectorLimit") + 1] == "10"


def test_unset_options_are_not_passed(sample_pdf, tmp_path):
    result = pdfalto.convert(sample_pdf, tmp_path / "out.xml")
    assert "-f" not in result.cmd
    assert "-noText" not in result.cmd


def test_extra_args_are_passed_through(sample_pdf, tmp_path):
    result = pdfalto.convert(
        sample_pdf, tmp_path / "out.xml", extra_args=["-fullFontName"]
    )
    assert "-fullFontName" in result.cmd


def test_no_text_produces_no_string_elements(sample_pdf, tmp_path):
    result = pdfalto.convert(sample_pdf, tmp_path / "out.xml", no_text=True)
    assert "CONTENT=" not in result.read_text()


def test_output_directory_is_created(sample_pdf, tmp_path):
    out = tmp_path / "nested" / "dir" / "out.xml"
    result = pdfalto.convert(sample_pdf, out)
    assert result.alto.is_file()


def test_convert_to_string(sample_pdf):
    xml = pdfalto.convert_to_string(sample_pdf)
    assert xml.lstrip().startswith("<?xml")
    assert 'CONTENT="Hello"' in xml


def test_convert_to_string_rejects_output(sample_pdf):
    with pytest.raises(TypeError):
        pdfalto.convert_to_string(sample_pdf, output="x.xml")


def test_missing_input_raises_file_not_found(tmp_path):
    with pytest.raises(FileNotFoundError):
        pdfalto.convert(tmp_path / "nope.pdf")


def test_corrupt_pdf_raises_pdfalto_error(tmp_path):
    broken = tmp_path / "broken.pdf"
    broken.write_bytes(b"not a pdf at all")
    with pytest.raises(pdfalto.PdfAltoError) as excinfo:
        pdfalto.convert(broken, tmp_path / "out.xml")
    assert excinfo.value.returncode == 1
    assert "could not be opened" in str(excinfo.value)


def test_check_false_reports_the_failure_instead_of_raising(tmp_path):
    broken = tmp_path / "broken.pdf"
    broken.write_bytes(b"not a pdf at all")
    result = pdfalto.convert(broken, tmp_path / "out.xml", check=False)
    assert result.returncode == 1


def test_run_is_a_raw_escape_hatch(sample_pdf, tmp_path):
    proc = pdfalto.run([str(sample_pdf), str(tmp_path / "out.xml")])
    assert proc.returncode == 0
    assert (tmp_path / "out.xml").is_file()


def test_binary_override(monkeypatch, tmp_path):
    monkeypatch.setenv("PDFALTO_BINARY", str(tmp_path / "does-not-exist"))
    with pytest.raises(FileNotFoundError):
        pdfalto.binary_path()


def test_console_script_matches_the_binary():
    from pdfalto.__main__ import main  # noqa: F401  (import must succeed)

    proc = subprocess.run(
        [sys.executable, "-m", "pdfalto", "-v"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    assert "pdfalto version" in proc.stdout


def test_source_tree_fallback_is_used_when_the_wheel_has_no_bin(
    monkeypatch, tmp_path
):
    from pdfalto import _binary

    built = tmp_path / "pdfalto"
    built.write_text("#!/bin/sh\n")
    built.chmod(0o755)

    monkeypatch.setattr(_binary, "_BIN_DIR", tmp_path / "absent")
    monkeypatch.setattr(_binary, "_source_tree_candidates", lambda: iter([built]))
    assert _binary.binary_path() == built


def test_missing_binary_reports_how_to_get_one(monkeypatch, tmp_path):
    from pdfalto import _binary

    monkeypatch.setattr(_binary, "_BIN_DIR", tmp_path / "absent")
    monkeypatch.setattr(_binary, "_source_tree_candidates", lambda: iter([]))
    with pytest.raises(FileNotFoundError, match="pip install pdfalto"):
        _binary.binary_path()
