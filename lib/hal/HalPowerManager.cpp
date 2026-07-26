#include "HalPowerManager.h"

#include <BoardConfig.h>
#include <Logging.h>
#include <PowerManager.h>
#include <WiFi.h>
#include <driver/gpio.h>
#include <esp_sleep.h>
#include <soc/soc_caps.h>

#include <cassert>

#include "HalGPIO.h"

HalPowerManager powerManager;  // Singleton instance

void HalPowerManager::begin() {
  if (BoardConfig::ACTIVE.batteryAdc >= 0) {
    pinMode(BoardConfig::ACTIVE.batteryAdc, INPUT);
  }
  normalFreq = getCpuFrequencyMhz();
  modeMutex = xSemaphoreCreateMutex();
  assert(modeMutex != nullptr);
}

void HalPowerManager::setPowerSaving(bool enabled) {
  if (normalFreq <= 0) {
    return;  // invalid state
  }

  auto wifiMode = WiFi.getMode();
  if (wifiMode != WIFI_MODE_NULL) {
    // Wifi is active, force disabling power saving
    enabled = false;
  }

  // Note: We don't use mutex here to avoid too much overhead,
  // it's not very important if we read a slightly stale value for currentLockMode
  const LockMode mode = currentLockMode;

  if (mode == None && enabled && !isLowPower) {
    LOG_DBG("PWR", "Going to low-power mode");
    if (!setCpuFrequencyMhz(LOW_POWER_FREQ)) {
      LOG_DBG("PWR", "Failed to set CPU frequency = %d MHz", LOW_POWER_FREQ);
      return;
    }
    isLowPower = true;

  } else if ((!enabled || mode != None) && isLowPower) {
    LOG_DBG("PWR", "Restoring normal CPU frequency");
    if (!setCpuFrequencyMhz(normalFreq)) {
      LOG_DBG("PWR", "Failed to set CPU frequency = %d MHz", normalFreq);
      return;
    }
    isLowPower = false;
  }

  // Otherwise, no change needed
}

void HalPowerManager::startDeepSleep(HalGPIO& gpio) const {
#ifdef ENABLE_SERIAL_LOG
  // Tear down HWCDC so the host sees a clean disconnect and the peripheral
  // doesn't hold power domains that interfere with USB-powered GPIO wake.
  // logSerial is the raw HWCDC reference; Serial is the MySerialImpl proxy
  // (which doesn't expose end()).
  logSerial.end();
#endif

#if !SOC_PM_SUPPORT_EXT1_WAKEUP
  if (gpio.isXteinkDevice() && !gpio.deviceIsX3()) {
    // X4 GPIO13 is connected to the battery latch MOSFET. Keeping it low powers
    // the MCU off on battery, while the SDK wake source still handles USB power.
    constexpr gpio_num_t GPIO_SPIWP = GPIO_NUM_13;
    gpio_set_direction(GPIO_SPIWP, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_SPIWP, 0);
    gpio_hold_en(GPIO_SPIWP);
  }
#endif

  // Cut the gated peripheral rails (touch/SD/EPD on boards like the Sticky) and
  // hold the enables off through deep sleep — otherwise the GT911 and SD card
  // stay powered all through "off" and drain the battery. No-op on boards with
  // no switched rails (X4/X3). Trade-off: no touch-to-wake; wake is the power
  // button. Must run after display.deepSleep() so the panel controller gets its
  // deep-sleep command while its rail is still up (enterDeepSleep() in main.cpp
  // guarantees that ordering).
  freeink::PowerManager::powerDownRailsForSleep();

  // M5Paper S3: the power button is wired to the AXP2101 PMIC, not a GPIO, so deep
  // sleep has no wake source (armPowerButtonWakeup() would no-op and the device
  // could never wake). Instead pulse GPIO44 (PWROFF) to have the PMIC cut power to
  // the whole board; a power-button press then powers it back on as a cold boot,
  // matching the stock/fork behavior. A short timer wake is armed as a fallback for
  // the USB-powered case, where the PMIC can't fully cut power.
  if (BoardConfig::isM5PaperS3()) {
    constexpr gpio_num_t PWROFF_PULSE_PIN = GPIO_NUM_44;
    pinMode(PWROFF_PULSE_PIN, OUTPUT);
    digitalWrite(PWROFF_PULSE_PIN, HIGH);
    delay(100);
    digitalWrite(PWROFF_PULSE_PIN, LOW);
    esp_sleep_enable_timer_wakeup(5ULL * 1000 * 1000);  // USB-powered fallback
    freeink::PowerManager::deepSleep();                 // does not return
    return;
  }

  // Waits for the power button to be physically released (so holding it doesn't
  // immediately wake the device again), then arms the wake source and sleeps.
  freeink::PowerManager::deepSleepUntilPowerButton();
}

