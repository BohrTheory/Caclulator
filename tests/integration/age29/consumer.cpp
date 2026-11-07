#include "calculator_abi_v1.h"
#include <atomic>
#include <cfloat>
#include <charconv>
#include <cmath>
#include <cstring>
#include <gtest/gtest.h>
#include <limits>
#include <string>
#include <thread>
#include <vector>
static uint64_t bits(double v) {
  uint64_t b;
  memcpy(&b, &v, 8);
  return b;
}
static std::string transport(double v) {
  if (v == 0 && std::signbit(v))
    return "-0";
  char b[64];
  auto r = std::to_chars(b, b + 64, v, std::chars_format::general,
                         std::numeric_limits<double>::max_digits10);
  EXPECT_EQ(r.ec, std::errc());
  return std::string(b, r.ptr);
}
static double call(const std::string &s) {
  double v = 0;
  size_t r = 0, e = 0;
  char d[64];
  EXPECT_EQ(calc_evaluate_v1((const uint8_t *)s.data(), s.size(), 0, &v, d, 64,
                             &r, &e),
            CALC_OK);
  return v;
}
static bool accepts(uint32_t version) {
  return (version >> 16) == 1 &&
         (version & 0xffff) >= (CALC_ABI_VERSION & 0xffff);
}
TEST(Consumer, AbiCompatibility) {
  ASSERT_TRUE(accepts(calc_abi_version()));
  // Synthetic version values exercise consumer policy only; no replacement
  // producer.
  ASSERT_TRUE(!accepts(0x00020000));
  ASSERT_TRUE(!accepts(0x00000000));
  ASSERT_TRUE(accepts(0x00010001));
}
TEST(Consumer, LosslessTransport) {
  for (double x :
       {-0., DBL_TRUE_MIN, DBL_MIN, DBL_MAX, -1.25e-200, 1.25e200, 1. / 3.}) {
    auto s = transport(x);
    double y = call(s);
    ASSERT_EQ(bits(x), bits(y)) << s;
  }
}
TEST(Consumer, RetainedValueContinuation) {
  double third = call("1/3");
  ASSERT_TRUE(call("(" + transport(third) + ")*3") == 1);
  double sum = call("0.1+0.2");
  ASSERT_TRUE(bits(call("(" + transport(sum) + ")-0.3")) == bits(0x1p-54));
}
TEST(Consumer, ConcurrentDisjointOutputs) {
  std::atomic<unsigned> failures{0};
  std::vector<std::thread> workers;
  for (int t = 0; t < 8; ++t)
    workers.emplace_back([t, &failures]() {
      for (int i = 0; i < 1000; ++i) {
        double v = 99;
        size_t r = 99, e = 99;
        char d[64];
        const char *s = i % 2 ? "6*7" : "1/-0";
        auto a = calc_evaluate_v1((const uint8_t *)s, strlen(s), t % 2, &v, d,
                                  64, &r, &e);
        if (i % 2) {
          if (a != CALC_OK || v != 42 || memcmp(d, "42", 3) || r != 3 ||
              e != SIZE_MAX)
            ++failures;
        } else if (a != CALC_DIVIDE_BY_ZERO || v != 0 || r != 0 || e != 1 ||
                   d[0])
          ++failures;
      }
    });
  for (auto &w : workers)
    w.join();
  ASSERT_EQ(failures.load(), 0u);
}
