# Calculator product specification v1

**Status:** Product and numerical contract agreed for M1; revised after independent review, pending fresh review
**Scope:** AGE-6 M1 basic desktop calculator behavior and a separately gated scientific extension plan
**Source of scope:** AGE-6 assignment `f7171dcf-31e7-494b-9033-8dbc91af42b4`, accepted parent AGE-3 `dd2750ff-6eb9-4f64-a1d3-270f9a7a511c`, project profile `task-handoff/references/projects/calculator.md`, and numerical alignment task AGE-11 `3e6942f1-2885-4092-bde0-c905ecc77277`.
**Decision authority:** Product behavior follows the accepted owner scope; ordinary arithmetic and numerical defaults are team decisions aligned with Tech Lead's AGE-11 proposal, not new owner decisions.

## 1. User problem and outcome

People need a desktop calculator that accepts ordinary arithmetic predictably, shows a readable result, and lets them correct or recover from common input mistakes without restarting the application. M1 succeeds when users can enter basic and parenthesized expressions, understand which operations will be applied, correct the current entry, and recover from invalid expressions and division by zero.

## 2. Product boundary

### M1: basic calculator

- Four arithmetic operators, decimal values, signed values, parentheses, and standard precedence.
- Expression editing: clear, backspace, sign toggle, evaluate, repeated equals, and continuing with an operator after a result.
- Keyboard input, visible focus behavior, errors, and recovery.
- Deterministic display rules and numeric accuracy expectations.
- Same behavior on macOS arm64 and Linux x86_64 (Ubuntu 24.04 baseline), the supported platforms named in the accepted project profile.

### Planned scientific extension (separately gated)

Define acceptance coverage for square root, power, common logarithms, natural logarithms, sine, inverse sine, and degree/radian angle modes. Scientific implementation is out of M1 and may begin only after owner acceptance of the scientific v1 scope. No factorial, history, graphs, memory registers, unit conversion, or other unapproved additions are included.

## 3. Decisions, proposals, assumptions, and gaps

