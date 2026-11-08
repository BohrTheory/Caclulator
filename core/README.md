# M1 calculator core

`calculator_core` is a dependency-free Rust staticlib/rlib. Business logic has no
presentation framework dependency. The generated Rust declarations and producer
signature checks are consumed from `../contracts/generated/` unchanged.

Accepted architecture: `67100ba0ce1d7aaf4fcafadb9101465b78d19829`.
Accepted product/spec files: `752d0961c8aacb2e79addeb6d1e26a7ea820d734`.
AGE-25 integrated input base: `7452dbdcc315e604c05a609994c3301de57865ba`.
Historical/provisioned base: `3ef202581d4e784d0f9da508767577888a4fc272`.
The native acceptance decisions in AGE-25 supersede historical pending-review text
in those unchanged input documents.

## Build and verify

For the macOS arm64 producer, set `MACOSX_DEPLOYMENT_TARGET=14.0` consistently
with the future C++ consumer, then run:

```sh
cargo test --manifest-path core/Cargo.toml
cargo test --manifest-path core/Cargo.toml --release
cargo build --manifest-path core/Cargo.toml --release
python3 tools/generate_abi.py --check
cargo tree --manifest-path core/Cargo.toml
cargo rustc --manifest-path core/Cargo.toml --release -- --print native-static-libs
nm -gU core/target/release/libcalculator_core.a
```

The staticlib is `core/target/release/libcalculator_core.a`. The local macOS arm64
Rust 1.89.0 build reports `-lSystem -lc -lm`; consumers must obtain flags for their
actual target rather than reuse these on Linux. Release uses panic unwinding.
The public C header is `include/calculator_abi_v1.h`. Only `calc_abi_version` and
`calc_evaluate_v1` are calculator exports. No heap ownership crosses that ABI.
Caller pointer preconditions are documented in the canonical behavioral contract;
null/size checks cannot validate arbitrary address liveness.

## Implementation and unit evidence

Parsing validates the complete syntax before numeric conversion or arithmetic.
The parser emits a flat postfix program and evaluation uses an explicit value
stack. Binary chains have bounded parser recursion by precedence levels, and unary
and parenthesis descent share the 64-level limit. The 4096-byte input bound limits
program size and work. Operation metadata and implementations form an immutable
source registry, with uniqueness/arity checks in units; there is no plugin loader.
Formatting rounds binary64 to 15 significant digits using Rust's decimal formatter,
then selects notation using the rounded exponent.

The FFI initializes outputs after argument checks, evaluates/formats privately,
and commits successful value/required length together. Short buffers receive no
partial display. Test-only thread-local panic injection exercises containment,
including a payload with a panicking destructor; caught payloads are deliberately
forgotten on that exceptional path. No panic trigger is exposed in production.
Allocation aborts, invalid memory access and externally altered floating-point
rounding/flush modes are outside the unwind promise.

The unit suite covers exact arithmetic, precedence/associativity, unary/decimal/
exponent grammar, malformed input and UTF-8 offsets, syntax-before-arithmetic and
left-to-right error precedence, signed-zero division, finite overflow/underflow,
format ties/notation transitions, exact-bit transport (including subnormal/normal/
maximum finite/-0 plus 10,000 deterministic finite bit patterns), continuation
precision, stateless RHS/repeat arithmetic, 4096/4097 bytes, unary/parenthesis and
mixed depth 64/65, long binary chains, registry invariants, concurrent calls,
FFI null/no-write/reset/capacity/canary rules, and panic recovery. Editing state,
repeat descriptor selection and call counts belong to Experience/Integration QA.

Actual logs, environment, command exits, archive/source hashes and final commit
are recorded in the immutable AGE-25 implementation-evidence document and bundle.
These units are author evidence. No C/C++ linked consumer, sanitizer, Linux build,
minimum macOS 14 runtime, Qt app, GUI, package, CI or release verification is claimed.
GitHub push/PR/merge/release are excluded from AGE-25. Scientific implementation
requires both owner M1 release acceptance and an accepted compatible scientific
contract; neither is supplied by this component.
