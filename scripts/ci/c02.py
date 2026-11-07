"""Verify the seven accepted C02 inputs; keep controls outside the source."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys

HASHES = {
    "contracts/calculator-abi-v1.json": "ccbec1a3a05efd4aefb4cef1bcbe79756c4c2d7b3d13538076a73d5da67bd215",
    "contracts/generated/abi_generated.rs": "71a17f1ced618eef12d31b2b2339663c5402bc6dbd5ecb5e6e914019f1b5732c",
    "contracts/generated/producer_checks.rs": "81cc8b51e89328e19c30d7f4669736b2ce9bd64ddcd9ef70314c12bbaf04431d",
    "include/calculator_abi_v1.h": "4153ef424f94c5f5461e07f2d3e20048375fc75ce622687ba8b9adac86c8b8da",
    "examples/abi/header_c.c": "b87bb3848ce1c6f50f790a698a3b8ca2bbaf27ca4d6fef740c67150a0ee7a89a",
    "examples/abi/header_cpp.cpp": "3aa47e622607b381444b3a71d9fb26db3f61cefe76bcf25d1b6a328412411141",
    "tools/generate_abi.py": "ee1ca9657334a2ae1cbf27981df534d74f93ad8a0a646588a5dacccf6c9d0aba",
}
OUTPUTS = list(HASHES)[1:4]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument("--source", type=Path, required=True)
p.add_argument("--expected-sha", required=True)
p.add_argument("--output", type=Path, required=True)
a = p.parse_args()
source, output = a.source.resolve(), a.output.resolve()
if source == output or source in output.parents:
    p.error("Output must be outside the immutable source")
output.mkdir(parents=True, exist_ok=False)
(output / "logs").mkdir()
commands = []
report = dict(candidate=a.expected_sha, workflow=os.getenv("WORKFLOW_SHA"),
    run=os.getenv("GITHUB_RUN_ID"), attempt=os.getenv("GITHUB_RUN_ATTEMPT"),
    platform=platform.platform(), architecture=platform.machine(), python=sys.version,
    image=os.getenv("ImageVersion"), controls=[], status="running")


def save(name, value):
    (output / name).write_text(json.dumps(value, indent=2) + "\n")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def run(argv, cwd=source, expected=0, diagnostic=None):
    argv = list(map(str, argv))
    result = subprocess.run(argv, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log = "logs/%02d.log" % len(commands)
    (output / log).write_bytes(result.stdout)
    commands.append(dict(argv=argv, cwd=str(cwd), exit=result.returncode,
                         expectedExit=expected, diagnostic=diagnostic, log=log))
    save("commands.json", commands)
    require(result.returncode == expected, "Unexpected command exit: " + " ".join(argv))
    require(diagnostic is None or diagnostic.encode() in result.stdout, "Missing expected diagnostic")
    return result.stdout


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    require(re.fullmatch(r"[0-9a-f]{40}", a.expected_sha), "Expected a full candidate SHA")
    require(run(["git", "rev-parse", "HEAD"]).decode().strip() == a.expected_sha, "Candidate SHA mismatch")
    manifest = {}
    for name, expected in HASHES.items():
        path = source / name
        require(path.is_file() and not path.is_symlink(), "Missing/nonregular C02 input: " + name)
        require(digest(path) == expected, "Accepted C02 hash mismatch: " + name)
        blob = run(["git", "rev-parse", a.expected_sha + ":" + name]).decode().strip()
        require(run(["git", "cat-file", "blob", blob]) == path.read_bytes(), "Uncommitted C02 bytes: " + name)
        manifest[name] = dict(sha256=expected, gitBlob=blob)
    return manifest


def snapshot(root):
    return {name: digest(root / name) if (root / name).is_file() else None for name in HASHES}


def fresh(label):
    root = output / "cases" / label
    for name in HASHES:
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(source / name, path)
    return root


def generator(root, expected=0, diagnostic=None, check=True):
    run([sys.executable, "tools/generate_abi.py", *(["--check"] if check else [])],
        root, expected, diagnostic)


try:
    if os.getenv("GITHUB_ACTIONS") == "true":
        mac = platform.system() == "Darwin"
        require(platform.machine() == ("arm64" if mac else "x86_64"), "Runner architecture mismatch")
        require(platform.mac_ver()[0].split(".")[0] == "15" if mac else
                'VERSION_ID="24.04"' in Path("/etc/os-release").read_text(), "Runner OS mismatch")
    report["inputs"] = inputs()
    for argv in [["uname", "-a"], ["clang", "--version"], ["clang++", "--version"], ["rustc", "-vV"]]:
        run(argv)
    if platform.system() == "Darwin":
        run(["sw_vers"])
        run(["xcrun", "--show-sdk-version"])
    else:
        run(["cat", "/etc/os-release"])
    root = fresh("baseline")
    generator(root)
    require(snapshot(root) == HASHES, "--check rewrote baseline")
    report["controls"].append("check")
    for name in OUTPUTS:
        (root / name).unlink()
    generator(root, check=False)
    require(snapshot(root) == HASHES, "Regeneration differs from committed bytes")
    generator(root)
    report["controls"].append("regenerate")
    for index, name in enumerate(OUTPUTS):
        for mode in ["stale", "missing"]:
            case = fresh(mode + str(index))
            if mode == "stale":
                (case / name).write_bytes((case / name).read_bytes() + b"\n// stale control\n")
            else:
                (case / name).unlink()
            before = snapshot(case)
            generator(case, 1, "Stale ABI artifacts: " + name)
            require(snapshot(case) == before, "--check rewrote " + mode + " input")
            report["controls"].append(mode + ":" + name)
    cases = [("schema", "schema_version", 2, "unsupported schema_version"),
             ("version", "abi_version", 0, "abi_version differs from CALC_ABI_VERSION")]
    cases += [("u32-" + label, None, value, "invalid u32 constant: CALC_OK") for label, value in
              [("negative", -1), ("overflow", 2**32), ("bool", True), ("fraction", 1.5), ("string", "0")]]
    for label, key, value, diagnostic in cases:
        case = fresh(label)
        path = case / "contracts/calculator-abi-v1.json"
        spec = json.loads(path.read_text())
        if key:
            spec[key] = value
        else:
            spec["constants"]["CALC_OK"] = value
        path.write_text(json.dumps(spec))
        before = snapshot(case)
        generator(case, 1, diagnostic)
        require(snapshot(case) == before, "Invalid schema check modified inputs")
        report["controls"].append(label)
    for label, compiler, standard, name in [("c11", "clang", "c11", "header_c.c"),
                                            ("cpp17", "clang++", "c++17", "header_cpp.cpp")]:
        run([compiler, "-std=" + standard, "-Wall", "-Wextra", "-Werror", "-pedantic",
             "-Iinclude", "-c", "examples/abi/" + name, "-o", output / (label + ".o")], root)
        report["controls"].append(label)
    run(["rustc", "--edition=2021", "--crate-type=lib", "--emit=metadata", "-D", "warnings",
         "contracts/generated/abi_generated.rs", "-o", output / "abi.rmeta"], root)
    report["controls"].append("rust-metadata")
    require(snapshot(root) == HASHES and inputs() == report["inputs"], "Source changed during controls")
    report["status"] = "passed"
except (OSError, ValueError) as error:
    report.update(status="failed", error=str(error))
finally:
    save("result.json", report)
    save("SHA256SUMS.json", {str(f.relative_to(output)): digest(f) for f in output.rglob("*")
                            if f.is_file() and f.name != "SHA256SUMS.json"})
print(json.dumps(report, indent=2))
raise SystemExit(0 if report["status"] == "passed" else 1)
