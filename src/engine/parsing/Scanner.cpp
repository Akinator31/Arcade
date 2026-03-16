#include "Scanner.hpp"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <functional>
#include <string>

Scanner::Scanner(const char *content) : _content(content), _pos(0), _len(strlen(content)) {
}

bool Scanner::isDone() const {
    return this->_pos >= this->_len;
}

void Scanner::advance() {
    this->_pos += 1;
}

char Scanner::pop() {
    const char value = this->_content[this->_pos];
    this->_pos += 1;
    return value;
}

char Scanner::peek() const {
    return this->_content[this->_pos];
}

uint32_t Scanner::skip_while(const std::function<bool(char)> &cmp) {
    uint32_t count = 0;
    while (!this->isDone() && cmp(this->peek())) {
        count += 1;
        this->advance();
    }
    return count;
}

uint64_t Scanner::pos() const {
    return this->_pos;
}

void Scanner::skip_whitespace() {
    this->skip_while(static_cast<int (*)(int)>(std::isspace));
}

std::string Scanner::take_while(const std::function<bool(char)> &cmp) {
    std::string str;
    while (!this->isDone() && cmp(this->peek())) {
        str.push_back(this->peek());
        this->advance();
    }
    return str;
}

std::string Scanner::take_identifier() {
    if (!this->expect(isalpha) && !this->expect('_')) {
        return {};
    }

    std::string content = this->take_while([](char c) { return isalnum(c) || c == '_' || c == '<' || c == '>'; });
    return content;
}

int Scanner::expect(const char value) const {
    return this->peek() == value;
}

[[nodiscard]] int Scanner::expect(const std::function<int(char)> &cmp) const {
    return cmp(this->peek());
}

void Scanner::save() {
    this->_save = this->_pos;
}

void Scanner::restore() {
    this->_pos = this->_save;
}

int Scanner::expect(const std::string &str) {
    this->save();
    const bool result = std::ranges::all_of(str, [this](const char value) {
        if (this->expect(value)) {
            this->advance();
            return true;
        }
        return false;
    });
    if (!result) {
        this->restore();
    }
    return true;
}

std::string Scanner::take_rest() const {
    return &this->_content[this->_pos];
}
