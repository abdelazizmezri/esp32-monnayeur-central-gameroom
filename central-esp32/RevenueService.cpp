#include "RevenueService.h"
#include "AppConfig.h"
#include "StorageService.h"
#include "RevenueJournal.h"
#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <time.h>

namespace {
using namespace RevenueJournal;
constexpr char TEMP_REVENUE_FILE[] = "/revenue.tmp";
constexpr char BACKUP_REVENUE_FILE[] = "/revenue.bak";
constexpr unsigned long COMPACTION_CHECK_INTERVAL_MS = 3600000;
bool gFsReady = false;
bool gJournalClean = false;
unsigned long gLastFsAttempt = 0, gLastCompactionCheck = 0;
size_t gLastCompactedSize = 0;
uint64_t gLastSequence = 0;

bool timeIsReady() { return time(nullptr) >= 1704067200; }
String formatTime(time_t value, const char* format) {
  if (value <= 0) return "";
  struct tm local;
  localtime_r(&value,&local);
  char buffer[40];
  return strftime(buffer,sizeof(buffer),format,&local) ? String(buffer) : String("");
}

bool fail(AppState& state, const String& error) {
  state.revenueStorageReady = false;
  state.revenueStorageError = error;
  return false;
}

// Une fin déchirée est réparable ; une corruption intérieure bloque le journal.
// Aucune ancienne recette valide n'est supprimée pour recréer un fichier vide.
bool scanFile(const char* path, bool& legacy, size_t& validEnd,
              bool& tornTail, Entry& latest) {
  File file = LittleFS.open(path,FILE_READ);
  uint8_t header[HEADER_SIZE];
  if (!file || file.read(header,HEADER_SIZE) != HEADER_SIZE) { if(file) file.close(); return false; }
  legacy = memcmp(header,LEGACY_HEADER,HEADER_SIZE) == 0;
  if (!legacy && memcmp(header,HEADER,HEADER_SIZE)) { file.close(); return false; }
  size_t recordSize = legacy ? 16 : RECORD_SIZE;
  validEnd = HEADER_SIZE; tornTail = false; latest = Entry();
  uint8_t bytes[RECORD_SIZE];
  while (file.available() >= (int)recordSize) {
    Entry e;
    if (file.read(bytes,recordSize) != recordSize) { file.close(); return false; }
    bool valid = legacy ? decodeLegacy(bytes,e) : decode(bytes,e);
    if (!valid) {
      Entry pending;
      // Un bloc complet mais invalide n'est supprimé que si sa préparation
      // durable permet de le reconstruire. Sinon conserver la preuve de corruption.
      tornTail = file.available() == 0 && !legacy &&
                 StorageService::loadRevenueTransaction(pending) == 1 &&
                 pending.sequence == latest.sequence+1;
      file.close();
      return tornTail;
    }
    if (e.sequence > latest.sequence) latest = e;
    validEnd += recordSize;
  }
  tornTail = file.available() != 0;
  file.close();
  return true;
}

bool completeFile(const char* path) {
  bool legacy, tail; size_t end; Entry last;
  return scanFile(path,legacy,end,tail,last) && !tail;
}

bool writeEntry(File& file, const Entry& e) {
  uint8_t bytes[RECORD_SIZE]; encode(e,bytes);
  return file.write(bytes,sizeof(bytes)) == sizeof(bytes);
}

bool promoteTemp() {
  if (!completeFile(TEMP_REVENUE_FILE)) return false;
  bool hadMain = LittleFS.exists(AppConfig::REVENUE_FILE);
  if (hadMain) {
    if (LittleFS.exists(BACKUP_REVENUE_FILE) && !LittleFS.remove(BACKUP_REVENUE_FILE)) return false;
    if (!LittleFS.rename(AppConfig::REVENUE_FILE,BACKUP_REVENUE_FILE)) return false;
  }
  if (!LittleFS.rename(TEMP_REVENUE_FILE,AppConfig::REVENUE_FILE)) {
    if (hadMain) LittleFS.rename(BACKUP_REVENUE_FILE,AppConfig::REVENUE_FILE);
    return false;
  }
  if (hadMain) LittleFS.remove(BACKUP_REVENUE_FILE);
  return true;
}

bool recoverFiles() {
  // Le fichier principal reste prioritaire, y compris avec une dernière écriture incomplète.
  if (LittleFS.exists(AppConfig::REVENUE_FILE)) return true;
  if (LittleFS.exists(TEMP_REVENUE_FILE) && completeFile(TEMP_REVENUE_FILE))
    return LittleFS.rename(TEMP_REVENUE_FILE,AppConfig::REVENUE_FILE);
  if (LittleFS.exists(BACKUP_REVENUE_FILE))
    return completeFile(BACKUP_REVENUE_FILE) &&
           LittleFS.rename(BACKUP_REVENUE_FILE,AppConfig::REVENUE_FILE);
  // Première initialisation : aucun journal déjà engagé n'existe.
  File file = LittleFS.open(TEMP_REVENUE_FILE,FILE_WRITE);
  if (!file) return false;
  bool ok = file.write(HEADER,HEADER_SIZE) == HEADER_SIZE;
  file.flush(); ok = ok && file.getWriteError() == 0; file.close();
  return ok && promoteTemp();
}

bool rewriteFile(bool compact, bool legacy, size_t validEnd) {
  File input = LittleFS.open(AppConfig::REVENUE_FILE,FILE_READ);
  File output = LittleFS.open(TEMP_REVENUE_FILE,FILE_WRITE);
  if (!input || !output) { if(input) input.close(); if(output) output.close(); return false; }
  bool ok = output.write(HEADER,HEADER_SIZE) == HEADER_SIZE;
  input.seek(HEADER_SIZE);
  uint8_t bytes[RECORD_SIZE];
  size_t recordSize = legacy ? 16 : RECORD_SIZE;
  Entry aggregate;
  bool hasAggregate = false;
  for (size_t offset = HEADER_SIZE; ok && offset < validEnd; offset += recordSize) {
    Entry e;
    ok = input.read(bytes,recordSize) == recordSize &&
         (legacy ? decodeLegacy(bytes,e) : decode(bytes,e));
    if (!ok) break;
    if (compact && hasAggregate && merge(aggregate,e)) continue;
    if (hasAggregate) ok = writeEntry(output,aggregate);
    aggregate = e; hasAggregate = true;
  }
  if (ok && hasAggregate) ok = writeEntry(output,aggregate);
  input.close(); output.flush(); ok = ok && output.getWriteError() == 0; output.close();
  return ok && promoteTemp();
}

bool validateRevenueFile(AppState& state) {
  if (!recoverFiles()) return fail(state,"Impossible de récupérer le journal LittleFS.");
  bool legacy, tail; size_t end; Entry latest;
  if (!scanFile(AppConfig::REVENUE_FILE,legacy,end,tail,latest))
    return fail(state,"Journal incompatible ou corruption intérieure ; fichier conservé.");
  if ((legacy || tail) && !rewriteFile(false,legacy,end))
    return fail(state,"Échec de migration/réparation ; anciennes recettes conservées.");
  // Nettoyer uniquement après validation du principal et récupération réussie.
  if (LittleFS.exists(TEMP_REVENUE_FILE)) LittleFS.remove(TEMP_REVENUE_FILE);
  if (LittleFS.exists(BACKUP_REVENUE_FILE)) LittleFS.remove(BACKUP_REVENUE_FILE);
  gLastSequence = latest.sequence;
  gJournalClean = true;
  return true;
}

bool mountFs(AppState& state) {
  gLastFsAttempt = millis();
  Preferences prefs;
  prefs.begin("revenue",true); bool initialized = prefs.getBool("fsInit",false); prefs.end();
  if (!LittleFS.begin(false,"/littlefs",10,"littlefs")) {
    if (initialized || !LittleFS.begin(true,"/littlefs",10,"littlefs")) {
      gFsReady = false;
      return fail(state,"LittleFS indisponible ; aucun formatage d'un stockage déjà initialisé.");
    }
  }
  // Ne jamais accepter une recette tant que le marqueur anti-formatage n'est pas durable.
  if (!prefs.begin("revenue",false)) {
    LittleFS.end(); gFsReady = false;
    return fail(state,"NVS indisponible ; initialisation du stockage suspendue.");
  }
  bool marked = prefs.putBool("fsInit",true) == 1 && prefs.getBool("fsInit",false);
  prefs.end();
  if (!marked) {
    LittleFS.end(); gFsReady = false;
    return fail(state,"Impossible de protéger LittleFS contre un formatage automatique.");
  }
  if (!validateRevenueFile(state)) { LittleFS.end(); gFsReady = false; return false; }
  gFsReady = true; state.revenueStorageReady = true; state.revenueStorageError = "";
  return true;
}

bool appendEntry(AppState& state, const Entry& e) {
  if (!gJournalClean && !validateRevenueFile(state)) return false;
  if (gLastSequence >= e.sequence) return true; // écriture déjà engagée avant la coupure
  size_t total = LittleFS.totalBytes(), used = LittleFS.usedBytes();
  if (!total || used > total || total-used < RECORD_SIZE+4096)
    return fail(state,"Partition LittleFS pleine ; transaction conservée en NVS.");
  File file = LittleFS.open(AppConfig::REVENUE_FILE,FILE_APPEND);
  if (!file) return fail(state,"Impossible d'ouvrir le journal LittleFS.");
  size_t offset = file.size();
  gJournalClean = false;
  bool ok = writeEntry(file,e);
  file.flush(); ok = ok && file.getWriteError() == 0; file.close();
  // Relecture binaire après fermeture, pour détecter une écriture partielle.
  uint8_t expected[RECORD_SIZE], actual[RECORD_SIZE]; encode(e,expected);
  File check = LittleFS.open(AppConfig::REVENUE_FILE,FILE_READ);
  ok = ok && check && check.size() == offset+RECORD_SIZE && check.seek(offset) &&
       check.read(actual,RECORD_SIZE) == RECORD_SIZE && !memcmp(expected,actual,RECORD_SIZE);
  if (check) check.close();
  if (!ok) return fail(state,"Échec d'écriture LittleFS ; transaction conservée en NVS.");
  gLastSequence = e.sequence; gJournalClean = true;
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
} // namespace

namespace RevenueService {
bool recoverPending(AppState& state) {
  if (!state.creditStorageReady) return fail(state,"Instantané des crédits NVS indisponible ; comptage suspendu.");
  Entry e;
  int status = StorageService::loadRevenueTransaction(e);
  state.revenueTransactionPending = status != 0;
  if (status < 0) return fail(state,"Transaction NVS illisible ; reprise suspendue.");
  if (!gFsReady) return false;
  if (!status) {
    if (state.legacyRevenueSequence)
      return fail(state,"Transaction historique attendue mais absente ; reprise suspendue.");
    if (gLastSequence > state.lastRevenueSequence)
      return fail(state,"Journal et crédits incohérents ; reprise automatique suspendue.");
    state.revenueStorageReady = true; state.revenueStorageError = "";
    return true;
  }
  if (e.sequence > state.lastRevenueSequence && e.sequence != state.lastRevenueSequence+1)
    return fail(state,"Séquence de transaction NVS incohérente.");
  if (!appendEntry(state,e)) return false;
  if (e.sequence > state.lastRevenueSequence) {
    AppState next = state;
    if (e.balanceAfter > INT32_MAX || e.pendingAfter > INT32_MAX || e.pulsesPerCoin > INT32_MAX)
      return fail(state,"Transaction hors limite ; crédits conservés.");
    if (state.legacyRevenueSequence == e.sequence) {
      // Le solde initial V4 contient déjà tous les anciens reliquats. Seules
      // les impulsions de cette préparation sont encore à créditer, une fois.
      if ((uint64_t)state.availablePieces + e.pulses > INT32_MAX)
        return fail(state,"Migration de transaction hors limite.");
      next.availablePieces = state.availablePieces + e.pulses;
    } else {
      if (e.pulsesPerCoin != 1 || e.pendingAfter != 0)
        return fail(state,"Unité de transaction incompatible avec le solde V4.");
      next.availablePieces = e.balanceAfter;
    }
    next.legacyRevenueSequence = 0;
    next.lastRevenueSequence = e.sequence;
    if (!StorageService::saveCredits(next))
      return fail(state,"Échec NVS des crédits ; transaction conservée pour reprise.");
    state.availablePieces = next.availablePieces;
    state.legacyRevenueSequence = 0;
    state.lastRevenueSequence = next.lastRevenueSequence;
  }
  if (!StorageService::clearRevenueTransaction()) return fail(state,"Finalisation NVS en attente.");
  state.revenueTransactionPending = false;
  state.revenueStorageReady = true; state.revenueStorageError = "";
  return true;
}

void begin(AppState& state) {
  configTzTime(AppConfig::TIMEZONE,AppConfig::NTP_SERVER_1,AppConfig::NTP_SERVER_2);
  state.revenueTimeReady = timeIsReady();
  if (mountFs(state)) recoverPending(state);
}

void update(AppState& state) {
  state.revenueTimeReady = timeIsReady();
  if (!gFsReady && millis()-gLastFsAttempt >= AppConfig::REVENUE_STORAGE_RETRY_INTERVAL_MS) {
    if (mountFs(state)) recoverPending(state);
  }
  if (!gFsReady || state.revenueTransactionPending || !state.creditStorageReady ||
      millis()-gLastCompactionCheck < COMPACTION_CHECK_INTERVAL_MS) return;
  gLastCompactionCheck = millis();
  File file = LittleFS.open(AppConfig::REVENUE_FILE,FILE_READ);
  size_t size = file ? file.size() : 0; if(file) file.close();
  if (LittleFS.totalBytes() && LittleFS.usedBytes()*100 >= LittleFS.totalBytes()*40 &&
      size > gLastCompactedSize+32768) {
    bool legacy,tail; size_t end; Entry last;
    if (scanFile(AppConfig::REVENUE_FILE,legacy,end,tail,last) && !tail &&
        rewriteFile(true,legacy,end)) {
      File result = LittleFS.open(AppConfig::REVENUE_FILE,FILE_READ);
      gLastCompactedSize = result ? result.size() : size; if(result) result.close();
    }
  }
}

bool recordPulses(AppState& state, int pulses, uint8_t currency,
                  uint32_t value, uint32_t rate, uint32_t epoch) {
  if (pulses <= 0 || !recoverPending(state) || !Money::validValue(currency,value) ||
      (currency == Money::DZD && !Money::validRate(rate))) return false;
  uint64_t credits = pulses;
  if (credits + state.availablePieces > INT32_MAX || state.lastRevenueSequence == UINT64_MAX)
    return fail(state,"Solde ou séquence hors limite.");
  Entry e;
  e.epoch = epoch; e.currency = currency; e.pulseValue = value;
  e.rate = currency == Money::DZD ? rate : 0;
  e.nativeAmount = (uint64_t)pulses*value;
  e.convertedMillimes = (uint64_t)pulses*Money::toMillimes(currency,value,rate);
  e.pulses = pulses; e.credits = credits; e.sequence = state.lastRevenueSequence+1;
  e.balanceAfter = state.availablePieces+credits; e.pendingAfter = 0;
  e.pulsesPerCoin = 1; // Compatibilité binaire REV2, les nouveaux crédits sont des impulsions.
  if (!StorageService::stageRevenueTransaction(e))
    return fail(state,"Échec NVS de préparation ; impulsions encore en mémoire vive.");
  state.revenueTransactionPending = true;
  // L'appelant peut maintenant retirer ces impulsions de sa file RAM : elles
  // sont durables en NVS même si l'étape LittleFS suivante échoue.
  recoverPending(state);
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
  report.currentRateMicros = state.dzdToTndRateMicros;
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
    if (file.read(record,RECORD_SIZE) != RECORD_SIZE) break;
    Entry e;
    if (!decode(record,e)) { report.corruptRecords++; continue; }
    time_t epoch = e.epoch;
    if (epoch <= 0) {
      report.undatedMillimes += e.convertedMillimes;
      if (e.currency == Money::DZD) report.undatedDzdCentimes += e.nativeAmount;
      else report.undatedTndMillimes += e.nativeAmount;
      report.undatedEvents += e.events;
      continue;
    }
    if (epoch < start || epoch >= end) continue;
    int index = 0;
    if (period == "day") { struct tm local; localtime_r(&epoch,&local); index = local.tm_hour; }
    else index = (int)((epoch-start)/86400);
    if (index < 0 || index >= (int)report.buckets.size()) continue;
    RevenueBucket& bucket = report.buckets[index];
    bucket.amountMillimes += e.convertedMillimes;
    if (e.currency == Money::DZD) {
      bucket.dzdCentimes += e.nativeAmount; report.totalDzdCentimes += e.nativeAmount;
    } else {
      bucket.tndMillimes += e.nativeAmount; report.totalTndMillimes += e.nativeAmount;
    }
    bucket.pulses += e.pulses; bucket.credits += e.credits; bucket.events += e.events;
    report.totalMillimes += e.convertedMillimes;
    report.pulses += e.pulses; report.credits += e.credits; report.events += e.events;
  }
  if (file.available()) report.corruptRecords++;
  file.close();
  return true;
}

String formatMinor(uint64_t minor, unsigned decimals) {
  uint64_t scale = decimals == 2 ? 100 : decimals == 3 ? 1000 : 1000000;
  char text[48];
  snprintf(text,sizeof(text),"%llu,%0*llu",(unsigned long long)(minor/scale),
           (int)decimals,(unsigned long long)(minor%scale));
  return String(text);
}

String buildCsv(const RevenueReport& report) {
  String label = report.period == "day" ? "Jour" : report.period == "week" ? "Semaine" : "Mois";
  String csv = "\xEF\xBB\xBF";
  csv.reserve(768+report.buckets.size()*120);
  csv += "Période;\""+label+"\"\r\nPlage;\""+report.rangeLabel+"\"\r\n";
  csv += "Total encaissé TND;"+formatMinor(report.totalTndMillimes,3)+"\r\n";
  csv += "Total encaissé DZD;"+formatMinor(report.totalDzdCentimes,2)+"\r\n";
  csv += "Total général TND;"+formatMinor(report.totalMillimes,3)+"\r\n";
  csv += "Taux actuel (1 DZD en TND);"+(report.currentRateMicros ? formatMinor(report.currentRateMicros,6) : String("Non renseigné"))+"\r\n";
  csv += "Conversion;Taux historique enregistré par encaissement - arrondi au millime par impulsion\r\n";
  csv += "Clé;Libellé;Encaissé TND;Encaissé DZD;Total général TND;Impulsions;Crédits historiques / pièces;Événements\r\n";
  for (const auto& b : report.buckets) {
    csv += "\""+b.key+"\";\""+b.label+"\";"+formatMinor(b.tndMillimes,3)+";"+
           formatMinor(b.dzdCentimes,2)+";"+formatMinor(b.amountMillimes,3)+";"+
           String(b.pulses)+";"+String(b.credits)+";"+String(b.events)+"\r\n";
  }
  if (report.undatedEvents)
    csv += "NON_DATÉ;Hors période avant NTP;"+formatMinor(report.undatedTndMillimes,3)+";"+
           formatMinor(report.undatedDzdCentimes,2)+";"+formatMinor(report.undatedMillimes,3)+";;;"+
           String(report.undatedEvents)+"\r\n";
  return csv;
}
} // namespace RevenueService
