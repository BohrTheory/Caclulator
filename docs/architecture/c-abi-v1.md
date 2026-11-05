# C ABI v1.0 behavioral contract

State: numerical and repeated-equals behavior aligned with AGE-6 spec commit
`752d0961c8aacb2e79addeb6d1e26a7ea820d734`; separate spec acceptance and renewed
exact-pair review pending.
Canonical declarations: `contracts/calculator-abi-v1.json`; generated header
`include/calculator_abi_v1.h`. The ABI exposes only `calc_abi_version` and
`calc_evaluate_v1`. No Qt, Rust strings/vectors, C++ classes, enums with uncertain
width, structs passed by value, or exceptions cross this boundary.

## Types and version

`uint32_t` maps to Rust `u32`, `size_t` to `usize`, `double` to IEEE-754 binary64
`f64`, `uint8_t` to `u8`, and `char` to `core::ffi::c_char`. Targets are 64-bit.
Pointers use the platform C ABI and natural alignment. Integers are in host byte
order; this is an in-process interface, never a serialized wire format.
`calc_abi_version()` is allocation-free, thread-safe, and returns `0x00010000`:
major in high 16 bits, minor in low 16 bits. Reject a different major before use.
The version function has a permanent name and signature. Consumers require the
major to match and minor to be at least their required minor; v1.0 requires zero.

```c
uint32_t calc_evaluate_v1(
    const uint8_t *expression, size_t expression_len, uint32_t angle_mode,
    double *out_value, char *display, size_t display_capacity,
    size_t *out_required, size_t *out_error_offset);
```

## Ownership, pointers, nulls and threads

All memory belongs to the caller. Input is borrowed read-only for the duration of
the call; no pointer or view is retained. Output buffers/scalars are exclusively
borrowed for the duration. Caller may free/reuse them after return using its own
allocator. There is no Rust-owned output, handle, destructor or ABI free operation.
Calls are synchronous, reentrant and independent; concurrent calls require disjoint
output storage and no concurrent writer to input. Sharing immutable input is safe.
There is no last-error global, implicit angle mode, locale or retained result.

Required scalar outputs are `out_value`, `out_required`, `out_error_offset`; none
may be null. `expression == NULL` is accepted only for `expression_len == 0`.
`display == NULL` is accepted only for `display_capacity == 0` (size query).
A non-null display with zero capacity is accepted but never dereferenced.
Input length excludes a terminator. No read beyond length; input need not terminate.
Capacity includes room for the terminating NUL. `out_required` includes that NUL.

For accessed regions, the caller guarantees live valid allocations, correct scalar
alignment and sufficient actual sizes. Each output region is disjoint from input
and every other output region. Scalar outputs must not alias one another. Claimed
capacity cannot exceed the actual writable allocation. Length/capacity must not
wrap pointer arithmetic or exceed `PTRDIFF_MAX`; implementation rejects oversized
numeric length/capacity before pointer arithmetic. Non-null zero-length buffers are
not accessed. Arbitrary dangling, forged, misaligned, overlapping or undersized
addresses violate preconditions; they are not guaranteed to return an error and
can cause undefined behavior. Null and size checks do not validate address liveness.

## Validation order and output transaction

1. Check required scalar nulls, null/length pairs, unknown angle value, and
   `PTRDIFF_MAX` length/capacity violations. Return INVALID_ARGUMENT without
   touching any output when any of these fails. All non-null accessed pointers
   must still meet the caller preconditions; no invalid-address probing occurs.
2. Initialize valid scalar outputs to value `0.0`, required `0`, offset `SIZE_MAX`;
   set `display[0] = '\0'` only when capacity is positive.
3. Enforce input byte limit 4096 (LIMIT, offset `SIZE_MAX`, no input read when too
   long); validate the complete UTF-8 span (INVALID_UTF8 at first invalid sequence);
   parse the whole expression including lexical validation (SYNTAX); enforce max
   recursive parser nesting 64 (LIMIT at the token which would exceed it).