| ID | Kind | Statement | Basis / follow-up |
|---|---|---|---|
| D-01 | Scope decision | M1 covers basic arithmetic plus parentheses, editing, keyboard/focus, errors, and precision/display. | AGE-6 assignment. |
| D-02 | Team product decision | Conventional precedence: parentheses, unary sign, multiplication/division, addition/subtraction; equal-precedence binary operations associate left to right. No sequential/immediate mode in M1. | AGE-6 recommends this model and explicitly says not to request an owner answer. |
| D-03 | Team technical decision | IEEE-754 binary64, round-to-nearest ties-even, finite results only. Either signed zero divisor is `DIVIDE_BY_ZERO`; underflow follows binary64 subnormal/zero behavior; overflow is `RANGE`. | Aligned with Tech Lead through AGE-11; numeric tolerance: `abs(actual-expected) <= max(1e-15, 1e-12*abs(expected))`; integer examples remain exact. |
| D-04 | Team technical decision | Rust owns locale-independent formatting: decimal point `.`, 15 significant digits, ties-to-even, strip fractional trailing zeroes, signed zero as `0`; lowercase `e` when rounded decimal exponent `< -6` or `>= 15`, with no plus sign or redundant exponent zeroes. | Aligned with Tech Lead through AGE-11. `0.1 + 0.2` has separate numeric tolerance and exact display oracle `0.3`. |
| D-05 | Team technical decision | ASCII decimal/scientific literals and `+ - * /`, unary signs and parentheses; expression limit 4096 UTF-8 bytes, nesting depth 64, embedded NUL rejected. Percent is excluded from M1. | Aligned with Tech Lead through AGE-11; detailed grammar below. |
| D-06 | Product decision | Evaluator consumes a complete expression; UI owns editing, repeated-equals, and continuation. Continued calculations use the unrounded numeric result, never a reparsed display string. | AGE-6 behavior scope aligned with AGE-11 stateless evaluator proposal. |
| D-07 | Product/technical decision | Repeated Equals selects the rightmost binary operator at the outermost expression level after repeatedly removing balanced parentheses that enclose the entire expression. Its full right-hand suffix is the repeat operand, evaluated by Rust. This is a lexical editing rule, independent of precedence and key-entry history. | Bureau Director arbitration AGE-22 comment `073134b1-715a-44c9-8d42-574d99a853a7`; exact cases are in R-EQUALS/U-12/I-12/G-12. No C ABI change. |
| A-01 | Assumption | The desktop has a visible expression/result display, digit/operator controls, Clear, Backspace, sign toggle, Equals, and a keyboard-accessible focus model. | Defined by AGE-6 behavior requirements; verify in implementation. |
| A-02 | Assumption | Rust owns arithmetic and formatting behind the C ABI; UI editing does not duplicate calculation semantics. | Calculator profile and AGE-7 architecture assignment. |
| G-01 | Research gap | Actual GUI automation readiness is not established by the profile alone. | macOS needs Appium Mac2, full Xcode, Accessibility permission and a GUI session; Linux needs AT-SPI/PyGObject in Xvfb/Openbox/D-Bus with accessible Qt. Record per-platform execution; a missing GUI setup blocks only GUI scenarios. |
| G-02 | Coordination status | Numerical defaults and ordinary precedence are aligned with Tech Lead's proposal in AGE-11. | Report immutable spec commit and exact semantics to AGE-7; independent review remains pending. |
| G-03 | Owner gate | Scientific planning may proceed, but implementation requires both recorded Project Owner acceptance of the M1 first release and a revision-pinned, accepted scientific v1 contract compatible with the canonical ABI. The scientific contract must define operations, domains, angle behavior, numerical accuracy/tolerance, errors, and display expectations. | Binding AGE-15 arbitration, Decision 1, comment `3fd83498-8c70-4658-85b1-3fbe0ea74673`; operation-list approval alone is insufficient. |

## 4. Behavior requirements

### 4.1 Expression grammar and calculation

The calculator accepts finite ASCII decimal and scientific-notation literals (for example `12`, `.5`, `5.`, `1.25e-3`), unary `+`/`-`, parentheses, and binary `+`, `-`, `*`, `/`. The full expression is limited to 4096 UTF-8 bytes and nesting depth 64; embedded NUL is invalid. Whitespace is ignored. A number has at most one decimal point and one exponent marker, with an optional exponent sign and at least one exponent digit. The grammar rejects implicit multiplication (`2(3)`), adjacent operators without a valid unary sign (`2*/3`), empty parentheses, unmatched parentheses, and empty expressions. A leading sign is valid; signs are also valid after an operator or opening parenthesis (`2*-3`, `(-3)`).

D-02 precedence applies. Parentheses group expressions; multiplication and division bind more tightly than addition and subtraction; operators within one precedence level associate left to right. No sequential/immediate mode is included in M1.

### 4.2 Editing and evaluation

