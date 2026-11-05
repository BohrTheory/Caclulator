# Calculator CI and recovery

This workflow builds immutable producer/Experience/QA inputs separately. Their full
SHAs live in the workflow and runner; later component revisions require an explicit
candidate decision and renewed review. `CANDIDATE_SHA` names the workflow branch
head, while the three source pins identify the application actually built. These
are separate identities. Checkouts must be published before hosted execution.
An authenticated source-publication manifest with privacy/size dispositions is
still required; these reproduction pins alone do not establish hosted fetchability.
Architecture `67100ba0ce1d7aaf4fcafadb9101465b78d19829` and specification
`752d0961c8aacb2e79addeb6d1e26a7ea820d734` remain the accepted contract references.

Standard runners: Ubuntu 24.04 x86_64 and macOS 15 arm64. Qt 6.8.3, Rust 1.89.0,
xmake 2.9.9, Python 3.12.10 and action commits are pinned. Runner image, compiler,
SDK, native link flags and installed Linux package versions are retained. Hosted
images and apt dependencies still roll; logs record actual versions. This does
not claim a hermetic or bit-reproducible hosted image.
Runner labels were checked against [GitHub's runner reference](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).
Hosted execution fails on an unexpected OS, architecture, or source checkout SHA.
Local source exports rely on verified blob manifests and are not CI evidence.

Producer build/unit/package steps run before Experience/QA fetch, preserving
independent core evidence when application inputs are missing. A failed input
checkout still fails the required platform job; core success cannot pass it.

Each platform uploads command logs/exits, native archive provenance, application
package, file hashes and SHA256SUMS. Failed jobs retain diagnostics and remain
failed. Download the artifact from its job summary, verify its Actions digest,
then verify the application tarball with SHA256SUMS. Linux requires Ubuntu 24.04;
macOS bundles are ad-hoc signed, without notarization or public-release approval.

Normal, ASan/UBSan and TSan instrument the C/C++ test clients, not Rust/std.
Qt widget tests use injected recording evaluators; these are framework units,
not external GUI conformance. macOS 14 runtime, external GUI on both platforms,
test-only external panic fixture integration, alternate archives, independent
review, formal GitHub approval and owner release acceptance remain separate gates.

Required check names are `build-test-ubuntu-24.04` and `build-test-macos-15`.
Before merging, the authorized gateway must re-read current PR head, both successful
checks bound to that SHA, all assigned independent verdicts, formal APPROVE from
Review identity, expected Review App check, resolved blocking discussions and no
unresolved owner decision. Missing/stale evidence fails closed. This workflow
does not enforce branch protection or provide the merge gateway.

Recovery: retain failed-run logs, fix environment/workflow errors, publish a new
candidate and repeat checks/review for its SHA. Route product defects to their
authors. Never remove a required failing check. Roll back a merged workflow by a
reviewed revert; restore only a previously owner-accepted, hash-verified package.
Never reset an author worktree, change branch protection, or treat artifact
retention as permanent release storage. No public release is performed here.
