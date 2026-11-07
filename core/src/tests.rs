use crate::{abi_generated::*, *};
use std::ptr;
fn success(text: &str, value: f64, display: &str) {
    let actual = evaluate(text.as_bytes()).unwrap();
    assert_eq!(actual.0.to_bits(), value.to_bits(), "{text}");
    assert_eq!(actual.1, display, "{text}");
}
fn failure(text: &[u8], status: u32, offset: usize) {
    assert_eq!(evaluate(text).unwrap_err(), error(status, offset));
}
#[test]
fn arithmetic_grammar_and_display() {
    for (text, value, display) in [
        ("2+3", 5.0, "5"),
        ("7-10", -3.0, "-3"),
        ("6*7", 42.0, "42"),
        ("8/2", 4.0, "4"),
        (".5+5.", 5.5, "5.5"),
        ("1.25+2.5", 3.75, "3.75"),
        ("1.25e-3*1000", 1.25, "1.25"),
        ("-3+1", -2.0, "-2"),
        ("2*-3", -6.0, "-6"),
        ("(-3)*2", -6.0, "-6"),
        ("2+3*4", 14.0, "14"),
        ("(2+3)*4", 20.0, "20"),
        ("8/4/2", 1.0, "1"),
        ("8-4-2", 2.0, "2"),
        ("--2", 2.0, "2"),
        (" \t+2\r\n", 2.0, "2"),
        ("1/3", 1.0 / 3.0, "0.333333333333333"),
        ("0.1+0.2", 0.1 + 0.2, "0.3"),
        ("1e-6", 1e-6, "0.000001"),
        ("1e14", 1e14, "100000000000000"),
        ("1e-7", 1e-7, "1e-7"),
        ("1e15", 1e15, "1e15"),
        ("-0", -0.0, "0"),
        ("1e-300*1e-300", 0.0, "0"),
        ("-1e-300*1e-300", -0.0, "0"),
    ] {
        success(text, value, display);
    }
    let (v, d) = evaluate(b"0.1+0.2").unwrap();
    assert!((v - 0.3).abs() <= 1e-15_f64.max(1e-12 * 0.3));
    assert_eq!(d, "0.3");
}
#[test]
fn syntax_offsets_and_precedence() {
    for (s, p) in [
        ("", 0),
        ("1+", 2),
        ("(", 1),
        (")", 0),
        ("()", 1),
        ("2(3)", 1),
        ("2*/3", 2),
        (".", 0),
        ("1e", 2),
        ("1e+", 3),
        ("1.2.3", 3),
        ("1e2e3", 3),
        ("2%3", 1),
        ("NaN", 0),
        ("Inf", 0),
        ("sqrt(9)", 0),
        ("1,2", 1),
        ("1\0x", 1),
        ("1\u{a0}", 1),
        ("1/0+", 4),
        ("1e309+", 6),
    ] {
        failure(s.as_bytes(), CALC_SYNTAX, p);
    }
    failure(b"1\xff", CALC_INVALID_UTF8, 1);
    failure(&[b'1', 0xe2, 0x82], CALC_INVALID_UTF8, 1);
    failure(b"1/0\xff", CALC_INVALID_UTF8, 3);
    failure(b"1/0", CALC_DIVIDE_BY_ZERO, 1);
    failure(b"1/-0", CALC_DIVIDE_BY_ZERO, 1);
    failure(b"1/1e-9999", CALC_DIVIDE_BY_ZERO, 1);
    failure(b"1e309", CALC_RANGE, 0);
    failure(b"1e308*10", CALC_RANGE, 5);
    failure(b"1/0+1e309", CALC_DIVIDE_BY_ZERO, 1);
    failure(b"1e309+1/0", CALC_RANGE, 0);
    success("2+3", 5.0, "5");
}
#[test]
fn bounds_and_long_chains() {
    let text = format!("{}1{}", "(".repeat(64), ")".repeat(64));
    success(&text, 1.0, "1");
    failure(
        format!("{}1{}", "(".repeat(65), ")".repeat(65)).as_bytes(),
        CALC_LIMIT,
        64,
    );
    success(&format!("{}1", "+".repeat(64)), 1.0, "1");
    failure(format!("{}1", "+".repeat(65)).as_bytes(), CALC_LIMIT, 64);
    failure(format!("{}?", "(".repeat(65)).as_bytes(), CALC_LIMIT, 64);
    success(&format!("1{}", " ".repeat(4095)), 1.0, "1");
    failure(&vec![0xff; 4097], CALC_LIMIT, CALC_NO_ERROR_OFFSET);
    let chain = format!("{}1 ", "1+".repeat(2047));
    assert_eq!(chain.len(), 4096);
    success(&chain, 2048.0, "2048");
}
#[test]
fn exact_bit_literal_transport_and_continuation() {
    let literals = [
        ("-0", (-0.0_f64).to_bits()),
        ("4.9406564584124654e-324", 1),
        ("2.2250738585072014e-308", f64::MIN_POSITIVE.to_bits()),
        ("1.7976931348623157e308", f64::MAX.to_bits()),
        (
            "-1.2345678901234567e+42",
            (-1.2345678901234567e42_f64).to_bits(),
        ),
    ];
    for (text, bits) in literals {
        assert_eq!(evaluate(text.as_bytes()).unwrap().0.to_bits(), bits);
    }
    // Exercise many independent binary64 values through lossless decimal literals.
    let mut bits = 1_u64;
    for _ in 0..10000 {
        bits = bits.wrapping_mul(6364136223846793005).wrapping_add(1);
        let v = f64::from_bits(bits);
        if v.is_finite() {
            let text = format!("{v:.17e}");
            assert_eq!(evaluate(text.as_bytes()).unwrap().0.to_bits(), bits);
        }
    }
    let third = evaluate(b"1/3").unwrap().0;
    success(&format!("({third:.17e})*3"), 1.0, "1");
    let sum = evaluate(b"0.1+0.2").unwrap().0;
    success(
        &format!("({sum:.17e})-0.3"),
        2_f64.powi(-54),
        "5.55111512312578e-17",
    );
}
#[test]
fn formatter_rounding_and_notation_after_rounding() {
    for (v, s) in [
        (123456789012344.5, "123456789012344"),
        (123456789012345.5, "123456789012346"),
        (999999999999999.5, "1e15"),
        (9.999999999999999e-7, "0.000001"),
        (1.2300, "1.23"),
        (-1e15, "-1e15"),
        (f64::MAX, "1.79769313486232e308"),
        (f64::from_bits(1), "4.94065645841247e-324"),
    ] {
        assert_eq!(format::display(v), s);
    }
}
struct Outputs {
    value: f64,
    required: usize,
    offset: usize,
    buffer: [u8; 66],
}
impl Outputs {
    fn new() -> Self {
        Self {
            value: 123.0,
            required: 456,
            offset: 789,
            buffer: [0x55; 66],
        }
    }
    unsafe fn call(&mut self, input: *const u8, len: usize, cap: usize) -> u32 {
        unsafe {
            ffi::calc_evaluate_v1(
                input,
                len,
                CALC_ANGLE_RAD,
                &mut self.value,
                self.buffer.as_mut_ptr().add(1).cast(),
                cap,
                &mut self.required,
                &mut self.offset,
            )
        }
    }
    fn reset(&self, offset: usize) {
        assert_eq!(self.value.to_bits(), 0.0_f64.to_bits());
        assert_eq!(self.required, 0);
        assert_eq!(self.offset, offset);
        assert_eq!(self.buffer[1], 0);
        self.canaries();
    }
    fn canaries(&self) {
        assert_eq!(self.buffer[0], 0x55);
        assert_eq!(self.buffer[65], 0x55);
    }
}
#[test]
fn ffi_buffers_queries_and_no_partial_writes() {
    unsafe {
        assert_eq!(ffi::calc_abi_version(), CALC_ABI_VERSION);
        for cap in [0, 1, 2, 64] {
            let mut o = Outputs::new();
            let status = o.call(b"2+3".as_ptr(), 3, cap);
            assert_eq!(
                status,
                if cap < 2 {
                    CALC_BUFFER_TOO_SMALL
                } else {
                    CALC_OK
                }
            );
            assert_eq!(o.value, 5.0);
            assert_eq!(o.required, 2);
            assert_eq!(o.offset, usize::MAX);
            if cap == 0 {
                assert_eq!(o.buffer, [0x55; 66]);
            } else if cap == 1 {
                assert_eq!(o.buffer[1], 0);
                assert_eq!(o.buffer[2], 0x55);
            } else {
                assert_eq!(&o.buffer[1..3], b"5\0");
            }
            o.canaries();
        }
        let mut o = Outputs::new();
        assert_eq!(
            ffi::calc_evaluate_v1(
                b"2+3".as_ptr(),
                3,
                1,
                &mut o.value,
                ptr::null_mut(),
                0,
                &mut o.required,
                &mut o.offset
            ),
            CALC_BUFFER_TOO_SMALL
        );
        assert_eq!(o.value, 5.0);
        assert_eq!(o.required, 2);
    }
}
#[test]
fn ffi_invalid_arguments_leave_every_output_untouched() {
    unsafe {
        for case in 0..9 {
            let mut o = Outputs::new();
            let mut v = &mut o.value as *mut f64;
            let mut r = &mut o.required as *mut usize;
            let mut e = &mut o.offset as *mut usize;
            let mut input = b"2+3".as_ptr();
            let mut len = 3;
            let mut d = o.buffer.as_mut_ptr().add(1).cast();
            let mut cap = 64;
            let mut angle = 0;
            match case {
                0 => v = ptr::null_mut(),
                1 => r = ptr::null_mut(),
                2 => e = ptr::null_mut(),
                3 => input = ptr::null(),
                4 => d = ptr::null_mut(),
                5 => angle = 2,
                6 => len = usize::MAX,
                7 => cap = usize::MAX,
                8 => {
                    input = ptr::null();
                    len = 4097;
                    angle = 2;
                }
                _ => unreachable!(),
            }
            assert_eq!(
                ffi::calc_evaluate_v1(input, len, angle, v, d, cap, r, e),
                CALC_INVALID_ARGUMENT
            );
            assert_eq!(o.value, 123.0);
            assert_eq!(o.required, 456);
            assert_eq!(o.offset, 789);
            assert_eq!(o.buffer, [0x55; 66]);
        }
    }
}
#[test]
fn ffi_error_reset_precedence_and_recovery() {
    unsafe {
        for (text, status, offset) in [
            (b"".as_slice(), CALC_SYNTAX, 0),
            (b"1\0x", CALC_SYNTAX, 1),
            (b"1\xff", CALC_INVALID_UTF8, 1),
            (b"1/0+", CALC_SYNTAX, 4),
            (b"1/-0", CALC_DIVIDE_BY_ZERO, 1),
            (b"1e309", CALC_RANGE, 0),
        ] {
            let mut o = Outputs::new();
            assert_eq!(o.call(text.as_ptr(), text.len(), 64), status);
            o.reset(offset);
            assert_eq!(o.call(b"2+3".as_ptr(), 3, 64), CALC_OK);
            let mut o = Outputs::new();
            assert_eq!(o.call(text.as_ptr(), text.len(), 0), status);
            assert_eq!(o.required, 0);
            assert_eq!(o.buffer, [0x55; 66]);
        }
        let mut o = Outputs::new();
        assert_eq!(o.call(ptr::null(), 0, 64), CALC_SYNTAX);
        o.reset(0);
        // A live oversized input is rejected before UTF-8 validation.
        assert_eq!(o.call(vec![0xff; 4097].as_ptr(), 4097, 64), CALC_LIMIT);
        o.reset(usize::MAX);
    }
}
#[test]
fn ffi_panic_payload_contained_and_next_call_recovers() {
    unsafe {
        let mut o = Outputs::new();
        ffi::PANIC_NEXT.with(|f| f.set(true));
        assert_eq!(o.call(b"2+3".as_ptr(), 3, 64), CALC_INTERNAL);
        o.reset(usize::MAX);
        assert_eq!(o.call(b"2+3".as_ptr(), 3, 64), CALC_OK);
        assert_eq!(o.value, 5.0);
    }
}
#[test]
fn registry_invariants() {
    use registry::*;
    for (i, op) in OPERATIONS.iter().enumerate() {
        assert_eq!(op.module, "basic");
        assert!(!op.key.is_empty());
        assert_eq!(op.arity, if op.fixity == Fixity::Prefix { 1 } else { 2 });
        for other in &OPERATIONS[..i] {
            assert_ne!(op.key, other.key);
            assert!((op.token, op.fixity) != (other.token, other.fixity));
        }
    }
}
#[test]
fn concurrent_stateless_calls() {
    let threads: Vec<_> = (0..8)
        .map(|_| {
            std::thread::spawn(|| {
                for _ in 0..100 {
                    unsafe {
                        let mut o = Outputs::new();
                        assert_eq!(o.call(b"2+3*4".as_ptr(), 5, 64), CALC_OK);
                        assert_eq!(o.value, 14.0);
                    }
                }
            })
        })
        .collect();
    for thread in threads {
        thread.join().unwrap();
    }
}

