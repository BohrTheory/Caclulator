#!/usr/bin/env python3
"""Build pinned inputs; preserve every command/exit; never accept a failed check."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import shlex
import shutil
import subprocess
import tarfile

PINS = {"producer": "74f4d57957c701da2a7705c47303c2e770888758",
        "experience": "35611d018ca82f86ed9a3271128999012789b39a",
        "qa": "6fff4dc5229ea88a2f55a274ab2ab561a3760065"}
p = argparse.ArgumentParser()
for name in PINS:
    p.add_argument("--" + name, type=Path, required=name == "producer")
p.add_argument("--producer-only", action="store_true")
p.add_argument("--output", type=Path, required=True)
p.add_argument("--qt", type=Path)
p.add_argument("--native-only", action="store_true")
a = p.parse_args()
if not a.producer_only and any(getattr(a, name) is None for name in PINS):
    p.error("All source inputs are required for application tests")
out = a.output.resolve()
out.mkdir(parents=True, exist_ok=False)  # A fresh directory is a clean-build gate.
logs = out / "logs"
logs.mkdir()
records = []
env = dict(os.environ, CARGO_TARGET_DIR=str(out / "cargo"),
           XMAKE_GLOBALDIR=str(out / "xmake-global"),
           ASAN_OPTIONS="detect_leaks=0", UBSAN_OPTIONS="halt_on_error=1",
           TSAN_OPTIONS="halt_on_error=1")
mac = platform.system() == "Darwin"
if mac:
    env["MACOSX_DEPLOYMENT_TARGET"] = "14.0"


def save(name, value):
    (out / name).write_text(json.dumps(value, indent=2) + "\n")


def run(argv, cwd=None):
    argv = list(map(str, argv))
    log = logs / ("%02d.log" % len(records))
    with log.open("w") as stream:
        result = subprocess.run(argv, cwd=cwd, env=env, stdout=stream,
                                stderr=subprocess.STDOUT)
    text = log.read_text(errors="replace")
    records.append(dict(argv=argv, cwd=str(cwd or Path.cwd()),
                        exit=result.returncode, log=str(log.relative_to(out)),
                        environment={k: env[k] for k in ["CARGO_TARGET_DIR", "XMAKE_GLOBALDIR",
                         "ASAN_OPTIONS", "UBSAN_OPTIONS", "TSAN_OPTIONS", "MACOSX_DEPLOYMENT_TARGET"] if k in env}))
    save("commands.json", records)
    print("EXIT", result.returncode, shlex.join(argv), "\n", text[-2000:], flush=True)
    if result.returncode:
        raise SystemExit(result.returncode if result.returncode > 0 else 1)
    return text


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


sources = {name: getattr(a, name).resolve() for name in PINS if getattr(a, name)}
if os.getenv("GITHUB_ACTIONS") == "true":
    expected = "arm64" if mac else "x86_64"
    if platform.machine() != expected:
        raise SystemExit("Runner architecture mismatch")
    if mac and platform.mac_ver()[0].split(".")[0] != "15":
        raise SystemExit("Expected actual macOS 15 runner")
    if not mac and 'VERSION_ID="24.04"' not in Path("/etc/os-release").read_text():
        raise SystemExit("Expected actual Ubuntu 24.04 runner")
for name, source in sources.items():
    if (source / ".git").exists():
        if run(["git", "rev-parse", "HEAD"], source).strip() != PINS[name]:
            raise SystemExit("Checkout SHA mismatch: " + name)
    elif os.getenv("GITHUB_ACTIONS") == "true":
        raise SystemExit("Missing Git checkout identity: " + name)
save("identity.json", dict(pins=PINS, platform=platform.platform(),
     architecture=platform.machine(), candidate=os.getenv("CANDIDATE_SHA"),
     workflow_sha=os.getenv("WORKFLOW_SHA"), run=os.getenv("GITHUB_RUN_ID"),
     attempt=os.getenv("GITHUB_RUN_ATTEMPT"), image=os.getenv("ImageVersion"),
     instrumented="C/C++ harness only; Rust archive/std uninstrumented",
     compatibility="Deployment target 14.0 is not macOS 14 execution"))
run(["uname", "-a"])
run(["sw_vers"] if mac else ["cat", "/etc/os-release"])
for cmd in [["rustc", "-vV"], ["cargo", "--version"],
            ["clang", "--version"], ["clang++", "--version"]]:
    version = run(cmd)
    if cmd[0] == "rustc" and not version.startswith("rustc 1.89.0 "):
        raise SystemExit("Rust version mismatch")
if mac:
    run(["xcrun", "--show-sdk-version"])
producer = sources["producer"]
run(["python3", producer / "tools/generate_abi.py", "--check"])
cargo = ["cargo", "--manifest-path", producer / "core/Cargo.toml"]
# Cargo requires the subcommand before its manifest option.
run(["cargo", "test", "--locked", *cargo[1:]])
run(["cargo", "tree", "--locked", *cargo[1:]])
native = run(["cargo", "rustc", "--locked", *cargo[1:], "--release",
              "--", "--print", "native-static-libs"])
matches = re.findall(r"native-static-libs: (.+)", native)
if not matches:
    raise SystemExit("Missing producer native link evidence")
links = shlex.split(matches[-1])
if any(not flag.startswith("-l") for flag in links):
    raise SystemExit("Unexpected native link flag; needs explicit review")
archive = out / "cargo/release/libcalculator_core.a"
run(["file", archive])
symbols = run(["nm", "-gU" if mac else "-g", archive])
for symbol in ["calc_abi_version", "calc_evaluate_v1"]:
    if not re.search(r"\bT\s+_?" + symbol + r"$", symbols, re.M):
        raise SystemExit("Missing exported symbol: " + symbol)
save("producer.json", dict(source=PINS["producer"], archiveSHA256=digest(archive),
                           nativeLinks=links, rebuilt=True))
if a.producer_only:
    filename = out / ("producer-" + platform.system().lower() + "-" + platform.machine() + ".tar.gz")
    with tarfile.open(filename, "w:gz") as tar:
        tar.add(archive, arcname="libcalculator_core.a")
        tar.add(producer / "include/calculator_abi_v1.h", arcname="calculator_abi_v1.h")
    (out / "SHA256SUMS").write_text(digest(filename) + "  " + filename.name + "\n")
    save("result.json", dict(status="producer-units-passed", source=PINS["producer"], sha256=digest(filename)))
    raise SystemExit(0)
qa = sources["qa"] / "tests/integration/age29"
for name, filename in [("producer", "fixture-manifest.json"),
                       ("experience", "experience-manifest.json")]:
    manifest = json.loads((qa / filename).read_text())
    if manifest["sourceCommit"] != PINS[name]:
        raise SystemExit("Wrong source pin: " + name)
    for file, expected in manifest["sourceFiles"].items():
        if digest(sources[name] / file) != expected:
            raise SystemExit("Source hash mismatch: " + name + "/" + file)
ui = sources["experience"]
for mode, flags in [("normal", []),
                    ("asan-ubsan", ["-fsanitize=address,undefined"]),
                    ("tsan", ["-fsanitize=thread"])]:
    for name, compiler, standard, inputs in [
        ("boundary", "clang", "c11", [qa / "boundary.c"]),
        ("consumer", "clang++", "c++17", [qa / "consumer.cpp"]),
        ("controller", "clang++", "c++17", [qa / "controller.cpp",
         ui / "ui/src/adapter.cpp", ui / "ui/src/controller.cpp"])]:
        exe = out / (name + "-" + mode)
        run([compiler, "-std=" + standard, "-Wall", "-Wextra", "-Werror",
             "-g", "-O1", "-fno-omit-frame-pointer", "-I" + str(producer / "include"),
             "-I" + str(ui / "ui/src"), *inputs, archive, *links, *flags,
             *(["-mmacosx-version-min=14.0"] if mac else ["-pthread"]), "-o", exe])
        run([exe])
if a.native_only:
    raise SystemExit(0)
if not a.qt:
    raise SystemExit("--qt is required for application/widget builds")
qt = a.qt.resolve()
if "2.9.9" not in run(["xmake", "--version"]):
    raise SystemExit("xmake version mismatch")
if "QT_VERSION:6.8.3" not in run([qt / "bin/qmake", "-query"]):
    raise SystemExit("Qt version mismatch")
# Keep source snapshots immutable; xmake writes its cache into this disposable copy.
app = out / "app-source"
app.mkdir()
for folder in ["ui", "include"]:
    shutil.copytree(ui / folder, app / folder)
shutil.copy2(ui / "xmake.lua", app)
run(["xmake", "f", "-c", "-m", "release", "--app=y", "--qt_units=y",
     "--qt=" + str(qt), "--rust_archive=" + str(archive),
     "--rust_native_links=" + ",".join(flag[2:] for flag in links), "-y"], app)
for target in ["client_units", "widget_units", "calculator"]:
    run(["xmake", "build", "-v", target], app)
run(["xmake", "run", "client_units"], app)
run(["xmake", "run", "widget_units"], app)
package = out / "package"
package.mkdir()
shutil.copy2(archive, package)
shutil.copy2(producer / "include/calculator_abi_v1.h", package)
shutil.copy2(producer / "LICENSE", package)
if mac:
    bundles = list((app / "build").rglob("calculator.app"))
    if len(bundles) != 1:
        raise SystemExit("Expected exactly one built app bundle")
    bundle = package / "Calculator.app"
    shutil.copytree(bundles[0], bundle, symlinks=True)
    run([qt / "bin/macdeployqt", bundle, "-always-overwrite"])
    run(["codesign", "--force", "--deep", "--sign", "-", bundle])
    run(["codesign", "--verify", "--deep", "--strict", bundle])
else:
    binaries = [file for file in (app / "build").rglob("calculator") if file.is_file()]
    if len(binaries) != 1:
        raise SystemExit("Expected exactly one built calculator")
    shutil.copy2(binaries[0], package / "calculator")
    lib = package / "lib"
    lib.mkdir()
    plugins = package / "plugins/platforms"
    plugins.mkdir(parents=True)
    for plugin in ["libqxcb.so", "libqoffscreen.so"]:
        shutil.copy2(qt / "plugins/platforms" / plugin, plugins)
    pending = [package / "calculator", *plugins.iterdir()]
    while pending:
        output = run(["ldd", pending.pop()])
        if "not found" in output:
            raise SystemExit("Unresolved package dependency")
        for file in re.findall(r"=> (/\S+)", output):
            dep = Path(file)
            if re.match(r"lib(c|m|pthread|rt|dl|util|resolv)\.so", dep.name):
                continue  # Ubuntu 24.04 supplies the glibc runtime/loader.
            dest = lib / dep.name
            if not dest.exists():
                shutil.copy2(dep, dest)
                pending.append(dest)
    launcher = package / "Calculator"
    launcher.write_text('#!/bin/sh\nset -eu\nroot=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\n'
                        'export LD_LIBRARY_PATH="$root/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"\n'
                        'export QT_PLUGIN_PATH="$root/plugins"\nexec "$root/calculator" "$@"\n')
    launcher.chmod(0o755)
save("package-manifest.json", {str(f.relative_to(package)): digest(f)
                               for f in package.rglob("*") if f.is_file()})
filename = out / ("calculator-" + platform.system().lower() + "-" + platform.machine() + ".tar.gz")
with tarfile.open(filename, "w:gz") as tar:
    tar.add(package, arcname="Calculator")
(out / "SHA256SUMS").write_text(digest(filename) + "  " + filename.name + "\n")
save("result.json", dict(status="build-and-tests-passed", package=filename.name,
     sha256=digest(filename), pins=PINS, externalGUI="unexecuted",
     macOS14Runtime="unexecuted", ownerReleaseAcceptance="pending"))
