# AGE-7 candidate 2: PM numerical reconciliation

Task `66531c23-4c1f-4c00-8edb-d28bc611539b`, project
`50271359-4516-466c-8325-0d234bd5ddb0`, parent AGE-3
`dd2750ff-6eb9-4f64-a1d3-270f9a7a511c`.
Author `a8ab3e67-2131-425e-b539-7273ba1bc46b`, run
`b432833c-7d6e-4b3d-86c9-30fe99785657`.
Repository `https://github.com/BohrTheory/Caclulator`;
branch `codex/AGE-7-a8ab3e67-2131-425e-b539-7273ba1bc46b`;
worktree `/Users/andrejpersin/Projects/ideas/BhorTheory/Caclulator-worktrees/codex/AGE-7-a8ab3e67-2131-425e-b539-7273ba1bc46b`.
Base/prior candidate `5fd95cfad404d85b72c6a9d4663b1d607aec5e6d`.
The immutable commit containing this record is the updated contract reference;
its full SHA is supplied in the Paperclip handoff (no self-referential hash).

## Source and disposition

Read PM comment `ede93b3d-eb06-443d-a465-842f070b7932` and both product files
from actual Git object `c5ff879a8df5a673f3b8dae7d5e46fe30ec67549`:
`docs/product/calculator-spec-v1.md`, `docs/product/acceptance-cases-v1.md`.
Read accepted AGE-3, company task-handoff policy and calculator profile.
The PM revision agrees the candidate's precedence, grammar, binary64 behavior,
limits, display and tolerance. The earlier numerical-agreement dependency is
resolved; the PM files explicitly still await independent review.

Changed only `docs/architecture/`: README, ADR-0001, behavioral contract,
compatibility plan, integration packets, historical verification disposition,
and this Markdown/JSON evidence pair. Canonical JSON, all generated declarations,
examples and tools are unchanged. ABI remains 1.0. No implementation or workflow
has been added; no source/branch from another task was edited or imported.

The required continuation implementation serializes retained binary64 values into
round-trip ASCII and sends complete expressions to Rust. It never reparses display
text. Documented bit-identity and precision-sensitive integration oracles are
requirements for future implementation, not claims of runtime execution here.
The allowed failed-expression visual default is to retain it for backspace repair.

## Actual verification versus planned checks

[pm-reconciliation.json](pm-reconciliation.json) records run identity, source SHA,
product blob digests, actual commands/exits and all eight tested-input digests.
All eight match the original [verification.json](verification.json) exactly.
Original C11/C++17/Rust compilation, positive/negative signature fixtures and drift
checks remain historical execution evidence for those identical inputs; they were
not rerun or relabelled as this run's checks.

Actual commands, all exit 0:

```sh
python3 tools/generate_abi.py --check
git diff --check
git diff --exit-code 5fd95cfad404d85b72c6a9d4663b1d607aec5e6d -- contracts include tools examples
```

Digest comparison used Python `hashlib.sha256(Path(path).read_bytes()).hexdigest()`
against every `input_sha256` entry in the original report; all eight matched.
Spec inputs were read with `git show <spec SHA>:<path>` and hashed, not taken from
another author's mutable checkout. After commit, verify with `git diff --check
HEAD^ HEAD` and record the resulting SHA/clean status in Paperclip.

The first `git add docs/architecture` failed with exit 128 because the sandbox
rejected creation of the assigned worktree metadata `index.lock`. A bounded
task-authorized staging/commit escalation follows; its outcome is recorded in the
Paperclip handoff. No change to global Git configuration or credentials is required.

Not executed: actual producer/export/link/run, numerical/transport runtime tests,
Qt build, GUI, Linux, packaging, sanitizers/concurrency/panic injection. Their direct
owners and requirements remain in integration-packets.md. No push/PR/merge/release
was attempted; verified bot integration and branch/review controls remain required.

## Remaining work and exact next accountable actions

1. Director through existing AGE-16 (`46cbeea7-a6ec-4f47-8004-dc762eb3c507`)
   routes candidate 2 to Review Coordinator, Domain Reviewer and Quality Reviewer.
   Use a disposable read-only source copy of the new immutable SHA; write build
   outputs externally. Review the complete contract and generator, including this
   semantic amendment. A prior-candidate verdict cannot approve this commit.
   Return severity/path/reproduction findings and an explicit current-SHA verdict.
2. PM through existing AGE-11 (`3e6942f1-2885-4092-bde0-c905ecc77277`)
   clarifies repeat-descriptor selection for compound expressions. Question comment
   `1f16d77f-3a37-4ba5-ae6e-ab3b0242772a` was corrected by
   `166f83c8-6984-46dc-a573-efeb886b70b3`: the recommendation is the last entered
   outermost binary operator and its RHS, giving subsequent equals results 56 for
   `2+3*4`, 80 for `(2+3)*4`, 98 for `2*(3+4)`. These are proposals awaiting PM;
   the earlier 26 example was an author mistake and is superseded. No policy is
   accepted by silence. This affects compound-repeat UI acceptance, not the agreed
   stateless core arithmetic. A decision needing parser metadata returns to Tech
   Lead for an explicit ABI amendment before dependent implementation.
3. Tech Lead resolves independent findings and any actual interface impact, then
   supplies the accepted contract SHA. Project Lead releases/reuses component tasks
   with that SHA plus accepted spec SHA, each in its own isolated workspace using
   integration-packets.md. Missing GUI/GitHub does not gate unrelated source work.
   M2 retains the owner acceptance gates; neither peer agreement nor review waives them.

AGE-7 is not marked done. The prior attempt to set `in_review` failed with HTTP422
`invalid_issue_disposition` / missing `review_path`. Preserve actual `in_progress`
until authorized native routing establishes a recognized review participant; do
not invent an owner approval or timer to force a transition. No second scheduler,
private task queue, or duplicate review task has been created.

## Candidate 4 supersession notice

This record and its original JSON remain historical evidence. Active spec selection
and the former pending compound-repeat question are superseded by
[repeat-policy-reconciliation.md](repeat-policy-reconciliation.md), pinning spec
`752d0961c8aacb2e79addeb6d1e26a7ea820d734`. Prior adverse verdicts and F1/F2 closure
are preserved in their assessed scope; none is an approval of the new pair.