4. Evaluate the completed syntax tree left-to-right for operands and format into
   private temporary storage. The first encountered evaluation error wins.
   No caller output is partially committed. On parse/evaluation/limit/panic errors,
   retain the initialized outputs except the documented error offset.
5. On successful evaluation set the finite `out_value`, NUL-inclusive
   `out_required`, and `SIZE_MAX` offset. If capacity is insufficient return
   BUFFER_TOO_SMALL, leaving the display empty (if present); otherwise copy the
   complete result including NUL and return OK. No truncation ever succeeds.

Syntax is checked before arithmetic: `1/0+` returns SYNTAX, not DIVIDE_BY_ZERO.
Within parsing, the first error discovered by a deterministic left-to-right parser
wins; depth LIMIT can precede a later syntax error. No reads past a reported UTF-8
error. Even if the caller supplies no display storage, parse/evaluation errors take
precedence over BUFFER_TOO_SMALL. Failed calls retain no state; the next valid call
must work. INTERNAL is reserved for caught internal failure and never reports
exception text, addresses, or panic payloads.

## Status table

| Code | Name | Condition | Offset / result |
|---:|---|---|---|
| 0 | OK | Successful evaluation and complete display | `SIZE_MAX`; value and display valid |
| 1 | INVALID_ARGUMENT | Null contract, unknown angle, impossible numeric size | All outputs untouched |
| 2 | INVALID_UTF8 | Malformed UTF-8 | First invalid sequence's byte index; value 0 |
| 3 | SYNTAX | Empty/incomplete input, NUL, invalid token, unknown function | First offending byte, or length at EOF; value 0 |
| 4 | DIVIDE_BY_ZERO | Denominator is positive or negative exact zero | Division operator byte index; value 0 |
| 5 | DOMAIN | Recognized operation's invalid real domain (reserved M2) | Operation token start; value 0 |
| 6 | RANGE | Literal or evaluated result overflows finite binary64 | Literal/operator start; value 0 |
| 7 | BUFFER_TOO_SMALL | Successful calculation but capacity < required | `SIZE_MAX`; value/required valid, no partial display |
| 8 | LIMIT | Input >4096 bytes or parse nesting >64 | `SIZE_MAX` for byte limit, offending token for depth; value 0 |
| 9 | INTERNAL | Contained unexpected Rust panic/failure | `SIZE_MAX`; value 0 |

For 2–6, 8–9, required is 0 and a present nonempty display buffer contains an empty
string. Unknown future statuses must be treated as generic failure by consumers;
never display `out_value` as a valid result except for OK or BUFFER_TOO_SMALL.
Offsets are zero-based UTF-8 byte offsets into the exact submitted expression, not
UTF-16 positions or visual columns. UI maps them to its string representation.

## Numerical and language contract

M1 grammar, ASCII tokens and ASCII whitespace SP/TAB/CR/LF only:

```text
expression = sum
sum        = product { ('+' | '-') product }
product    = unary { ('*' | '/') unary }
unary      = ('+' | '-') unary | primary
primary    = number | '(' expression ')'
number     = (DIGIT+ ('.' DIGIT*)? | '.' DIGIT+) ([eE] [+-]? DIGIT+)?
```

No implicit multiplication, percent, identifiers, Unicode operator aliases,
thousands separators, decimal commas, `NaN` or `Inf`. UI may map ×/÷/− keys/buttons
to ASCII before submission. Entire span must parse. Trailing `.` is a valid number;
`.` alone is not. Unary signs bind tighter than multiplication, binary operators
associate left; `2+3*4=14`, `(2+3)*4=20`, `8/4/2=1`, `-2*-3=6`.
Nesting counts recursive unary and parenthesized descent; the outer expression is
depth zero. Iterative binary chains must not grow this nesting count or unbounded
evaluation recursion (bounded token count follows the byte limit).

