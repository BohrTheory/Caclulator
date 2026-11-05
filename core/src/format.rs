//! Rust's decimal formatter rounds the exact binary64 to nearest, ties-even.
pub fn display(value: f64) -> String {
    if value == 0.0 {
        return "0".into();
    }
    // Round first; notation depends on the rounded decimal exponent.
    let rounded = format!("{value:.14e}");
    let (mantissa, exponent) = rounded.split_once('e').unwrap();
    let exponent: i32 = exponent.parse().unwrap();
    let negative = mantissa.starts_with('-');
    let digits = mantissa.trim_start_matches('-').replace('.', "");
    if !(-6..15).contains(&exponent) {
        format!(
            "{}e{}",
            mantissa.trim_end_matches('0').trim_end_matches('.'),
            exponent
        )
    } else {
        let point = exponent + 1;
        let mut text = if point <= 0 {
            format!("0.{}{}", "0".repeat((-point) as usize), digits)
        } else if point as usize >= digits.len() {
            format!("{}{}", digits, "0".repeat(point as usize - digits.len()))
        } else {
            format!(
                "{}.{}",
                &digits[..point as usize],
                &digits[point as usize..]
            )
        };
        if text.contains('.') {
            text = text.trim_end_matches('0').trim_end_matches('.').into();
        }
        if negative {
            text.insert(0, '-');
        }
        text
    }
}
