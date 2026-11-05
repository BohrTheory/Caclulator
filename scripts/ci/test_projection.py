"""Verifier contract tests use synthetic bytes, never claim application coverage."""
import copy
import hashlib
import json
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
        return verify(path, self.sources, self.pins, lambda _: head, unpublished, hosted)

    def test_private_proof_is_explicitly_not_hosted_evidence(self):
        proof = self.check()
        self.assertFalse(proof["hostedEvidenceEligible"])
        self.assertEqual(proof["inputs"]["qa"]["verifiedFileCount"], len(REQUIRED["qa"]))
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

    def test_reviewed_transport_identity_must_match_git_reader(self):
        for name, root in self.sources.items():
            (root / ".git").write_text("synthetic test marker; no real Git invocation")
            self.lock["inputs"][name]["transportSha"] = "a" * 40
        with self.assertRaises(ValueError):
            self.check(unpublished=False, head="b" * 40)
        proof = self.check(unpublished=False, head="a" * 40)
        self.assertTrue(proof["hostedEvidenceEligible"])


if __name__ == "__main__":
    unittest.main()
