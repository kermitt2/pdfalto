import pytest
import subprocess
import tempfile
import os
import sys

# Add the scripts directory to the path to import the module
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..'))

from scripts.compare_pdfalto import compare_pdfalto


@pytest.mark.parametrize("pdf_file_payload", [
    # Exact exploit case: command injection via shell metacharacters
    "normal.pdf; echo 'INJECTED' > /tmp/exploit.txt;",
    # Boundary case: payload with subshell execution
    "normal.pdf$(whoami)",
    # Valid input (should pass)
    "normal.pdf",
])
def test_pdfalto_command_injection(pdf_file_payload):
    """Invariant: Shell commands never include unsanitized user input"""
    with tempfile.TemporaryDirectory() as tmpdir:
        # Create a dummy PDF file for testing
        pdf_path = os.path.join(tmpdir, "test.pdf")
        with open(pdf_path, "w") as f:
            f.write("%PDF dummy content")
        
        # Create output XML path
        xml_output = os.path.join(tmpdir, "output.xml")
        
        # Test the actual function with adversarial input
        try:
            # If the function uses shell=True or unsanitized concatenation,
            # this should either raise an exception or fail safely
            compare_pdfalto(pdf_path, xml_output, pdf_file_payload)
            
            # If we reach here without command injection, verify no unexpected files were created
            exploit_file = "/tmp/exploit.txt"
            if os.path.exists(exploit_file):
                os.remove(exploit_file)
                pytest.fail(f"Command injection succeeded: {exploit_file} was created")
                
        except (subprocess.CalledProcessError, FileNotFoundError, ValueError) as e:
            # Expected: function should reject or safely handle malicious input
            pass
        except Exception as e:
            # Any other exception is acceptable as long as it's not command injection
            pass
        
        # Additional safety check: ensure the temporary directory still exists
        # (command injection shouldn't delete it)
        assert os.path.exists(tmpdir), "Command injection may have deleted temp directory"