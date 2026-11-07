# Proposed PR: implement M1 Qt calculator client and boundary units

PR URL: unavailable. Publication is prohibited by this assignment; no verified
role-scoped GitHub App gateway/remote head/CI exists. This is a local review draft.

The calculator client now retains full binary64 values through continuation and
repeated Equals, delegates all arithmetic and display formatting to the C ABI,
and preserves failed expressions for correction. It adds Qt Widgets controls,
window-scoped keyboard routing, stable focus traversal, named accessible controls,
error presentation and xmake targets for client units, widget units and a real app.

Author validation: `python3 ui/tests/run_checks.py` builds/runs the C++ client units
through xmake and checks generated ABI declarations. Evaluator/FFI doubles are
explicitly scripted; host decimal conversion supplies only transport bit evidence.
The full U-12 sequence/lifecycle matrix and PM editor/status/display handoff fixtures
are covered. See `ui/evidence/verification.json` and individual command logs.

Qt SDK configuration fails here. Widget units and the app have not compiled/run;
no real Rust producer is linked. External GUI on both platforms, minimum OS,
package/CI checks and independent review remain open. Integration QA must consume
immutable matching Domain/Experience results before real boundary validation.
Review Coordinator selects independent Experience and Quality reviewers. This draft
requests no merge/release and contains no independent approval.
