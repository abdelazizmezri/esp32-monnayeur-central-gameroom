#pragma once

#include <Arduino.h>
#include <vector>

#include "AppState.h"

struct RevenueBucket {
  String key;
  String label;
  int64_t amountMillimes = 0; // total général converti en TND
  int64_t tndMillimes = 0;
  int64_t dzdCentimes = 0;
  uint32_t pulses = 0;
  uint32_t credits = 0;
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
  int64_t totalMillimes = 0;
  int64_t totalTndMillimes = 0;
  int64_t totalDzdCentimes = 0;
  uint32_t currentRateMicros = 0;
  int64_t undatedMillimes = 0;
  int64_t undatedTndMillimes = 0;
  int64_t undatedDzdCentimes = 0;
  uint32_t pulses = 0;
  uint32_t credits = 0;
  uint32_t events = 0;
  uint32_t undatedEvents = 0;
  uint32_t corruptRecords = 0;
  std::vector<RevenueBucket> buckets;
};

namespace RevenueService {
  void begin(AppState& state);
  void update(AppState& state);

  bool recoverPending(AppState& state);
  // true = opération préparée durablement en NVS (finalisation éventuellement en attente).
  bool recordPulses(AppState& state, int pulses, uint8_t currency,
                    uint32_t value, uint32_t rate, uint32_t epoch);
  String formatMinor(uint64_t minor, unsigned decimals);

  bool buildReport(AppState& state, const String& period,
                   const String& anchorDate, RevenueReport& report,
                   String& error);
  String buildCsv(const RevenueReport& report);
}
