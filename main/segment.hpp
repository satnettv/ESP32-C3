#pragma once
#include <cstdint>

struct segment {
    uint8_t *data = 0;
    size_t size = 0;
    bool operator==(const char* str) {
        size_t pos = 0;
        while(*str) {
            if (size == pos) return false;
            if (data[pos] != *str) return false;
            ++str;
            ++pos;
        }
        return true;
    }
    operator bool() const {
        return size > 0;
    }
    uint8_t &operator[](size_t pos) const {
        return data[pos];
    }
    template <size_t S> void operator=(const char(&str)[S]) {
        data = (uint8_t*)str;
        size = S - 1;
    }
    void operator=(const segment &other) {
        data = other.data;
        size = other.size;
    }
    template <size_t S> segment(const char(&str)[S]) {
        *this = str;
    }
    segment(uint8_t *d, size_t s): data(d), size(s) {

    }
    segment(const std::string &str) {
        data = (uint8_t*)str.data();
        size = str.size();
    }
    segment(const char *str) {
        data = (uint8_t*)str;
        size = strlen(str);
    }
    segment() {

    }
    segment(const segment &other) {
        data = other.data;
        size = other.size;
    }
    void advance(size_t s) {
        data += s;
        size -= s;
    }
    uint8_t *begin() {
        return data;
    }
    uint8_t *end() {
        return data + size;
    }
    size_t move_to(segment &other) {
        auto sz = std::min(size, other.size);
        memcpy(other.data, data, sz);
        advance(sz);
        other.advance(sz);
        return sz;
    }
    size_t move_to(segment &&other) {
        auto sz = std::min(size, other.size);
        memcpy(other.data, data, sz);
        advance(sz);
        other.advance(sz);
        return sz;
    }
    void dump_hex() {
        printf("segment/");
        for(auto ch: *this) {
            printf("%02X ", ch);
        }
        printf("/segment\n");
    }
};