Arithmetic and decimal conversion use binary64 round-to-nearest ties-even, finite
results only; callers must retain the normal floating-point environment (no changed
rounding mode or flush-to-zero setting). No fast-math or reassociation. Literal
overflow and intermediate nonfinite arithmetic return RANGE. Gradual underflow and
rounding to zero are permitted; zero denominators then return DIVIDE_BY_ZERO. An
underflow is not silently promoted to a nonzero number. Signed zero is numeric
binary64 but displays `0`. `angle_mode` accepts RAD=0 or DEG=1; no M1 arithmetic
depends on it. They are per-call context for approved M2 only.

Rust display uses 15 significant decimal digits rounded ties-even from the binary64
value. Choose plain notation for rounded decimal exponent -6 through 14, scientific
otherwise. Strip insignificant fractional trailing zeros and redundant point;
scientific notation uses lowercase `e`, no exponent plus sign or leading zeros.
Decimal point is always `.`; no locale or digit grouping. Examples: `0.1+0.2` displays
`0.3`, `1/3` displays `0.333333333333333`, `1e15` displays `1e15`, `1e-7` displays
`1e-7`, zero displays `0`. Output ASCII is valid UTF-8. Maximum complete display
storage is CALC_DISPLAY_CAPACITY=64 bytes including NUL. Exceeding that internal
bound is INTERNAL, not a truncated result. Display bytes are presentation only;
continuation follows the lossless transport rules below.

Numeric tests use independent expected values; agreed tolerance is
`abs(actual-expected) <= max(1e-15, 1e-12*abs(expected))` for approximate examples.
Exactly representable elementary integer cases must be exact. Display strings have
exact expectations separately. Do not use approximate tolerance to bless overflow,
domain failures, signed-zero divisor handling, or the wrong operation order.

## Retained values and stateless continuation

The UI retains the successful binary64 `out_value` independently of the 15-digit
display. To continue with an operator, serialize that retained value as an ASCII
decimal/scientific literal that round-trips to the identical binary64 bits. C++17
`std::to_chars` with `std::chars_format::general` and
`std::numeric_limits<double>::max_digits10` precision is the adapter strategy;
check its error result. Preserve the sign of zero (explicitly emit `-0` when needed).
Transport text is not displayed and must never be derived from display bytes.
The producer's decimal conversion must correctly round these literals, including
subnormals, to the original binary64 value under the required rounding environment.
Wrap transported literals in parentheses when composing a full expression, including
negative values; the normal input byte/depth limits still apply to the full payload.

### Repeat selection and transactional controller state

The source is the pinned AGE-6 D-07/§4.2 and U-12/I-12/G-12, incorporating AGE-22.
The prior open question is resolved. The existing stateless ABI supports this rule;
no schema, symbol, signature or version change is required.

On Equals for a new/edited expression:

1. Invalidate any old descriptor. Submit the **complete current expression** to
   Rust for validation/evaluation. Only a successful adapter result (OK, after any
   successful BUFFER_TOO_SMALL retry) can proceed. Save the full-result binary64
   **and display bytes before reusing any output storage** for RHS evaluation.
2. Build a whitespace-free lexical view from current text/token spans. Repeatedly
   remove only balanced parenthesis pairs enclosing the entire view: an opening
   parenthesis must match the last token, not close before it. Select the rightmost
   binary `+ - * /` at depth zero. Token roles distinguish an operand from an
   operator: `+/-` where an operand is expected are unary, and `+/-` within a
   numeric exponent token are exponent signs. Neither is a binary candidate.
   Parentheses update depth; do not descend into operand groups to find a candidate.
   Keep the **entire suffix** after the selected operator, including its groups
   and unary signs, as the RHS span. Current text wins over chronological key history.
   This scan performs no precedence resolution, expression-tree construction or
   numerical evaluation. Rust remains the syntax and arithmetic authority.
3. If there is a candidate, submit that complete RHS expression through Rust even
   if it looks like a single literal. Retain its successful binary64 value together
   with the operator, separately from the saved full-result binary64 and display.
   Do not replace the full result/display with the RHS output. Commit controller
   result and descriptor only when all required calls/conversions succeed. If no
   candidate exists, commit the full result with no descriptor and make no RHS call.
