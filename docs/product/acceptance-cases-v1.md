# Calculator v1 acceptance cases and QA traceability

**Companion specification:** [`calculator-spec-v1.md`](calculator-spec-v1.md)
**Status:** Product/numerical contract aligned with Tech Lead and AGE-22 arbitration; revised after independent review, pending fresh review.
**Oracle rule:** Numerical value assertions and displayed-string assertions are separate. Unit and integration scenarios can run without a GUI; external GUI scenarios require the application and run on macOS arm64 and Linux x86_64 (Ubuntu 24.04 baseline).

## Test layers

- **Unit (U):** parser/evaluator, editing-state reducer, formatter, and error classification with independently calculable expected values.
- **Integration (I):** UI actions/keyboard events wired through application state to result/error output; includes state continuation and recovery.
- **External GUI (G):** visible controls, focus, keyboard use, and displayed result/error in the built application on macOS arm64 and Linux x86_64 (Ubuntu 24.04 baseline). macOS requires Appium Mac2, full Xcode, Accessibility permission, and a GUI session; Linux requires AT-SPI/PyGObject in Xvfb/Openbox/D-Bus and accessible Qt.

All M1 GUI scenarios are required on both platforms. A failure in either is a release failure. Unit/integration scenarios are platform-independent unless the implementation uses platform-specific parsing or formatting.

## M1 requirement-to-QA matrix

| Req. | Acceptance scenario / independent oracle | U | I | G (each platform) |
|---|---|---|---|---|
| R-ARITH | `2 + 3 =` -> numeric `5`, display `5`. | U-01 | I-01 | G-01 |
| R-ARITH | `7 - 10 =` -> numeric `-3`, display `-3`. | U-02 | I-02 | G-02 |
| R-ARITH | `6 * 7 =` -> numeric `42`, display `42`. | U-03 | I-03 | G-03 |
| R-ARITH | `8 / 2 =` -> numeric `4`, display `4`. | U-04 | I-04 | G-04 |
| R-DECIMAL | Enter `1.25 + 2.5 =` -> `3.75`; duplicate decimal/exponent markers are rejected without changing value. `.5 + 5. =` -> `5.5`; `1.25e-3 * 1000 =` -> numeric `1.25`, display `1.25`. | U-05 | I-05 | G-05 |
| R-NEGATIVE | `-3 + 1 =` -> `-2`; `2 * -3 =` -> `-6`; `(-3) * 2 =` -> `-6`. | U-06 | I-06 | G-06 |
| R-GRAMMAR | `2 + 3 * 4 =` -> `14`; `(2 + 3) * 4 =` -> `20`; `8 / 4 / 2 =` -> `1` (left associative). | U-07 | I-07 | G-07 |
| R-GRAMMAR | Incomplete `2 + =`, unmatched `(` / `)`, empty `()`, implicit multiply `2(3)`, and invalid sequence `2*/3` show error on Equals; app remains responsive. | U-08 | I-08 | G-08 |
| R-CLEAR | Enter partial `123.4`, Clear -> empty ready display; then `2+3=` -> `5`. | U-09 | I-09 | G-09 |
| R-BACKSPACE | Enter `123`, Backspace -> `12`; enter decimal/backspace cases preserve a valid editable state; Backspace after result makes result editable. | U-10 | I-10 | G-10 |
| R-SIGN | Toggle sign of active `3` -> `-3`, toggle again -> `3`; in `2 * 3`, toggle active entry gives `2 * -3` -> `-6`. | U-11 | I-11 | G-11 |
| R-EQUALS | Apply the lexical descriptor rule and exact compound-expression oracles in U-12. Include edited-expression invalidation (`2+3*5`, Backspace, `4`, Equals, Equals -> `14`, `56`), wrapper/unary/exponent signs, and no-descriptor cases. | U-12 | I-12 | G-12 |
| R-CONTINUE | `5 * 2 =` -> `10`, then `+ 3 =` -> `13`. After a result, digit `7` starts a new expression; binary operator continues from result. | U-13 | I-13 | G-13 |
| R-DIV0 | `8 / 0 =` shows divide-by-zero error, no numeric result; Clear then `8/2=` -> `4`; fresh digit after error starts a new expression and evaluates. | U-14 | I-14 | G-14 |
| R-KEYBOARD | While window focused, keyboard `2+3`, Enter -> `5`; `*4=` -> `20`; Backspace and Escape map to Backspace and Clear. | U-15 | I-15 | G-15 |
| R-FOCUS | Tab/Shift+Tab reaches all interactive controls in stable order; current focus is visible; Space/Enter activates focused control. Typing does not trigger calculator actions while app is unfocused. | — | I-16 | G-16 |
| R-PRECISION | `0.1 + 0.2 =`: `abs(actual - 0.3) <= max(1e-15, 1e-12 * 0.3)`; displayed result is exactly `0.3`. | U-17 | I-17 | G-17 |
| R-FORMAT | Integral results omit `.0`, trailing fractional zeroes are removed, `-0` displays `0`, decimal separator is `.` on both platforms. | U-18 | I-18 | G-18 |
| R-RANGE | `1e-6` and `1e14` render in fixed format; `1e-7` and `1e15` in compact scientific format. Overflow returns `RANGE`; underflow follows binary64 subnormal/zero behavior. | U-19 | I-19 | G-19 |
| R-RECOVER | Malformed expression error followed by Clear and valid arithmetic works; backspace correction can repair a retained invalid expression if the UI retains it. | U-20 | I-20 | G-20 |
| R-PERCENT | Percent behavior is excluded from M1; no percent control, key binding, or implicit percent calculation is exposed. | U-21 | I-21 | G-21 |

