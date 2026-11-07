# GoogleTest migration (AGE-198)

The AGE-29 native checks now run as 41 named GoogleTest tests instead of three
processes with custom CHECK macros. GoogleTest reports failures, supports filters
and shuffled runs, and emits XML. There are no application implementation changes.
The C boundary suite calls Rust through `c_consumer.c`, compiled separately as C11.
The C++ consumer and Controller/Adapter suites also link the real Rust archive.
Integration QA retains ownership of these integration checks; this migration does
not claim independent review or replace component units or external GUI QA.

## Contract and oracles

Semantics are unchanged from the original AGE-29 fixtures at main base
`c9a59564a98aa7e90c7c4714da3fc01655283f18`. Product cases are specified in
`docs/product/calculator-spec-v1.md` and `docs/product/acceptance-cases-v1.md`,
published in `13773178bafc82271602076877c68bb1e6ed9b95` from accepted source
`752d0961c8aacb2e79addeb6d1e26a7ea820d734`. The accepted ABI contract revision is
`67100ba0ce1d7aaf4fcafadb9101465b78d19829`,
`contracts/calculator-abi-v1.json` / `docs/architecture/c-abi-v1.md`.

| Suite | Cases and oracle provenance |
| --- | --- |
| AbiBoundary (7 tests) | Exact arithmetic and precedence; ties-to-even display, fixed/scientific thresholds; invalid arguments with no writes; all capacities 0–64 and canaries; 4096/4097 bytes and 64/65 nesting; UTF-8 precedence and byte offsets; syntax/domain/range errors. ABI v1 plus product numeric/display/error rules. |
| Consumer (4 tests) | ABI version policy; binary64 transport at signed zero, subnormal/normal minima, maximum and decimal extremes; retained values; 8 threads × 1000 independent calls. ABI reentrancy/lifetime/version rules and exact bit equality. |
| ControllerContract (30 tests) | Repeat-equals matrix, editing/clear/sign/keyboard, error recovery, overflow, lossless continuation, formatting; injected statuses, retry and conversion failures. Product D-07 and I-09–I-20 plus model/adapter contract. |

Integer expectations follow arithmetic and precedence. Repeats use the last
lexical binary operator and the evaluated RHS, as D-07 specifies (e.g.
`2+3*4`: 14 → 56 → 224). Numeric approximations use the accepted tolerance
`max(1e-15, 1e-12*abs(expected))`; display is exact. IEEE-754 binary64 round-to-nearest
arithmetic derives `(0.1+0.2)-0.3 = 2^-54` and the retained `1/3 * 3 = 1`.
Synthetic version/status/conversion injections are explicitly consumer tests,
not claims that Rust emitted those injected failures.

## Reproduction

Use Python 3.12+, Rust 1.89.0 and clang with C11/C++17. Export or check out the
reviewed producer `aced823a191263dca6706fbb994d8f688d8a1f2c` and UI
`1ef2945a294e7aaae2b1436ecef24e1d5bad2eb4` into separate read-only inputs.
Build the archive with `cargo build --locked --release --manifest-path
<producer>/core/Cargo.toml`; obtain platform link flags with `cargo rustc --locked
--release --manifest-path <producer>/core/Cargo.toml -- --print native-static-libs`.

```sh
python3 tests/integration/age29/run_gtest.py \
  --producer <producer> --experience <experience> \
  --archive <cargo-target>/release/libcalculator_core.a \
  --output <fresh-output-directory> --mode normal \
  --link=-lSystem --link=-lresolv --link=-lc --link=-lm
```

The example link flags are for macOS; use the producer's emitted flags on Linux.
Repeat with `--mode asan-ubsan` and `--mode tsan`. GoogleTest 1.17.0 is downloaded
from the URL in the runner and verified against its SHA256 before extraction.
Network access is required. Build output must be fresh. `results.xml` and
`test-inputs.json` record results and source/archive/framework hashes. A filter
selecting zero tests fails. Run the built executable with `--gtest_shuffle
--gtest_repeat=5` to check fixture isolation.

`python3 scripts/ci/build.py ... --native-only` runs the same three modes after
verifying pinned producer/UI/historical QA inputs. It executes tests from the
candidate repository and copies XML/provenance into the existing CI log artifact.
Historical projection hashes remain unchanged; they identify historical sources,
while candidate test-source hashes are recorded separately.

## Evidence and handoff

The AGE-198 test packet records the exact test/combined SHA, commands, logs and
red/green results. Red proofs use disposable UI copies: change Clear to a no-op
and run PartialClear/ResultClear (two failures); omit repeated-equals and run
RepeatMatrix (one failure). Both mutants must compile and produce assertion
failures, not compiler errors. Never commit either mutation.

Local validation covers macOS arm64; hosted macOS 15 and Ubuntu 24.04, Qt widget
execution, packaging and GUI remain separate checks. Sanitizers instrument the
C/C++ harness and GoogleTest; the Rust archive and std are not instrumented.
Consume the dedicated test commit in a separate implementation worktree if
subsequent implementation is needed. Behavioral expectation changes require
contract-owner confirmation when semantics change. Review both tests and code.