- **Digit/decimal entry:** append to the active numeric token. Do not allow a second decimal point or exponent marker in that token. `e`/`E` starts an exponent only after a mantissa; an optional `+`/`-` immediately after it is the exponent sign and requires at least one following digit. Starting a new numeric token after a binary operator begins a fresh token.
- **Clear:** remove the whole expression/result/error and return to an empty ready state.
- **Backspace:** remove the last character of the active entry. If no entry character remains, leave a valid editable state; never crash or silently evaluate. Backspace after a result returns the displayed result to an editable numeric entry.
- **Sign toggle:** toggle the sign of the active numeric entry; when starting an entry, establish a negative sign for the next value. Do not mutate unrelated earlier operands.
- **Equals:** evaluate only a complete valid expression. On success, show result. On error, show a concise error indicator and follow recovery rules below.
- **Repeated equals:** After a successful full-expression evaluation, remove whitespace and repeatedly remove only balanced parenthesis pairs enclosing the entire expression. In that view, select the rightmost binary `+`, `-`, `*`, or `/` at parenthesis depth zero; signs that are unary or part of a numeric exponent are not binary operators. The entire suffix to the right is the RHS expression. Do not descend into operand groups. Rust remains authoritative for validating/evaluating the full expression and RHS. Retain the resulting operator and binary64 RHS alongside the full-expression result. Each later Equals evaluates `current-result op retained-RHS` through Rust, retaining full precision and the same descriptor. A successful new Equals after a changed expression establishes a new descriptor from that expression. Clear or any expression edit invalidates the old descriptor; operator continuation builds a new expression and establishes its descriptor on Equals. If there is no outer-level binary operator, Equals leaves the result unchanged. Any evaluation/transport failure follows normal error recovery and invalidates the descriptor; no failure sentinel or stale operand is reused. Examples: `2 + 3 =` -> `5`, then `8`, then `11`; `2 + 3 * 4 =` -> `14`, then `56`; `(2 + 3) * 4 =` -> `20`, then `80`; `2 * (3 + 4) =` -> `14`, then `98`. The editing metadata may track token roles, parenthesis depth, and source spans or rebuild them from current text; the UI does not implement precedence, parse/evaluate arithmetic, or infer the repeat operand from chronological key history. Submit the selected RHS span through the existing evaluator and retain its binary64 result. Serialize retained values with the ABI's round-trip representation (max_digits10, preserving signed zero), never the 15-digit display text.
- **Continuation after result:** entering a binary operator continues from the last result (`5`, `*`, `2`, `=` -> `10`). Entering a digit or decimal point after a result starts a new expression. Parentheses and unary sign behavior must not append to a completed expression ambiguously; a digit starts fresh, while a binary operator continues. Continuation uses the retained numeric result and never reparses the displayed decimal string.
- **Percent:** percent is excluded from M1; no percent button, key binding, or implicit percentage behavior is accepted.

### 4.3 Error and recovery

Division by zero, malformed/incomplete expressions on Equals, unmatched parentheses, invalid numeric input, non-finite overflow, and scientific domain errors display an error state rather than a fabricated numeric result. The application remains responsive. Clear always returns to empty-ready state. A digit/decimal input after an error starts a fresh expression. Backspace may edit the invalid expression if the UI retains it; otherwise it is a no-op. The implementation must choose and document whether the failed expression remains visible; this visual detail is a Tech Lead/UI confirmation item, not a calculation oracle.

### 4.4 Keyboard and focus

While the calculator window is active, digits, `.`, `e`/`E` (exponent marker when valid), `+`, `-`, `*`, `/`, `(`, `)`, Enter or `=` map to their corresponding actions. Backspace maps to Backspace; Escape maps to Clear. Keyboard activation must not depend on clicking a particular button first. Tab/Shift+Tab visits interactive controls in a stable order; the focused control is visibly indicated and can be activated with Space or Enter. No global keyboard interception is required while the application is unfocused.

### 4.5 Precision and display

Internal arithmetic is IEEE-754 binary64 with round-to-nearest ties-even. Only finite results are accepted. Numeric oracle tolerance is `abs(actual - expected) <= max(1e-15, 1e-12 * abs(expected))`; listed integer examples must remain exact. Numeric and displayed-string oracles are separate: `0.1 + 0.2` must be within tolerance of `0.3` and display exactly `0.3`.

Rust formats results using 15 significant decimal digits, ties-to-even, removing fractional trailing zeroes and a trailing decimal point. The decimal separator is always `.`. Signed zero displays `0`. Use fixed notation for rounded decimal exponents from `-6` through `14`, and lowercase scientific notation outside that range; scientific exponents have no `+` sign or redundant leading zeroes (for example `1e-7`, `1e15`). Underflow follows binary64 subnormal/zero behavior. Overflow to infinity is a `RANGE` error. Display text is not the source for subsequent calculation: UI continuation retains the underlying numeric result.

