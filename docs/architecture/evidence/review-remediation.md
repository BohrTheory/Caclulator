# AGE-7 review remediation — F1 and F2

Author: Technical Lead `a8ab3e67-2131-425e-b539-7273ba1bc46b`.
Run: `b3daf5ef-fa1b-4de9-b75f-843e0b780217`, 2026-11-05 local date.
Source task AGE-7, parent AGE-3; project/repository and contract index are in
[README](../README.md). Prior candidate `e5aea8fc091b2b98ce90a3abd21912f72cc090f6`,
original base `3ef202581d4e784d0f9da508767577888a4fc272`, accepted PM spec
`c5ff879a8df5a673f3b8dae7d5e46fe30ec67549`. The immutable containing commit is the
new candidate; its full SHA and exact-commit archive results are recorded in the
Paperclip AGE-7 resubmission. Reports made before commit identify the prior HEAD
and dirty state honestly; input SHA-256 values bind the actual tested bytes.

## Requested changes and implementation

Review Coordinator comment `50de0ebb-608e-4151-acab-a8d85db97ef9`, aggregate
AGE-18 and independent AGE-19/AGE-20 requested both changes.

- F1: `tools/check_abi.py` grants owner-write permission only to its disposable
  copied drift header. Read-only source files and directories remain unchanged.
  If the source root lacks `.git`, the report explicitly records
  `source_identity.kind=metadata_absent`, null HEAD/branch and the provenance
  limitation. No Git commands run against archives or enclosing repositories.
  Checkout reports include HEAD, branch and dirty state. Source hashes remain
  mandatory evidence; reviewers must bind them to their own verified export SHA.
  Subprocess launch failures record a failed row with null exit code; fixture
  setup exceptions record their traceback and still produce the JSON report.
  Expected exit 1 alone cannot satisfy a negative fixture: drift must report
  `Stale ABI artifacts: include/calculator_abi_v1.h`, signature mismatch must
  report Rust `error[E0308]: mismatched types`.
- F2: `nm -u` must return success AND contain the exact undefined
  `_calc_evaluate_v1` token on Darwin or `calc_evaluate_v1` on Linux. Bare Apple
  and `U name` GNU/LLVM output forms are recognized. Substrings, mangled names,
  definitions and empty output do not satisfy the assertion. Real compiled
  unrelated and C++-linkage fixtures exercise negative symbol checks. Native
  host compiler defaults are used; cross-target verification is not provided.
  This inspects a consumer reference only, not producer exports or link/run.

No ABI declarations, generation rules, examples, numerical semantics or component
boundaries changed. No migration/version bump is needed. Historical evidence is
preserved with supersession notes.

## Actual author verification before commit

- `python3 tools/check_abi.py --report docs/architecture/evidence/review-remediation.json`
  exited 0 on macOS arm64. [Structured report](review-remediation.json) contains
  command/cwd/exit/stdout/stderr, diagnostic and symbol validation, tool versions,
  dirty source identity and all nine input SHA-256 digests.
- `PYTHONDONTWRITEBYTECODE=1 python3 tools/test_check_abi.py` exited 0, seven tests
  passed. [Exact log](checker-regressions.txt). Tests invoke the real checker in
  disposable copies: read-only archive preserves every source byte and mode;
  unrelated and mangled consumer references each fail despite nm exit 0;
  missing fixture input still writes a failing report; unrelated exit 1 does
  not count as drift rejection; missing executable is a failure; Darwin/Linux
  spelling parser cases pass. Linux parser fixtures are not Linux execution.
- `git diff --check` exited 0. Exact-commit source manifest and read-only archive
  invocation are performed after commit and returned in the Paperclip handoff.

## Independent reproduction

Reviewers use their own output directory and disposable source export; never edit
another author's worktree. Set `candidate` to the full SHA from the resubmission,
then run from a clone containing that immutable object:

```sh
review_dir=$(mktemp -d /tmp/age7-review.XXXXXX)
mkdir "$review_dir/source"
git archive --format=tar "$candidate" > "$review_dir/source.tar"
tar -xf "$review_dir/source.tar" -C "$review_dir/source"
chmod -R a-w "$review_dir/source"
(cd "$review_dir/source" && python3 tools/generate_abi.py --check)
(cd "$review_dir/source" && python3 tools/check_abi.py --report "$review_dir/abi.json")
(cd "$review_dir/source" && PYTHONDONTWRITEBYTECODE=1 python3 tools/test_check_abi.py)
```

All three Python invocations must exit 0. The checker JSON must have `passed=true`,
all result rows passed and explicit metadata absence. Compare its input digests
with files from `git show "$candidate:<path>"`; a supplied SHA string alone is
not verification. Compare source file digests and permission modes before/after
execution. Place outputs outside the read-only source. Keep exact export SHA,
archive digest, command results and reports in your task evidence.

## Remaining work and next accountable role

Review Coordinator `070eb2bb-d921-431a-8a07-b944e1b86a86` through existing AGE-18
must obtain renewed Domain AGE-19 and Quality AGE-20 assessments of the NEW SHA,
using their own disposable read-only exports and separate outputs. This author
verification is not independent approval. Existing architecture/contract and
integration packets remain the review scope in addition to the tooling fixes.

PM AGE-11 still owns compound-expression repeat descriptor selection. No new PM
answer was found; corrected 56/80/98 outcomes remain proposals, not accepted
product policy. No ABI change is inferred. Final contract acceptance remains
pending that decision and independent current-SHA review.

Real producer/export/link/run, runtime numeric/display/transport/transaction,
panic/concurrency/sanitizer, Linux, macOS minimum-version, Qt/GUI and packaging
checks remain unexecuted here. Their owners and exact future acceptance are in
[integration-packets.md](../integration-packets.md). GitHub publication, PR/head,
CI and formal review-App checks remain undelivered; no personal-token fallback,
push, merge or release is claimed. Owner M1 acceptance still gates M2.

## Candidate 4 supersession notice

This record and its original JSON remain historical evidence. Active spec selection
and the former pending compound-repeat question are superseded by
[repeat-policy-reconciliation.md](repeat-policy-reconciliation.md), pinning spec
`752d0961c8aacb2e79addeb6d1e26a7ea820d734`. Prior adverse verdicts and F1/F2 closure
are preserved in their assessed scope; none is an approval of the new pair.
