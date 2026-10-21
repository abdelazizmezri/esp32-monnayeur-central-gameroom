#pragma once
#include "FS.h"
class FakeLittleFS {
public:
  std::map<std::string, std::shared_ptr<std::vector<uint8_t>>> files;
  bool begin(bool, const char*, int, const char*) { return true; }
  void end() {}
  bool exists(const char* path) { return files.count(path); }
  File open(const char* path, const char* mode) {
    if (!strcmp(mode, FILE_READ) && !exists(path)) return File();
    if (!exists(path) || !strcmp(mode, FILE_WRITE)) files[path] = std::make_shared<std::vector<uint8_t>>();
    return File(files[path], !strcmp(mode, FILE_APPEND));
  }
  bool remove(const char* path) { return files.erase(path); }
  bool rename(const char* before, const char* after) {
    if (!exists(before) || exists(after)) return false;
    files[after] = files[before]; files.erase(before); return true;
  }
  size_t totalBytes() { return 655360; }
  size_t usedBytes() { size_t used = 0; for (auto& item : files) used += item.second->size(); return used; }
};
inline FakeLittleFS LittleFS;
