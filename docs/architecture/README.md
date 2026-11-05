# Calculator architecture contract — AGE-7

Revision: ABI 1.0 / architecture candidate 4, 2026-11-05.
State: numerical agreement and AGE-22 repeated-equals policy incorporated;
independent review of this architecture/spec pair and separate spec acceptance pending.
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
Previous candidate: `eabbeb9581d157d573b81f7a8551a33826ecc79e`
(F3 changes requested against the current spec; F1/F2 closure preserved).
F3 amendment run: `07f75a6a-3dca-4d02-bfdc-773450523cec`.
Remediation run: `b3daf5ef-fa1b-4de9-b75f-843e0b780217`.
PM spec/acceptance commit: `752d0961c8aacb2e79addeb6d1e26a7ea820d734`,
`docs/product/{calculator-spec-v1,acceptance-cases-v1}.md` (on the PM branch;
not copied into this architecture worktree).

- [ADR: boundaries, canonical source and extension design](adr-0001-c-abi-and-modules.md)
- [Normative ABI semantics, errors and examples](c-abi-v1.md)
- [Dependency map and executable handoff packets](integration-packets.md)
- [Compatibility and migration](compatibility.md)
- [F3 spec alignment, verification and renewed-review handoff](evidence/repeat-policy-reconciliation.md)
- [F1/F2 remediation and archive reproduction](evidence/review-remediation.md)
- [Actual verification and remaining checks](evidence/verification.md)
- [Canonical machine-readable declarations](../../contracts/calculator-abi-v1.json)
- [Generated public header](../../include/calculator_abi_v1.h)

The JSON is the single source of declarations and constant values; c-abi-v1.md
is the normative behavioral contract. Generated C and Rust files are never edited
independently. Product requirements in AGE-6 are authoritative for numerical
and interaction behavior. The pinned PM revision incorporates AGE-22 decision
`073134b1-715a-44c9-8d42-574d99a853a7` in D-07/§4.2 and U-12/I-12/G-12,
resolving the former compound-repeat policy gap. The earlier numerical agreement
and old spec pin are preserved in historical evidence. Incorporation/arbitration
is not independent spec acceptance, architecture review or owner release acceptance.
No calculator implementation or CI workflow is included in this task.

Historical recovery notes about an uninvokable agent are preserved in Paperclip.
This run supersedes their implication that drafting must wait for manual staffing;
it does not claim that all environments or remote delivery gates are repaired.

Next gate: renewed exact-pair review through existing coordinator AGE-18, Domain
AGE-19 and Quality AGE-20, using native stage `ca2a0d26-a9b2-4b98-9007-56e842f3c67a`.
Then reference both accepted spec and contract SHAs in component tasks. A queued
handoff is not a verdict. See the F3 record above for current changes and evidence;
[prior reconciliation](evidence/pm-reconciliation.md) preserves historical context.
M2 implementation requires both recorded owner acceptance of the M1 first release
and a revision-pinned accepted scientific contract compatible with the ABI, covering
operations, domains, angles, numerical accuracy/tolerance, errors and display.
Planning may proceed; neither implementation gate is recorded as satisfied here.
