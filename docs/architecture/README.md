# Calculator architecture contract — AGE-7

Revision: ABI 1.0 / architecture candidate 1, 2026-11-05.
State: ready for peer agreement and independent review, not yet approved for dependent implementation.

Authority: accepted AGE-3 (`dd2750ff-6eb9-4f64-a1d3-270f9a7a511c`),
AGE-7 (`66531c23-4c1f-4c00-8edb-d28bc611539b`), calculator project profile,
and company task-handoff policy. Project `50271359-4516-466c-8325-0d234bd5ddb0`,
repository `https://github.com/BohrTheory/Caclulator`.

Owner: Technical Lead, agent `a8ab3e67-2131-425e-b539-7273ba1bc46b`.
Base commit: `3ef202581d4e784d0f9da508767577888a4fc272`.
Author branch: `codex/AGE-7-a8ab3e67-2131-425e-b539-7273ba1bc46b` in its assigned isolated worktree.
Run: `0d84077a-f6d4-49b0-a433-1c0824778f9c`.

- [ADR: boundaries, canonical source and extension design](adr-0001-c-abi-and-modules.md)
- [Normative ABI semantics, errors and examples](c-abi-v1.md)
- [Dependency map and executable handoff packets](integration-packets.md)
- [Compatibility and migration](compatibility.md)
- [Actual verification and remaining checks](evidence/verification.md)
- [Canonical machine-readable declarations](../../contracts/calculator-abi-v1.json)
- [Generated public header](../../include/calculator_abi_v1.h)

The JSON is the single source of declarations and constant values; c-abi-v1.md
is the normative behavioral contract. Generated C and Rust files are never edited
independently. Product requirements in AGE-6 remain authoritative for numerical
and interaction behavior once peer-agreed; the numerical section here is an explicit
proposal until AGE-11 records PM agreement and an immutable AGE-6 revision.
No calculator implementation or CI workflow is included in this task.

Historical recovery notes about an uninvokable agent are preserved in Paperclip.
This run supersedes their implication that drafting must wait for manual staffing;
it does not claim that all environments or remote delivery gates are repaired.

Next gates: PM numerical agreement (AGE-11), independent current-SHA architecture
and generator/header review assigned through a neutral routing role, then reference
both accepted spec and contract SHAs in component tasks. A queued handoff is not a verdict.