## Scenario definitions

Each row above maps to same-numbered U/I/G scenario. G scenarios must be executed once on each supported platform; the table's G-N identifier is a platform pair, not a single run.

### Unit scenarios

| ID | Setup/action | Expected result |
|---|---|---|
| U-01–U-04 | Evaluate the basic arithmetic expressions from R-ARITH. | Exact expected values and strings in the matrix. |
| U-05 | Parse/format `.5`, `5.`, `1.25e-3`, and `1.25 + 2.5`; attempt duplicate decimal/exponent markers. | Values `0.5`, `5`, `0.00125`, `3.75`; `1.25e-3 * 1000` gives exact `1.25`; token remains valid after rejected duplicate. |
| U-06 | Evaluate leading, post-operator, and parenthesized negative operands. | `-2`, `-6`, `-6`. |
| U-07 | Evaluate precedence, parentheses, and division associativity. | `14`, `20`, `1`. |
| U-08 | Parse/evaluate malformed expressions. | Classified syntax/incomplete-expression error; no crash or numeric result. |
| U-09 | Apply Clear to partial, result, and error states. | Empty ready state. |
| U-10 | Apply backspace to integer, decimal, sign, and result entries. | Last editable character removed; state remains valid/editable. |
| U-11 | Toggle sign on current entry across positive/negative values. | Only active operand sign changes. |
| U-12 | For each expression, after successful full evaluation remove whitespace and repeatedly strip only balanced parenthesis pairs that enclose the whole expression. Select the rightmost binary `+ - * /` at depth zero; unary signs and exponent signs are excluded; RHS is the complete suffix. Rust validates/evaluates the full expression and selected RHS. Repeat Equals by evaluating `current op retained-RHS` through Rust, retaining the same descriptor and binary64 values; clear/edit invalidates it. | `2+3*4`: descriptor `* 4`, results `14, 56, 224`; `(2+3)*4`: `* 4`, `20, 80, 320`; `2*(3+4)`: `* 7`, `14, 98, 686`; `2+3`: `+ 3`, `5, 8, 11`; `8/4/2`: `/ 2`, `1, 0.5, 0.25`; `2*-3`: `* -3`, `-6, 18, -54`; `2+1e-3`: `+ 1e-3`, displays `2.001, 2.002, 2.003` with D-03 numeric tolerance; `((2+3))`: `+ 3`, `5, 8, 11`; `-(2+3)` and `7`: no descriptor, result unchanged across Equals. After entering `2+3*5`, Backspace, `4`, Equals, Equals: `14`, `56` (current text wins over stale key metadata). Retained values use ABI round-trip serialization (max_digits10, preserve signed zero), never display strings. |
| U-13 | Apply operator continuation and digit-after-result transitions. | Continuation `13`; digit begins a new expression. |
| U-14 | Evaluate zero denominator and subsequent reset/fresh-entry state. | Divide-by-zero error; clean recovery. |
| U-15 | Map key symbols to calculator actions. | Expected action enumeration, no platform-specific ambiguity. |
| U-17 | Evaluate `0.1 + 0.2`. | `abs(actual - 0.3) <= max(1e-15, 1e-12 * 0.3)`; formatter output exactly `0.3`. |
| U-18 | Format integer, trailing zeros, negative zero, decimal separator. | 15 significant digits, ties-to-even, `.` separator, trailing fractional zeroes removed, `-0` as `0`. |
| U-19 | Format range boundaries and overflow/underflow values. | `1e-6` and `1e14` fixed; `1e-7` and `1e15` scientific; overflow -> `RANGE`; underflow follows binary64. |
| U-20 | Recover state from syntax error by Clear or editing. | Valid subsequent expression evaluates. |
| U-21 | Inspect supported actions and key mapping. | No percent control or percent key action; percent syntax is rejected as invalid input. |

