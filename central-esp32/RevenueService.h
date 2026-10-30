#pragma once

#include <Arduino.h>
#include <vector>

#include "AppState.h"
#include "RevenueJournal.h"

struct RevenueBucket {
  String key;
  String label;
  int64_t amountMinor = 0; // Uniquement la devise et la session sélectionnées
  int64_t tndMillimes = 0;
  int64_t dzdCentimes = 0;
  uint32_t pulses = 0;
  uint32_t credits = 0;
  uint32_t events = 0;
};

struct RevenueSession {
  uint32_t id = 0;
  uint8_t currency = Money::TND;
  uint64_t amountMinor = 0;
  uint32_t events = 0;
};

struct RevenueReport {
  String period;
  String rangeLabel;
  String startDate;
  String endDate;
  String generatedAt;
  bool storageReady = false;
  bool timeReady = false;
  uint64_t fileBytes = 0;
  uint64_t storageUsedBytes = 0;
  uint64_t storageTotalBytes = 0;
  uint32_t sessionId = 0;
  uint8_t currency = Money::TND;
  bool activeSession = true;
  uint64_t sessionAmountMinor = 0;
  uint32_t sessionEvents = 0;
  std::vector<RevenueSession> sessions;
  int64_t totalMinor = 0;
  int64_t totalTndMillimes = 0;
  int64_t totalDzdCentimes = 0;
  int64_t undatedMinor = 0;
  int64_t undatedTndMillimes = 0;
  int64_t undatedDzdCentimes = 0;
  uint32_t pulses = 0;
  uint32_t credits = 0;
  uint32_t events = 0;
  uint32_t undatedEvents = 0;
  uint32_t corruptRecords = 0;
  std::vector<RevenueBucket> buckets;
};

struct RevenueArchiveSnapshot {
  uint64_t sequence = 0;
  uint32_t sessionId = 0;
  uint8_t currency = Money::TND;
  size_t fileBytes = 0;
  uint32_t fingerprint = 0;
};

namespace RevenueService {
  bool prepareArchive(AppState& state, RevenueArchiveSnapshot& snapshot, String& error);
  bool readArchivePage(AppState& state, const RevenueArchiveSnapshot& snapshot,
                       size_t offset, std::vector<RevenueJournal::Entry>& entries, String& error);
  bool clearArchivedRevenue(AppState& state, const RevenueArchiveSnapshot& snapshot, String& error);
  void begin(AppState& state);
  void update(AppState& state);

  bool recoverPending(AppState& state);
  // true = opération préparée durablement en NVS (finalisation éventuellement en attente).
  bool recordPulses(AppState& state, int pulses, uint8_t currency,
                    uint32_t value, uint32_t epoch);
  bool sessionTotals(AppState& state, uint64_t& amount, uint32_t& events, String& error);
  String formatMinor(uint64_t minor, unsigned decimals);

  bool buildReport(AppState& state, const String& period,
                   const String& anchorDate, RevenueReport& report,
                   String& error, const String& session = "", const String& currency = "");
  String buildCsv(const RevenueReport& report);
}