## 5. Scientific extension plan (not M1 implementation authorization)

Proposed v1 scientific functions: `sqrt(x)`, `x^y`, `log10(x)`, `ln(x)`, `sin(x)`, and `asin(x)`, with degree/radian angle selection. Scientific planning and case refinement may proceed now. Implementation is prohibited until both recorded Project Owner acceptance of the M1 first release and a revision-pinned accepted scientific v1 contract compatible with the canonical ABI are present. That contract must cover the accepted operation list, domains, angle behavior, numerical accuracy/tolerance, error behavior, and display expectations; an operation-list approval alone does not satisfy the gate. The default angle mode proposal is degrees because the assigned examples include `sin(30 DEG)` and `asin(0.5 DEG)`; mode persistence and where the mode is displayed require resolution in that contract. Power follows conventional precedence above multiplication; exponentiation is proposed right-associative, with unary-minus interaction to be explicitly agreed before implementation. Parenthesized function notation is the acceptance-level semantic model independent of button layout.

| Function | Independently calculable oracle | Domain / error expectation |
|---|---|---|
| `sqrt(9)` | `3` | Negative real input -> domain error (`sqrt(-1)`). |
| `2^10` | `1024` | Finite real result required; overflow -> error. Negative base with non-integer exponent -> domain error unless later product decision defines complex values (out of current proposed scope). |
| `log10(1000)` | `3` | Input must be positive. |
| `ln(e)` | `1` | Input must be positive; `ln(0)` -> domain error. |
| `sin(30 DEG)` | approximately `0.5` | Finite real angle; degrees mode converts before evaluation. |
| `sin(pi/2 RAD)` | approximately `1` | Radians mode; `pi` constant availability is a product/interface decision still to confirm. |
| `asin(0.5 DEG)` | approximately `30` | Input in `[-1,1]`; output units follow selected angle mode. |

Scientific numerical tolerance remains a product acceptance proposal: absolute error `<= 1e-12` for exact/simple values above and `<= 1e-10` for transcendental functions, independent of display formatting. NaN/infinite values are errors, not displayable results. RAD/DEG selectors may be reserved at the ABI boundary but have no M1 behavior. M2 remains planned; AGE-3’s broad two-iteration brief does not waive the two explicit implementation gates above.

## 6. Prioritized backlog

| Priority | Item | Completion evidence / dependency |
|---|---|---|
| P0 | Implement numerical semantics and canonical formatting agreed with Tech Lead. | D-03 through D-05 and all M1 numeric cases in `acceptance-cases-v1.md`. |
| P0 | Implement basic expression grammar, precedence, arithmetic, editing, repeated equals, result continuation, and recovery. | All M1 cases in `acceptance-cases-v1.md`; unit and integration mapping completed. |
| P0 | Verify keyboard/focus and visible result/error behavior on each supported desktop platform. | External GUI scenarios on macOS arm64 and Linux x86_64 (Ubuntu 24.04 baseline), using the profile GUI prerequisites. |
| P1 | Finalize scientific scope and preserve its implementation gates. | Record Project Owner acceptance of the M1 first release and a revision-pinned accepted scientific v1 contract compatible with the canonical ABI, covering operation/domain/angle/numerical/error/display behavior; only then close G-03 for implementation. |
| P2 | Add any additional functions or conveniences. | Out of scope until separately proposed and accepted; no implicit backlog commitment. |

## 7. Release acceptance summary

M1 is acceptable when all M1 cases pass with required unit, integration, and external-GUI evidence in the companion matrix, numeric and displayed-result assertions are both satisfied, and independent review is complete. GUI evidence is required on macOS arm64 and Linux x86_64 (Ubuntu 24.04 baseline). Scientific acceptance cases are planning evidence only until the two G-03 gates are satisfied; they are not an M1 release gate.
