#pragma once

class String {
    char *start, *end;

public:
    String();
    String(const String &s);
    String(String &&s);
    String & operator=(const String &s);
    String & operator=(String &&s);
    String(const char *s);
    explicit String(int n, int base = 10);
    ~String();

    unsigned int length() const { return end - start; }
    const char *data() const { return start; }
    const char *c_str() const { return start; }
    char *c_str() { return start; }
    void construct(const char *s, unsigned int len);
};

String operator+(const String &lhs, const String &rhs);
String operator+(const String &lhs, const char *rhs);
String operator+(const char *lhs, const String &rhs);

