#include "PosteControlService.h"

#include "DisplayService.h"
#include "PosteConfig.h"
#include "RelayService.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace PosteControlService {
  static_assert(PosteConfig::CONTROL_TASK_INTERVAL_MS > 0,
                "CONTROL_TASK_INTERVAL_MS must be greater than zero");

  static constexpr uint32_t CONTROL_TASK_STACK_SIZE = 4096;
  static constexpr UBaseType_t CONTROL_TASK_PRIORITY = 2;

  static bool dedicatedTaskReady = false;

  static void runControlCycle() {
    RelayService::update();
    DisplayService::update(RelayService::getSessionSnapshot());
  }

  static void controlTask(void*) {
    TickType_t lastWakeAt = xTaskGetTickCount();
    const TickType_t period = pdMS_TO_TICKS(PosteConfig::CONTROL_TASK_INTERVAL_MS);

    for (;;) {
      runControlCycle();
      vTaskDelayUntil(&lastWakeAt, period);
    }
  }

  void begin() {
    bool mutexReady = RelayService::begin();
    DisplayService::begin(RelayService::getSessionSnapshot());

    if (!mutexReady) {
      Serial.println("Control task disabled: session mutex allocation failed.");
      return;
    }

    BaseType_t result = xTaskCreatePinnedToCore(
        controlTask,
        "poste-control",
        CONTROL_TASK_STACK_SIZE,
        nullptr,
        CONTROL_TASK_PRIORITY,
        nullptr,
        ARDUINO_RUNNING_CORE);

    dedicatedTaskReady = result == pdPASS;
    if (dedicatedTaskReady) {
      Serial.println("Poste control task started.");
    } else {
      Serial.println("Control task allocation failed; using loop fallback.");
    }
  }

  void loop() {
    if (!dedicatedTaskReady) {
      runControlCycle();
    }
  }
}
