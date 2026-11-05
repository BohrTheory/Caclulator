#include "calculator.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <vector>

namespace calculator {
Adapter::Adapter(VersionFunction v, EvaluateFunction e, size_t c)
    : version_(v), evaluate_(e), capacity_(std::min<size_t>(c, CALC_DISPLAY_CAPACITY)) {}
Evaluation Adapter::operator()(const std::string& utf8) const noexcept {
    try {
        if (!version_ || !evaluate_ || (version_() >> 16) != (CALC_ABI_VERSION >> 16))
            return {CALC_INTERNAL, 0, {}, CALC_NO_ERROR_OFFSET};
        // Copy to stable owned storage; all outputs have separate live allocations.
        const std::string input = utf8;
        std::vector<char> display(capacity_);
        for (int attempt = 0; attempt < 2; ++attempt) {
            double value = 0;
            size_t required = 0, offset = CALC_NO_ERROR_OFFSET;
            const auto status = evaluate_(reinterpret_cast<const uint8_t*>(input.data()),
                input.size(), CALC_ANGLE_RAD, &value,
                display.empty() ? nullptr : display.data(), display.size(), &required, &offset);
            if (status == CALC_BUFFER_TOO_SMALL && attempt == 0 &&
                required > display.size() && required <= CALC_DISPLAY_CAPACITY) {
                display.assign(required, '\0');
                continue;
            }
            if (status != CALC_OK) return {status, 0, {}, offset};
            if (!std::isfinite(value) || required < 2 || required > display.size() ||
                offset != CALC_NO_ERROR_OFFSET || display[required - 1] != '\0')
                return {};
            // Producer promises canonical ASCII; reject malformed/partial strings.
            for (size_t i = 0; i + 1 < required; ++i) {
                char ch = display[i];
                if (!(ch >= '0' && ch <= '9') && ch != '-' && ch != '.' && ch != 'e') return {};
            }
            return {CALC_OK, value, std::string(display.data(), required - 1), offset};
        }
    } catch (...) { /* allocation/conversion failures stay on the C++ side */ }
    return {};
}
std::optional<std::string> transport(double value) {
    if (!std::isfinite(value)) return std::nullopt;
    if (value == 0 && std::signbit(value)) return "-0";
    char buffer[64];
    const auto result = std::to_chars(buffer, buffer + sizeof buffer, value,
        std::chars_format::general, std::numeric_limits<double>::max_digits10);
    if (result.ec != std::errc()) return std::nullopt;
    return std::string(buffer, result.ptr);
}
size_t utf16Position(std::string_view input, size_t offset) {
    if (offset == CALC_NO_ERROR_OFFSET) return offset;
    offset = std::min(offset, input.size());
    size_t units = 0;
    for (size_t i = 0; i < offset;) {
        auto ch = static_cast<unsigned char>(input[i]);
        size_t length = ch < 0x80 ? 1 : (ch >= 0xF0 ? 4 : (ch >= 0xE0 ? 3 : 2));
        if (i + length > offset) break;
        i += length;
        units += length == 4 ? 2 : 1;
    }
    return units;
}
std::string_view errorKey(uint32_t status) {
    switch (status) {
    case CALC_INVALID_ARGUMENT: return "Invalid calculator request";
    case CALC_INVALID_UTF8: return "Invalid text encoding";
    case CALC_SYNTAX: return "Check the expression";
    case CALC_DIVIDE_BY_ZERO: return "Cannot divide by zero";
    case CALC_DOMAIN: return "Value outside operation domain";
    case CALC_RANGE: return "Result outside finite range";
    case CALC_BUFFER_TOO_SMALL: return "Result display unavailable";
    case CALC_LIMIT: return "Expression is too long or too deeply nested";
    default: return "Calculation unavailable";
    }
}
}
