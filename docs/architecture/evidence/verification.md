# AGE-7 verification evidence

Run `0d84077a-f6d4-49b0-a433-1c0824778f9c`, author agent
`a8ab3e67-2131-425e-b539-7273ba1bc46b`, task AGE-7, 2026-11-05 local date.
Full commands, stdout/stderr, exit codes, platform, tool versions, source branch/base
and SHA-256 input digests: [verification.json](verification.json).
The report records the pre-commit base; the immutable containing commit binds all
tested inputs. The Paperclip handoff supplies that full resulting commit SHA.

Reproduce from repository root:

```sh
python3 tools/generate_abi.py --check
python3 tools/check_abi.py --report /tmp/age7-verification.json
git diff --check
```

Actual checks on macOS arm64:

| Check | Outcome |
|---|---|
| Generated C header/Rust declarations match canonical JSON | Passed |
| C11 header example, `-Wall -Wextra -Werror -pedantic`, compile to object | Passed |
| C++17 header example, same warnings, compile to object | Passed |
| Rust generated constant/function-pointer types, edition 2024 | Passed |
| C++ undefined symbol inspection (`nm -u`) | Unmangled `_calc_evaluate_v1` reference observed; not linked |
| Deliberate generated-header drift in disposable copy | Correctly rejected with exit 1 |
| Declaration-only fixture with matching producer signatures | Compiled |
| Declaration-only fixture changing `double*` equivalent to `f32*` | Correctly rejected with Rust type mismatch / exit 1 |

Temporary object/rlib files and negative fixtures are deleted by the verification
script after recording output. These are declaration checks; neither fixture is a
calculator implementation. The Rust signature-check mechanism has been exercised,
but it has not yet been included in an actual producer (none exists in this task).
The negative checks' exit 1 is expected evidence, not an unresolved build failure.

Git execution: the first `git add contracts docs/architecture examples/abi include
tools` failed with `Operation not permitted` creating the assigned worktree's
`index.lock` under the canonical `.git/worktrees/` directory (exit 128). The same
bounded stage/commit operation succeeded via task-authorized sandbox escalation.
The inherited local author identity was replaced with explicit Technical Lead agent
metadata before handoff; no global Git configuration or remote identity was changed.

Not executed:

- Real Rust staticlib export/link/run, arithmetic/parser/formatter tests, panic
  injection, concurrent calls and sanitizer tests: Domain Developer/Integration QA
  must supply the real producer after contract/spec acceptance.
- Linux C/C++ compile or linked execution: no Linux runner used by this task.
- Qt app build, both-platform external GUI tests and packaging: no app implemented;
  their own environment dependencies remain with DevOps/System QA.
- Independent current-SHA review: delegated through a neutral routing owner; a
  routing receipt is not an approval.
- GitHub push/PR/merge/release: not attempted; intended integration and branch/review
  controls remain required. No personal credential fallback.

Historical candidate-1 disposition: numerical agreement was pending at the time of
this run. Superseded by PM comment `ede93b3d-eb06-443d-a465-842f070b7932` and spec
commit `c5ff879a8df5a673f3b8dae7d5e46fe30ec67549`. Numerical agreement is now
incorporated; independent current-SHA contract review still remains pending. See
[reconciliation evidence](pm-reconciliation.md); the original execution report above
is preserved and is not relabelled as a new run.


Candidate-2 review supersession: independent AGE-19/AGE-20 assessments requested
changes at `e5aea8fc091b2b98ce90a3abd21912f72cc090f6`. The original checker did
not support read-only Git archives and did not assert the observed nm symbol.
The historical results above remain evidence for their original scope, not proof
of archive reproducibility or an effective symbol assertion. Candidate-3 fixes,
new execution evidence and negative regressions are in
[review-remediation.md](review-remediation.md). New-SHA independent review is pending.

## Candidate 4 supersession notice

This record and its original JSON remain historical evidence. Active spec selection
and the former pending compound-repeat question are superseded by
[repeat-policy-reconciliation.md](repeat-policy-reconciliation.md), pinning spec
`752d0961c8aacb2e79addeb6d1e26a7ea820d734`. Prior adverse verdicts and F1/F2 closure
are preserved in their assessed scope; none is an approval of the new pair.
