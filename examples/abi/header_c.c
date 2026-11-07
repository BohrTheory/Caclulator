#include "calculator_abi_v1.h"
#include <float.h>

_Static_assert(sizeof(uint32_t) == 4, "ABI requires 32-bit uint32_t");
_Static_assert(sizeof(size_t) == 8, "v1 targets 64-bit platforms");
_Static_assert(sizeof(double) == 8 && DBL_MANT_DIG == 53 && DBL_MAX_EXP == 1024,
               "ABI requires binary64 double");

/* Compile-only contract example. Linking requires the real Rust staticlib. */
int main(void) {
    const uint8_t expression[] = "2+3*4";
    double value = 0.0;
    char display[CALC_DISPLAY_CAPACITY];
    size_t required = 0;
    size_t offset = CALC_NO_ERROR_OFFSET;
    if ((calc_abi_version() >> 16) != 1) return 2;
    uint32_t status = calc_evaluate_v1(expression, sizeof(expression) - 1,
        CALC_ANGLE_RAD, &value, display, sizeof(display), &required, &offset);
    return status == CALC_OK && value == 14.0 ? 0 : 1;
}
