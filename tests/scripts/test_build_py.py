import io
import unittest
from contextlib import redirect_stderr, redirect_stdout
from unittest.mock import patch
import sys
import os

# Add project root to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))

import build


class TestBuildPy(unittest.TestCase):
    @patch("subprocess.run")
    @patch("platform.system", return_value="Linux")
    def test_enable_gpu_flag_is_deprecated_noop(self, mock_system, mock_run):
        """GPU support is compiled unconditionally, so --enable-gpu must not reach CMake.

        The flag stays accepted for script compatibility but has to announce that it is
        a no-op (regression guard for BL-5: the switch used to be silently ignored).
        """
        test_args = ["build.py", "--action", "configure", "--enable-gpu"]
        stdout, stderr = io.StringIO(), io.StringIO()
        with patch.object(sys, "argv", test_args), redirect_stdout(
            stdout
        ), redirect_stderr(stderr):
            build.main()

        for call in mock_run.call_args_list:
            args = call[0][0]
            self.assertNotIn(
                "-DFACEFUSION_ENABLE_GPU=ON",
                args,
                "build.py must not forward FACEFUSION_ENABLE_GPU (no CMake consumer)",
            )

        combined = (stdout.getvalue() + stderr.getvalue()).lower()
        self.assertIn("--enable-gpu", combined)
        self.assertIn("no-op", combined)


if __name__ == "__main__":
    unittest.main()
