# AGE-7 candidate 4 — F3 spec and repeat-policy reconciliation

Author `a8ab3e67-2131-425e-b539-7273ba1bc46b`; run
`07f75a6a-3dca-4d02-bfdc-773450523cec`; 2026-11-05.
Project `50271359-4516-466c-8325-0d234bd5ddb0`, source AGE-7
`66531c23-4c1f-4c00-8edb-d28bc611539b`, parent AGE-3
`dd2750ff-6eb9-4f64-a1d3-270f9a7a511c`.
Repository `https://github.com/BohrTheory/Caclulator`; branch
`codex/AGE-7-a8ab3e67-2131-425e-b539-7273ba1bc46b`; assigned worktree
`/Users/andrejpersin/Projects/ideas/BhorTheory/Caclulator-worktrees/codex/AGE-7-a8ab3e67-2131-425e-b539-7273ba1bc46b`.
Parent architecture `eabbeb9581d157d573b81f7a8551a33826ecc79e`; original base
`3ef202581d4e784d0f9da508767577888a4fc272`.
Current spec `752d0961c8aacb2e79addeb6d1e26a7ea820d734`, both
`docs/product/calculator-spec-v1.md` and `docs/product/acceptance-cases-v1.md`,
read from immutable Git blobs. The containing commit is the new architecture
candidate; its full SHA and postcommit evidence are in the Paperclip handoff.
No self-referential commit hash is embedded in this file.

## Source and bounded amendment

The accepted task, current parent, company task-handoff policy and calculator profile
were read. Source author-return packet: AGE-7 `review-packet-752d096`, immutable
revision `a5817e2f-80bb-49bf-9c4a-0dcacadda711`; coordinator comment
`06e6a86b-3775-4373-9878-6cff51a7be20`. Both independent addenda request changes:
Domain AGE-19 revision `14249f39-1fbf-4eba-bf64-07c485ba3ab9`, Quality AGE-20
revision `feb72ce1-0761-46f8-88bd-992d936865e0`. Prior F1/F2 closure and old-pair
approvals remain historical and do not approve this amended pair.

| F3 requirement | Amendment / manual consistency inspection |
|---|---|
| 1. Current immutable pins and accurate acceptance | README, ADR, c-abi and common task envelope pin 752d096; all developer/QA packets inherit both product-file pins. Compatibility explicitly labels its old candidate paragraph historical. Incorporated policy is distinct from independent spec acceptance. |
| 2. Lexical selection | Normative full-text-first procedure strips balanced whole wrappers, selects rightmost depth-zero binary operator, excludes unary/exponent signs and retains complete RHS. Rust evaluates full and RHS; no chronological selection, operand-group descent or C++ precedence engine. |
| 3. Storage/transport | Save full value and display before RHS output reuse; separate operator/RHS/full value; max_digits10 and signed zero preserved, parenthesized operands, caller-owned live disjoint storage and status rules on every call. |
| 4. Lifecycle | Clear/edit invalidate; continuation establishes new descriptor on successful Equals; full/RHS/transport/repeat failures invalidate and recover; repeat keeps descriptor; no descriptor means no evaluator call. |
| 5. QA coverage | All U-12 sequences explicitly mapped to I-12 and G-12 on both platforms, with separate exact numeric, display and bit assertions; lifecycle/failure stages, storage reuse, no-call, continuation/recovery and retry coverage. Injected tests are labelled and do not replace real producer tests. |
| 6. M2 gates | README, ADR, compatibility and readiness/DevOps packet retain both owner acceptance of M1 first release and revision-pinned accepted scientific contract with operations/domains/angles/numeric/error/display behavior. Planning permitted; neither gate claimed passed. |

Changed only `docs/architecture/`: five active documents (README, ADR, c-abi,
compatibility, integration-packets), supersession notices appended to three prior
evidence Markdown records, and this record plus its JSON. Original evidence JSON
and regression logs are unchanged. No calculator component or workflow implementation.
Canonical JSON, generated C/Rust declarations, generator, checker, regressions and
examples are byte-identical to the parent; ABI remains 1.0 with no migration of
signatures. Experience/QA must migrate task inputs and behavior cases per compatibility.md.

## Actual verification versus reuse

[Structured evidence](repeat-policy-reconciliation.json) records command arrays,
cwd, exit codes, stdout/stderr, source digests and explicit scope. Actual checks:

- `python3 tools/generate_abi.py --check` — exit 0, generated artifacts match.
- `git diff --check` — exit 0.
- `git diff --exit-code eabbeb9581d157d573b81f7a8551a33826ecc79e -- contracts include tools examples` — exit 0.
- Python evidence assertions — all nine prior checker-input SHA-256 values match;
  original historical JSON/log bytes unchanged; active pins/stale-text absence,
  Markdown file links and every U-12 oracle row checked. This is documentation/input
  consistency inspection, not execution of the described algorithm or calculator.
- Manual full diff comparison with pinned D-07/§4.2, U-12/I-12/G-12 and the six
  author-return requirements above; corrected a draft recovery example before commit.

Initial verification harness attempt (`python3 /tmp/age7-verify-f3.py`) exited 1:
the link check reached this record before its JSON report existed. The harness now
reserves the report path before checking links and replaces it with the completed
report. The subsequent run passed. This was evidence-generation ordering, not a
calculator/runtime failure; the diagnostic is retained in JSON and the run transcript.

Prior C11/C++17/Rust compilation, negative fixtures, symbol-reference checks,
read-only archive checks and seven checker regressions are **reused, not rerun**.
They bind unchanged inputs in `review-remediation.json` and the prior exact-archive
Paperclip evidence revision `a221b9b1-5014-4dd4-88f9-7883afd35a50`. No executable
source changed, so broad reruns would not verify the new documentation semantics.
The new immutable SHA still requires independent review.

## Remaining checks and exact next action

Unexecuted: real producer/export/link/run, numerical/display/transport/output
transactions and repeated-equals runtime, panic/concurrency/sanitizers, Linux,
minimum macOS runtime, Qt/GUI/packages and trusted CI. Detailed implementer/QA
owners, cases and future commands remain in integration-packets.md. Spec acceptance
and exact-pair architecture approval remain separate requirements.

Review Coordinator `070eb2bb-d921-431a-8a07-b944e1b86a86`, existing AGE-18
`77ebc667-df64-4733-91ad-52be1a58d58b`, must obtain renewed Domain AGE-19
`ad607500-308c-4b06-a39b-3ca368a121af` and Quality AGE-20
`f12562ce-e597-4124-ab75-7ce1080d4290` verdicts for the new architecture SHA plus
spec 752d096 through existing stage `ca2a0d26-a9b2-4b98-9007-56e842f3c67a`.
Reviewers use their own disposable read-only exact-SHA exports and external writable
evidence; no author-checkout edits. Review the full contract scope and six F3 rows,
not just byte stability. Record severity/path/reproduction and exact-pair verdict;
return defects to this author. No duplicate tasks or new arbitration are needed.
After required acceptance, Lead updates/reuses component tasks with both accepted
pins and each author's isolated workspace. No source/spec closure dependency cycle.

GitHub publication remains undelivered under Director disposition
`02395565-54e6-47b5-9782-55cc780f7f23`: existing AGE-4/AGE-3 own verified App
installation/gateway, real PR head, CI provenance and branch/review controls.
That disposition is reused; no discovery rerun, personal-token fallback, remote
push, PR, merge, release, access or credential change is claimed. Independent local
review may proceed. Both M2 gates and owner release acceptance remain intact.
