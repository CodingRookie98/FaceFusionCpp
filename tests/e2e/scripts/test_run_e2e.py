import unittest
import os
import tempfile
import shutil
from pathlib import Path

# Placeholder for the module we want to test
# We will import run_e2e dynamically or expect it to be in the python path

class TestRunE2E(unittest.TestCase):
    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        self.config_dir = Path(self.test_dir) / "configs"
        self.config_dir.mkdir()
        
        # Create dummy configs
        (self.config_dir / "test1.yaml").touch()
        (self.config_dir / "test2.yaml").touch()
        (self.config_dir / "ignore.txt").touch()

    def tearDown(self):
        shutil.rmtree(self.test_dir)

    def test_find_configs(self):
        # This test expects run_e2e module to exist and have find_configs
        try:
            import run_e2e
        except ImportError:
            self.fail("Could not import run_e2e")

        configs = run_e2e.find_configs(self.config_dir)
        self.assertEqual(len(configs), 2)
        self.assertIn(self.config_dir / "test1.yaml", configs)
        self.assertIn(self.config_dir / "test2.yaml", configs)

if __name__ == '__main__':
    unittest.main()
