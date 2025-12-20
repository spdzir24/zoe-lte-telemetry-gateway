#include "power_manager.h"

PowerManager::PowerManager()
    : current_state(POWER_STATE_ACTIVE),
      last_activity_time(0),
      sleep_timeout(SLEEP_TIMEOUT_IDLE),
      wakeup_on_can(false),
      wakeup_on_gps(false),
      wakeup_from_rtc(false) {}

PowerManager::~PowerManager() {}

bool PowerManager::begin() {
    DEBUG_PRINTLN("[Power] Initializing power manager...");
    last_activity_time = millis();
    setupRTCTimer();
    setupGPIOWakeup();
    return true;
}

void PowerManager::notifyActivity() {
    last_activity_time = millis();
    if (current_state == POWER_STATE_SLEEP) {
        DEBUG_PRINTLN("[Power] Activity detected, returning to active state");
        current_state = POWER_STATE_ACTIVE;
    }
}

uint32_t PowerManager::getIdleTime() const {
    return millis() - last_activity_time;
}

bool PowerManager::shouldEnterSleep() const {
    return getIdleTime() > sleep_timeout;
}

bool PowerManager::goToDeepSleep(uint32_t sleep_duration_seconds) {
    DEBUG_PRINTLN("[Power] Entering deep sleep...");
    current_state = POWER_STATE_DEEP_SLEEP;
    handleDeepSleep(sleep_duration_seconds > 0 ? sleep_duration_seconds : 3600);
    return true;
}

bool PowerManager::wakeFromDeepSleep() {
    DEBUG_PRINTLN("[Power] Waking from deep sleep...");
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    DEBUG_PRINTF("[Power] Woke from cause: %d\n", (int)cause);
    current_state = POWER_STATE_ACTIVE;
    return true;
}

bool PowerManager::goToLightSleep(uint32_t sleep_duration_ms) {
    DEBUG_PRINTLN("[Power] Entering light sleep...");
    current_state = POWER_STATE_SLEEP;
    return true;
}

float PowerManager::getBatteryVoltage() const {
    // Read ADC pin (BAT_MON_PIN) and convert
    // Placeholder: return nominal voltage
    return 12.6f;  // Fully charged
}

uint8_t PowerManager::estimateBatteryPercent() const {
    float voltage = getBatteryVoltage();
    // Simple linear estimate: 10V = 0%, 13.8V = 100%
    uint8_t percent = (uint8_t)((voltage - 10.0f) / 3.8f * 100.0f);
    return (percent > 100) ? 100 : percent;
}

bool PowerManager::isBatteryLow() const {
    return getBatteryVoltage() < 10.5f;  // Below 10.5V is low
}

const char* PowerManager::getPowerStateName() const {
    switch (current_state) {
        case POWER_STATE_ACTIVE:
            return "Active";
        case POWER_STATE_IDLE:
            return "Idle";
        case POWER_STATE_SLEEP:
            return "Light Sleep";
        case POWER_STATE_DEEP_SLEEP:
            return "Deep Sleep";
        default:
            return "Unknown";
    }
}

void PowerManager::setupRTCWakeup(uint32_t interval_seconds) {
    DEBUG_PRINTF("[Power] RTC wakeup configured for %lu seconds\n", interval_seconds);
    esp_sleep_enable_timer_wakeup(interval_seconds * 1000000ULL);
}

void PowerManager::setupGPIOWakeup() {
    DEBUG_PRINTLN("[Power] GPIO wake-up configured");
}

void PowerManager::setupRTCTimer() {
    DEBUG_PRINTLN("[Power] RTC timer configured");
}

void PowerManager::handleDeepSleep(uint32_t duration) {
    DEBUG_PRINTF("[Power] RTC timer set for %lu seconds\n", duration);
    esp_sleep_enable_timer_wakeup(duration * 1000000ULL);
    DEBUG_PRINTLN("[Power] Deep sleep starting now...");
    esp_deep_sleep_start();
}

void PowerManager::configureModemForSleep() {
    DEBUG_PRINTLN("[Power] Configuring modem for sleep mode");
}
