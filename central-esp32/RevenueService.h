#pragma once

#include <Arduino.h>
#include <vector>

#include "AppState.h"

struct RevenueBucket {
  String key;
  String label;
  int64_t amountMillimes = 0;
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
  int64_t undatedMillimes = 0;
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

  // Enregistre l'argent réellement reçu. Une simulation de crédit ne doit pas
  // appeler cette fonction.
  bool recordPulses(AppState& state, int pulses, int generatedCredits,
                    const String& source = "monnayeur");

  bool buildReport(AppState& state, const String& period,
                   const String& anchorDate, RevenueReport& report,
                   String& error);
  String buildCsv(const RevenueReport& report);
}
