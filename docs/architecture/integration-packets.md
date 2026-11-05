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
| M1 behavior, numeric model/display/tolerance revision | PM AGE-6 commit `752d0961c8aacb2e79addeb6d1e26a7ea820d734`; AGE-11 numerical agreement received | Numerical and lexical repeat policy incorporated; separate independent spec acceptance remains required |
| Canonical ABI + ADR, independent current-SHA verdict | Tech Lead AGE-7; neutral review routing | Component implementation against fixed interface |
| Rust compiler/Cargo | DevOps AGE-4 capability evidence | Core compile/unit and linked integration |
| C++ compiler, xmake, Qt Widgets | DevOps AGE-4 capability evidence | UI build/unit and desktop packaging; no Qt needed for core or C ABI integration |
| Actual producer archive and C++ consumer | Domain and Experience Developers | Integration QA real-link tests |
| Built app plus GUI driver/session | DevOps and System QA | Platform-specific external GUI E2E |
| Verified bot gateway and branch controls | DevOps/Lead | Push/PR/merge only |
| Recorded owner acceptance of M1 first release AND revision-pinned accepted scientific v1 contract compatible with ABI (operations/domains/angles/numerical accuracy and tolerance/errors/display) | Lead AGE-3 / PM scientific contract | M2 implementation only; planning may proceed; neither gate is inferred satisfied |

AGE-11 supplied its numerical decision; it must not depend on completion of AGE-7.
AGE-22 repeat policy is incorporated in the pinned spec D-07/§4.2 and U-12/I-12/G-12.
The former PM clarification is resolved; independent spec and exact-pair architecture
acceptance remain separate gates. No new owner question or arbitration is needed.
Review tasks must not wait for AGE-7 to close; they review its candidate SHA.
Use real issue IDs for real prerequisites only. Do not add a global environment,
staffing, GUI or parent-completion blocker to independent source work.

## Common task envelope

Project `50271359-4516-466c-8325-0d234bd5ddb0`, parent delivery AGE-3
`dd2750ff-6eb9-4f64-a1d3-270f9a7a511c`, source architecture AGE-7
`66531c23-4c1f-4c00-8edb-d28bc611539b`, repository
`https://github.com/BohrTheory/Caclulator`. Initial base
`3ef202581d4e784d0f9da508767577888a4fc272`; execution tasks must replace this with
the accepted contract commit plus spec `752d0961c8aacb2e79addeb6d1e26a7ea820d734`
in their packet (both product files: `docs/product/calculator-spec-v1.md` and
`docs/product/acceptance-cases-v1.md`). Every Domain, Experience, Integration QA,
System QA and DevOps packet below inherits that exact spec pin; incorporation is
not independent acceptance. Record their own
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
Implement the normative selection/transaction/lifecycle in c-abi-v1.md: evaluate
current full text in Rust, save full-result binary64 and display before reusing
outputs, strip balanced whole-expression wrappers, select the rightmost depth-zero
binary operator excluding unary/exponent signs, and send its entire RHS suffix to
Rust. Do not descend into operand groups or select from key history; C++ has no
numerical evaluator or precedence engine. Keep operator, RHS value and full result
separate. All calls obey storage lifetime/disjoint-output/size/status rules.
Commit only after required calls succeed. Repeated Equals uses parenthesized
lossless values, retains the same descriptor, and updates only result/display.
No descriptor means unchanged result/display and no evaluator call.
Clear/any edit invalidates; operator continuation creates a new expression and
establishes a new descriptor only on successful Equals. Full/RHS/transport/repeat
failure invalidates and enters recovery without stale operands or failure sentinels.
Preserve the failed expression for backspace correction; Clear/fresh digit recovery
follows AGE-6. The tables below specify required acceptance, not executed tests.

Acceptance: developer unit checks for PM state transitions, all keyboard bindings,
focus, error recovery, UTF-8 conversion lifetime, error offset mapping, buffer retry,
ABI mismatch and unknown status. Assert exact retained-value transport, `1/3=`
then `*3=` numeric exactly 1, and `0.1+0.2=` then `-0.3=` numeric `2^-54`;
repeat the retained RHS without accumulating display-rounding loss. Test signed
zero/subnormal/max-finite transport by bits. Execute every repeat and lifecycle row
below in controller unit tests, including observable adapter-call counts. Clearly
label injected failure/test-double cases; they do not prove real Rust execution.
These are planned tests, not executed.
Build with xmake using actual Rust archive for the
app. Deliver exact build/test commands, outputs, target/Qt version, artifact hash and
SHA. Experience Reviewer plus Quality Reviewer assess interface usage independently.

