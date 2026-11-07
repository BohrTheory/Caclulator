//! Framework-independent M1 calculator and its caller-owned C boundary.
#[rustfmt::skip]
#[path = "../../contracts/generated/abi_generated.rs"]
pub mod abi_generated;
mod evaluator;
mod format;
mod operations {
    pub mod basic;
}
pub mod ffi;
mod parser;
mod registry;
include!("../../contracts/generated/producer_checks.rs");

#[derive(Debug, PartialEq)]
pub struct Error {
    pub status: u32,
    pub offset: usize,
}
pub(crate) type Result<T> = std::result::Result<T, Error>;
pub(crate) fn error(status: u32, offset: usize) -> Error {
    Error { status, offset }
}

/// Evaluate a complete UTF-8 byte span. Errors use original byte offsets.
pub fn evaluate(input: &[u8]) -> Result<(f64, String)> {
    use abi_generated::*;
    if input.len() > CALC_MAX_INPUT_BYTES as usize {
        return Err(error(CALC_LIMIT, CALC_NO_ERROR_OFFSET));
    }
    let text = std::str::from_utf8(input).map_err(|e| error(CALC_INVALID_UTF8, e.valid_up_to()))?;
    let program = parser::parse(text)?;
    let value = evaluator::evaluate(&program)?;
    Ok((value, format::display(value)))
}
#[cfg(test)]
mod tests;
