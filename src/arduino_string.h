#pragma once

#include <cstddef>
#include <cstring>
#include <utility>

class String {
    char *start, *end;

    static String concat(const char *a, size_t la, const char *b, size_t lb);
    void construct(const char *s, unsigned int len);

    friend String operator+(const String &lhs, const String &rhs);
    friend String operator+(const String &lhs, const char *rhs);
    friend String operator+(const char *lhs, const String &rhs);

public:
    String() : start(nullptr), end(nullptr) { }
    String(const String &s) { operator=(s); }
    String(String &&s)      { operator=(std::move(s)); }
    String & operator=(const String &s) { construct(s.start, s.length()); return *this; }
    String & operator=(String &&s)
    {
        std::swap(start, s.start);
        std::swap(end, s.end);
        return *this;
    }

    String(const char *s) { construct(s, strlen(s) + 1); }
    explicit String(int n, int base = 10);

    ~String() { delete[] start; }

    unsigned int length() const { return end - start; }
    const char *data() const { return start; }
    const char *c_str() const { return start; }
    char *c_str() { return start; }

};

inline String operator+(const String &lhs, const String &rhs) { return String::concat(lhs.data(), lhs.length(), rhs.data(), rhs.length()); }
inline String operator+(const String &lhs, const char   *rhs) { return String::concat(lhs.data(), lhs.length(), rhs, std::strlen(rhs)); }
inline String operator+(const char   *lhs, const String &rhs) { return String::concat(lhs, std::strlen(lhs), rhs.data(), rhs.length()); }
