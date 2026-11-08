# ADR-0002: GoogleTest contract v1 (AGE-201)

Status: proposed for independent review; enforcement implementation pending.
Owner: Technical Lead. Project: 50271359-4516-466c-8325-0d234bd5ddb0.
Parent: AGE-199. Migration author: AGE-198; do not duplicate that migration.
Contract revision: `calculator-native-tests/v1`; changes require contract-owner approval.

## Decision and dependency direction

C++ behavioral and integration assertions use GoogleTest TEST/TEST_F/TEST_P
and EXPECT/ASSERT assertions. Use upstream gtest_main, or a minimal main returning
RUN_ALL_TESTS after InitGoogleTest. No custom CHECK/main/PASS-count runner.
Cargo remains the Rust unit-test runner. Python may orchestrate builds, inventory,
policy checks and evidence; it must not implement a second assertion/test runner.
Qt/xmake remain the application build. Do not introduce CMake as a second build.

Dependency map: Cargo core -> Rust staticlib -> C ABI v1 -> C/C++ consumer;
Qt Controller/Adapter -> C ABI v1; GoogleTest suites -> these real components.
Rust has no Qt/GoogleTest dependency. C ABI signatures, ownership, statuses,
numeric tolerances, display and accepted product behavior remain unchanged.
Compile a C11 bridge separately to preserve C header/caller compatibility.
Keep Qt widget tests linked to real Qt and Rust. Native tests are not GUI E2E.

## Canonical execution/evidence interface

The existing CI entrypoint `scripts/ci/build.py` owns process exit propagation.
Migration `tests/integration/age29/run_gtest.py` is build orchestration, not a runner.
Pin GoogleTest 1.17.0 archive SHA256
`65fab701d9829d38cb77c14acdc431d2108bfdbf8979e40eb8ae567edf10b27c`.
Use fresh output directories, upstream discovery `--gtest_list_tests`, then run
unfiltered with `--gtest_output=xml:<unique-file>` in each required mode.
Discovery alone does not execute assertions. Preserve process stdout/stderr/exit;
any build/test failure, signal, missing/malformed report, zero tests, disabled or
skipped required case, or inventory/report mismatch must fail the job nonzero.
Do not turn failures into success using log matches, catch-and-continue or shell pipes.

Reviewed `test-inventory.json` v1 must enumerate each target and exact required
suite.case IDs, source-relative paths and acceptance-case mapping. Discovery and
XML executed case sets must match that inventory, not only a total count.
Parameterized names must be recorded as discovered. Changes require independent
coverage review; no automatic baseline regeneration during CI.
`test-evidence.json` v1 records candidate/workflow/base SHA, platform/architecture,
source paths+SHA256, binary+Rust archive SHA256, framework digest, mode, argv/cwd,
exit code, inventory/report/log paths+SHA256 and required/executed/skipped counts.
Use existing commands.json, identity.json and test-inputs.json where compatible;
add missing fields rather than introducing a replacement test scheduler.
Upload discovery, XML, provenance and failure logs with existing always() artifact
step. Failure before execution records the failed phase; never fabricate XML pass.

## Pin audit and compatibility migration

Audited baseline: a9603acf4ada481db5c5442340d5efb64dd9bedd;
parent's earlier baseline: c9a59564a98aa7e90c7c4714da3fc01655283f18.
Workflow producer/UI/QA transports: aced823a191263dca6706fbb994d8f688d8a1f2c,
1ef2945a294e7aaae2b1436ecef24e1d5bad2eb4, a7604a96feb7bc994d157a9df5be2b8ee47817d9.
Original PINS are 74f4d57957c701da2a7705c47303c2e770888758,
35611d018ca82f86ed9a3271128999012789b39a, 6fff4dc5229ea88a2f55a274ab2ab561a3760065.
Verifier/lock SHAs: 1294afe9ff9253475e9bf171afa3f50f220edbaf /
3de67a789222b46a9ef44cafd5b5263887fa33ee. Historical lock hashes cover old
boundary.c/consumer.cpp/controller.cpp AND UI client/widget units/run_checks.py.
They prove historical identity, not execution of migrated candidate tests.

AGE-198 commit 300f44e0e4f7a8264950818bb449ee8df387d9d2 builds candidate native
sources while preserving historical input verification. Retain that separation.
Before integration, audit every executed target including client_units/widget_units:
select candidate test sources explicitly and record their hashes; keep real pinned
product inputs unless separately reviewed. Do not rewrite old manifests to bless
new tests or silently claim pinned UI tests meet this contract. If UI migration is
still needed, create a bounded follow-up before claiming full rule enforcement.

## Regression guard acceptance and implementation packet

Next owner: Project Lead (AGE-199), to route implementation through approved
model-routing. AGE-198 keeps native migration ownership. Unit Test Engineer owns
behavioral guard tests; DevOps owns CI wiring; Integration QA owns real linkage.
Review Coordinator independently routes architecture, test and CI review.
Allowed paths: docs/testing, scripts/ci, tests/integration, candidate UI test/build
paths as separately assigned. No product behavior, credentials or access changes.

Add a policy check to existing CI before native/Qt builds. Enumerate ALL compiled
behavioral test sources from explicit build targets, recursively including local
headers/helpers. Reject legacy CHECK definitions, custom exit/assert/pass-count
harnesses and unregistered test executables; permit the assertion-free C11 bridge.
A token scan is only a tripwire, not proof of framework use: require GoogleTest
linkage, discovery and exact inventory/report agreement for every required target.
Read-only historical fixtures are exempt only when not compiled as candidate tests.
Do not ban application main, Cargo assertions or upstream GoogleTest sources.

Required negative examples in disposable copies: restore baseline boundary.c and
its old build invocation; hide CHECK in a local helper; drop a target; filter all
cases out; swallow a failing GoogleTest exit; substitute stale historical QA/UI
sources; remove XML. Each must be rejected nonzero with a named reason.
Positive example: migrated suites discover expected cases and pass with XML.
Behavioral red: compiled Clear mutant fails PartialClear/ResultClear nonzero.
Preserve all previous oracle cases; AGE-198 maps 7 ABI + 4 consumer + 30 controller
cases. Count alone does not prove preserved coverage. Do not change expectations
merely to pass a mutant or policy check.

Required hosted matrix: Ubuntu 24.04 Linux x86_64 and macOS 15 arm64; normal,
ASan/UBSan and TSan native checks, Cargo units and real Qt client/widget checks.
External GUI remains separate. Record unavailable checks as blocked/unexecuted.
Deliver PR URL, exact base/head, commands/results, inventory/reports and independent
review receipts; target <=200 changed lines per meaningful PR, preserve size gates.
Rollback: revert enforcement/migration together only after independent review;
never restore a custom runner as a workaround. No merge before exact-head gates.

Reference: [GoogleTest discovery and XML documentation](https://google.github.io/googletest/advanced.html).
