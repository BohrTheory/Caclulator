# Compatibility and migration procedure

ABI 1.0 is the first public contract candidate; there is no existing calculator
implementation/data to migrate from base `3ef202581d4e784d0f9da508767577888a4fc272`.
This is not permission to overwrite uncommitted work in other task worktrees.

Candidate 2 reconciliation: PM spec `c5ff879a8df5a673f3b8dae7d5e46fe30ec67549`
confirms arithmetic/formatting and requires retained binary64 continuation/repeat.
Compared with candidate 1 `5fd95cfad404d85b72c6a9d4663b1d607aec5e6d`, the JSON,
generated C/Rust declarations, symbols and ABI version remain byte-identical.
No regeneration or consumer signature migration is needed. Consumers must implement
the now-required lossless transport; new source integration combines this contract
commit with the PM spec commit in its own task worktree and reruns integration
checks. Neither candidate has an independent approval in this record. Existing
verification is retained, and this amended head needs a fresh independent verdict.

The symbol `calc_evaluate_v1`, its argument order/types, constants, buffer/lifetime
rules and established error meanings remain fixed across v1 minor revisions.
Unknown status values are generic errors. Additive new symbols and new status values
require a minor version increment and tests against old consumers. Existing values
are never renumbered. Breaking signatures or ownership rules require a new major
and new symbol (e.g. `calc_evaluate_v2`); keep v1 concurrently during a documented
transition or explicitly require all consumers to rebuild together. Never silently
change a v1 function under the same symbol.

Grammar, precedence, formatting and numeric tolerances also have compatibility
impact even when the C layout is unchanged. The ABI version is not a product feature
query. M2 UI and core ship as one matched bundle, with product/spec revision recorded
in its manifest; an M1 core is not interchangeable with an M2 UI merely because both
report ABI major 1. M2 adds operations through the registry; existing M1 expressions
retain their result/error/display behavior unless PM explicitly revises acceptance.
Runtime plugin compatibility is out of scope.

For each change:

1. Tech Lead records the problem and impacted signatures, producers, consumers,
   numeric cases, package targets, tests and documents in a new/amending ADR.
2. PM agrees any semantic change; Director resolves technical disputes. Substantive
   product scope changes go through the existing owner decision route.
3. Edit canonical JSON and behavioral contract together, regenerate artifacts and
   run `python3 tools/generate_abi.py --check`. Do not patch a generated copy alone.
4. Generate a compatibility matrix: old consumer/new library, new consumer/new
   library, and new consumer/old library version rejection or fallback. For static
   linking these are separate executables, each built against its actual header.
5. Producer compiles generated signature checks; Integration QA links actual C and
   C++ consumers against each applicable target archive and checks symbols/errors,
   buffer bounds, concurrency and complete regression. Record exact target/SDK and
   native link flags. A header syntax check does not satisfy these rows.
6. Independent reviewers assess the exact head SHA and evidence. A subsequent
   commit needs renewed review. Lead carries explicit readiness gates into tasks.
7. Update all consuming tasks to immutable contract/spec SHAs. DevOps builds matching
   packages and manifests. Keep rollback artifacts from the prior accepted package;
   rollback changes the whole matched app/core bundle, never only one member.

No push, PR, merge, release, App permission or credential change is performed by
this architecture task. Those operations retain the verified identity, protected
branch and independent review gates from AGE-3.
