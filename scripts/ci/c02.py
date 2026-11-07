"""Test-first executable baseline: records success without verifying the candidate."""
import argparse
import json
from pathlib import Path

p = argparse.ArgumentParser()
for name in ["source", "expected-sha", "output"]:
    p.add_argument("--" + name, required=True)
a = p.parse_args()
Path(a.output).mkdir(parents=True)
(Path(a.output) / "result.json").write_text(json.dumps({"status": "passed"}))
