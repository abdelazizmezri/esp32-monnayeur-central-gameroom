#include "RevenueService.h"

#include "AppConfig.h"

#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <time.h>

namespace {
constexpr size_t HEADER_SIZE = 16;
constexpr size_t RECORD_SIZE = 16;
constexpr char TEMP_REVENUE_FILE[] = "/revenue.tmp";
constexpr char BACKUP_REVENUE_FILE[] = "/revenue.bak";
constexpr unsigned long COMPACTION_CHECK_INTERVAL_MS = 3600000;
constexpr uint8_t FILE_HEADER[HEADER_SIZE] = {
  'P', 'S', 'T', 'M', 'R', 'E', 'V', '1',
  1, 0, 0, 0, RECORD_SIZE, 0, 0, 0
};

bool gFsReady = false;
unsigned long gLastFsAttempt = 0;
unsigned long gLastCompactionCheck = 0;
size_t gLastCompactedSize = 0;

bool fsWasInitialized() {
  Preferences prefs;
  prefs.begin("revenue", true);
  bool initialized = prefs.getBool("fsInit", false);
  prefs.end();
  return initialized;
}

void markFsInitialized() {
  Preferences prefs;
  prefs.begin("revenue", false);
  prefs.putBool("fsInit", true);
  prefs.end();
}

bool timeIsReady() {
  return time(nullptr) >= 1704067200;  // 2024-01-01 UTC
}

String formatTime(time_t value, const char* format) {
  if (value <= 0) return "";
  struct tm local;
  localtime_r(&value, &local);
  char buffer[40];
  if (strftime(buffer, sizeof(buffer), format, &local) == 0) return "";
  return String(buffer);
}

void writeU16(uint8_t* target, uint16_t value) {
  target[0] = (uint8_t)(value & 0xff);
  target[1] = (uint8_t)((value >> 8) & 0xff);
}

void writeU32(uint8_t* target, uint32_t value) {
  target[0] = (uint8_t)(value & 0xff);
  target[1] = (uint8_t)((value >> 8) & 0xff);
  target[2] = (uint8_t)((value >> 16) & 0xff);
  target[3] = (uint8_t)((value >> 24) & 0xff);
}

uint16_t readU16(const uint8_t* source) {
  return (uint16_t)source[0] | ((uint16_t)source[1] << 8);
}

uint32_t readU32(const uint8_t* source) {
  return (uint32_t)source[0] | ((uint32_t)source[1] << 8) |
         ((uint32_t)source[2] << 16) | ((uint32_t)source[3] << 24);
}

uint16_t crc16(const uint8_t* data, size_t length) {
  uint16_t crc = 0xffff;
  for (size_t index = 0; index < length; index++) {
    crc ^= (uint16_t)data[index] << 8;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021)
                           : (uint16_t)(crc << 1);
    }
  }
  return crc;
}

bool writeRecord(File& file, uint32_t epoch, uint32_t amount,
                 uint16_t pulses, uint16_t credits) {
  uint8_t record[RECORD_SIZE] = {};
  writeU32(record, epoch);
  writeU32(record + 4, amount);
  writeU16(record + 8, pulses);
  writeU16(record + 10, credits);
  writeU16(record + 14, crc16(record, 14));
  return file.write(record, RECORD_SIZE) == RECORD_SIZE;
}

bool validFileShape(const char* path) {
  File file = LittleFS.open(path, FILE_READ);
  if (!file || file.size() < HEADER_SIZE ||
      (file.size() - HEADER_SIZE) % RECORD_SIZE != 0) {
    if (file) file.close();
    return false;
  }
  uint8_t header[HEADER_SIZE];
  size_t read = file.read(header, HEADER_SIZE);
  file.close();
  return read == HEADER_SIZE && memcmp(header, FILE_HEADER, HEADER_SIZE) == 0;
}