#[test]
fn ffi_depth_and_byte_boundaries_with_real_allocations() {
    unsafe {
        for (text, status, offset) in [
            (
                format!("{}1{}", "(".repeat(64), ")".repeat(64)),
                CALC_OK,
                usize::MAX,
            ),
            (
                format!("{}1{}", "(".repeat(65), ")".repeat(65)),
                CALC_LIMIT,
                64,
            ),
            (format!("1{}", " ".repeat(4095)), CALC_OK, usize::MAX),
            (format!("1{}", " ".repeat(4096)), CALC_LIMIT, usize::MAX),
            (
                format!("{}1{}", "-(".repeat(32), ")".repeat(32)),
                CALC_OK,
                usize::MAX,
            ),
            (
                format!("{}-1{}", "-(".repeat(32), ")".repeat(32)),
                CALC_LIMIT,
                64,
            ),
        ] {
            let mut o = Outputs::new();
            assert_eq!(o.call(text.as_ptr(), text.len(), 64), status);
            if status != CALC_OK {
                o.reset(offset);
            } else {
                assert_eq!(o.value, 1.0);
            }
        }
    }
}
#[test]
fn stateless_rhs_and_repeat_arithmetic_oracles() {
    for (text, value, display) in [
        ("14*4", 56.0, "56"),
        ("56*4", 224.0, "224"),
        ("20*4", 80.0, "80"),
        ("80*4", 320.0, "320"),
        ("3+4", 7.0, "7"),
        ("14*7", 98.0, "98"),
        ("98*7", 686.0, "686"),
        ("5+3", 8.0, "8"),
        ("8+3", 11.0, "11"),
        ("1/2", 0.5, "0.5"),
        ("0.5/2", 0.25, "0.25"),
        ("(-6)*(-3)", 18.0, "18"),
        ("18*(-3)", -54.0, "-54"),
        ("((2+3))", 5.0, "5"),
        ("-(2+3)", -5.0, "-5"),
    ] {
        success(text, value, display);
    }
    let mut v = evaluate(b"2+1e-3").unwrap().0;
    for (expected, display) in [(2.002, "2.002"), (2.003, "2.003")] {
        let actual = evaluate(format!("({v:.17e})+(1e-3)").as_bytes()).unwrap();
        assert!((actual.0 - expected).abs() <= 1e-15_f64.max(1e-12 * expected));
        assert_eq!(actual.1, display);
        v = actual.0;
    }
}
