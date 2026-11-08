#!/usr/bin/env python3
"""Compile declaration examples and check drift; does not implement or link a core."""
import argparse
import datetime
import hashlib
import json
import platform
import shutil
import stat
import sys
import traceback
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def has_evaluate_reference(output, system):
    """Match nm -u tokens, never a substring of a C++ mangled symbol."""
    if system not in ("Darwin", "Linux"):
        raise ValueError("unsupported symbol platform: " + system)
    required = "_calc_evaluate_v1" if system == "Darwin" else "calc_evaluate_v1"
    # Apple nm -u emits a bare name; GNU/LLVM nm also emit `U name`.
    return any(line.split() in ([required], ["U", required])
               for line in output.splitlines())


def run(results, command, expected=0, cwd=ROOT, diagnostic=None, validator=None):
    row = {"command": command, "cwd": str(cwd), "expected_exit_code": expected}
    try:
        proc = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
        row.update(exit_code=proc.returncode, stdout=proc.stdout, stderr=proc.stderr)
        row["passed"] = proc.returncode == expected
        if diagnostic is not None:
            row["required_diagnostic"] = diagnostic
            row["diagnostic_matched"] = diagnostic in proc.stderr
            row["passed"] &= row["diagnostic_matched"]
        if validator is not None:
            row["validation_passed"] = validator(proc.stdout)
            row["passed"] &= row["validation_passed"]
    except OSError as error:
        row.update(exit_code=None, stdout="", stderr=str(error), passed=False,
                   setup_error=type(error).__name__)
    results.append(row)
    return row


def check(results, identity):
    def execute(command, **kwargs):
        return run(results, command, **kwargs)

    # Do not accidentally identify an enclosing repository for an archive.
    if (ROOT / ".git").exists():
        identity["kind"] = "git_checkout"
        for key, command in [("head", ["git", "rev-parse", "HEAD"]),
                             ("branch", ["git", "branch", "--show-current"]),
                             ("status", ["git", "status", "--porcelain=v1"])]:
            row = execute(command)
            identity[key] = row["stdout"].strip() if row["passed"] else None
    else:
        identity.update(kind="metadata_absent", head=None, branch=None,
                        reason="No .git at source root; no commit identity inferred. "
                               "Reviewer must bind input hashes to the exported commit.")

    with tempfile.TemporaryDirectory(prefix="age7-abi-") as directory:
        tmp = Path(directory)
        for command in [["clang", "--version"], ["clang++", "--version"],
                        ["rustc", "--version"], [sys.executable, "--version"],
                        [sys.executable, "tools/generate_abi.py", "--check"]]:
            execute(command)
        for compiler, standard, source, output in [
                ("clang", "c11", "header_c.c", "c.o"),
                ("clang++", "c++17", "header_cpp.cpp", "cpp.o")]:
            execute([compiler, "-std=" + standard, "-Wall", "-Wextra", "-Werror",
                     "-pedantic", "-Iinclude", "-c", "examples/abi/" + source,
                     "-o", str(tmp / output)])
        execute(["rustc", "--edition=2024", "--crate-type=lib",
                 "contracts/generated/abi_generated.rs", "-o", str(tmp / "types.rlib")])
        execute(["nm", "-u", str(tmp / "cpp.o")],
                validator=lambda output: has_evaluate_reference(output, platform.system()))

        # Exercise the same validator against actual unrelated and mangled objects.
        for name, source in [
                ("missing", "int unrelated() { return 0; }\n"),
                ("mangled", "extern int calc_evaluate_v1(int);\n"
                            "int reference() { return calc_evaluate_v1(1); }\n")]:
            path = tmp / (name + ".cpp")
            path.write_text(source)
            execute(["clang++", "-std=c++17", "-c", str(path),
                     "-o", str(tmp / (name + ".o"))])
            execute(["nm", "-u", str(tmp / (name + ".o"))],
                    validator=lambda output: not has_evaluate_reference(output, platform.system()))

        fixture = tmp / "drift"
        files = ["tools/generate_abi.py", "contracts/calculator-abi-v1.json",
                 "include/calculator_abi_v1.h", "contracts/generated/abi_generated.rs",
                 "contracts/generated/producer_checks.rs"]
        for name in files:
            dest = fixture / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, dest)
        header = fixture / "include/calculator_abi_v1.h"
        # Only the disposable fixture is writable; source permissions stay intact.
        header.chmod(header.stat().st_mode | stat.S_IWUSR)
        with header.open("a") as handle:
            handle.write("/* deliberate drift in disposable copy */\n")
        execute([sys.executable, "tools/generate_abi.py", "--check"], expected=1,
                cwd=fixture, diagnostic="Stale ABI artifacts: include/calculator_abi_v1.h")

        # Declaration-only producer fixtures; neither is a calculator library.
        module = (ROOT / "contracts/generated/abi_generated.rs").as_posix()
        checks = (ROOT / "contracts/generated/producer_checks.rs").as_posix()
        prefix = f'#![allow(dead_code)]\n#[path = "{module}"] mod abi_generated;\n'
        signatures = """mod ffi { unsafe extern "C" {
pub fn calc_abi_version() -> u32;
pub fn calc_evaluate_v1(expression: *const u8, expression_len: usize,
    angle_mode: u32, out_value: *mut f64, display: *mut core::ffi::c_char,
    display_capacity: usize, out_required: *mut usize,
    out_error_offset: *mut usize) -> u32;
} }
"""
        for name, source, expected in [
                ("matching", signatures, 0),
                ("mismatch", signatures.replace("out_value: *mut f64", "out_value: *mut f32"), 1)]:
            path = tmp / (name + ".rs")
            path.write_text(prefix + source + f'include!("{checks}");\n')
            execute(["rustc", "--edition=2024", "--crate-type=lib", str(path),
                     "-o", str(tmp / (name + ".rlib"))], expected=expected,
                    diagnostic="error[E0308]: mismatched types" if expected else None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    results, identity, hashes = [], {}, {}
    try:
        check(results, identity)
    except Exception:
        results.append({"command": ["checker setup"], "passed": False,
                        "exit_code": None, "expected_exit_code": 0,
                        "stderr": traceback.format_exc()})
    try:
        inputs = sorted(ROOT.glob("contracts/**/*.json")) + sorted(ROOT.glob("contracts/**/*.rs"))
        inputs += sorted(ROOT.glob("include/*.h")) + sorted(ROOT.glob("examples/abi/*"))
        inputs += sorted(ROOT.glob("tools/*.py"))
        for path in inputs:
            hashes[str(path.relative_to(ROOT))] = hashlib.sha256(path.read_bytes()).hexdigest()
    except Exception:
        results.append({"command": ["input hashing"], "passed": False,
                        "exit_code": None, "expected_exit_code": 0,
                        "stderr": traceback.format_exc()})
    passed = bool(results) and all(row["passed"] for row in results)
    report = {"task": "AGE-7", "recorded_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "platform": platform.platform(), "machine": platform.machine(),
              "scope": "declarations/generation and consumer reference only; no producer linked or executed",
              "source_identity": identity, "passed": passed,
              "results": results, "input_sha256": hashes}
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    for row in results:
        print(("PASS" if row["passed"] else "FAIL") + " " + " ".join(row["command"]) +
              f" (exit {row['exit_code']}, expected {row['expected_exit_code']})")
    raise SystemExit(0 if passed else 1)


if __name__ == "__main__":
    main()