void recoverInterruptedCompaction() {
  bool mainValid = LittleFS.exists(AppConfig::REVENUE_FILE) &&
                   validFileShape(AppConfig::REVENUE_FILE);
  if (mainValid) {
    if (LittleFS.exists(TEMP_REVENUE_FILE)) LittleFS.remove(TEMP_REVENUE_FILE);
    if (LittleFS.exists(BACKUP_REVENUE_FILE)) LittleFS.remove(BACKUP_REVENUE_FILE);
    return;
  }

  if (LittleFS.exists(AppConfig::REVENUE_FILE)) {
    LittleFS.remove(AppConfig::REVENUE_FILE);
  }
  if (LittleFS.exists(TEMP_REVENUE_FILE) && validFileShape(TEMP_REVENUE_FILE) &&
      LittleFS.rename(TEMP_REVENUE_FILE, AppConfig::REVENUE_FILE)) {
    if (LittleFS.exists(BACKUP_REVENUE_FILE)) LittleFS.remove(BACKUP_REVENUE_FILE);
    return;
  }
  if (LittleFS.exists(TEMP_REVENUE_FILE)) LittleFS.remove(TEMP_REVENUE_FILE);
  if (LittleFS.exists(BACKUP_REVENUE_FILE) && validFileShape(BACKUP_REVENUE_FILE)) {
    LittleFS.rename(BACKUP_REVENUE_FILE, AppConfig::REVENUE_FILE);
  }
}

bool validateRevenueFile(String& error) {
  recoverInterruptedCompaction();
  if (!LittleFS.exists(AppConfig::REVENUE_FILE)) {
    File file = LittleFS.open(AppConfig::REVENUE_FILE, FILE_WRITE);
    if (!file) {
      error = "Impossible de créer le journal LittleFS.";
      return false;
    }
    size_t written = file.write(FILE_HEADER, HEADER_SIZE);
    file.flush();
    file.close();
    if (written != HEADER_SIZE) {
      error = "Échec d'initialisation du journal LittleFS.";
      return false;
    }
    return true;
  }

  if (!validFileShape(AppConfig::REVENUE_FILE)) {
    error = "Format du journal LittleFS incompatible.";
    return false;
  }
  return true;
}

bool compactRevenueFile() {
  File input = LittleFS.open(AppConfig::REVENUE_FILE, FILE_READ);
  if (!input || input.size() < HEADER_SIZE) {
    if (input) input.close();
    return false;
  }
  input.seek(HEADER_SIZE);
  if (LittleFS.exists(TEMP_REVENUE_FILE)) LittleFS.remove(TEMP_REVENUE_FILE);
  File output = LittleFS.open(TEMP_REVENUE_FILE, FILE_WRITE);
  if (!output || output.write(FILE_HEADER, HEADER_SIZE) != HEADER_SIZE) {
    if (output) output.close();
    input.close();
    return false;
  }

  uint8_t record[RECORD_SIZE];
  uint32_t currentHour = UINT32_MAX;
  uint32_t outputEpoch = 0;
  uint32_t amount = 0;
  uint32_t pulses = 0;
  uint32_t credits = 0;
  bool ok = true;

  auto flushBucket = [&]() {
    if (currentHour == UINT32_MAX) return true;
    if (amount > UINT32_MAX || pulses > UINT16_MAX || credits > UINT16_MAX) {
      return false;
    }
    return writeRecord(output, outputEpoch, amount, (uint16_t)pulses,
                       (uint16_t)credits);
  };

  while (ok && input.available() >= (int)RECORD_SIZE) {
    if (input.read(record, RECORD_SIZE) != RECORD_SIZE) break;
    if (readU16(record + 14) != crc16(record, 14)) continue;
    uint32_t epoch = readU32(record);
    uint32_t recordAmount = readU32(record + 4);
    uint32_t recordPulses = readU16(record + 8);
    uint32_t recordCredits = readU16(record + 10);
    uint32_t hour = epoch == 0 ? 0 : epoch / 3600;
    bool overflow = (uint64_t)amount + recordAmount > UINT32_MAX ||
                    pulses + recordPulses > UINT16_MAX ||
                    credits + recordCredits > UINT16_MAX;
    if (currentHour != UINT32_MAX && (hour != currentHour || overflow)) {
      ok = flushBucket();
      amount = pulses = credits = 0;
    }
    if (currentHour == UINT32_MAX || hour != currentHour || overflow) {
      currentHour = hour;
      outputEpoch = epoch == 0 ? 0 : hour * 3600;
    }
    amount += recordAmount;
    pulses += recordPulses;
    credits += recordCredits;
  }
  if (ok) ok = flushBucket();
  input.close();
  output.flush();
  ok = ok && output.getWriteError() == 0;
  output.close();
  if (!ok || !validFileShape(TEMP_REVENUE_FILE)) {
    LittleFS.remove(TEMP_REVENUE_FILE);
    return false;
  }

  if (LittleFS.exists(BACKUP_REVENUE_FILE)) LittleFS.remove(BACKUP_REVENUE_FILE);
  if (!LittleFS.rename(AppConfig::REVENUE_FILE, BACKUP_REVENUE_FILE)) return false;
  if (!LittleFS.rename(TEMP_REVENUE_FILE, AppConfig::REVENUE_FILE)) {
    LittleFS.rename(BACKUP_REVENUE_FILE, AppConfig::REVENUE_FILE);
    return false;
  }
  LittleFS.remove(BACKUP_REVENUE_FILE);
  return true;
}

