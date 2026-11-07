# M1 Experience component

Accepted architecture: `67100ba0ce1d7aaf4fcafadb9101465b78d19829`.
Accepted product specification (both files): `752d0961c8aacb2e79addeb6d1e26a7ea820d734`.
The later native acceptance decisions recorded in the assignment supersede historical
pending-review wording in those immutable files. ABI declarations are unchanged.

`calculator_client` is the C++17 editor/controller and C ABI adapter. `calculator`
is a Qt 6 Widgets app linked only to a supplied **real** Rust archive. No mock app
or C++ arithmetic/precedence/formatting engine is provided. Lexical repeat selection
runs only after the full expression succeeds in Rust; Rust also evaluates the RHS.
The controller commits full result/display and descriptor atomically. Automatic
continuation/repeat use parenthesized max_digits10 binary64 transport. Backspace
and sign after a result deliberately edit its displayed numeric entry.

Failed expressions stay visible for correction. Result text is cleared on error;
errors have stable translated messages and byte offsets converted to UTF-16 positions.
An RHS-stage failure shows a generic message without a location because its byte
offset belongs to the compact RHS, rather than to the displayed full expression.
Clear/fresh digit/decimal/backspace recover; edits immediately discard descriptors.
Oversized input is rejected with a limit error, keeping the previous bounded text.

Controls have accessible names, native widget roles, an alert for error changes,
result value notifications, a visible focus outline and a stable row-major Tab order.
The window handles supported keys only in its active window, including when a button
has focus. Typed expression characters return focus to the window. Enter evaluates
when the window has focus; Enter/Space activates a focused button. Tab/Shift+Tab
retains native focus navigation. Initial focus belongs to the window. Percent has
no control/action. Parenthesis after a result starts a fresh entry; sign edits the
result entry. These choices avoid appending to a completed expression ambiguously.

## Local source/unit commands

No Qt or Rust is needed for the scripted client units:

```sh
export XMAKE_GLOBALDIR="$PWD/build/xmake-global"
xmake f -m debug --app=n --qt_units=n -y
xmake build -v client_units
xmake run client_units
xmake test -v client_units
python3 tools/generate_abi.py --check
```

`python3 ui/tests/run_checks.py` captures these checks, compiler/tool observations,
source digests and exact argv/cwd/exits/stdout/stderr in `ui/evidence/verification.json`.
These are author units using **scripted recording evaluator/FFI doubles**. They
assert supplied independent oracles, call payloads, counts, state and error handling;
they do not establish Rust arithmetic or formatting. Transport bit checks use the
host `strtod` as a separate representation oracle, not the absent Rust producer.
No required Qt/GUI/platform check is counted as passed by this runner.

With a qualified Qt SDK (future commands; currently blocked):

```sh
xmake f -m debug --app=n --qt_units=y --qt=/absolute/qualified/qt -y
xmake build -v widget_units
xmake run widget_units
```

Widget units use Qt Test event injection and a recording evaluator. They cover
keyboard/focus/name/error wiring and are not external GUI or real integration tests.
They have not compiled/run in this environment; `qt-build-attempt.json` records the
failed SDK configuration. A later `xmake build widget_units` exit 0 under the prior
non-Qt configuration was a no-op, **not** a compiled widget target.

## Real app / boundary handoff

Integration QA/DevOps must first receive an immutable Domain producer commit,
archive SHA-256, matching native target and recorded `rustc --print native-static-libs`
link requirements. Then build with qualified Qt and matching target/deployment flags:

```sh
xmake f -m debug --app=y --qt_units=n --qt=/absolute/qualified/qt \
  --rust_archive=/absolute/immutable/libcalculator_core.a \
  --rust_native_links=<comma-separated-producer-native-libraries> -y
xmake build -v calculator
```

These are **planned**, not executed app commands. xmake does not build Cargo or pick
another task's archive implicitly. An absent archive is an explicit app-target error.
DevOps owns qualified platform flags, Qt/tool versions and packaging; coordinate
root xmake changes with Experience. The target names are now concrete coordination
inputs. Do not copy macOS link libraries to Linux.

Remaining acceptance: widget compilation/runtime, real controller/adapter/Rust
I-01..21 including every I-12 storage/lifecycle/precision/bit/call row; independent
Experience/Quality review; external G-01..21 on macOS arm64 and Ubuntu 24.04 x86_64;
minimum macOS 14 execution; packages/hashes/CI and formal GitHub review. Actual Qt
SDK/GUI/Linux runners and an immutable producer are unavailable to this run. No
PR/push/merge/release is authorized; the verified App gateway is absent. No PR URL
exists. This source/unit result does not establish a release or independent verdict.
M2 retains both owner M1 release acceptance and a compatible accepted pinned
scientific contract as prerequisites.