### Integration and external GUI scenarios

For I-01–I-15 and I-17–I-21, send the equivalent UI/action sequence through the application controller and assert final expression state, error/result state, numeric value, and display string as applicable. I-12 executes every U-12 expression and repeat sequence through the controller, including current-expression recomputation after edit, descriptor invalidation on edit/clear, and no-op Equals when no descriptor exists. I-16 asserts focus order and key routing through the UI framework.

For G-01–G-21, use the visible application on each supported platform. The operator records platform/OS version, application build SHA, exact input sequence, observed display, and pass/fail. G-12 executes every U-12 expression and repeat sequence visibly, including the edited-expression case and operationless repeat. G-16 additionally records the focus traversal and verifies that an unfocused application receives no calculator shortcut actions. Screenshot or equivalent captured evidence should accompany failures and be retained with the run record. GUI prerequisites that are unavailable are recorded as blocked; they are not passed or treated as blockers to unit/integration work.

## Scientific extension cases (planned; gated)

These define the proposed scientific acceptance surface only. Planning and case refinement may proceed, but implementation requires both recorded Project Owner acceptance of the M1 first release and a revision-pinned accepted scientific v1 contract compatible with the canonical ABI. The contract must define operations, domains, angle behavior, numerical accuracy/tolerance, errors, and display expectations. Operation-list approval alone is insufficient. This preserves AGE-15 arbitration Decision 1 (comment `3fd83498-8c70-4658-85b1-3fbe0ea74673`). These are not M1 release tests.

| Req. | Case | Numeric oracle | U/I/G mapping after acceptance |
|---|---|---|---|
| S-SQRT | `sqrt(9)` | `3` (absolute tolerance `1e-12`) | U-S01 / I-S01 / G-S01 on both platforms |
| S-POWER | `2^10` | `1024` (`1e-12`) | U-S02 / I-S02 / G-S02 |
| S-LOG | `log10(1000)` | `3` (`1e-12`) | U-S03 / I-S03 / G-S03 |
| S-LN | `ln(e)` | `1` (`1e-12`); `ln(0)` domain error | U-S04 / I-S04 / G-S04 |
| S-SIN | `sin(30 DEG)` | `0.5` (`1e-10`) | U-S05 / I-S05 / G-S05 |
| S-SIN-RAD | `sin(pi/2 RAD)` | `1` (`1e-10`); confirm `pi` availability | U-S06 / I-S06 / G-S06 |
| S-ASIN | `asin(0.5 DEG)` | `30` degrees (`1e-10`) | U-S07 / I-S07 / G-S07 |
| S-DOMAIN | `sqrt(-1)` | Domain error, no NaN/complex display | U-S08 / I-S08 / G-S08 |
| S-DOMAIN | `asin(2)` | Domain error | U-S09 / I-S09 / G-S09 |

Scientific tests use an approved numerical tolerance independently from displayed precision. Before cases are promoted to release gates, the accepted scientific contract must also settle mode default/persistence, `pi` entry, exponent/unary-sign precedence, function entry grammar, and error recovery.

## Evidence and release record

The QA run record must include spec revision or commit SHA, implementation build SHA, supported platform/OS version, test-layer ID, exact command/action, actual result, expected result, pass/fail, and retained evidence for failures. Do not report planned or unavailable GUI checks as passed. Platform names and GUI prerequisites above come from the accepted calculator profile; profile inclusion does not prove either platform's driver is currently ready.
