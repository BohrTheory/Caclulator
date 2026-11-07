"""Behavioral checks for fail-closed C02 candidate validation (AGE-177)."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

HELPER = Path(__file__).with_name("c02.py").resolve()
SOURCE = "bfbe65304546ca3cdf702fbd5af85b4791ef4115"
PATHS = ["contracts/calculator-abi-v1.json", "contracts/generated/abi_generated.rs",
         "contracts/generated/producer_checks.rs", "include/calculator_abi_v1.h",
         "examples/abi/header_c.c", "examples/abi/header_cpp.cpp", "tools/generate_abi.py"]


class CandidateChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.fixture = Path(cls.temp.name) / "fixture"
        cls.fixture.mkdir()
        for name in PATHS:
            target = cls.fixture / name
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(subprocess.check_output(["git", "show", SOURCE + ":" + name]))
        subprocess.run(["git", "init", "-q", cls.fixture], check=True)
        subprocess.run(["git", "-C", cls.fixture, "add", "."], check=True)
        subprocess.run(["git", "-C", cls.fixture, "-c", "user.name=C02 Fixture",
                        "-c", "user.email=fixture@example.invalid", "-c", "commit.gpgsign=false",
                        "commit", "-qm", "Accepted seven-file fixture"], check=True)
        cls.sha = subprocess.check_output(["git", "-C", cls.fixture, "rev-parse", "HEAD"], text=True).strip()

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def invoke(self, change=None, expected=None):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            source = root / "source"
            shutil.copytree(self.fixture, source)
            if change:
                change(source)
            result = subprocess.run([sys.executable, HELPER, "--source", source,
                "--expected-sha", expected or self.sha, "--output", root / "results"],
                env=dict(os.environ, WORKFLOW_SHA="fixture-workflow"), capture_output=True, text=True)
            report = json.loads((root / "results/result.json").read_text())
            return result.returncode, report

    def test_missing_each_required_input_is_rejected(self):
        for name in PATHS:
            with self.subTest(path=name):
                code, _ = self.invoke(lambda source: (source / name).unlink())
                self.assertNotEqual(code, 0, "accepted missing " + name)

    def test_changed_input_is_rejected(self):
        code, _ = self.invoke(lambda source: (source / PATHS[3]).write_text("wrong header"))
        self.assertNotEqual(code, 0, "accepted changed candidate bytes")

    def test_wrong_candidate_sha_is_rejected(self):
        code, _ = self.invoke(expected="0" * 40)
        self.assertNotEqual(code, 0, "accepted wrong candidate SHA")

    def test_exact_candidate_executes_all_controls(self):
        code, report = self.invoke()
        self.assertEqual(code, 0, report)
        self.assertEqual(report["candidate"], self.sha)
        self.assertEqual(set(report["inputs"]), set(PATHS))
        for name in ["check", "regenerate", "schema", "version", "u32-negative",
                     "u32-overflow", "u32-bool", "u32-fraction", "u32-string",
                     "c11", "cpp17", "rust-metadata", *["stale:" + p for p in PATHS[1:4]],
                     *["missing:" + p for p in PATHS[1:4]]]:
            self.assertIn(name, report["controls"])


if __name__ == "__main__":
    unittest.main()
