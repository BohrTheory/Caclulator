#!/usr/bin/env python3
"""Regression checks for review archives and false-positive ABI verification."""
import hashlib
import json
import shutil
import stat
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

from check_abi import ROOT, has_evaluate_reference, run


class CheckerRegression(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="age7-checker-regression-")
        self.addCleanup(self.directory.cleanup)
        self.tmp = Path(self.directory.name)
        self.source = self.tmp / "source"
        for name in ("tools", "contracts", "include", "examples"):
            shutil.copytree(ROOT / name, self.source / name,
                            ignore=shutil.ignore_patterns("__pycache__"))
        # Only these disposable copies may be changed by a test.
        for path in self.source.rglob("*"):
            path.chmod(path.stat().st_mode | stat.S_IWUSR)
        self.addCleanup(self.restore_permissions)

    def restore_permissions(self):
        for path in [self.source, *self.source.rglob("*")]:
            path.chmod(path.stat().st_mode | stat.S_IWUSR)

    def invoke(self, expected):
        report_path = self.tmp / "report.json"
        proc = subprocess.run([sys.executable, "tools/check_abi.py", "--report", str(report_path)],
                              cwd=self.source, text=True, capture_output=True)
        self.assertEqual(proc.returncode, expected, proc.stdout + proc.stderr)
        self.assertTrue(report_path.is_file(), proc.stderr)
        report = json.loads(report_path.read_text())
        self.assertEqual(report["passed"], expected == 0)
        self.assertEqual(report["source_identity"]["kind"], "metadata_absent")
        self.assertIsNone(report["source_identity"]["head"])
        return report

    def test_read_only_archive_preserves_bytes_and_permissions(self):
        for path in [self.source, *self.source.rglob("*")]:
            path.chmod(path.stat().st_mode & ~0o222)
        def snapshot():
            return {str(p.relative_to(self.source)): (stat.S_IMODE(p.stat().st_mode),
                    hashlib.sha256(p.read_bytes()).hexdigest() if p.is_file() else None)
                    for p in [self.source, *self.source.rglob("*")]}
        before = snapshot()
        report = self.invoke(0)
        self.assertEqual(snapshot(), before)
        self.assertTrue(all(row["passed"] for row in report["results"]))

    def test_missing_symbol_fails_despite_nm_success(self):
        (self.source / "examples/abi/header_cpp.cpp").write_text("int main() { return 0; }\n")
        self.assert_symbol_failure()

    def test_mangled_symbol_fails_despite_nm_success(self):
        (self.source / "examples/abi/header_cpp.cpp").write_text(
            "extern int calc_evaluate_v1(int);\n"
            "int main() { return calc_evaluate_v1(1); }\n")
        self.assert_symbol_failure()

    def assert_symbol_failure(self):
        report = self.invoke(1)
        row = next(r for r in report["results"] if r["command"][0] == "nm")
        self.assertEqual(row["exit_code"], 0)
        self.assertFalse(row["passed"])
        self.assertFalse(row["validation_passed"])

    def test_fixture_setup_failure_preserves_report(self):
        (self.source / "contracts/calculator-abi-v1.json").unlink()
        report = self.invoke(1)
        row = next(r for r in report["results"] if r["command"] == ["checker setup"])
        self.assertIn("FileNotFoundError", row["stderr"])
        self.assertFalse(row["passed"])

    def test_unrelated_exit_one_is_not_expected_drift(self):
        generator = self.source / "tools/generate_abi.py"
        generator.write_text("from pathlib import Path\n"
                             "if Path.cwd().name == 'drift':\n"
                             "    raise RuntimeError('unrelated fixture failure')\n" + generator.read_text())
        report = self.invoke(1)
        row = next(r for r in report["results"] if r.get("required_diagnostic", "").startswith("Stale ABI"))
        self.assertEqual(row["exit_code"], 1)
        self.assertFalse(row["diagnostic_matched"])
        self.assertFalse(row["passed"])

    def test_missing_executable_is_not_expected_rejection(self):
        rows = []
        row = run(rows, [str(self.tmp / "missing-executable")], expected=1)
        self.assertFalse(row["passed"])
        self.assertIsNone(row["exit_code"])
        self.assertEqual(row["setup_error"], "FileNotFoundError")

    def test_platform_symbol_spelling(self):
        for system, symbol in [("Darwin", "_calc_evaluate_v1"), ("Linux", "calc_evaluate_v1")]:
            for output in (symbol + "\n", "                 U " + symbol + "\n"):
                self.assertTrue(has_evaluate_reference(output, system))
            for output in ("", "U _Z16calc_evaluate_v1i\n", "U " + symbol + "_other\n",
                           "00000000 T " + symbol + "\n"):
                self.assertFalse(has_evaluate_reference(output, system))
        self.assertFalse(has_evaluate_reference("U _calc_evaluate_v1\n", "Linux"))
        self.assertFalse(has_evaluate_reference("U calc_evaluate_v1\n", "Darwin"))


if __name__ == "__main__":
    unittest.main(verbosity=2)
