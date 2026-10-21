#pragma once
#include "Arduino.h"
#include <map>
#include <vector>
namespace FakeNvs {
using Bytes = std::vector<uint8_t>;
inline std::map<std::string, std::map<std::string, Bytes>> data;
// Après N écritures réussies, toutes les suivantes échouent jusqu'au « reboot ».
inline int writesLeft = -1;
inline bool writable() { if (!writesLeft) return false; if (writesLeft > 0) --writesLeft; return true; }
inline void reset() { data.clear(); writesLeft = -1; }
}
class Preferences {
  std::string ns;
  bool readOnly = true;
  const FakeNvs::Bytes* get(const char* key) const {
    auto space = FakeNvs::data.find(ns);
    if (space == FakeNvs::data.end()) return nullptr;
    auto entry = space->second.find(key);
    return entry == space->second.end() ? nullptr : &entry->second;
  }
public:
  bool begin(const char* name, bool ro = false) { ns = name; readOnly = ro; return true; }
  void end() { ns.clear(); }
  bool isKey(const char* key) const { return get(key); }
  size_t getBytesLength(const char* key) const { auto bytes = get(key); return bytes ? bytes->size() : 0; }
  size_t getBytes(const char* key, void* output, size_t size) const {
    auto bytes = get(key); if (!bytes || size < bytes->size()) return 0;
    memcpy(output, bytes->data(), bytes->size()); return bytes->size();
  }
  size_t putBytes(const char* key, const void* input, size_t size) {
    if (readOnly || !FakeNvs::writable()) return 0;
    const auto* bytes = static_cast<const uint8_t*>(input);
    FakeNvs::data[ns][key] = FakeNvs::Bytes(bytes, bytes + size); return size;
  }
  size_t putString(const char* key, const String& input) {
    return putBytes(key, input.c_str(), input.length()+1) ? input.length() : 0;
  }
  String getString(const char* key, const char* fallback = "") const {
    auto bytes = get(key); return bytes ? String(reinterpret_cast<const char*>(bytes->data())) : String(fallback);
  }
  int getInt(const char* key, int fallback = 0) const {
    auto bytes = get(key); int value;
    if (!bytes || bytes->size() != sizeof(value)) return fallback;
    memcpy(&value, bytes->data(), sizeof(value)); return value;
  }
  size_t putInt(const char* key, int value) { return putBytes(key, &value, sizeof(value)); }
  bool getBool(const char* key, bool fallback = false) const {
    auto bytes = get(key); return bytes && bytes->size() == 1 ? (*bytes)[0] != 0 : fallback;
  }
  size_t putBool(const char* key, bool value) { uint8_t byte = value; return putBytes(key, &byte, 1); }
  bool remove(const char* key) {
    if (readOnly || !FakeNvs::writable()) return false;
    return FakeNvs::data[ns].erase(key) != 0;
  }
};