bool mountFs(AppState& state) {
  gLastFsAttempt = millis();
  bool initialized = fsWasInitialized();
  if (!LittleFS.begin(false, "/littlefs", 10, "littlefs")) {
    if (initialized ||
        !LittleFS.begin(true, "/littlefs", 10, "littlefs")) {
      gFsReady = false;
      state.revenueStorageReady = false;
      state.revenueStorageError = initialized
        ? "LittleFS illisible; formatage automatique refusé pour protéger la recette."
        : "Partition LittleFS indisponible.";
      return false;
    }
  }

  String error;
  if (!validateRevenueFile(error)) {
    LittleFS.end();
    gFsReady = false;
    state.revenueStorageReady = false;
    state.revenueStorageError = error;
    return false;
  }

  gFsReady = true;
  state.revenueStorageReady = true;
  state.revenueStorageError = "";
  if (!initialized) markFsInitialized();
  return true;
}

bool parseAnchorDate(const String& input, struct tm& value) {
  int year = 0;
  int month = 0;
  int day = 0;
  if (input.length() == 10 &&
      sscanf(input.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
    memset(&value, 0, sizeof(value));
    value.tm_year = year - 1900;
    value.tm_mon = month - 1;
    value.tm_mday = day;
    value.tm_hour = 12;
    value.tm_isdst = -1;
    time_t checked = mktime(&value);
    if (checked <= 0) return false;
    struct tm normalized;
    localtime_r(&checked, &normalized);
    return normalized.tm_year == year - 1900 &&
           normalized.tm_mon == month - 1 && normalized.tm_mday == day;
  }

  if (!input.isEmpty()) return false;
  time_t now = time(nullptr);
  if (now <= 0) return false;
  localtime_r(&now, &value);
  value.tm_hour = 12;
  value.tm_min = 0;
  value.tm_sec = 0;
  value.tm_isdst = -1;
  return true;
}

void initializeBuckets(const String& period, time_t start, int count,
                       std::vector<RevenueBucket>& buckets) {
  static const char* WEEKDAYS[] = {
    "Dim", "Lun", "Mar", "Mer", "Jeu", "Ven", "Sam"
  };
  buckets.clear();
  buckets.reserve(count);

  for (int index = 0; index < count; index++) {
    struct tm cursor;
    localtime_r(&start, &cursor);
    if (period == "day") cursor.tm_hour = index;
    else cursor.tm_mday += index;
    cursor.tm_isdst = -1;
    time_t bucketTime = mktime(&cursor);
    struct tm local;
    localtime_r(&bucketTime, &local);

    RevenueBucket bucket;
    if (period == "day") {
      char label[8];
      snprintf(label, sizeof(label), "%02d:00", local.tm_hour);
      bucket.label = label;
      bucket.key = formatTime(bucketTime, "%Y-%m-%dT%H:00");
    } else if (period == "week") {
      bucket.label = String(WEEKDAYS[local.tm_wday]) + " " +
                     formatTime(bucketTime, "%d/%m");
      bucket.key = formatTime(bucketTime, "%Y-%m-%d");
    } else {
      bucket.label = formatTime(bucketTime, "%d/%m");
      bucket.key = formatTime(bucketTime, "%Y-%m-%d");
    }
    buckets.push_back(bucket);
  }
}
}  // namespace

