#include "calculator.hpp"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <gtest/gtest.h>
#include <limits>
using namespace calculator;
static size_t physical = 0;
static size_t failPhysical = 0;
static uint32_t observed(const uint8_t *p, size_t n, uint32_t a, double *v,
                         char *d, size_t c, size_t *r, size_t *e) {
  ++physical;
  if (failPhysical == physical) {
    *v = 0;
    *r = 0;
    *e = SIZE_MAX;
    if (c)
      d[0] = 0;
    return CALC_INTERNAL;
  }
  return calc_evaluate_v1(p, n, a, v, d, c, r, e);
}
static uint64_t bits(double v) {
  uint64_t b;
  memcpy(&b, &v, 8);
  return b;
}
struct Rig {
  Adapter adapter;
  size_t logical = 0;
  Controller controller;
  explicit Rig(size_t cap = 64)
      : adapter(calc_abi_version, observed, cap),
        controller([this](const std::string &s) {
          ++logical;
          return adapter(s);
        }) {}
  void input(const char *s) {
    for (; *s; ++s)
      controller.dispatch(Action::Input, *s);
  }
  void eq() { controller.dispatch(Action::Equals); }
  void ready() {
    const auto &s = controller.state();
    ASSERT_TRUE(s.phase == Phase::Editing && s.expression.empty() &&
                s.display.empty() && !s.value && !s.repeat &&
                s.error == CALC_OK && s.errorOffset == SIZE_MAX);
  }
  void edited(const char *text) {
    const auto &s = controller.state();
    ASSERT_TRUE(s.phase == Phase::Editing && s.expression == text &&
                s.display.empty() && !s.value && !s.repeat &&
                s.error == CALC_OK && s.errorOffset == SIZE_MAX);
  }
  void action(Action a) { controller.dispatch(a); }
  void result(double v, const char *d, bool approximate = false) {
    const auto &s = controller.state();
    ASSERT_TRUE(s.phase == Phase::Result);
    ASSERT_TRUE(s.value);
    ASSERT_TRUE(s.error == CALC_OK && s.errorOffset == SIZE_MAX);
    ASSERT_TRUE(approximate ? fabs(*s.value - v) <= fmax(1e-15, 1e-12 * fabs(v))
                            : bits(*s.value) == bits(v));
    ASSERT_TRUE(s.display == d);
  }
  void error(uint32_t e) {
    const auto &s = controller.state();
    ASSERT_TRUE(s.phase == Phase::Error && s.error == e && !s.value &&
                !s.repeat && s.display.empty());
  }
};
class ControllerContract : public ::testing::Test {
protected:
  void SetUp() override {
    physical = 0;
    failPhysical = 0;
  }
};
TEST_F(ControllerContract, RepeatMatrix) {
  physical = 0;
  failPhysical = 0;
  struct Row {
    const char *text;
    char op;
    double rhs;
    double value[3];
    const char *display[3];
    bool approx;
  };
  const Row rows[] = {
      {"2+3*4", '*', 4, {14, 56, 224}, {"14", "56", "224"}, false},
      {"(2+3)*4", '*', 4, {20, 80, 320}, {"20", "80", "320"}, false},
      {"2*(3+4)", '*', 7, {14, 98, 686}, {"14", "98", "686"}, false},
      {"2+3", '+', 3, {5, 8, 11}, {"5", "8", "11"}, false},
      {"((2+3))", '+', 3, {5, 8, 11}, {"5", "8", "11"}, false},
      {"8/4/2", '/', 2, {1, .5, .25}, {"1", "0.5", "0.25"}, false},
      {"2*-3", '*', -3, {-6, 18, -54}, {"-6", "18", "-54"}, false},
      {"2+1e-3",
       '+',
       1e-3,
       {2.001, 2.002, 2.003},
       {"2.001", "2.002", "2.003"},
       true},
      {"-(2+3)", 0, 0, {-5, -5, -5}, {"-5", "-5", "-5"}, false},
      {"7", 0, 0, {7, 7, 7}, {"7", "7", "7"}, false}};
  for (size_t capacity : {size_t(64), size_t(0), size_t(1)})
    for (const auto &row : rows) {
      SCOPED_TRACE(::testing::Message()
                   << row.text << " capacity=" << capacity);
      Rig r(capacity);
      size_t start = physical;
      r.input(row.text);
      for (int j = 0; j < 3; ++j) {
        r.eq();
        r.result(row.value[j], row.display[j], row.approx);
        const auto &s = r.controller.state();
        if (row.op) {
          ASSERT_TRUE(s.repeat && s.repeat->operation == row.op &&
                      bits(s.repeat->rhs) == bits(row.rhs));
          ASSERT_TRUE(r.logical == size_t(2 + j));
        } else {
          ASSERT_TRUE(!s.repeat);
          ASSERT_TRUE(r.logical == 1);
        }
        ASSERT_TRUE(physical - start == r.logical * (capacity < 2 ? 2 : 1));
      }
    }
}
TEST_F(ControllerContract, EditedRepeat) {
  {
    Rig r;
    r.input("2+3*5");
    r.action(Action::Backspace);
    r.input("4");
    r.eq();
    r.result(14, "14");
    r.eq();
    r.result(56, "56");
    ASSERT_TRUE(r.logical == 3);
  }
}
TEST_F(ControllerContract, ClearRepeat) {
  {
    Rig r;
    r.input("2+3");
    r.eq();
    r.action(Action::Clear);
    r.ready();
    r.input("7");
    r.eq();
    size_t n = r.logical;
    r.eq();
    r.result(7, "7");
    ASSERT_TRUE(r.logical == n);
  }
}
TEST_F(ControllerContract, OperatorContinuation) {
  {
    Rig r;
    r.input("2+3");
    r.eq();
    r.input("*2");
    ASSERT_TRUE(!r.controller.state().repeat);
    r.eq();
    r.result(10, "10");
    r.eq();
    r.result(20, "20");
    ASSERT_TRUE(r.logical == 5);
  }
}
TEST_F(ControllerContract, ResultEditsClearRepeat) {
  for (Action edit : {Action::Input, Action::Backspace, Action::Sign}) {
    Rig r;
    r.input("2+3");
    r.eq();
    if (edit == Action::Input)
      r.input("7");
    else
      r.action(edit);
    ASSERT_TRUE(!r.controller.state().repeat && !r.controller.state().value);
    if (edit == Action::Backspace)
      r.input("7");
    r.eq();
    r.result(edit == Action::Sign ? -5 : 7, edit == Action::Sign ? "-5" : "7");
    size_t n = r.logical;
    r.eq();
    ASSERT_TRUE(n == r.logical);
  }
}
TEST_F(ControllerContract, ErrorRecovery) {
  for (const char *bad : {"1/0", "2+"})
    for (int recovery = 0; recovery < 4; ++recovery) {
      Rig r;
      r.input(bad);
      r.eq();
      r.error(bad[1] == '/' ? CALC_DIVIDE_BY_ZERO : CALC_SYNTAX);
      ASSERT_TRUE(r.logical == 1);
      if (recovery == 0) {
        r.action(Action::Clear);
        r.input("2+3");
        r.eq();
        r.result(5, "5");
      }
      if (recovery == 1) {
        r.input("7");
        r.eq();
        r.result(7, "7");
      }
      if (recovery == 2) {
        r.input(".5");
        r.eq();
        r.result(.5, "0.5");
      }
      if (recovery == 3) {
        r.action(Action::Backspace);
        r.input(bad[1] == '/' ? "2" : "+3");
        r.eq();
        r.result(bad[1] == '/' ? .5 : 5, bad[1] == '/' ? "0.5" : "5");
      }
    }
}
TEST_F(ControllerContract, RepeatOverflowRecovery) {
  {
    Rig r;
    r.input("1e154*1e154");
    r.eq();
    ASSERT_TRUE(r.controller.state().phase == Phase::Result);
    r.eq();
    r.error(CALC_RANGE);
    ASSERT_TRUE(r.logical == 3);
    r.input("7");
    r.eq();
    r.result(7, "7");
    size_t n = r.logical;
    r.eq();
    ASSERT_TRUE(n == r.logical);
  }
}
TEST_F(ControllerContract, RetainedThird) {
  {
    Rig r;
    r.input("1/3");
    r.eq();
    r.input("*3");
    r.eq();
    r.result(1, "1");
  }
}
TEST_F(ControllerContract, RetainedBinaryResidual) {
  {
    Rig r;
    r.input("0.1+0.2");
    r.eq();
    r.result(.3, "0.3", true);
    r.input("-0.3");
    r.eq();
    ASSERT_TRUE(r.controller.state().value);
    ASSERT_TRUE(bits(*r.controller.state().value) == bits(0x1p-54));
  }
}
TEST_F(ControllerContract, TransportBoundaries) {
  for (double x : {-0., std::numeric_limits<double>::denorm_min(),
                   std::numeric_limits<double>::min(),
                   std::numeric_limits<double>::max(), -1.25e-200}) {
    auto text = transport(x);
    ASSERT_TRUE(text);
    Adapter a(calc_abi_version, observed);
    auto v = a(*text);
    ASSERT_TRUE(v.ok() && bits(v.value) == bits(x));
  }
}
TEST_F(ControllerContract, DuplicateDecimal) {
  {
    Rig r;
    r.input("1.2.5");
    ASSERT_TRUE(r.controller.state().expression == "1.25");
    r.eq();
    r.result(1.25, "1.25");
  }
}
TEST_F(ControllerContract, DuplicateExponent) {
  {
    Rig r;
    r.input("1e2e");
    ASSERT_TRUE(r.controller.state().expression == "1e2");
    r.eq();
    r.result(100, "100");
  }
}
TEST_F(ControllerContract, Backspace) {
  {
    Rig r;
    r.input("123");
    r.action(Action::Backspace);
    r.eq();
    r.result(12, "12");
  }
}
TEST_F(ControllerContract, OperandSign) {
  {
    Rig r;
    r.input("2*3");
    r.action(Action::Sign);
    r.eq();
    r.result(-6, "-6");
  }
}
TEST_F(ControllerContract, ArithmeticFormatting) {
  // Independent arithmetic/format matrix through real Controller + Adapter +
  // Rust.
  struct Basic {
    const char *s;
    double v;
    const char *d;
  };
  for (auto row :
       {Basic{"7-10", -3, "-3"}, Basic{"6*7", 42, "42"}, Basic{"8/2", 4, "4"},
        Basic{"1.25+2.5", 3.75, "3.75"}, Basic{".5+5.", 5.5, "5.5"},
        Basic{"1.25e-3*1000", 1.25, "1.25"}, Basic{"-3+1", -2, "-2"},
        Basic{"(-3)*2", -6, "-6"}, Basic{"1e-6", 1e-6, "0.000001"},
        Basic{"1e14", 1e14, "100000000000000"}, Basic{"1e-7", 1e-7, "1e-7"},
        Basic{"1e15", 1e15, "1e15"}, Basic{"-0", -0., "0"},
        Basic{"1.2300", 1.23, "1.23"}}) {
    Rig r;
    r.input(row.s);
    r.eq();
    r.result(row.v, row.d);
  }
}
TEST_F(ControllerContract, SyntaxErrors) {
  for (const char *bad : {"(", ")", "()", "2(3)", "2*/3"}) {
    Rig r;
    r.input(bad);
    r.eq();
    r.error(CALC_SYNTAX);
  }
}
TEST_F(ControllerContract, RangeError) {
  {
    Rig r;
    r.input("1e309");
    r.eq();
    r.error(CALC_RANGE);
  }
}
TEST_F(ControllerContract, InjectedStatuses) {
  // Labelled injection: override an Evaluation at a controller boundary, while
  // all non-injected evaluations use the real adapter/producer. No injected
  // status is claimed as a Rust-produced failure.
  for (int stage = 1; stage <= 3; ++stage)
    for (uint32_t status : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 999u})
      for (int recovery = 0; recovery < 4; ++recovery) {
        SCOPED_TRACE(::testing::Message()
                     << "stage=" << stage << " recovery=" << recovery);
        Adapter adapter(calc_abi_version, observed);
        size_t calls = 0;
        bool injected = false;
        Controller c([&](const std::string &text) {
          ++calls;
          if (!injected && calls == size_t(stage)) {
            injected = true;
            return Evaluation{status, 0, {}, SIZE_MAX};
          }
          return adapter(text);
        });
        for (char ch : std::string("2+3"))
          c.dispatch(Action::Input, ch);
        c.dispatch(Action::Equals);
        if (stage == 3)
          c.dispatch(Action::Equals);
        ASSERT_TRUE(injected && calls == size_t(stage));
        ASSERT_TRUE(c.state().phase == Phase::Error &&
                    c.state().error == status && !c.state().value &&
                    !c.state().repeat && c.state().display.empty());
        if (recovery == 0) {
          c.dispatch(Action::Clear);
          c.dispatch(Action::Input, '7');
        }
        if (recovery == 1)
          c.dispatch(Action::Input, '7');
        if (recovery == 2) {
          c.dispatch(Action::Input, '.');
          c.dispatch(Action::Input, '5');
        }
        if (recovery == 3) {
          c.dispatch(Action::Backspace);
          if (stage == 3)
            c.dispatch(Action::Input, ')');
          else
            c.dispatch(Action::Input, '4');
        }
        c.dispatch(Action::Equals);
        ASSERT_TRUE(c.state().phase == Phase::Result && c.state().value);
        double expected = recovery == 2   ? .5
                          : recovery == 3 ? (stage == 3 ? 8 : 6)
                                          : 7;
        ASSERT_TRUE(*c.state().value == expected);
      }
}
TEST_F(ControllerContract, RetryFailureRecovery) {
  // Injected physical retry failure, with successful query and all other calls
  // forwarded to real Rust. Each recovery starts with an independent failure.
  for (int stage = 1; stage <= 3; ++stage)
    for (int recovery = 0; recovery < 4; ++recovery) {
      SCOPED_TRACE(::testing::Message()
                   << "stage=" << stage << " recovery=" << recovery);
      Rig r(0);
      size_t start = physical;
      failPhysical = start + size_t(stage * 2);
      r.input("2+3");
      r.eq();
      if (stage == 3)
        r.eq();
      r.error(CALC_INTERNAL);
      ASSERT_TRUE(physical == failPhysical);
      failPhysical = 0;
      if (recovery == 0) {
        r.action(Action::Clear);
        r.input("7");
      }
      if (recovery == 1)
        r.input("7");
      if (recovery == 2)
        r.input(".5");
      if (recovery == 3) {
        r.action(Action::Backspace);
        r.input(stage == 3 ? ")" : "4");
      }
      r.eq();
      r.result(recovery == 2   ? .5
               : recovery == 3 ? (stage == 3 ? 8 : 6)
                               : 7,
               recovery == 2   ? "0.5"
               : recovery == 3 ? (stage == 3 ? "8" : "6")
                               : "7");
    }
}
TEST_F(ControllerContract, TransportFailureRecovery) {
  // Null/throw/oversize transport failures: each stage and recovery gets a
  // fresh controller. Failed conversion never calls Rust and preserves current
  // text.
  for (int mode = 0; mode < 3; ++mode)
    for (bool continuation : {false, true})
      for (int recovery = 0; recovery < 4; ++recovery) {
        SCOPED_TRACE(::testing::Message()
                     << "mode=" << mode << " continuation=" << continuation
                     << " recovery=" << recovery);
        Adapter adapter(calc_abi_version, observed);
        size_t calls = 0;
        bool fail = false;
        Controller c(
            [&](const std::string &text) {
              ++calls;
              return adapter(text);
            },
            [&](double x) -> std::optional<std::string> {
              if (fail) {
                if (mode == 0)
                  return std::nullopt;
                if (mode == 1)
                  throw std::bad_alloc();
                return std::string(4095, '1');
              }
              return transport(x);
            });
        for (char ch : std::string("2+3"))
          c.dispatch(Action::Input, ch);
        c.dispatch(Action::Equals);
        size_t before = calls, physicalBefore = physical;
        fail = true;
        if (continuation)
          c.dispatch(Action::Input, '*');
        else
          c.dispatch(Action::Equals);
        ASSERT_TRUE(calls == before && physical == physicalBefore &&
                    c.state().phase == Phase::Error && !c.state().value &&
                    !c.state().repeat && c.state().display.empty());
        ASSERT_TRUE(c.state().error ==
                    (mode == 2 ? CALC_LIMIT : CALC_INTERNAL));
        ASSERT_TRUE(c.state().expression == "2+3");
        fail = false;
        if (recovery == 0) {
          c.dispatch(Action::Clear);
          ASSERT_TRUE(c.state().phase == Phase::Editing &&
                      c.state().expression.empty() &&
                      c.state().display.empty() && c.state().error == CALC_OK &&
                      c.state().errorOffset == SIZE_MAX && !c.state().value &&
                      !c.state().repeat);
          c.dispatch(Action::Input, '7');
        }
        if (recovery == 1)
          c.dispatch(Action::Input, '7');
        if (recovery == 2) {
          c.dispatch(Action::Input, '.');
          c.dispatch(Action::Input, '5');
        }
        if (recovery == 3) {
          c.dispatch(Action::Backspace);
          ASSERT_TRUE(c.state().expression == "2+");
          c.dispatch(Action::Input, '4');
        }
        ASSERT_TRUE(!c.state().value && !c.state().repeat &&
                    c.state().display.empty() &&
                    c.state().phase == Phase::Editing);
        c.dispatch(Action::Equals);
        const double expected = recovery == 2 ? .5 : recovery == 3 ? 6 : 7;
        const char *display = recovery == 2 ? "0.5" : recovery == 3 ? "6" : "7";
        ASSERT_TRUE(c.state().phase == Phase::Result && c.state().value &&
                    bits(*c.state().value) == bits(expected) &&
                    c.state().display == display &&
                    c.state().error == CALC_OK &&
                    c.state().errorOffset == SIZE_MAX);
        ASSERT_TRUE(calls == before + (recovery == 3 ? 2 : 1));
        ASSERT_TRUE(physical - physicalBefore == (recovery == 3 ? 2u : 1u));
        if (recovery == 3) {
          ASSERT_TRUE(c.state().repeat && c.state().repeat->operation == '+' &&
                      bits(c.state().repeat->rhs) == bits(4));
        } else {
          ASSERT_TRUE(!c.state().repeat);
          size_t count = calls;
          c.dispatch(Action::Equals);
          ASSERT_TRUE(calls == count && c.state().display == display &&
                      c.state().value &&
                      bits(*c.state().value) == bits(expected));
        }
      }
}
TEST_F(ControllerContract, PartialClear) {
  {
    Rig r;
    r.input("123.4");
    r.edited("123.4");
    size_t count = r.logical;
    r.action(Action::Clear);
    r.ready();
    ASSERT_TRUE(r.logical == count);
    r.input("2+3");
    r.eq();
    r.result(5, "5");
  }
}
TEST_F(ControllerContract, ResultClear) {
  {
    Rig r;
    r.input("2+3");
    r.eq();
    r.action(Action::Clear);
    r.ready();
  }
}
TEST_F(ControllerContract, DecimalBackspace) {
  {
    Rig r;
    r.input("12.3");
    r.action(Action::Backspace);
    r.edited("12.");
    r.action(Action::Backspace);
    r.edited("12");
    r.input(".5");
    r.eq();
    r.result(12.5, "12.5");
  }
}
TEST_F(ControllerContract, SignedBackspace) {
  {
    Rig r;
    r.input("-3");
    r.action(Action::Backspace);
    r.edited("-");
    r.input("2");
    r.eq();
    r.result(-2, "-2");
    r.action(Action::Clear);
    r.input("-");
    r.action(Action::Backspace);
    r.ready();
    r.input("2");
    r.eq();
    r.result(2, "2");
  }
}
TEST_F(ControllerContract, SignToggle) {
  {
    Rig r;
    r.input("3");
    r.action(Action::Sign);
    r.edited("-3");
    r.action(Action::Sign);
    r.edited("3");
    ASSERT_TRUE(r.logical == 0);
    r.eq();
    r.result(3, "3");
  }
}
TEST_F(ControllerContract, OperandSignToggle) {
  {
    Rig r;
    r.input("2*3");
    r.action(Action::Sign);
    r.edited("2*-3");
    r.action(Action::Sign);
    r.edited("2*3");
    r.eq();
    r.result(6, "6");
  }
}
TEST_F(ControllerContract, DecimalAfterResult) {
  {
    Rig r;
    r.input("2+3");
    r.eq();
    size_t count = r.logical;
    r.input(".");
    r.edited(".");
    ASSERT_TRUE(r.logical == count);
    r.input("5");
    r.eq();
    r.result(.5, "0.5");
    ASSERT_TRUE(!r.controller.state().repeat);
    count = r.logical;
    r.eq();
    ASSERT_TRUE(r.logical == count);
  }
}
TEST_F(ControllerContract, ContinuationAndFreshDigit) {
  {
    Rig r;
    r.input("5*2");
    r.eq();
    r.result(10, "10");
    r.input("+3");
    r.eq();
    r.result(13, "13");
    r.input("7");
    r.eq();
    r.result(7, "7");
  }
}
TEST_F(ControllerContract, SubnormalAndUnderflow) {
  {
    Rig r;
    r.input("5e-324");
    r.eq();
    r.result(std::numeric_limits<double>::denorm_min(),
             "4.94065645841247e-324");
    r.action(Action::Clear);
    r.input("-1e-999");
    r.eq();
    r.result(-0., "0");
  }
}
TEST_F(ControllerContract, KeyboardMapping) {
  ASSERT_TRUE(!keyAction('%'));
  ASSERT_TRUE(keyAction('\x1b')->action == Action::Clear);
  ASSERT_TRUE(keyAction('\n')->action == Action::Equals);
}
