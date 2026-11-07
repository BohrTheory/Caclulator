#include "calculator_abi_v1.h"

/* Compile as C11: the GoogleTest assertions exercise a real C caller too. */
uint32_t evaluate_from_c(const uint8_t *input, size_t length, uint32_t angle,
                         double *value, char *display, size_t capacity,
                         size_t *required, size_t *error) {
  return calc_evaluate_v1(input, length, angle, value, display, capacity,
                          required, error);
}
