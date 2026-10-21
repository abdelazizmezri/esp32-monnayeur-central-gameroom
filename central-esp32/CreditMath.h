#pragma once

#include <stdint.h>
#include <limits.h>

namespace CreditMath {
// Le reliquat est déjà un nombre d'impulsions : son ancien diviseur ne sert
// qu'à le valider. Le dernier tarif global valorise les anciens coins entiers.
inline bool migrate(int coins, int divisor, int pending, int pendingDivisor, int& pieces) {
  if (coins < 0 || divisor <= 0 || pending < 0 || pendingDivisor <= 0 || pending >= pendingDivisor)
    return false;
  int64_t total = (int64_t)coins * divisor + pending;
  if (total > INT32_MAX) return false;
  pieces = (int)total;
  return true;
}

inline bool assignment(int piecesRequired, int units, int durationPerUnit,
                       int remaining, int& cost, long& duration) {
  if (piecesRequired <= 0 || units <= 0 || durationPerUnit <= 0 || remaining < 0)
    return false;
  int64_t debit = (int64_t)piecesRequired * units;
  int64_t seconds = (int64_t)durationPerUnit * units;
  if (debit > INT32_MAX || seconds > INT32_MAX || seconds + remaining > INT32_MAX)
    return false;
  cost = (int)debit;
  duration = (long)seconds;
  return true;
}
}
