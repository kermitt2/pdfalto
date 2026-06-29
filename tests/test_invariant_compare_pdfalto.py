import pytest
from pathlib import Path
from unittest.mock import patch, MagicMock

import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from scripts.compare_pdfalto import run_pdfalto, alto_to_text

INJECTION_PAYLOADS = [
    "normal.pdf; echo 'INJECTED' > /tmp/exploit.txt;",
    "normal.pdf$(whoami)",
    "normal.pdf",
]


@pytest.mark.parametrize("pdf_payload", INJECTION_PAYLOADS)
def test_run_pdfalto_argv_list_no_shell(pdf_payload, tmp_path):
    """run_pdfalto must pass an argv list with shell=False, never a shell string."""
    pdfalto_exe = tmp_path / "pdfalto"
    pdf_path = tmp_path / pdf_payload
    output_xml = tmp_path / "output.xml"

    with patch("scripts.compare_pdfalto.subprocess.run") as mock_run:
        mock_run.return_value = MagicMock(returncode=0)
        output_xml.touch()
        run_pdfalto(pdfalto_exe, pdf_path, output_xml)

        assert mock_run.called, "subprocess.run was not called"
        call_args, call_kwargs = mock_run.call_args
        cmd = call_args[0]

        assert isinstance(cmd, list), "cmd must be a list, not a shell string"
        assert call_kwargs.get("shell", False) is False, "shell must be False"
        assert str(pdf_path) in cmd, "pdf_path must appear verbatim in argv, not be interpreted"


@pytest.mark.parametrize("xml_payload", INJECTION_PAYLOADS)
def test_alto_to_text_argv_list_no_shell(xml_payload, tmp_path):
    """alto_to_text must pass an argv list with shell=False, never a shell string."""
    xml_path = tmp_path / xml_payload
    xslt_path = tmp_path / "alto2txt.xsl"
    output_txt = tmp_path / "output.txt"

    with patch("scripts.compare_pdfalto.subprocess.run") as mock_run:
        mock_run.return_value = MagicMock(returncode=0, stdout="text output")
        alto_to_text(xml_path, xslt_path, output_txt)

        assert mock_run.called, "subprocess.run was not called"
        call_args, call_kwargs = mock_run.call_args
        cmd = call_args[0]

        assert isinstance(cmd, list), "cmd must be a list, not a shell string"
        assert call_kwargs.get("shell", False) is False, "shell must be False"
        assert str(xml_path) in cmd, "xml_path must appear verbatim in argv, not be interpreted"
