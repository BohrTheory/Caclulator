#include "calculator.hpp"
#include <algorithm>
#include <cmath>
#include <utility>

namespace calculator {
namespace {
bool digit(char c) { return c >= '0' && c <= '9'; }
bool binary(char c) { return c == '+' || c == '-' || c == '*' || c == '/'; }
bool whitespace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
// Locate the final numeric token (including exponent sign, excluding unary sign).
size_t numberStart(std::string_view text) {
    size_t i = text.size();
    while (i) {
        char c = text[i-1];
        if (digit(c) || c == '.' || c == 'e' || c == 'E') { --i; continue; }
        if ((c == '+' || c == '-') && i >= 2 && (text[i-2] == 'e' || text[i-2] == 'E')) { --i; continue; }
        break;
    }
    return i;
}
}
std::optional<LexicalRepeat> selectRepeat(std::string_view source) {
    std::string text;
    for (char c : source) if (!whitespace(c)) text += c;
    while (text.size() >= 2 && text.front() == '(' && text.back() == ')') {
        int depth = 0; bool whole = true;
        for (size_t i = 0; i < text.size(); ++i) {
            if (text[i] == '(') ++depth;
            if (text[i] == ')') --depth;
            if (depth == 0 && i != text.size() - 1) { whole = false; break; }
        }
        if (!whole || depth != 0) break;
        text = text.substr(1, text.size()-2);
    }
    int depth = 0; bool operandExpected = true;
    std::optional<size_t> selected;
    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '(') { ++depth; operandExpected = true; }
        else if (c == ')') { --depth; operandExpected = false; }
        else if (binary(c)) {
            if (i && (text[i-1] == 'e' || text[i-1] == 'E')) continue;
            if (depth == 0 && !operandExpected) selected = i;
            operandExpected = true;
        } else { operandExpected = false; }
    }
    if (!selected) return std::nullopt;
    return LexicalRepeat{text[*selected], text.substr(*selected+1)};
}
std::optional<KeyAction> keyAction(char key) {
    if (digit(key) || binary(key) || key == '.' || key == 'e' || key == 'E' || key == '(' || key == ')')
        return KeyAction{Action::Input, key};
    if (key == '=' || key == '\r' || key == '\n') return KeyAction{Action::Equals};
    if (key == '\b') return KeyAction{Action::Backspace};
    if (key == '\x1b') return KeyAction{Action::Clear};
    return std::nullopt;
}
Controller::Controller(Evaluate e, Transport t) : evaluate_(std::move(e)), convert_(std::move(t)) {}
void Controller::fail(uint32_t status, size_t offset) {
    state_.phase = Phase::Error;
    state_.error = status;
    state_.errorOffset = offset;
    state_.value.reset(); state_.repeat.reset(); state_.display.clear();
}
void Controller::edit() {
    state_.phase = Phase::Editing;
    state_.error = CALC_OK; state_.errorOffset = CALC_NO_ERROR_OFFSET;
    state_.repeat.reset(); state_.value.reset(); state_.display.clear();
}
std::optional<std::string> Controller::literal(double value) {
    auto text = convert_(value);
    if (!text || text->empty() || text->size() > CALC_MAX_INPUT_BYTES) return std::nullopt;
    return "(" + *text + ")";
}
void Controller::input(char c) {
    const auto mapped = keyAction(c);
    if (!mapped || mapped->action != Action::Input) return;
    std::string candidate = state_.expression;
    if (state_.phase == Phase::Result) {
        if (binary(c)) {
            auto text = literal(*state_.value);
            if (!text) { fail(CALC_INTERNAL); return; }
            candidate = *text;
        } else candidate.clear();
    } else if (state_.phase == Phase::Error && (digit(c) || c == '.')) candidate.clear();
    if (c == '.' || c == 'e' || c == 'E') {
        auto token = std::string_view(candidate).substr(numberStart(candidate));
        if (c == '.') {
            if (token.find_first_of(".eE") != std::string_view::npos) return;
        } else {
            if (token.find_first_of("eE") != std::string_view::npos ||
                std::none_of(token.begin(), token.end(), digit)) return;
        }
    }
    candidate += c;
    if (candidate.size() > CALC_MAX_INPUT_BYTES) { fail(CALC_LIMIT); return; }
    state_.expression = std::move(candidate); edit();
}
void Controller::sign() {
    if (state_.phase == Phase::Result) state_.expression = state_.display;
    auto& text = state_.expression;
    size_t start = numberStart(text);
    // A unary sign belongs to this entry only at start or after an operator/'('.
    if (start && (text[start-1] == '-' || text[start-1] == '+') &&
        (start == 1 || binary(text[start-2]) || text[start-2] == '(')) {
        if (text[start-1] == '-') text.erase(start-1, 1);
        else text[start-1] = '-';
    } else text.insert(start, 1, '-');
    edit();
}
void Controller::equals() {
    if (state_.phase == Phase::Result) {
        if (!state_.repeat) return;
        const auto repeat = *state_.repeat;
        const auto lhs = literal(*state_.value), rhs = literal(repeat.rhs);
        if (!lhs || !rhs) { fail(CALC_INTERNAL); return; }
        const std::string payload = *lhs + repeat.operation + *rhs;
        if (payload.size() > CALC_MAX_INPUT_BYTES) { fail(CALC_LIMIT); return; }
        state_.expression = payload; // preserve failed repeat for correction
        const auto result = evaluate_(payload);
        if (!result.ok() || !std::isfinite(result.value)) { fail(result.ok() ? CALC_INTERNAL : result.status, result.offset); return; }
        state_.value = result.value; state_.display = result.display;
        return;
    }
    state_.repeat.reset();
    const auto full = evaluate_(state_.expression); // own a copy before RHS call
    if (!full.ok() || !std::isfinite(full.value)) { fail(full.ok() ? CALC_INTERNAL : full.status, full.offset); return; }
    std::optional<Repeat> repeat;
    if (const auto selected = selectRepeat(state_.expression)) {
        const auto rhs = evaluate_(selected->rhs);
        if (!rhs.ok() || !std::isfinite(rhs.value)) { fail(rhs.ok() ? CALC_INTERNAL : rhs.status); return; }
        repeat = Repeat{selected->operation, rhs.value};
    }
    state_.phase = Phase::Result; state_.error = CALC_OK;
    state_.errorOffset = CALC_NO_ERROR_OFFSET;
    state_.value = full.value; state_.display = full.display; state_.repeat = repeat;
}
void Controller::dispatch(Action action, char ch) noexcept {
    try {
        switch (action) {
        case Action::Clear: state_ = {}; break;
        case Action::Input: input(ch); break;
        case Action::Equals: equals(); break;
        case Action::Sign: sign(); break;
        case Action::Backspace:
            if (state_.phase == Phase::Result) state_.expression = state_.display;
            if (!state_.expression.empty()) {
                size_t i = state_.expression.size()-1;
                while (i && (static_cast<unsigned char>(state_.expression[i]) & 0xc0) == 0x80) --i;
                state_.expression.erase(i);
            }
            edit(); break;
        }
    } catch (...) { fail(CALC_INTERNAL); }
}
}
