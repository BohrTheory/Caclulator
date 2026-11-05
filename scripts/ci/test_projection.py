"""Verifier contract tests use synthetic bytes, never claim application coverage."""
import copy
import json
import subprocess
import tempfile
import unittest
from pathlib import Path
from projection import REQUIRED, digest, payload_digest, verify


class ProjectionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.sources = {name: self.root / name for name in REQUIRED}
        self.pins = {name: str(index) * 40 for index, name in enumerate(REQUIRED, 1)}
        self.lock = {"schemaVersion": 1, "originalPins": self.pins,
            "baselineGitignoreSHA256": digest(b"baseline\n"),
            "historicalManifestHashes": {}, "inputs": {}}
        for name, paths in REQUIRED.items():
            for path in paths:
                f = self.sources[name] / path
                f.parent.mkdir(parents=True, exist_ok=True)
                f.write_bytes((name + "/" + path + "\n").encode())
        for name, filename in [("producer", "fixture-manifest.json"),
                               ("experience", "experience-manifest.json")]:
            source_files = self.hashes(name)
            source_files["docs/private-evidence.json"] = digest(b"omitted historical evidence")
            manifest = {"sourceCommit": self.pins[name], "sourceFiles": source_files}
            raw = json.dumps(manifest).encode()
            (self.sources["qa"] / "tests/integration/age29" / filename).write_bytes(raw)
            self.lock["historicalManifestHashes"][name] = digest(raw)
        for name, paths in REQUIRED.items():
            self.lock["inputs"][name] = {"included": sorted(paths), "transportSha": None,
                "omitted": [] if name == "qa" else ["docs/private-evidence.json"],
                "payloadSHA256": payload_digest(self.hashes(name))}
        self.lock["qaSourceHashes"] = self.hashes("qa")

    def hashes(self, name):
        return {p: digest((self.sources[name] / p).read_bytes()) for p in REQUIRED[name]}

    def check(self, lock=None, unpublished=True, hosted=False, head=None):
        path = self.root / "lock.json"
        path.write_text(json.dumps(self.lock if lock is None else lock))
        reader = lambda root: head if head is not None else self.git(root, "rev-parse", "HEAD").strip()
        return verify(path, self.sources, self.pins, reader, unpublished, hosted)

    def git(self, root, *args):
        return subprocess.check_output(["git", "-C", str(root), "-c", "core.hooksPath=/dev/null",
            "-c", "commit.gpgsign=false", *args], text=True)

    def commit(self, name):
        root = self.sources[name]
        self.git(root, "add", "--all")
        self.git(root, "commit", "--quiet", "-m", "Verifier fixture")
        self.lock["inputs"][name]["transportSha"] = self.git(root, "rev-parse", "HEAD").strip()

    def publish_fixtures(self):
        for name, root in self.sources.items():
            self.git(root, "init", "--quiet")
            self.git(root, "config", "user.name", "Projection test")
            self.git(root, "config", "user.email", "projection-test@example.invalid")
            (root / ".gitignore").write_text("baseline\n")
            self.commit(name)

    def test_private_proof_is_explicitly_not_hosted_evidence(self):
        proof = self.check()
        self.assertFalse(proof["hostedEvidenceEligible"])
        self.assertEqual(proof["inputs"]["qa"]["verifiedFileCount"], len(REQUIRED["qa"]))
        self.assertIsNone(proof["inputs"]["qa"]["omittedManifestFileCount"])
        self.assertEqual(proof["inputs"]["producer"]["omittedManifestFileCount"], 1)
        with self.assertRaises(ValueError):
            self.check(hosted=True)
        with self.assertRaises(ValueError):
            self.check(unpublished=False)

    def test_required_test_cannot_be_removed(self):
        self.lock["inputs"]["experience"]["included"].remove("ui/tests/client_units.cpp")
        with self.assertRaises(ValueError):
            self.check()

    def test_missing_and_modified_test_bytes_fail(self):
        f = self.sources["experience"] / "ui/tests/client_units.cpp"
        f.write_bytes(b"weakened test\n")
        with self.assertRaises(ValueError):
            self.check()
        f.unlink()
        with self.assertRaises(ValueError):
            self.check()

    def test_extra_and_linked_payload_fail(self):
        f = self.sources["experience"] / "unreviewed.txt"
        f.write_text("unexpected")
        with self.assertRaises(ValueError):
            self.check()
        f.unlink()
        f.symlink_to(self.sources["experience"] / "LICENSE")
        with self.assertRaises(ValueError):
            self.check()

    def test_historical_manifest_cannot_be_rewritten(self):
        f = self.sources["qa"] / "tests/integration/age29/experience-manifest.json"
        f.write_bytes(f.read_bytes() + b"\n")
        with self.assertRaises(ValueError):
            self.check()

    def test_omission_payload_and_original_identity_are_bound(self):
        for change in ["omitted", "payloadSHA256", "originalPins"]:
            with self.subTest(change=change):
                lock = copy.deepcopy(self.lock)
                if change == "originalPins":
                    lock[change]["producer"] = "0" * 40
                else:
                    lock["inputs"]["producer"][change] = [] if change == "omitted" else "0" * 64
                with self.assertRaises(ValueError):
                    self.check(lock)

    def test_clean_real_git_checkout_passes(self):
        self.publish_fixtures()
        proof = self.check(unpublished=False, hosted=True)
        self.assertTrue(proof["hostedEvidenceEligible"])
        for name, entry in proof["inputs"].items():
            self.assertEqual(entry["transportSha"], self.lock["inputs"][name]["transportSha"])

    def test_reviewed_transport_identity_must_match_git(self):
        self.publish_fixtures()
        with self.assertRaises(ValueError):
            self.check(unpublished=False, head="b" * 40)
        for entry in self.lock["inputs"].values():
            entry["transportSha"] = "a" * 40
        with self.assertRaisesRegex(ValueError, "Actual Git HEAD mismatch"):
            self.check(unpublished=False, head="a" * 40)

    def test_restored_working_bytes_cannot_hide_changed_committed_license(self):
        self.publish_fixtures()
        root = self.sources["producer"]
        license_file = root / "LICENSE"
        original = license_file.read_bytes()
        license_file.write_text("unreviewed committed license\n")
        self.commit("producer")
        license_file.write_bytes(original)
        self.git(root, "update-index", "--assume-unchanged", "LICENSE")
        self.assertEqual(self.git(root, "status", "--porcelain"), "")
        with self.assertRaisesRegex(ValueError, "Committed source hash mismatch: LICENSE"):
            self.check(unpublished=False, hosted=True)

    def test_deleted_working_file_cannot_hide_committed_extra(self):
        self.publish_fixtures()
        root = self.sources["producer"]
        extra = root / "unreviewed.txt"
        extra.write_text("committed extra\n")
        self.commit("producer")
        extra.unlink()
        self.git(root, "update-index", "--assume-unchanged", "unreviewed.txt")
        self.assertEqual(self.git(root, "status", "--porcelain"), "")
        with self.assertRaisesRegex(ValueError, "Unreviewed committed file: unreviewed.txt"):
            self.check(unpublished=False, hosted=True)

    def test_untracked_required_file_cannot_fill_missing_committed_input(self):
        self.publish_fixtures()
        root = self.sources["producer"]
        self.git(root, "rm", "--cached", "LICENSE")
        self.git(root, "commit", "--quiet", "-m", "Missing committed input")
        self.lock["inputs"]["producer"]["transportSha"] = self.git(root, "rev-parse", "HEAD").strip()
        with self.assertRaisesRegex(ValueError, "Committed source file set differs"):
            self.check(unpublished=False, hosted=True)

    def test_dirty_real_git_checkout_fails(self):
        self.publish_fixtures()
        root = self.sources["experience"]
        test = root / "ui/tests/client_units.cpp"
        original = test.read_bytes()
        test.write_bytes(original + b"dirty\n")
        with self.assertRaisesRegex(ValueError, "Source hash mismatch"):
            self.check(unpublished=False, hosted=True)
        self.git(root, "add", "ui/tests/client_units.cpp")
        test.write_bytes(original)
        with self.assertRaisesRegex(ValueError, "Source checkout is dirty"):
            self.check(unpublished=False, hosted=True)

    def test_committed_symlink_cannot_be_restored_as_regular_file(self):
        self.publish_fixtures()
        root = self.sources["producer"]
        license_file = root / "LICENSE"
        original = license_file.read_bytes()
        license_file.unlink()
        license_file.symlink_to("core/Cargo.toml")
        self.commit("producer")
        license_file.unlink()
        license_file.write_bytes(original)
        with self.assertRaisesRegex(ValueError, "Non-regular committed path: LICENSE"):
            self.check(unpublished=False, hosted=True)


if __name__ == "__main__":
    unittest.main()
