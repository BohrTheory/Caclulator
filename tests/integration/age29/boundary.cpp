#include "calculator_abi_v1.h"
#include <float.h>
#include <gtest/gtest.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern "C" uint32_t evaluate_from_c(const uint8_t *, size_t, uint32_t, double *,
                                    char *, size_t, size_t *, size_t *);
static void evaluate(const char *id, const uint8_t *s, size_t n,
                     uint32_t status, double expected, const char *text,
                     size_t offset, int approximate) {
  SCOPED_TRACE(id);
  unsigned char storage[66];
  memset(storage, 0xa5, sizeof storage);
  double value = 987;
  size_t required = 123, error = 456;
  uint32_t actual = evaluate_from_c(s, n, CALC_ANGLE_RAD, &value,
                                    (char *)storage + 1, 64, &required, &error);
  ASSERT_EQ(actual, status);
  ASSERT_EQ(error, offset);
  ASSERT_TRUE(storage[0] == 0xa5 && storage[65] == 0xa5);
  if (status == CALC_OK) {
    ASSERT_TRUE(approximate ? fabs(value - expected) <=
                                  fmax(1e-15, 1e-12 * fabs(expected))
                            : value == expected);
    ASSERT_EQ(required, strlen(text) + 1);
    ASSERT_LE(required, 64u);
    ASSERT_EQ(storage[required], 0);
    ASSERT_STREQ((char *)storage + 1, text);
    for (size_t i = required + 1; i < 65; ++i)
      ASSERT_TRUE(storage[i] == 0xa5);
  } else {
    ASSERT_TRUE(value == 0 && !signbit(value));
    ASSERT_TRUE(required == 0);
    ASSERT_TRUE(storage[1] == 0);
    for (size_t i = 2; i < 65; ++i)
      ASSERT_TRUE(storage[i] == 0xa5);
  }
}
static void ok(const char *s, double x, const char *text) {
  evaluate(s, (const uint8_t *)s, strlen(s), CALC_OK, x, text, SIZE_MAX, 0);
}
static void bad(const char *s, uint32_t status, size_t offset) {
  evaluate(s, (const uint8_t *)s, strlen(s), status, 0, "", offset, 0);
  ok("2+3", 5, "5");
}
TEST(AbiBoundary, BufferCapacitiesAndCanaries) {
  const uint8_t input[3] = {'2', '+', '3'};
  for (size_t capacity = 0; capacity <= 64; ++capacity) {
    SCOPED_TRACE(capacity);
    unsigned char a[66];
    memset(a, 0x5a, sizeof a);
    double v = 99;
    size_t r = 99, e = 99;
    uint32_t s =
        evaluate_from_c(input, 3, 1, &v, (char *)a + 1, capacity, &r, &e);
    ASSERT_TRUE(s == (capacity < 2 ? CALC_BUFFER_TOO_SMALL : CALC_OK));
    ASSERT_TRUE(v == 5 && r == 2 && e == SIZE_MAX);
    ASSERT_TRUE(a[0] == 0x5a);
    ASSERT_TRUE(a[65] == 0x5a);
    if (capacity == 0)
      ASSERT_TRUE(a[1] == 0x5a);
    else if (capacity == 1)
      ASSERT_TRUE(a[1] == 0);
    else
      ASSERT_TRUE(a[1] == '5' && a[2] == 0);
    for (size_t i = (capacity >= 2 ? 3 : capacity == 1 ? 2 : 1); i < 66; ++i)
      ASSERT_TRUE(a[i] == 0x5a);
  }
  double v;
  size_t r, e;
  ASSERT_TRUE(evaluate_from_c(input, 3, 0, &v, NULL, 0, &r, &e) ==
              CALC_BUFFER_TOO_SMALL);
  ASSERT_TRUE(v == 5 && r == 2 && e == SIZE_MAX);
  ASSERT_TRUE(memcmp(input, "2+3", 3) == 0);
}
TEST(AbiBoundary, InvalidArgumentsNeverWrite) {
  for (int k = 0; k < 9; ++k) {
    SCOPED_TRACE(k);
    double v = 42;
    size_t r = 43, e = 44;
    char d[64];
    memset(d, 0x5a, sizeof d);
    const uint8_t *in = (const uint8_t *)"2+3";
    size_t n = 3, cap = 64;
    uint32_t angle = 0;
    if (k == 3) {
      in = NULL;
      n = 1;
    }
    if (k == 4)
      cap = 1;
    if (k == 5)
      angle = 2;
    if (k == 6)
      n = (size_t)PTRDIFF_MAX + 1;
    if (k == 7)
      cap = (size_t)PTRDIFF_MAX + 1;
    if (k == 8)
      angle = UINT32_MAX;
    uint32_t s =
        evaluate_from_c(in, n, angle, k == 0 ? NULL : &v, k == 4 ? NULL : d,
                        cap, k == 1 ? NULL : &r, k == 2 ? NULL : &e);
    ASSERT_TRUE(s == CALC_INVALID_ARGUMENT);
    ASSERT_TRUE(v == 42 && r == 43 && e == 44);
    for (size_t i = 0; i < 64; ++i)
      ASSERT_TRUE(d[i] == 0x5a);
    ok("2+3", 5, "5");
  }
  evaluate("null-empty", NULL, 0, CALC_SYNTAX, 0, "", 0, 0);
}
TEST(AbiBoundary, InputAndNestingLimits) {
  char b[4098];
  memset(b, ' ', 4096);
  b[0] = '1';
  b[4096] = 0;
  ok(b, 1, "1");
  b[4096] = ' ';
  evaluate("4097-byte-limit", (uint8_t *)b, 4097, CALC_LIMIT, 0, "", SIZE_MAX,
           0);
  for (int n = 64; n <= 65; ++n) {
    memset(b, '(', n);
    b[n] = '1';
    memset(b + n + 1, ')', n);
    b[2 * n + 1] = 0;
    if (n == 64)
      ok(b, 1, "1");
    else
      bad(b, CALC_LIMIT, 64);
  }
  for (int n = 64; n <= 65; ++n) {
    memset(b, '-', n);
    b[n] = '1';
    b[n + 1] = 0;
    if (n == 64)
      ok(b, 1, "1");
    else
      bad(b, CALC_LIMIT, 64);
  }
  for (int n = 64; n <= 65; ++n) {
    memset(b, '(', 32);
    memset(b + 32, '-', n - 32);
    b[n] = '1';
    memset(b + n + 1, ')', 32);
    b[n + 33] = 0;
    if (n == 64)
      ok(b, 1, "1");
    else
      bad(b, CALC_LIMIT, 64);
  }
  for (int i = 0; i < 2048; ++i) {
    b[2 * i] = '1';
    b[2 * i + 1] = '+';
  }
  b[4095] = 0;
  ok(b, 2048, "2048");
}
TEST(AbiBoundary, ArithmeticFormattingAndErrors) {
  ASSERT_TRUE(sizeof(double) == 8 && sizeof(size_t) == 8);
  ASSERT_TRUE(calc_abi_version() == UINT32_C(0x10000));
  ASSERT_TRUE(calc_abi_version() == CALC_ABI_VERSION);
  ok("2+3", 5, "5");
  ok("7-10", -3, "-3");
  ok("6*7", 42, "42");
  ok("8/2", 4, "4");
  ok("1.25+2.5", 3.75, "3.75");
  ok(".5+5.", 5.5, "5.5");
  ok("1.25e-3*1000", 1.25, "1.25");
  ok("-3+1", -2, "-2");
  ok("2*-3", -6, "-6");
  ok("(-3)*2", -6, "-6");
  ok("2+3*4", 14, "14");
  ok("(2+3)*4", 20, "20");
  ok("8/4/2", 1, "1");
  ok(" \t2\r+\n3 ", 5, "5");
  evaluate("precision", (const uint8_t *)"0.1+0.2", 7, 0, .3, "0.3", SIZE_MAX,
           1);
}
TEST(AbiBoundary, DisplayRounding) {
  ok("1.2300", 1.23, "1.23");
  ok("-0", -0., "0");
  ok("1e-6", 1e-6, "0.000001");
  ok("1e14", 1e14, "100000000000000");
  ok("1e-7", 1e-7, "1e-7");
  ok("1e15", 1e15, "1e15");
  ok("100000000000000.5", 100000000000000.5, "100000000000000");
  ok("100000000000001.5", 100000000000001.5, "100000000000002");
  ok("999999999999999.5", 999999999999999.5, "1e15");
  ok("9.999999999999996e-7", 9.999999999999996e-7, "0.000001");
  ok("5e-324", DBL_TRUE_MIN, "4.94065645841247e-324");
  ok("-1e-999", -0., "0");
}
TEST(AbiBoundary, SyntaxAndEvaluationErrors) {
  bad("", CALC_SYNTAX, 0);
  bad("2+", CALC_SYNTAX, 2);
  bad("(", CALC_SYNTAX, 1);
  bad(")", CALC_SYNTAX, 0);
  bad("()", CALC_SYNTAX, 1);
  bad("2(3)", CALC_SYNTAX, 1);
  bad("2*/3", CALC_SYNTAX, 2);
  bad("1..2", CALC_SYNTAX, 2);
  bad("1e2e3", CALC_SYNTAX, 3);
  bad("1e+", CALC_SYNTAX, 3);
  bad("1,2", CALC_SYNTAX, 1);
  bad("NaN", CALC_SYNTAX, 0);
  bad("sqrt(9)", CALC_SYNTAX, 0);
  bad("2%", CALC_SYNTAX, 1);
  bad("1/0", CALC_DIVIDE_BY_ZERO, 1);
  bad("1/-0", CALC_DIVIDE_BY_ZERO, 1);
  bad("1/1e-999", CALC_DIVIDE_BY_ZERO, 1);
  bad("1e309", CALC_RANGE, 0);
  bad("1e308*10", CALC_RANGE, 5);
  bad("1/0+", CALC_SYNTAX, 4);
  bad("1e309+", CALC_SYNTAX, 6);
  bad("1/0+1e309", CALC_DIVIDE_BY_ZERO, 1);
}
TEST(AbiBoundary, Utf8AndByteOffsets) {
  const uint8_t nul[] = {49, 0, 50}, utf[] = {49, 255},
                precedence[] = {49, 47, 48, 43, 255};
  evaluate("embedded-NUL", nul, 3, CALC_SYNTAX, 0, "", 1, 0);
  evaluate("UTF8", utf, 2, CALC_INVALID_UTF8, 0, "", 1, 0);
  evaluate("whole-UTF8-before-syntax", precedence, 5, CALC_INVALID_UTF8, 0, "",
           4, 0);
  double v = 99;
  size_t required = 99, error = 99;
  ASSERT_TRUE(evaluate_from_c((const uint8_t *)"1/0", 3, 0, &v, NULL, 0,
                              &required, &error) == CALC_DIVIDE_BY_ZERO);
  ASSERT_TRUE(v == 0 && required == 0 && error == 1);
  ASSERT_TRUE(evaluate_from_c((const uint8_t *)"2+", 2, 0, &v, NULL, 0,
                              &required, &error) == CALC_SYNTAX);
  ASSERT_TRUE(v == 0 && required == 0 && error == 2);
  const uint8_t truncated[] = {49, 43, 0xe2, 0x82},
                multibyte[] = {49, 43, 0xc3, 0xa9};
  evaluate("UTF8-truncated", truncated, 4, CALC_INVALID_UTF8, 0, "", 2, 0);
  evaluate("valid-UTF8-invalid-token-byte-offset", multibyte, 4, CALC_SYNTAX, 0,
           "", 2, 0);
}
