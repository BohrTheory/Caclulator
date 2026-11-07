#pragma once
#include "calculator_abi_v1.h"
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace calculator {
struct Evaluation {
    uint32_t status = CALC_INTERNAL;
    double value = 0;
    std::string display;
    size_t offset = CALC_NO_ERROR_OFFSET;
    bool ok() const { return status == CALC_OK; }
};
using Evaluate = std::function<Evaluation(const std::string&)>;
using Transport = std::function<std::optional<std::string>(double)>;
using VersionFunction = decltype(&calc_abi_version);
using EvaluateFunction = decltype(&calc_evaluate_v1);
// Injected function pointers make boundary units possible without a Rust substitute app.
class Adapter {
public:
    Adapter(VersionFunction version, EvaluateFunction evaluate,
            size_t initialCapacity = CALC_DISPLAY_CAPACITY);
    Evaluation operator()(const std::string& utf8) const noexcept;
private:
    VersionFunction version_;
    EvaluateFunction evaluate_;
    size_t capacity_;
};
std::optional<std::string> transport(double value);
struct LexicalRepeat { char operation; std::string rhs; };
std::optional<LexicalRepeat> selectRepeat(std::string_view expression);
// UTF-8 byte offset -> UTF-16 position. An offset inside a sequence maps to its start.
size_t utf16Position(std::string_view utf8, size_t byteOffset);
std::string_view errorKey(uint32_t status);

enum class Phase { Editing, Result, Error };
struct Repeat { char operation; double rhs; };
struct State {
    Phase phase = Phase::Editing;
    std::string expression;
    std::string display;
    std::optional<double> value;
    std::optional<Repeat> repeat;
    uint32_t error = CALC_OK;
    size_t errorOffset = CALC_NO_ERROR_OFFSET;
};
enum class Action { Input, Clear, Backspace, Sign, Equals };
struct KeyAction { Action action; char input = 0; };
std::optional<KeyAction> keyAction(char key);
class Controller {
public:
    explicit Controller(Evaluate evaluate, Transport convert = transport);
    const State& state() const { return state_; }
    void dispatch(Action action, char input = 0) noexcept;
private:
    Evaluate evaluate_;
    Transport convert_;
    State state_;
    void fail(uint32_t status, size_t offset = CALC_NO_ERROR_OFFSET);
    void edit();
    void input(char ch);
    void equals();
    void sign();
    std::optional<std::string> literal(double value);
};
}
