#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <sstream>
#include <string>

class Scanner {
    const char *_content;
    uint64_t _save{};
    uint64_t _pos;
    uint64_t _len;

public:
    explicit Scanner(const char *content);

    [[nodiscard]] uint64_t pos() const;

    char pop();

    [[nodiscard]] char peek() const;

    void save();

    void restore();

    [[nodiscard]] int expect(char value) const;

    [[nodiscard]] int expect(const std::function<int(char)> &) const;

    int expect(const std::string &str);

    void advance();

    std::string take_while(const std::function<bool(char)> &cmp);

    std::string take_rest() const;

    std::string take_identifier();

    uint32_t skip_while(const std::function<bool(char)> &cmp);

    void skip_whitespace();

    template<typename T>
    std::optional<T> take_value() {
        const std::string content = this->take_while(
            [](const char value) { return std::isdigit(value) || value == '-' || value == '.'; });
        if (content.empty()) {
            return std::nullopt;
        }
        std::stringstream stream(content);
        T result;
        stream >> result;
        if (stream.fail()) {
            return std::nullopt;
        }
        return std::make_optional(result);
    }

    [[nodiscard]] bool isDone() const;
};