## Shared repeat acceptance: U-12 → I-12 → G-12

Expression rows come from spec `752d0961c8aacb2e79addeb6d1e26a7ea820d734` U-12,
with explicit architecture storage/call obligations. Each sequence includes the
initial Equals and two repeated Equals unless stated otherwise. Experience owns
controller unit coverage; Integration QA runs every sequence through the actual
controller/adapter/Rust library as I-12; System QA performs each visibly as G-12
on **both** macOS arm64 and Linux x86_64/Ubuntu 24.04. GUI verifies visible outputs;
unit/integration separately assert numeric values, descriptor state and call counts.
The evaluator under test must not generate the expected answers.

| Current expression / actions | Descriptor (Rust-evaluated RHS) | Numeric oracle | Exact display oracle | Layers |
|---|---|---|---|---|
| `2+3*4` | `*`, 4 | 14, 56, 224 exactly | `14`, `56`, `224` | U-12 / I-12 / G-12 both platforms |
| `(2+3)*4` | `*`, 4 | 20, 80, 320 exactly | `20`, `80`, `320` | U-12 / I-12 / G-12 both platforms |
| `2*(3+4)` | `*`, 7 from Rust RHS `3+4` in its group | 14, 98, 686 exactly | `14`, `98`, `686` | U-12 / I-12 / G-12 both platforms |
| `2+3` | `+`, 3 | 5, 8, 11 exactly | `5`, `8`, `11` | U-12 / I-12 / G-12 both platforms |
| `((2+3))` | `+`, 3; strip both whole wrappers | 5, 8, 11 exactly | `5`, `8`, `11` | U-12 / I-12 / G-12 both platforms |
| `8/4/2` | `/`, 2 | 1, 0.5, 0.25 exactly | `1`, `0.5`, `0.25` | U-12 / I-12 / G-12 both platforms |
| `2*-3` | `*`, -3; sign is unary | -6, 18, -54 exactly | `-6`, `18`, `-54` | U-12 / I-12 / G-12 both platforms |
| `2+1e-3` | `+`, Rust binary64 `1e-3`; exponent sign excluded | 2.001, 2.002, 2.003 under D-03 tolerance | `2.001`, `2.002`, `2.003` | U-12 / I-12 / G-12 both platforms |
| `-(2+3)` | none; no descent into operand group | -5 unchanged | `-5` unchanged | U-12 / I-12 / G-12 both platforms |
| `7` | none | 7 unchanged | `7` unchanged | U-12 / I-12 / G-12 both platforms |
| Enter `2+3*5`, Backspace, `4`, Equals, Equals | `*`, 4 from current text | 14, 56 exactly | `14`, `56` | U-12 / I-12 / G-12 both platforms |

Separate numeric/display/bit assertions are mandatory: D-03 tolerance applies only
to approximate numeric rows. It cannot prove transport identity, excuse the wrong
operator, or replace exact displays. Also run existing c-abi-v1.md transport oracles
(`1/3=` then `*3=` exactly 1; `0.1+0.2=` then `-0.3=` exactly positive `2^-54`)
and exact-bit identity for -0, minimum subnormal/normal, maximum finite and signed
exponent values. Record retained RHS bits across successful repeats and assert they
never change; full-result/display must survive reuse of RHS output storage.

### Lifecycle, failure and call coverage

Count logical adapter evaluations with a sufficient 64-byte buffer; any deliberately
exercised size-query/retry adds the documented ABI calls and must be logged separately.
For an expression with a descriptor there are two initial evaluations (full and RHS),
then one per repeat. With no descriptor there is one initial full evaluation, no RHS
call and zero calls for later Equals. Observe real calls in I-12 without replacing
the producer. Controller units may use a recording test double.

