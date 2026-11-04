# Calculator architecture contract — AGE-7

Revision: ABI 1.0 / architecture candidate 3, 2026-11-05.
State: numerical peer agreement incorporated; independent current-SHA review pending.
Dependent implementation requires that review gate.

Authority: accepted AGE-3 (`dd2750ff-6eb9-4f64-a1d3-270f9a7a511c`),
AGE-7 (`66531c23-4c1f-4c00-8edb-d28bc611539b`), calculator project profile,
and company task-handoff policy. Project `50271359-4516-466c-8325-0d234bd5ddb0`,
repository `https://github.com/BohrTheory/Caclulator`.

Owner: Technical Lead, agent `a8ab3e67-2131-425e-b539-7273ba1bc46b`.
Base commit: `3ef202581d4e784d0f9da508767577888a4fc272`.
Author branch: `codex/AGE-7-a8ab3e67-2131-425e-b539-7273ba1bc46b` in its assigned isolated worktree.
Initial run: `0d84077a-f6d4-49b0-a433-1c0824778f9c`.
Reconciliation run: `b432833c-7d6e-4b3d-86c9-30fe99785657`.
Previous candidate: `e5aea8fc091b2b98ce90a3abd21912f72cc090f6` (changes requested).
Remediation run: `b3daf5ef-fa1b-4de9-b75f-843e0b780217`.
PM spec/acceptance commit: `c5ff879a8df5a673f3b8dae7d5e46fe30ec67549`,
`docs/product/{calculator-spec-v1,acceptance-cases-v1}.md` (on the PM branch;
not copied into this architecture worktree).

- [ADR: boundaries, canonical source and extension design](adr-0001-c-abi-and-modules.md)
- [Normative ABI semantics, errors and examples](c-abi-v1.md)
- [Dependency map and executable handoff packets](integration-packets.md)
- [Compatibility and migration](compatibility.md)
- [F1/F2 remediation and archive reproduction](evidence/review-remediation.md)
- [Actual verification and remaining checks](evidence/verification.md)
- [Canonical machine-readable declarations](../../contracts/calculator-abi-v1.json)
- [Generated public header](../../include/calculator_abi_v1.h)

The JSON is the single source of declarations and constant values; c-abi-v1.md
is the normative behavioral contract. Generated C and Rust files are never edited
independently. Product requirements in AGE-6 are authoritative for numerical
and interaction behavior. PM comment `ede93b3d-eb06-443d-a465-842f070b7932`
supplied the immutable spec revision above and resolves the numerical proposal
dependency. Peer agreement is not independent review or owner release acceptance.
No calculator implementation or CI workflow is included in this task.

Historical recovery notes about an uninvokable agent are preserved in Paperclip.
This run supersedes their implication that drafting must wait for manual staffing;
it does not claim that all environments or remote delivery gates are repaired.

Next gate: independent current-SHA architecture and generator/header review through
Director routing AGE-16 (`46cbeea7-a6ec-4f47-8004-dc762eb3c507`), then reference
both accepted spec and contract SHAs in component tasks. A queued handoff is not a verdict.
See [reconciliation record](evidence/pm-reconciliation.md) for exact changes,
verification reuse, and the remaining compound-expression repeat policy question.
