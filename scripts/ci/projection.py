"""Validate a reviewed source projection without weakening source or test hashes."""
import hashlib
import json
import subprocess
from pathlib import Path

REQUIRED = {
    "producer": ["LICENSE", "core/Cargo.toml", "core/Cargo.lock",
        "core/src/lib.rs", "core/src/evaluator.rs", "core/src/ffi.rs",
        "core/src/format.rs", "core/src/operations/basic.rs", "core/src/parser.rs",
        "core/src/registry.rs", "core/src/tests.rs", "contracts/calculator-abi-v1.json",
        "contracts/generated/abi_generated.rs", "contracts/generated/producer_checks.rs",
        "include/calculator_abi_v1.h", "tools/generate_abi.py"],
    "experience": ["LICENSE", "include/calculator_abi_v1.h", "xmake.lua",
        "ui/src/adapter.cpp", "ui/src/calculator.hpp", "ui/src/controller.cpp",
        "ui/src/main.cpp", "ui/src/window.cpp", "ui/src/window.hpp",
        "ui/tests/client_units.cpp", "ui/tests/widget_units.cpp", "ui/tests/run_checks.py"],
    "qa": ["LICENSE", "tests/integration/age29/boundary.c",
        "tests/integration/age29/consumer.cpp", "tests/integration/age29/controller.cpp",
        "tests/integration/age29/fixture-manifest.json",
        "tests/integration/age29/experience-manifest.json"],
}
MANIFESTS = {"producer": "fixture-manifest.json", "experience": "experience-manifest.json"}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def payload_digest(files):
    return digest(json.dumps(files, sort_keys=True, separators=(",", ":")).encode())


def require(condition, message):
    if not condition:
        raise ValueError(message)


def verify_git_tree(root, transport, expected):
    """Bind the reviewed bytes to Git objects as well as the working files."""
    def git(*args):
        return subprocess.check_output(["git", "--no-replace-objects", "-C", str(root), *args])

    require(Path(git("rev-parse", "--show-toplevel").decode().strip()).resolve() == root.resolve(),
            "Source root must be the checkout root")
    require(git("rev-parse", "HEAD").decode().strip() == transport, "Actual Git HEAD mismatch")
    committed = {}
    for record in git("ls-tree", "-rz", "--full-tree", transport).split(b"\0"):
        if not record:
            continue
        metadata, path = record.split(b"\t", 1)
        mode, kind, oid = metadata.decode().split()
        path = path.decode()
        require(mode in {"100644", "100755"} and kind == "blob", "Non-regular committed path: " + path)
        require(path in expected, "Unreviewed committed file: " + path)
        committed[path] = digest(git("cat-file", "blob", oid))
        require(committed[path] == expected[path], "Committed source hash mismatch: " + path)
    require(set(committed) == set(expected), "Committed source file set differs from reviewed projection")
    require(not git("status", "--porcelain=v1", "--untracked-files=all"), "Source checkout is dirty")


def verify(lock_path, sources, original_pins, read_head, allow_unpublished=False, github_actions=False):
    raw = Path(lock_path).read_bytes()
    lock = json.loads(raw)
    require(lock.get("schemaVersion") == 1, "Unknown projection schema")
    require(lock.get("originalPins") == original_pins, "Original source pins changed")
    require(set(lock["inputs"]) == set(REQUIRED) == set(sources), "Input set changed")
    require(not (github_actions and allow_unpublished), "Hosted CI cannot accept unpublished inputs")
    roots = {name: Path(path) for name, path in sources.items()}
    qa = roots["qa"] / "tests/integration/age29"
    manifests = {}
    for name, filename in MANIFESTS.items():
        data = (qa / filename).read_bytes()
        require(digest(data) == lock["historicalManifestHashes"][name], "Historical manifest changed: " + name)
        manifests[name] = json.loads(data)
        require(manifests[name]["sourceCommit"] == original_pins[name], "Manifest source pin changed")
    proof = {"verificationVersion": 2, "contractSHA256": digest(raw), "inputs": {},
        "hostedEvidenceEligible": not allow_unpublished}
    for name, paths in REQUIRED.items():
        entry = lock["inputs"][name]
        require(entry["included"] == sorted(paths), "Required build/test input list changed: " + name)
        expected = {p: manifests[name]["sourceFiles"][p] for p in paths} if name in manifests else lock["qaSourceHashes"]
        require(set(expected) == set(paths), "QA source input list changed")
        if name in manifests:
            omitted = sorted(set(manifests[name]["sourceFiles"]) - set(paths))
            require(entry["omitted"] == omitted, "Historical omission ledger changed: " + name)
        else:
            require(entry["omitted"] == [], "QA projection cannot omit a required input")
        actual = {}
        root = roots[name]
        for p in paths:
            f = root / p
            require(f.is_file() and not f.is_symlink(), "Missing or linked input: " + name + "/" + p)
            actual[p] = digest(f.read_bytes())
            require(actual[p] == expected[p], "Source hash mismatch: " + name + "/" + p)
        allowed = set(paths)
        if (root / ".gitignore").exists():
            require(digest((root / ".gitignore").read_bytes()) == lock["baselineGitignoreSHA256"], "Baseline ignore file changed")
            allowed.add(".gitignore")
        for f in root.rglob("*"):
            rel = f.relative_to(root)
            if rel.parts[0] == ".git":
                continue
            require(not f.is_symlink(), "Linked path in source projection")
            if f.is_file():
                require(rel.as_posix() in allowed, "Unreviewed extra source file: " + name + "/" + rel.as_posix())
        payload = payload_digest(actual)
        require(payload == entry["payloadSHA256"], "Projection payload digest changed: " + name)
        transport = entry["transportSha"]
        if allow_unpublished:
            require(transport is None and not (root / ".git").exists(), "Unpublished check requires plain private exports")
        else:
            require(isinstance(transport, str) and len(transport) == 40 and all(c in "0123456789abcdef" for c in transport), "Reviewed transport SHA is required: " + name)
            require((root / ".git").exists(), "Hosted input must be a Git checkout")
            require(read_head(root) == transport, "Transport SHA mismatch: " + name)
            committed_expected = dict(expected)
            if ".gitignore" in allowed:
                committed_expected[".gitignore"] = lock["baselineGitignoreSHA256"]
            verify_git_tree(root, transport, committed_expected)
        proof["inputs"][name] = {"originalSourceSha": original_pins[name], "transportSha": transport,
            "payloadSHA256": payload, "verifiedFileCount": len(actual),
            "omittedManifestFileCount": len(entry["omitted"]) if name in manifests else None}
    return proof
