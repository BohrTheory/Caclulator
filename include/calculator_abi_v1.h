/* Generated from contracts/calculator-abi-v1.json; SHA-256 ccbec1a3a05efd4aefb4cef1bcbe79756c4c2d7b3d13538076a73d5da67bd215. DO NOT EDIT. */
#ifndef CALCULATOR_ABI_V1_H
#define CALCULATOR_ABI_V1_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CALC_ABI_VERSION UINT32_C(65536)
#define CALC_OK UINT32_C(0)
#define CALC_INVALID_ARGUMENT UINT32_C(1)
#define CALC_INVALID_UTF8 UINT32_C(2)
#define CALC_SYNTAX UINT32_C(3)
#define CALC_DIVIDE_BY_ZERO UINT32_C(4)
#define CALC_DOMAIN UINT32_C(5)
#define CALC_RANGE UINT32_C(6)
#define CALC_BUFFER_TOO_SMALL UINT32_C(7)
#define CALC_LIMIT UINT32_C(8)
#define CALC_INTERNAL UINT32_C(9)
#define CALC_ANGLE_RAD UINT32_C(0)
#define CALC_ANGLE_DEG UINT32_C(1)
#define CALC_MAX_INPUT_BYTES UINT32_C(4096)
#define CALC_MAX_PARSE_DEPTH UINT32_C(64)
#define CALC_DISPLAY_CAPACITY UINT32_C(64)
#define CALC_NO_ERROR_OFFSET SIZE_MAX
uint32_t calc_abi_version(void);
uint32_t calc_evaluate_v1(const uint8_t * expression, size_t expression_len, uint32_t angle_mode, double * out_value, char * display, size_t display_capacity, size_t * out_required, size_t * out_error_offset);
#ifdef __cplusplus
}
#endif
#endif
