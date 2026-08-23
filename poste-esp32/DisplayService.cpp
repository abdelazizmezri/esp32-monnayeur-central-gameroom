#include "DisplayService.h"

#include "PosteConfig.h"
#include "RelayService.h"

#include <Arduino.h>
#include <limits.h>

namespace DisplayService {
  static_assert(PosteConfig::TM1637_BRIGHTNESS >= 0 &&
                    PosteConfig::TM1637_BRIGHTNESS <= 7,
                "TM1637_BRIGHTNESS must be between 0 and 7");
  static_assert(PosteConfig::TM1637_CLK_PIN != PosteConfig::TM1637_DIO_PIN,
                "TM1637 CLK and DIO pins must be different");
  static_assert(PosteConfig::TM1637_CLK_PIN != PosteConfig::RELAY_PIN &&
                    PosteConfig::TM1637_DIO_PIN != PosteConfig::RELAY_PIN,
                "TM1637 pins must be different from RELAY_PIN");

  static constexpr uint8_t SEGMENTS_BY_DIGIT[] = {
      0x3f, 0x06, 0x5b, 0x4f, 0x66,
      0x6d, 0x7d, 0x07, 0x7f, 0x6f,
  };
  static constexpr uint8_t COLON_SEGMENT = 0x80;
  static constexpr unsigned long COLON_BLINK_INTERVAL_MS = 500;
  static constexpr unsigned long MAX_DISPLAYED_MINUTES = 99UL * 60UL + 59UL;

  enum class DisplayMode : uint8_t {
    Idle,
    Active,
    Recovery,
  };

  static unsigned long lastUpdateAtMs = 0;
  static unsigned long lastDisplayedSeconds = ULONG_MAX;
  static DisplayMode lastMode = DisplayMode::Idle;
  static bool lastBlinkPhase = false;
  static bool hasRendered = false;

  static void busDelay() {
    delayMicroseconds(5);
  }

  static void startTransmission() {
    digitalWrite(PosteConfig::TM1637_CLK_PIN, HIGH);
    digitalWrite(PosteConfig::TM1637_DIO_PIN, HIGH);
    busDelay();
    digitalWrite(PosteConfig::TM1637_DIO_PIN, LOW);
    busDelay();
    digitalWrite(PosteConfig::TM1637_CLK_PIN, LOW);
  }

  static void stopTransmission() {
    digitalWrite(PosteConfig::TM1637_CLK_PIN, LOW);
    digitalWrite(PosteConfig::TM1637_DIO_PIN, LOW);
    busDelay();
    digitalWrite(PosteConfig::TM1637_CLK_PIN, HIGH);
    busDelay();
    digitalWrite(PosteConfig::TM1637_DIO_PIN, HIGH);
    busDelay();
  }

  static void writeByte(uint8_t value) {
    for (uint8_t bit = 0; bit < 8; ++bit) {
      digitalWrite(PosteConfig::TM1637_CLK_PIN, LOW);
      digitalWrite(PosteConfig::TM1637_DIO_PIN,
                   (value & 0x01) != 0 ? HIGH : LOW);
      busDelay();
      digitalWrite(PosteConfig::TM1637_CLK_PIN, HIGH);
      busDelay();
      value >>= 1;
    }

    // Le TM1637 tire DIO a zero pendant l'acquittement. La ligne est relachee
    // ici ; l'absence d'afficheur ne doit jamais bloquer le firmware du poste.
    digitalWrite(PosteConfig::TM1637_CLK_PIN, LOW);
    digitalWrite(PosteConfig::TM1637_DIO_PIN, HIGH);
    busDelay();
    digitalWrite(PosteConfig::TM1637_CLK_PIN, HIGH);
    busDelay();
    digitalWrite(PosteConfig::TM1637_CLK_PIN, LOW);
  }

