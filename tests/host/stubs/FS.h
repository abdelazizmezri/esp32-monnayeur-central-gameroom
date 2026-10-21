#pragma once
#include "Arduino.h"
#include <map>
#include <memory>
#include <vector>
#define FILE_READ "r"
#define FILE_WRITE "w"
#define FILE_APPEND "a"
class File {
  std::shared_ptr<std::vector<uint8_t>> bytes;
  size_t cursor = 0;
public:
  File() = default;
  File(std::shared_ptr<std::vector<uint8_t>> data, bool append) : bytes(data), cursor(append ? data->size() : 0) {}
  explicit operator bool() const { return bool(bytes); }
  size_t size() const { return bytes ? bytes->size() : 0; }
  int available() const { return bytes ? (int)(bytes->size() - cursor) : 0; }
  bool seek(size_t offset) { if (!bytes || offset > size()) return false; cursor = offset; return true; }
  size_t read(uint8_t* output, size_t count) {
    if (!bytes) return 0;
    count = std::min(count, size() - cursor); memcpy(output, bytes->data()+cursor, count); cursor += count; return count;
  }
  size_t write(const uint8_t* input, size_t count) {
    if (!bytes) return 0;
    if (cursor + count > size()) bytes->resize(cursor + count);
    memcpy(bytes->data() + cursor, input, count); cursor += count; return count;
  }
  void flush() {}
  int getWriteError() const { return 0; }
  void close() { bytes.reset(); }
};
