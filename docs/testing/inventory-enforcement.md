# Inventory validation increment (AGE-217)

`scripts/ci/gtest_inventory.py` executes an upstream GoogleTest binary, requires
exact named discovery and XML case sets, and rejects process/report failures,
disabled/skipped/zero/duplicate cases and reused output directories. It strips
GoogleTest environment overrides and records command exits, logs and digests.
The existing required Ubuntu/macOS jobs execute its synthetic boundary tests.

The native inventory is a **draft requiring independent coverage approval**,
derived from PR15 `300f44e0e4f7a8264950818bb449ee8df387d9d2` (41 cases).
Example: `python3 scripts/ci/gtest_inventory.py --binary <calculator_tests>
--inventory docs/testing/test-inventory.json --target calculator_tests
--output <fresh-directory>`. No expectations are regenerated during CI.

This increment does not establish compiled source closure, framework linkage or
candidate Qt/UI execution. PR15 is still unmerged; current candidate client/widget
units use custom assertion runners. Lead must route their original author's
migration and reviewed target/helper inventory before full enforcement wiring.
Historical producer/UI/QA pins and strict projection policy remain unchanged.
Passing protocol tests or historical builds do not certify candidate product bytes.
Outstanding acceptance: actual-candidate compile/behavior red, stale source and
hidden helper rejection, missing target build, complete Qt/Cargo/sanitizer matrix,
source/archive/binary closure evidence and independent current-pair review.

Recovery: preserve failure artifacts; repair the responsible source/runner and
rerun the changed candidate. Never remove a required check or restore a legacy
runner to obtain green. A reviewed revert of this increment restores the prior
baseline only; it does not complete the enforcement assignment.
