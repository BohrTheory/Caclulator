use crate::{Result, abi_generated::*, error};
fn finite(value: f64, offset: usize) -> Result<f64> {
    if value.is_finite() {
        Ok(value)
    } else {
        Err(error(CALC_RANGE, offset))
    }
}
pub fn add(a: f64, b: f64, p: usize) -> Result<f64> {
    finite(a + b, p)
}
pub fn subtract(a: f64, b: f64, p: usize) -> Result<f64> {
    finite(a - b, p)
}
pub fn multiply(a: f64, b: f64, p: usize) -> Result<f64> {
    finite(a * b, p)
}
pub fn divide(a: f64, b: f64, p: usize) -> Result<f64> {
    if b == 0.0 {
        Err(error(CALC_DIVIDE_BY_ZERO, p))
    } else {
        finite(a / b, p)
    }
}
pub fn positive(a: f64, _: f64, _: usize) -> Result<f64> {
    Ok(a)
}
pub fn negative(a: f64, _: f64, _: usize) -> Result<f64> {
    Ok(-a)
}
