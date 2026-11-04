# Dependency map and integration task packets

These are detailed handoff inputs, not a parallel task queue. Paperclip remains the
assignment/status authority. AGE-7 authors architecture only. Lead/authorized routing
owner creates or reuses actual implementation tasks once the stated inputs exist.
Do not start implementation from this unapproved candidate. Every new task requests
its own isolated branch/worktree; reviewers receive a disposable read-only source
copy exported from the exact reviewed commit, with build output outside that copy.

## Readiness graph

| Input | Owner / authority | Blocks only |
|---|---|---|
| M1 behavior, numeric model/display/tolerance revision | PM AGE-6 commit `c5ff879a8df5a673f3b8dae7d5e46fe30ec67549`; AGE-11 numerical agreement received | Numerical dependency resolved; spec independent review remains required; compound repeat selection needs PM clarification for UI |
| Canonical ABI + ADR, independent current-SHA verdict | Tech Lead AGE-7; neutral review routing | Component implementation against fixed interface |
| Rust compiler/Cargo | DevOps AGE-4 capability evidence | Core compile/unit and linked integration |
| C++ compiler, xmake, Qt Widgets | DevOps AGE-4 capability evidence | UI build/unit and desktop packaging; no Qt needed for core or C ABI integration |
| Actual producer archive and C++ consumer | Domain and Experience Developers | Integration QA real-link tests |
| Built app plus GUI driver/session | DevOps and System QA | Platform-specific external GUI E2E |
| Verified bot gateway and branch controls | DevOps/Lead | Push/PR/merge only |
| Owner accepts M1 | Lead AGE-3 | M2 implementation, not M2 planning |

AGE-11 supplied its numerical decision; it must not depend on completion of AGE-7.
Track the remaining compound-repeat UI clarification with PM without reopening
the agreed arithmetic/formatting contract or blocking independent core work.
Review tasks must not wait for AGE-7 to close; they review its candidate SHA.
Use real issue IDs for real prerequisites only. Do not add a global environment,
staffing, GUI or parent-completion blocker to independent source work.

## Common task envelope

Project `50271359-4516-466c-8325-0d234bd5ddb0`, parent delivery AGE-3
`dd2750ff-6eb9-4f64-a1d3-270f9a7a511c`, source architecture AGE-7
`66531c23-4c1f-4c00-8edb-d28bc611539b`, repository
`https://github.com/BohrTheory/Caclulator`. Initial base
`3ef202581d4e784d0f9da508767577888a4fc272`; execution tasks must replace this with
the accepted contract commit plus spec `c5ff879a8df5a673f3b8dae7d5e46fe30ec67549`
in their packet and record their own
actual base/current SHA, branch, absolute task worktree and run/agent IDs.

Each recipient reads the calculator profile and task-handoff policy. No paid API
fallback, secret transfer, access change or personal GitHub credentials. Use only
provided local tools and approved task metadata allowance; no global installation.
No publication by implication. Budget is one bounded task/run and the existing
subscription limits; stop and preserve work on quota/environment failure. Route
defects to the author with reproduction, technical disputes to Director, product
scope forks through PM/Lead. Preserve existing work; no reset or edits to another
checkout. Handoff requires exact commands/exit codes/log paths, actual versus
unexecuted checks, remaining blockers and next responsible agent. No claim of
approval without a verdict naming the current SHA.

## Domain developer packet

Goal: evaluate accepted M1 expressions via the actual Rust staticlib, independent
of Qt. Allowed paths: `core/`, its Cargo manifests/lockfile and core unit tests.
Generated files come from the architecture commit; signature/schema changes return
to Tech Lead. Excluded: UI, workflows, scientific functions, runtime plugins.

Inputs: accepted AGE-6 spec/acceptance cases; `c-abi-v1.md`, canonical JSON, generated
Rust module/checks and header at accepted SHA. Dependencies: these accepted artifacts
and working Rust/Cargo only. Import generated types as `crate::abi_generated`, expose
`crate::ffi::{calc_abi_version,calc_evaluate_v1}`, and include generated producer checks.
Export the symbols with stable C linkage (edition-appropriate no-mangle attribute).

Implement parser, immutable operation registry, basic operations, canonical display,
FFI validation/transaction and unwind containment exactly as specified. No Qt in
Cargo dependency graph. Bound parser/evaluator stack and work for adversarial legal
input. No heap ownership crosses the ABI. Separate pure domain types from FFI raw
pointers. Use generated constants instead of copied discriminants.

Acceptance/unit evidence: exact integer results 5/-3/42/4; precedence 14/20;
left associativity, unary signs, decimal editing literals, exponent boundaries,
numeric tolerance and exact display strings from spec; malformed/empty UTF-8,
NUL, incomplete expression, divide by signed zero, finite overflow/underflow,
correctly rounded parsing of lossless transported binary64 literals including
minimum subnormal and maximum finite values (exact bit identity),
4096/4097-byte and depth64/65 boundaries; all documented null cases, size-query,
exact/short capacity, no output writes on INVALID_ARGUMENT, output reset on errors,
panic injection containment and recovery. No undefined-address tests as success cases.