4. For later Equals with a descriptor, serialize current result and retained RHS
   by the lossless rule above and evaluate `(result)op(RHS)` through Rust. Retain
   the same operator/RHS across successful repeats; update only the result/display.
   Do not derive a new descriptor by scanning this synthetic repeat expression.
   With no descriptor, repeated Equals leaves the result/display unchanged and
   makes **no evaluator call**.

Every full/RHS/repeat call obeys input lifetime, disjoint output regions, actual
allocation size, UTF-8 byte length, status and output-transaction rules above.
A sequential call may reuse storage only after the needed values/bytes are copied;
concurrent calls require independent output storage. Prefer the sufficient 64-byte
buffer; a size-query/short-buffer response is not a UI commit until a successful
retry supplies the complete display. Check conversion/retry errors and input limits.
Never use numeric failure sentinels, stale outputs, or display text as operands.

Clear and **any expression edit** invalidate the descriptor, including backspace,
sign toggle and new digit/decimal entry. Operator continuation serializes the saved
result into a new parenthesized expression, invalidates the old descriptor, and
establishes a new descriptor only on successful Equals for that new expression.
Full-expression, RHS, transport or repeat failure invalidates the descriptor and
enters normal error recovery; an intermediate success must not escape as a final
result. Preserve the failed expression for backspace correction. Clear returns to
empty-ready; a fresh digit/decimal after error starts a new expression. Backspace
correction must evaluate the corrected current text, never reuse an old operand.

Mandatory integration oracles: after `1/3=`, continuing `*3=` yields numeric exactly
`1` and display `1`; using displayed `0.333333333333333` as input fails the numeric
oracle. After `0.1+0.2=`, continuing `-0.3=` yields the positive binary64 value
`2^-54`, not zero. Test transport identity by exact bits for negative zero, minimum
positive subnormal, minimum positive normal, maximum finite, and representative
negative/exponent values. Approximate tolerance cannot prove lossless transport.
Backspace-after-result intentionally enters the displayed number into an editable
entry per AGE-6; that explicit edit is distinct from automatic continuation/repeat.

## Call examples and mandatory cases

`examples/abi/header_c.c` and `header_cpp.cpp` compile without Rust or Qt, but their
executables require the actual staticlib. They are not implementation substitutes.

For `"2+3"`, length 3, RAD and valid scalar outputs:

1. Call with `display=NULL, capacity=0`: BUFFER_TOO_SMALL, value 5, required 2,
   offset SIZE_MAX. No caller allocation is retained.
2. Allocate at least 2 bytes and repeat the same input: OK, bytes `{'5','\0'}`.
3. Capacity 1: BUFFER_TOO_SMALL, value 5, required 2, display[0] is NUL.
4. A fixed 64-byte buffer is enough for any successful v1 calculation; retries
   are deterministic for identical immutable input and angle mode.

| Input / condition | Expected status and important result |
|---|---|
| `2+3*4` | OK, 14, `14` |
| `(2+3)*4` | OK, 20, `20` |
| `0.1+0.2` | OK, within numeric tolerance; display `0.3` |
| `1/0` or `1/-0` | DIVIDE_BY_ZERO, offset 1 |
| `1+` | SYNTAX, offset 2 (EOF) |
| null input with length 0 | SYNTAX, offset 0 |
| null input with length 1 | INVALID_ARGUMENT, outputs untouched |
| null required scalar | INVALID_ARGUMENT, other outputs untouched |
| null display with capacity 1 | INVALID_ARGUMENT, outputs untouched |
| angle mode 2 | INVALID_ARGUMENT, outputs untouched |
| bytes `31 00 32` | SYNTAX, offset 1; embedded NUL not terminator |
| bytes `31 ff` | INVALID_UTF8, offset 1 |
| `1e309` | RANGE, offset 0 |
| `1e308*10` | RANGE, offset 5 |
| 4097 bytes | LIMIT, SIZE_MAX; no input read |
| 65 nested opening parentheses | LIMIT at byte 64 |
| valid expression after any handled error | Same result as fresh process |

No real producer, linked-consumer, sanitizer or panic injection checks have run in
AGE-7. Their exact obligations and owners are in integration-packets.md.
