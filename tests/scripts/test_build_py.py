import unittest
from unittest.mock import patch, MagicMock
import sys
import os

# Add project root to sys.path
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..")))

import build


class TestBuildPy(unittest.TestCase):
    @patch("subprocess.run")
    @patch("platform.system", return_value="Linux")
    def test_enable_gpu_flag(self, mock_system, mock_run):
        # Mock sys.argv
        test_args = ["build.py", "--action", "configure", "--enable-gpu"]
        with patch.object(sys, "argv", test_args):
            build.main()

        # Check if cmake was called with -DFACEFUSION_ENABLE_GPU=ON
        found_gpu_flag = False
        for call in mock_run.call_args_list:
            args = call[0][0]
            if "-DFACEFUSION_ENABLE_GPU=ON" in args:
                found_gpu_flag = True
                break

        self.assertTrue(
            found_gpu_flag, "Should have passed -DFACEFUSION_ENABLE_GPU=ON to cmake"
        )


if __name__ == "__main__":
    unittest.main()