Expected commands once implemented: `cargo test --manifest-path core/Cargo.toml`,
`cargo build --manifest-path core/Cargo.toml --release`, generation check, dependency
inspection proving no Qt. Record real commands if package/target names differ via
approved contract amendment. Deliver archive path/target/hash, symbols, exact Rust
version/native link requirements and local commit. Independent reviewers: Domain
Reviewer and Quality Reviewer for FFI; coordinator records their current-SHA verdicts.

## Experience developer packet

Goal: keyboard/button-driven Qt Widgets calculator that delegates calculations and
formatting to Rust. Allowed paths: `ui/`, xmake app target and UI/adapter unit tests;
coordinate root build files with DevOps. Excluded: core arithmetic, ABI edits,
workflows, history/memory/graphs, scientific implementation.

Inputs: accepted PM editing/focus/sign/clear/repeat-equals/error recovery cases and
accepted ABI header/SHA. Source implementation requires those contracts; GUI build
requires C++/Qt/xmake. UI scaffolding and adapter unit tests can use a test double,
clearly labelled; mocks do not satisfy real integration acceptance.

Adapter verifies ABI version, retains UTF-8 storage throughout each call, uses a
64-byte display buffer or correct size-query/retry, maps byte offsets to Qt text,
maps every status to stable localizable error presentation, handles unknown status
as failure, and never shows the numeric failure sentinel as a result. Exceptions
from C++ allocations stay on C++ side. Editor owns state but no arithmetic; it uses
the agreed continuation policy and clears stale results after input changes.
Retain successful binary64 results and repeat operands separately from display.
Use the lossless ASCII transport in c-abi-v1.md; Rust evaluates any compound repeat
operand. Never reparse the 15-digit display for automatic continuation/repeat.
PM must settle descriptor selection for compound expressions before that UI behavior
is accepted; do not infer it by introducing a second evaluator. Preserve the failed
expression for backspace correction; Clear/fresh digit recovery follows AGE-6.

Acceptance: developer unit checks for PM state transitions, all keyboard bindings,
focus, error recovery, UTF-8 conversion lifetime, error offset mapping, buffer retry,
ABI mismatch and unknown status. Assert exact retained-value transport, `1/3=`
then `*3=` numeric exactly 1, and `0.1+0.2=` then `-0.3=` numeric `2^-54`;
repeat the retained RHS without accumulating display-rounding loss. Test signed
zero/subnormal/max-finite transport by bits. These are planned tests, not executed.
Build with xmake using actual Rust archive for the
app. Deliver exact build/test commands, outputs, target/Qt version, artifact hash and
SHA. Experience Reviewer plus Quality Reviewer assess interface usage independently.

## Integration QA packet

Goal: prove actual C/C++ → C ABI → Rust linkage on both platform targets. Allowed
paths: `tests/integration/` and test build integration; never replace producer with
a mock. Needs accepted spec/ABI, built real archive, C/C++ tools and native link
libraries; Qt/GUI driver is not a prerequisite. Independent implementation tests
must not reuse the evaluator as their oracle.

Compile C11 and C++17 clients against the generated header and actual archive. Run
normal numeric/display cases plus every status/ownership table row: size query,
exact capacity, one-byte-short capacity, canaries around writable buffers,
untouched outputs on INVALID_ARGUMENT, empty outputs on other errors, embedded NUL,
malformed UTF-8, input/depth bounds, signed-zero divisor, overflow and recovery.
Include the exact-bit transport and retained-value continuation cases from
c-abi-v1.md through the real adapter/producer; tolerant numeric checks alone do
not satisfy that acceptance.
Run repeated calls and concurrent disjoint-output calls; verify no hidden angle or
error state and no cross-allocator free requirement. Use sanitizers where supported
and identify which language/runtime was instrumented. Test-only panic injection in
the producer must prove INTERNAL and subsequent call recovery, without shipping a
public panic trigger. Preserve a symbol inspection and both platform link logs.

Required evidence: actual command/exit/log/SHA, compiler/OS/target, archive hash,
assertion cases and failures; distinguish compile-only, real link, real execution,
sanitizer and cross-platform rows. Current AGE-7 examples are compile-only and do
not discharge these obligations. Quality Reviewer independently reviews test
coverage/oracles and evidence. Return defects to the owning developer with minimized
input and expected/actual result; contract ambiguity goes to Tech Lead/Director.

## System QA / DevOps packet

System QA consumes matched built packages and accepted PM requirement mapping. Run
external GUI scenarios on macOS arm64 with Appium Mac2/full Xcode/Accessibility and
Linux x86_64 with AT-SPI/PyGObject/Xvfb/Openbox/D-Bus. Verify real keyboard/focus,
editing, clear, arithmetic, error display/recovery and the shipped package. Save
driver logs, step outcomes and screenshots for failures with package SHA/hash.
Missing driver/session blocks that platform test only. Direct ABI calls are not E2E.
Experience and Quality Reviewers assess system evidence.

DevOps pins tool versions and confirms the proposed macOS14/Ubuntu24.04 baseline,
records target consistency, Rust native link dependencies, packaging and clean-build
commands, and produces checksummed matched app/core bundles. Any minimum-OS change
returns to Tech Lead for impact review. DevOps owns future workflows; AGE-7 does not
author them. Review of architecture readiness does not assert product release checks
have run. Lead retains the original M1/M2 owner acceptance and protected merge gates.