HalPowerManager::WakeReason HalPowerManager::lightSleepUntilTouch(const uint32_t timeoutMs) const {
  const int8_t touchInt = BoardConfig::ACTIVE.touch.irq;

  // The GT911 INT idles HIGH (InputManager leaves it INPUT_PULLUP after the wakeup
  // sequence) and is pulled LOW when a touch is reported, so wake on the low level.
  if (touchInt >= 0) {
    const auto pin = static_cast<gpio_num_t>(touchInt);
    gpio_wakeup_enable(pin, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();
  }

  if (timeoutMs > 0) {
    esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(timeoutMs) * 1000ULL);
  } else {
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
  }

  const esp_err_t err = esp_light_sleep_start();
  const esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

  // Disarm so the next call starts from a known state.
  if (touchInt >= 0) {
    gpio_wakeup_disable(static_cast<gpio_num_t>(touchInt));
  }
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);

  if (err != ESP_OK) {
    // Sleep was rejected because a wake source was already asserted — a finger
    // still resting on the panel holds INT low. esp_sleep_get_wakeup_cause() then
    // still reports the *previous* sleep, so don't trust it: report Touch, which
    // returns the caller to the UI (where the touch is handled normally).
    LOG_DBG("PWR", "Light sleep rejected (err=%d)", static_cast<int>(err));
    return WakeReason::Touch;
  }

  return (cause == ESP_SLEEP_WAKEUP_TIMER) ? WakeReason::Timer : WakeReason::Touch;
}

uint16_t HalPowerManager::getBatteryPercentage() const {
  static const BatteryMonitor battery;
  if (BoardConfig::ACTIVE.batteryGauge.gaugeAddr != 0) {
    const unsigned long now = millis();
    if (_batteryLastPollMs != 0 && (now - _batteryLastPollMs) < BATTERY_POLL_MS) {
      return _batteryCachedPercent;
    }

    _batteryLastPollMs = now;
    uint16_t percent = 0;
    if (!battery.readPercentageChecked(percent)) {
      return _batteryCachedPercent;
    }
    _batteryCachedPercent = percent;
    return _batteryCachedPercent;
  }

  // smooth the battery %.
  if (_batteryCachedPercent == 0) {
    _batteryCachedPercent = 10 * battery.readPercentage();
  } else {
    _batteryCachedPercent = (_batteryCachedPercent * 9 + battery.readPercentage() * 10) / 10;
  }
  return _batteryCachedPercent / 10;
}

HalPowerManager::Lock::Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  // Current limitation: only one lock at a time
  if (powerManager.currentLockMode != None) {
    LOG_ERR("PWR", "Lock already held, ignore");
    valid = false;
  } else {
    powerManager.currentLockMode = NormalSpeed;
    valid = true;
  }
  xSemaphoreGive(powerManager.modeMutex);
  if (valid) {
    // Immediately restore normal CPU frequency if currently in low-power mode
    powerManager.setPowerSaving(false);
  }
}

HalPowerManager::Lock::~Lock() {
  xSemaphoreTake(powerManager.modeMutex, portMAX_DELAY);
  if (valid) {
    powerManager.currentLockMode = None;
  }
  xSemaphoreGive(powerManager.modeMutex);
}
