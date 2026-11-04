#!/usr/bin/env python3
"""Compile declaration examples and check drift; does not implement or link a core."""
import argparse
import datetime
import hashlib
import json
import platform
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    results = []

    def run(command, expected=0, cwd=ROOT):
        proc = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
        results.append({"command": command, "cwd": str(cwd),
                        "exit_code": proc.returncode, "expected_exit_code": expected,
                        "passed": proc.returncode == expected,
                        "stdout": proc.stdout, "stderr": proc.stderr})

    with tempfile.TemporaryDirectory(prefix="age7-abi-") as directory:
        tmp = Path(directory)
        for command in [["clang", "--version"], ["clang++", "--version"],
                        ["rustc", "--version"], ["python3", "--version"],
                        ["git", "rev-parse", "HEAD"],
                        ["git", "branch", "--show-current"],
                        ["python3", "tools/generate_abi.py", "--check"]]:
            run(command)
        for compiler, standard, source, output in [
                ("clang", "c11", "header_c.c", "c.o"),
                ("clang++", "c++17", "header_cpp.cpp", "cpp.o")]:
            run([compiler, "-std=" + standard, "-Wall", "-Wextra", "-Werror",
                 "-pedantic", "-Iinclude", "-c", "examples/abi/" + source,
                 "-o", str(tmp / output)])
        run(["rustc", "--edition=2024", "--crate-type=lib",
             "contracts/generated/abi_generated.rs", "-o", str(tmp / "types.rlib")])
        run(["nm", "-u", str(tmp / "cpp.o")])

        fixture = tmp / "drift"
        files = ["tools/generate_abi.py", "contracts/calculator-abi-v1.json",
                 "include/calculator_abi_v1.h", "contracts/generated/abi_generated.rs",
                 "contracts/generated/producer_checks.rs"]
        for name in files:
            dest = fixture / name
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(ROOT / name, dest)
        with (fixture / "include/calculator_abi_v1.h").open("a") as handle:
            handle.write("/* deliberate drift in disposable copy */\n")
        run(["python3", "tools/generate_abi.py", "--check"], expected=1, cwd=fixture)

        # Compile declaration-only producer fixtures, not a calculator/mock library.
        # The negative fixture demonstrates that the generated checks reject drift.
        module = (ROOT / "contracts/generated/abi_generated.rs").as_posix()
        checks = (ROOT / "contracts/generated/producer_checks.rs").as_posix()
        prefix = f'#![allow(dead_code)]\n#[path = "{module}"] mod abi_generated;\n'
        signatures = '''mod ffi { unsafe extern "C" {
pub fn calc_abi_version() -> u32;
pub fn calc_evaluate_v1(expression: *const u8, expression_len: usize,
    angle_mode: u32, out_value: *mut f64, display: *mut core::ffi::c_char,
    display_capacity: usize, out_required: *mut usize,
    out_error_offset: *mut usize) -> u32;
} }
'''
        for name, text, expected in [
                ("matching", signatures, 0),
                ("mismatch", signatures.replace("out_value: *mut f64", "out_value: *mut f32"), 1)]:
            path = tmp / (name + ".rs")
            path.write_text(prefix + text + f'include!("{checks}");\n')
            run(["rustc", "--edition=2024", "--crate-type=lib", str(path),
                 "-o", str(tmp / (name + ".rlib"))], expected=expected)

    inputs = sorted(ROOT.glob("contracts/**/*.json")) + sorted(ROOT.glob("contracts/**/*.rs"))
    inputs += sorted(ROOT.glob("include/*.h")) + sorted(ROOT.glob("examples/abi/*"))
    inputs += [ROOT / "tools/generate_abi.py", ROOT / "tools/check_abi.py"]
    report = {"task": "AGE-7", "recorded_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "platform": platform.platform(), "machine": platform.machine(),
              "scope": "declarations/generation only, no real producer linked or executed",
              "results": results,
              "input_sha256": {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in inputs}}
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    for result in results:
        print(("PASS" if result["passed"] else "FAIL") + " " + " ".join(result["command"]) +
              f" (exit {result['exit_code']}, expected {result['expected_exit_code']})")
    raise SystemExit(0 if all(result["passed"] for result in results) else 1)


if __name__ == "__main__":
    main()