| Stage / action | Required state and oracle | Evidence owner / layer |
|---|---|---|
| Full/RHS storage reuse: `2*(3+4)=` | Save full value/display 14 before RHS gives 7; commit result 14 and RHS 7; next Equals 98. No aliasing during either call. | Experience U-12; real Integration I-12; visible G-12 |
| Clear after `2+3=`; then `7=`, `=` | Clear removes descriptor/result/error; 7 remains 7; final Equals makes no call. | U/I-12; G-12 both platforms |
| Digit/decimal, backspace, sign-toggle edits after result | Each immediately invalidates descriptor. `2+3=`, digit `7`, `=`, `=` gives 7, 7; backspace/sign changes are evaluated from edited text only. | U/I-12 plus U/I-10/11/13; visible G cases both platforms |
| Operator continuation: `2+3=`, `*2=`, `=` | Old `+3` invalidated; new descriptor `*2` established only on successful Equals; values/displays 5, 10, 20 exactly. | U/I-12/13; G-12/13 both platforms |
| Full-expression failure: `1/0=` or `2+=` | Normal error; descriptor invalid; no RHS call; no numeric sentinel becomes visible result. Clear then `2+3=` gives 5. After the `2+=` error, Backspace removes `+`; entering `+3=` gives 5. | U/I-12/14/20; visible error/recovery G cases both platforms |
| RHS-stage failure after successful full evaluation (injected adapter error) | Do not commit saved full success or failed RHS; invalidate descriptor, error state, no repeat using stale data. Test INVALID_ARGUMENT/unknown status and a producer error such as INTERNAL; untouched/reset output rules cannot be mistaken for success. | Experience U-12 and labelled integration fault-injection; not ordinary production/GUI conformance |
| Transport conversion/composition failure on continuation or repeat (injected conversion failure; real payload limit where applicable) | No evaluator call for failed conversion; invalidate descriptor, show error, discard failed payload; never fall back to display parsing. Clear/fresh digit recovery succeeds. | U/I-12/13 failure harness; label injected vs real failure evidence |
| Repeat evaluation failure: `1e154*1e154=`, `=` | Initial finite full result; repeated multiplication overflows to RANGE; descriptor invalid, no stale numeric result; fresh `7=` gives 7 and next Equals makes no call. | U/I-12/19/20; error/recovery G cases both platforms |
| Size-query/retry or retry failure at full/RHS/repeat stage | BUFFER_TOO_SMALL is not final UI success; successful retry preserves correct result/descriptor. Failed retry invalidates descriptor and follows recovery. Input immutable and outputs disjoint on each call. | U adapter tests; real I buffer tests plus labelled failure injection |

After each failure stage test Clear, fresh digit/decimal and retained-expression
backspace recovery. A handled failure must not poison the next valid call. Injected
failures test controller handling only; real producer panic/transaction tests remain
Integration QA obligations. G-12 is required for every U-12 expression above and the
user-observable lifecycle cases; internal call counts/bit checks and injected-only
paths require U/I evidence and are not claimed from screenshots.

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
not satisfy that acceptance. Execute the shared I-12 matrix and lifecycle cases
through the real controller/adapter/producer, with direct C/C++ transport probes
where needed. Keep injected failure-harness evidence separate from real-library
results and record no-call assertions, not just unchanged display.
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
editing, clear, arithmetic, error display/recovery and the shipped package. G-12
must execute every shared U-12 sequence above on each platform, plus visible
clear/edit/continuation/failure recovery; reference the exact spec/contract pair. Save
driver logs, step outcomes and screenshots for failures with package SHA/hash.
Missing driver/session blocks that platform test only. Direct ABI calls are not E2E.
Experience and Quality Reviewers assess system evidence.

DevOps pins tool versions and confirms the proposed macOS14/Ubuntu24.04 baseline,
records target consistency, Rust native link dependencies, packaging and clean-build
commands, and produces checksummed matched app/core bundles. Any minimum-OS change
returns to Tech Lead for impact review. DevOps owns future workflows; AGE-7 does not
author them. Review of architecture readiness does not assert product release checks
have run. Lead retains protected merge gates and both M2 prerequisites: recorded
owner acceptance of the M1 first release AND a revision-pinned accepted scientific
contract compatible with ABI covering operations/domains/angles/numerical accuracy
and tolerance/errors/display. Planning is allowed; implementation is gated.
