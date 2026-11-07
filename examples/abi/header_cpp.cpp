#include "calculator_abi_v1.h"
#include <cstddef>
#include <limits>
#include <type_traits>

static_assert(sizeof(std::size_t) == 8);
static_assert(std::numeric_limits<double>::is_iec559);
using Evaluate = uint32_t (*)(const uint8_t *, size_t, uint32_t, double *,
                             char *, size_t, size_t *, size_t *);
static_assert(std::is_same_v<decltype(&calc_evaluate_v1), Evaluate>);

/* No Qt, allocation, or implementation substitutes. */
int main() {
    const uint8_t expression[] = "1/0";
    double value = 0.0;
    char display[CALC_DISPLAY_CAPACITY]{};
    size_t required = 0;
    size_t offset = CALC_NO_ERROR_OFFSET;
    const auto status = calc_evaluate_v1(expression, sizeof(expression) - 1,
        CALC_ANGLE_RAD, &value, display, sizeof(display), &required, &offset);
    return status == CALC_DIVIDE_BY_ZERO ? 0 : 1;
}