namespace RevenueService {
void begin(AppState& state) {
  configTzTime(AppConfig::TIMEZONE, AppConfig::NTP_SERVER_1,
               AppConfig::NTP_SERVER_2);
  state.revenueTimeReady = timeIsReady();
  mountFs(state);
}

void update(AppState& state) {
  state.revenueTimeReady = timeIsReady();
  if (!gFsReady &&
      millis() - gLastFsAttempt >= AppConfig::REVENUE_STORAGE_RETRY_INTERVAL_MS) {
    mountFs(state);
  }
  if (!gFsReady ||
      millis() - gLastCompactionCheck < COMPACTION_CHECK_INTERVAL_MS) return;

  gLastCompactionCheck = millis();
  size_t total = LittleFS.totalBytes();
  size_t used = LittleFS.usedBytes();
  File file = LittleFS.open(AppConfig::REVENUE_FILE, FILE_READ);
  size_t fileSize = file ? file.size() : 0;
  if (file) file.close();
  if (total > 0 && used * 100 >= total * 40 &&
      fileSize > gLastCompactedSize + 32768) {
    if (compactRevenueFile()) {
      File compacted = LittleFS.open(AppConfig::REVENUE_FILE, FILE_READ);
      gLastCompactedSize = compacted ? compacted.size() : fileSize;
      if (compacted) compacted.close();
    }
  }
}

bool recordPulses(AppState& state, int pulses, int generatedCredits,
                  const String& source) {
  (void)source;
  if (pulses <= 0) return true;
  if (!gFsReady) {
    if (millis() - gLastFsAttempt <
        AppConfig::REVENUE_STORAGE_RETRY_INTERVAL_MS) return false;
    if (!mountFs(state)) return false;
  }

  int64_t amount64 = (int64_t)pulses * state.pulseValueMillimes;
  if (amount64 < 0 || amount64 > UINT32_MAX || generatedCredits < 0 ||
      generatedCredits > UINT16_MAX || pulses > UINT16_MAX) {
    state.revenueStorageReady = false;
    state.revenueStorageError = "Valeur de recette hors limite.";
    return false;
  }

  uint8_t record[RECORD_SIZE] = {};
  writeU32(record, timeIsReady() ? (uint32_t)time(nullptr) : 0);
  writeU32(record + 4, (uint32_t)amount64);
  writeU16(record + 8, (uint16_t)pulses);
  writeU16(record + 10, (uint16_t)generatedCredits);
  writeU16(record + 14, crc16(record, 14));

  size_t totalBytes = LittleFS.totalBytes();
  size_t usedBytes = LittleFS.usedBytes();
  if (totalBytes == 0 || usedBytes > totalBytes ||
      totalBytes - usedBytes < RECORD_SIZE) {
    state.revenueStorageReady = false;
    state.revenueStorageError = "Partition LittleFS pleine.";
    return false;
  }

  File file = LittleFS.open(AppConfig::REVENUE_FILE, FILE_APPEND);
  if (!file) {
    state.revenueStorageReady = false;
    state.revenueStorageError = "Impossible d'ouvrir le journal LittleFS.";
    return false;
  }
  size_t written = file.write(record, RECORD_SIZE);
  file.flush();
  bool ok = written == RECORD_SIZE && file.getWriteError() == 0;
  file.close();

  if (!ok) {
    state.revenueStorageReady = false;
    state.revenueStorageError = "Échec d'écriture dans LittleFS.";
    return false;
  }
  state.revenueStorageReady = true;
  state.revenueStorageError = "";
  return true;
}

bool buildReport(AppState& state, const String& period,
                 const String& anchorDate, RevenueReport& report,
                 String& error) {
  if (period != "day" && period != "week" && period != "month") {
    error = "Période invalide.";
    return false;
  }

  struct tm anchor;
  if (!parseAnchorDate(anchorDate, anchor)) {
    error = "Date de référence invalide.";
    return false;
  }

  anchor.tm_hour = 0;
  anchor.tm_min = 0;
  anchor.tm_sec = 0;
  anchor.tm_isdst = -1;
  if (period == "week") {
    time_t normalized = mktime(&anchor);
    localtime_r(&normalized, &anchor);
    anchor.tm_mday -= (anchor.tm_wday + 6) % 7;
  } else if (period == "month") {
    anchor.tm_mday = 1;
  }
  time_t start = mktime(&anchor);

  struct tm endTm;
  localtime_r(&start, &endTm);
  int bucketCount = 0;
  if (period == "day") {
    bucketCount = 24;
    endTm.tm_mday += 1;
  } else if (period == "week") {
    bucketCount = 7;
    endTm.tm_mday += 7;
  } else {
    struct tm nextMonth = endTm;
    nextMonth.tm_mon += 1;
    nextMonth.tm_isdst = -1;
    time_t nextMonthTime = mktime(&nextMonth);
    bucketCount = (int)((nextMonthTime - start) / 86400);
    endTm = nextMonth;
  }
  endTm.tm_isdst = -1;
  time_t end = mktime(&endTm);

  report = RevenueReport();
  report.period = period;
  report.storageReady = gFsReady && state.revenueStorageReady;
  report.timeReady = timeIsReady();
  report.storageUsedBytes = gFsReady ? LittleFS.usedBytes() : 0;
  report.storageTotalBytes = gFsReady ? LittleFS.totalBytes() : 0;
  report.startDate = formatTime(start, "%Y-%m-%d");
  report.endDate = formatTime(end - 1, "%Y-%m-%d");
  report.generatedAt = report.timeReady
                         ? formatTime(time(nullptr), "%Y-%m-%dT%H:%M:%S%z")
                         : "Horloge non synchronisée";
  if (period == "day") {
    report.rangeLabel = formatTime(start, "%d/%m/%Y");
  } else if (period == "week") {
    report.rangeLabel = formatTime(start, "%d/%m/%Y") + " — " +
                        formatTime(end - 1, "%d/%m/%Y");
  } else {
    static const char* MONTHS[] = {
      "janvier", "février", "mars", "avril", "mai", "juin",
      "juillet", "août", "septembre", "octobre", "novembre", "décembre"
    };
    struct tm local;
    localtime_r(&start, &local);
    report.rangeLabel = String(MONTHS[local.tm_mon]) + " " +
                        String(local.tm_year + 1900);
  }
  initializeBuckets(period, start, bucketCount, report.buckets);

  if (!gFsReady) return true;
  File file = LittleFS.open(AppConfig::REVENUE_FILE, FILE_READ);
  if (!file || file.size() < HEADER_SIZE) {
    if (file) file.close();
    LittleFS.end();
    gFsReady = false;
    state.revenueStorageReady = false;
    state.revenueStorageError = "Impossible de lire le journal LittleFS.";
    report.storageReady = false;
    return true;
  }
  report.fileBytes = file.size();
  file.seek(HEADER_SIZE);

  uint8_t record[RECORD_SIZE];
  while (file.available() >= (int)RECORD_SIZE) {
    if (file.read(record, RECORD_SIZE) != RECORD_SIZE) break;
    if (readU16(record + 14) != crc16(record, 14)) {
      report.corruptRecords++;
      continue;
    }
    time_t epoch = (time_t)readU32(record);
    int64_t amount = readU32(record + 4);
    uint32_t pulses = readU16(record + 8);
    uint32_t credits = readU16(record + 10);
    if (epoch <= 0) {
      report.undatedMillimes += amount;
      report.undatedEvents++;
      continue;
    }
    if (epoch < start || epoch >= end) continue;

    int index = 0;
    if (period == "day") {
      struct tm local;
      localtime_r(&epoch, &local);
      index = local.tm_hour;
    } else {
      index = (int)((epoch - start) / 86400);
    }
    if (index < 0 || index >= (int)report.buckets.size()) continue;

    RevenueBucket& bucket = report.buckets[index];
    bucket.amountMillimes += amount;
    bucket.pulses += pulses;
    bucket.credits += credits;
    bucket.events++;
    report.totalMillimes += amount;
    report.pulses += pulses;
    report.credits += credits;
    report.events++;
  }
  file.close();
  return true;
}

String buildCsv(const RevenueReport& report) {
  String csv;
  csv.reserve(256 + report.buckets.size() * 64);
  csv += "Periode;\"" + report.period + "\"\r\n";
  csv += "Plage;\"" + report.rangeLabel + "\"\r\n";
  csv += "Total_TND;" + String((double)report.totalMillimes / 1000.0, 3) + "\r\n";
  csv += "Impulsions;" + String(report.pulses) + "\r\n";
  csv += "Credits_generes;" + String(report.credits) + "\r\n\r\n";
  csv += "Cle;Libelle;Recette_TND;Impulsions;Credits;Evenements\r\n";
  for (const auto& bucket : report.buckets) {
    csv += "\"" + bucket.key + "\";\"" + bucket.label + "\";";
    csv += String((double)bucket.amountMillimes / 1000.0, 3) + ";";
    csv += String(bucket.pulses) + ";" + String(bucket.credits) + ";";
    csv += String(bucket.events) + "\r\n";
  }
  if (report.undatedEvents > 0) {
    csv += "NON_DATE;Impulsions avant synchronisation;";
    csv += String((double)report.undatedMillimes / 1000.0, 3) + ";;;;\r\n";
  }
  return csv;
}
}  // namespace RevenueService
