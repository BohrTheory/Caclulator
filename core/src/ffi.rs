use crate::abi_generated::*;
use std::{
    ffi::c_char,
    panic::{AssertUnwindSafe, catch_unwind},
};
#[unsafe(no_mangle)]
pub extern "C" fn calc_abi_version() -> u32 {
    CALC_ABI_VERSION
}

/// # Safety
/// Accessed pointers must satisfy the allocation/alignment/disjointness requirements
/// in docs/architecture/c-abi-v1.md. Null and numeric-size validation is performed here.
#[unsafe(no_mangle)]
pub unsafe extern "C" fn calc_evaluate_v1(
    expression: *const u8,
    expression_len: usize,
    angle_mode: u32,
    out_value: *mut f64,
    display: *mut c_char,
    display_capacity: usize,
    out_required: *mut usize,
    out_error_offset: *mut usize,
) -> u32 {
    if out_value.is_null()
        || out_required.is_null()
        || out_error_offset.is_null()
        || (expression.is_null() && expression_len != 0)
        || (display.is_null() && display_capacity != 0)
        || !matches!(angle_mode, CALC_ANGLE_RAD | CALC_ANGLE_DEG)
        || expression_len > isize::MAX as usize
        || display_capacity > isize::MAX as usize
    {
        return CALC_INVALID_ARGUMENT;
    }
    // SAFETY: caller's valid exclusive output allocations, after logical validation.
    unsafe {
        *out_value = 0.0;
        *out_required = 0;
        *out_error_offset = CALC_NO_ERROR_OFFSET;
        if display_capacity > 0 {
            *display = 0;
        }
    }
    let result = catch_unwind(AssertUnwindSafe(|| {
        #[cfg(test)]
        PANIC_NEXT.with(|flag| {
            if flag.replace(false) {
                std::panic::panic_any(PanicPayload);
            }
        });
        if expression_len > CALC_MAX_INPUT_BYTES as usize {
            return Err(crate::error(CALC_LIMIT, CALC_NO_ERROR_OFFSET));
        }
        let input = if expression_len == 0 {
            &[]
        } else {
            // SAFETY: checked numeric bound; caller guarantees a live immutable span.
            unsafe { std::slice::from_raw_parts(expression, expression_len) }
        };
        let (value, text) = crate::evaluate(input)?;
        if text.len() + 1 > CALC_DISPLAY_CAPACITY as usize {
            return Err(crate::error(CALC_INTERNAL, CALC_NO_ERROR_OFFSET));
        }
        Ok((value, text))
    }));
    match result {
        Err(payload) => {
            std::mem::forget(payload);
            CALC_INTERNAL
        }
        Ok(Err(error)) => {
            unsafe {
                *out_error_offset = error.offset;
            }
            error.status
        }
        Ok(Ok((value, text))) => unsafe {
            *out_value = value;
            *out_required = text.len() + 1;
            if display_capacity < text.len() + 1 {
                return CALC_BUFFER_TOO_SMALL;
            }
            std::ptr::copy_nonoverlapping(text.as_ptr(), display.cast(), text.len());
            *display.add(text.len()) = 0;
            CALC_OK
        },
    }
}
#[cfg(test)]
thread_local! {pub(crate) static PANIC_NEXT: std::cell::Cell<bool> = const {std::cell::Cell::new(false)};}
#[cfg(test)]
struct PanicPayload;
#[cfg(test)]
impl Drop for PanicPayload {
    fn drop(&mut self) {
        panic!("panic payload must not be dropped across FFI");
    }
}
