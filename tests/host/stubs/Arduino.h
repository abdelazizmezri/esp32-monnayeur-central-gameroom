#pragma once
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <type_traits>
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
#define ARDUINOJSON_ENABLE_ARDUINO_PRINT 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_PROGMEM 0
class String {
  std::string value;
public:
  String() = default;
  String(const char* text) : value(text ? text : "") {}
  String(const std::string& text) : value(text) {}
  template<class T, typename = std::enable_if_t<std::is_arithmetic_v<T>>>
  String(T number) : value(std::to_string(number)) {}
  String& operator=(const char* text) { value = text ? text : ""; return *this; }
  const char* c_str() const { return value.c_str(); }
  size_t length() const { return value.length(); }
  bool isEmpty() const { return value.empty(); }
  char charAt(size_t index) const { return value[index]; }
  void reserve(size_t size) { value.reserve(size); }
  bool concat(const char* text) { value += text; return true; }
  void trim() {
    auto begin = value.find_first_not_of(" \t\r\n"), end = value.find_last_not_of(" \t\r\n");
    value = begin == std::string::npos ? "" : value.substr(begin, end - begin + 1);
  }
  void toLowerCase() { for (char& c : value) c = (char)std::tolower((unsigned char)c); }
  String& operator+=(const String& other) { value += other.value; return *this; }
  friend String operator+(String left, const String& right) { return left += right; }
  friend bool operator==(const String& left, const String& right) { return left.value == right.value; }
  friend bool operator!=(const String& left, const String& right) { return !(left == right); }
};
inline unsigned long millis() { return 100000; }
inline void configTzTime(const char*, const char*, const char*) {}