  static void writeSegments(const uint8_t segments[4]) {
    startTransmission();
    writeByte(0x40);  // Increment automatique de l'adresse.
    stopTransmission();

    startTransmission();
    writeByte(0xc0);  // Premiere position de l'afficheur.
    for (uint8_t index = 0; index < 4; ++index) {
      writeByte(segments[index]);
    }
    stopTransmission();

    startTransmission();
    writeByte(0x88 | static_cast<uint8_t>(PosteConfig::TM1637_BRIGHTNESS));
    stopTransmission();
  }

  static void renderBlank() {
    const uint8_t segments[4] = {0, 0, 0, 0};
    writeSegments(segments);
  }

  static void renderTime(unsigned long remainingSeconds, bool colonVisible) {
    uint8_t values[4];

    if (remainingSeconds >= 3600UL) {
      unsigned long totalMinutes = remainingSeconds / 60UL;
      if (remainingSeconds % 60UL != 0) {
        ++totalMinutes;
      }
      if (totalMinutes > MAX_DISPLAYED_MINUTES) {
        totalMinutes = MAX_DISPLAYED_MINUTES;
      }

      unsigned long hours = totalMinutes / 60UL;
      unsigned long minutes = totalMinutes % 60UL;
      values[0] = hours / 10UL;
      values[1] = hours % 10UL;
      values[2] = minutes / 10UL;
      values[3] = minutes % 10UL;
    } else {
      unsigned long minutes = remainingSeconds / 60UL;
      unsigned long seconds = remainingSeconds % 60UL;
      values[0] = minutes / 10UL;
      values[1] = minutes % 10UL;
      values[2] = seconds / 10UL;
      values[3] = seconds % 10UL;
    }

    uint8_t segments[4];
    for (uint8_t index = 0; index < 4; ++index) {
      segments[index] = SEGMENTS_BY_DIGIT[values[index]];
    }
    if (colonVisible) {
      segments[1] |= COLON_SEGMENT;
    }
    writeSegments(segments);
  }

  static void refresh(const PosteState& state, bool force) {
    unsigned long now = millis();
    bool active = state.relayState;
    DisplayMode mode = active
                           ? DisplayMode::Active
                           : (state.recoveryPending ? DisplayMode::Recovery
                                                    : DisplayMode::Idle);

    long activeRemaining = active ? RelayService::getRemainingSeconds(state) : 0;
    unsigned long displayedSeconds = activeRemaining > 0
                                         ? static_cast<unsigned long>(activeRemaining)
                                         : (mode == DisplayMode::Recovery
                                                ? state.recoveryRemainingSeconds
                                                : 0UL);
    bool blinkPhase = mode == DisplayMode::Idle ||
                      ((now / COLON_BLINK_INTERVAL_MS) & 1UL) == 0;

    if (!force && hasRendered && displayedSeconds == lastDisplayedSeconds &&
        mode == lastMode && blinkPhase == lastBlinkPhase) {
      return;
    }

    if (mode == DisplayMode::Recovery && !blinkPhase) {
      renderBlank();
    } else {
      bool colonVisible = mode != DisplayMode::Active || blinkPhase;
      renderTime(displayedSeconds, colonVisible);
    }

    lastDisplayedSeconds = displayedSeconds;
    lastMode = mode;
    lastBlinkPhase = blinkPhase;
    hasRendered = true;
  }

  void begin(const PosteState& state) {
    pinMode(PosteConfig::TM1637_CLK_PIN, OUTPUT_OPEN_DRAIN);
    pinMode(PosteConfig::TM1637_DIO_PIN, OUTPUT_OPEN_DRAIN);
    digitalWrite(PosteConfig::TM1637_CLK_PIN, HIGH);
    digitalWrite(PosteConfig::TM1637_DIO_PIN, HIGH);
    refresh(state, true);
    lastUpdateAtMs = millis();
  }

  void update(const PosteState& state) {
    unsigned long now = millis();
    if (now - lastUpdateAtMs < PosteConfig::DISPLAY_UPDATE_INTERVAL_MS) {
      return;
    }

    lastUpdateAtMs = now;
    refresh(state, false);
  }
}
