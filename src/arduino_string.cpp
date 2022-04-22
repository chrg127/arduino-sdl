#include "arduino_string.h"

#include <cstdio>
#include <utility>
#include <cstring>
#include <charconv>

String::String() : start(nullptr), end(nullptr) { }
String::String(const String &s) { operator=(s); }
String::String(String &&s)      { operator=(std::move(s)); }

String & String::operator=(const String &s)
{
    construct(s.start, s.length());
    return *this;
}

String & String::operator=(String &&s)
{
    std::swap(start, s.start);
    std::swap(end, s.end);
    return *this;
}

String::String(const char *s)
{
    construct(s, strlen(s) + 1);
}

String::String(int n, int base)
{
    start = new char[1024];
    end = start + 1024;
    memset(start, 0, 1024);
    auto err = std::to_chars(start, end, n, base);
    if (err.ec != std::errc())
        fprintf(stderr, "warning: couldn't convert %d to string\n", n);
}

String::~String()
{
    delete[] start;
}

void String::construct(const char *s, unsigned int len)
{
    start = new char[len];
    std::memcpy(start, s, len);
    end = start + len;
    end[-1] = '\0';
}

String concat(const char *a, size_t la, const char *b, size_t lb)
{
    char *r = new char[la + lb + 1];
    std::memcpy(r, a, la);
    std::memcpy(r + la, b, lb);
    r[la+lb] = '\0';
    String s;
    s.construct(r, la+lb+1);
    return s;
}

String operator+(const String &lhs, const String &rhs)
{
    return concat(lhs.data(), lhs.length(), rhs.data(), rhs.length());
}

String operator+(const String &lhs, const char *rhs)
{
    return concat(lhs.data(), lhs.length(), rhs, std::strlen(rhs));
}

String operator+(const char *lhs, const String &rhs)
{
    return concat(lhs, std::strlen(lhs), rhs.data(), rhs.length());
}

