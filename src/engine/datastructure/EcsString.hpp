#pragma once
#include <cstddef>
#include <cstring>
#include <string>
#include <string_view>

namespace datastructures {

template <std::size_t N = 64> struct EcsString {
    char buf[N] = {};

    EcsString() = default;

    explicit EcsString(const char* s) {
        std::strncpy(this->buf, s, N);
        this->buf[N - 1] = '\0';
    }

    explicit EcsString(const std::string& s) : EcsString(s.c_str()) {
    }

    explicit EcsString(const std::string_view &s) {
        std::size_t len = s.size() < N - 1 ? s.size() : N - 1;
        std::memcpy(this->buf, s.data(), len);
        this->buf[len] = '\0';
    }

    EcsString& operator=(const char* s) {
        std::strncpy(this->buf, s, N);
        this->buf[N - 1] = '\0';
        return *this;
    }

    EcsString& operator=(const std::string& s) {
        return *this = s.c_str();
    }

    EcsString& operator=(std::string_view s) {
        std::size_t len = s.size() < N - 1 ? s.size() : N - 1;
        std::memcpy(this->buf, s.data(), len);
        this->buf[len] = '\0';
        return *this;
    }

    bool operator==(const EcsString& o) const {
        return std::strncmp(this->buf, o.buf, N) == 0;
    }

    bool operator==(const char* s) const {
        return std::strncmp(this->buf, s, N) == 0;
    }

    bool operator!=(const EcsString& o) const {
        return !(*this == o);
    }
    bool operator!=(const char* s) const {
        return !(*this == s);
    }

    [[nodiscard]] const char* c_str() const {
        return this->buf;
    }
    char* data() {
        return this->buf;
    }

    [[nodiscard]] std::size_t size() const {
        return std::strlen(this->buf);
    }
    [[nodiscard]] bool empty() const {
        return this->buf[0] == '\0';
    }

    static constexpr std::size_t capacity() {
        return N - 1;
    }

    [[nodiscard]] std::string str() const {
        return std::string(this->buf);
    }
    explicit  operator std::string_view() const {
        return std::string_view(this->buf);
    }
    explicit operator const char*() const {
        return this->buf;
    }
};

} // namespace datastructures
