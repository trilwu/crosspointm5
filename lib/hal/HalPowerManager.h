#pragma once

#include <Arduino.h>
#include <BatteryMonitor.h>
#include <InputManager.h>
#include <Logging.h>
#include <freertos/semphr.h>

#include <cassert>

#include "HalGPIO.h"

class HalPowerManager;
extern HalPowerManager powerManager;  // Singleton

class HalPowerManager {
  int normalFreq = 0;  // MHz
  bool isLowPower = false;

  mutable int _batteryCachedPercent = 0;         // Last read battery percentage (0-100)
  mutable unsigned long _batteryLastPollMs = 0;  // Timestamp of last battery read in milliseconds

  enum LockMode { None, NormalSpeed };
  LockMode currentLockMode = None;
  SemaphoreHandle_t modeMutex = nullptr;  // Protect access to currentLockMode

 public:
#if BOARD_HAS_PSRAM
  static constexpr int LOW_POWER_FREQ = 80;  // MHz
#else
  static constexpr int LOW_POWER_FREQ = 10;  // MHz
#endif
  static constexpr unsigned long IDLE_POWER_SAVING_MS = 3000;  // ms
  static constexpr unsigned long BATTERY_POLL_MS = 1500;       // ms

  void begin();

  // Control CPU frequency for power saving
  void setPowerSaving(bool enabled);

  // Setup wake up GPIO and enter deep sleep
  // Should be called inside main loop() to handle the currentLockMode
  void startDeepSleep(HalGPIO& gpio) const;

  // Why lightSleepUntilTouch() returned.
  enum class WakeReason { Touch, Timer };

  // Fallback wake period used when timeoutMs is 0 but no INT wake source could be
  // armed. Sleeping with nothing armed would strand the device permanently.
  static constexpr uint32_t NO_WAKE_SOURCE_FALLBACK_MS = 60000;

  // Light sleep (RAM retained, instant resume) until the touch controller asserts
  // its INT line, or until timeoutMs elapses (0 = wait for touch only).
  //
  // A 0 timeout is promoted to a NO_WAKE_SOURCE_FALLBACK_MS timer when no INT pin
  // could be armed (no touch IRQ configured, or gpio_wakeup_enable() failed), so
  // this never sleeps without a wake source.
  //
  // Only used on a board that must stay powered while "asleep" (the wall-clock
  // sleep screen): deep sleep would be far cheaper, but a GT911 INT on GPIO48 is
  // not RTC-capable — only GPIO0-21 are on the ESP32-S3 — so it could not wake on
  // touch. Light sleep can wake on any GPIO.
  WakeReason lightSleepUntilTouch(uint32_t timeoutMs) const;

  // Get battery percentage (range 0-100)
  uint16_t getBatteryPercentage() const;

  // RAII helper class to manage power saving locks
  // Usage: create an instance of Lock in a scope to disable power saving, for example when running a task that needs
  // full performance. When the Lock instance is destroyed (goes out of scope), power saving will be re-enabled.
  class Lock {
    friend class HalPowerManager;
    bool valid = false;

   public:
    explicit Lock();
    ~Lock();

    // Non-copyable and non-movable
    Lock(const Lock&) = delete;
    Lock& operator=(const Lock&) = delete;
    Lock(Lock&&) = delete;
    Lock& operator=(Lock&&) = delete;
  };
};
