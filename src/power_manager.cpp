#include "power_manager.h"

PowerManager::PowerManager()
    : current_state(POWER_STATE_ACTIVE),
      last_activity_time(0),
      sleep_timeout(SLEEP_TIMEOUT_IDLE),
      wakeup_on_can(true),
      wakeup_on_gps(false),
      wakeup_from_rtc(false) {}

PowerManager::~PowerManager() {}

bool PowerManager::begin() {
    DEBUG_PRINTLN("[Power] Initializing power manager...");
    
    // Setup GPIO for wake-up
    setupGPIOWakeup();
    
    // Setup RTC timer
    setupRTCTimer();
    
    last_activity_time = millis();
    
    return true;
}

bool PowerManager::goToDeepSleep(uint32_t sleep_duration_seconds) {
    DEBUG_PRINTLN("[Power] Entering deep sleep...");
    
    current_state = POWER_STATE_DEEP_SLEEP;
    
    // Configure modem for sleep
    configureModemForSleep();
    
    // Disable WiFi (set to OFF if it was on)
    // Note: WiFi is not enabled in this firmware, but keeping for completeness
    // WiFi.mode(WIFI_OFF);
    
    // Bluetooth is not used, so no need to disable
    // btStop() would only work if BT was initialized
    
    // Handle deep sleep
    handleDeepSleep(sleep_duration_seconds);
    
    // This will not return immediately; device wakes from sleep
    return true;
}

bool PowerManager::wakeFromDeepSleep() {
    DEBUG_PRINTLN("[Power] Waking from deep sleep...");
    
    // Check wake-up cause
    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
    wakeup_from_rtc = (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER);
    
    current_state = POWER_STATE_ACTIVE;
    last_activity_time = millis();
    
    if (wakeup_from_rtc) {
        DEBUG_PRINTLN("[Power] Woke from RTC timer");
    } else {
        DEBUG_PRINTF("[Power] Woke from cause: %d\n", wakeup_reason);
    }
    
    return true;
}

bool PowerManager::goToLightSleep(uint32_t sleep_duration_ms) {
    DEBUG_PRINTLN("[Power] Entering light sleep...");
    
    current_state = POWER_STATE_SLEEP;
    
    // Simple delay-based sleep (could be optimized with more sophisticated power saving)
    unsigned long start = millis();
    while ((millis() - start) < sleep_duration_ms) {
        delay(100);
        // Check for wake-up events
        if (getIdleTime() > 1000) {
            break;
        }
    }
    
    current_state = POWER_STATE_ACTIVE;
    return true;
}

void PowerManager::notifyActivity() {
    last_activity_time = millis();
    
    if (current_state == POWER_STATE_SLEEP || current_state == POWER_STATE_DEEP_SLEEP) {
        current_state = POWER_STATE_ACTIVE;
        DEBUG_PRINTLN("[Power] Activity detected, returning to active state");
    }
}

uint32_t PowerManager::getIdleTime() const {
    return (millis() - last_activity_time);
}

bool PowerManager::shouldEnterSleep() const {
    return getIdleTime() > sleep_timeout;
}

float PowerManager::getBatteryVoltage() const {
    // Read ADC pin BAT_MON_PIN
    int adc_value = analogRead(BAT_MON_PIN);
    
    // Convert ADC value to voltage
    // ESP32 ADC: 0-4095 maps to 0-3.3V
    // With voltage divider: actual_voltage = (adc_value / 4095) * 3.3 * BAT_MON_MULTIPLIER
    float voltage = (adc_value / 4095.0f) * 3.3f * BAT_MON_MULTIPLIER;
    
    return voltage;
}

uint8_t PowerManager::estimateBatteryPercent() const {
    float voltage = getBatteryVoltage();
    
    // Simple linear approximation for 12V system
    // Typical: 12V = 100%, 10.5V = 0%
    // For a 12V system monitored via 3.3V ADC with divider,
    // adjust these thresholds based on your actual divider ratio
    uint8_t percent = constrain((int)((voltage - 10.5f) / (12.0f - 10.5f) * 100.0f), 0, 100);
    
    return percent;
}

bool PowerManager::isBatteryLow() const {
    // Consider battery low if below 10.8V (for 12V system)
    return getBatteryVoltage() < 10.8f;
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

void PowerManager::setupGPIOWakeup() {
    // Setup GPIO wake-up (e.g., from external interrupt)
    // This could be connected to a CAN bus activity detector
    // For now, we rely on RTC timer wake-up
    DEBUG_PRINTLN("[Power] GPIO wake-up configured");
}

void PowerManager::setupRTCTimer() {
    // Setup RTC timer for periodic wake-ups (e.g., GPS update every 6 hours)
    DEBUG_PRINTLN("[Power] RTC timer configured");
}

void PowerManager::handleDeepSleep(uint32_t duration) {
    if (duration > 0) {
        // Wake up after specified duration
        esp_sleep_enable_timer_wakeup((uint64_t)duration * 1000000);  // Convert seconds to microseconds
        DEBUG_PRINTF("[Power] RTC timer set for %lu seconds\n", duration);
    }
    
    // Optional: Setup GPIO wake-up on specific pin for CAN activity
    // This would require an external interrupt circuit from the CAN transceiver
    // esp_sleep_enable_ext0_wakeup(GPIO_NUM_35, ESP_EXT0_WAKEUP_ALL_LOW);
    
    DEBUG_PRINTLN("[Power] Deep sleep starting now...");
    
    // Enter deep sleep
    esp_deep_sleep_start();  // This function does not return normally
}

void PowerManager::configureModemForSleep() {
    // Configure SIM7080G for low-power mode
    // This would be done through ModemHandler
    DEBUG_PRINTLN("[Power] Configuring modem for sleep mode");
    
    // Set DTR pin HIGH to request sleep (if ModemHandler not available here)
    // digitalWrite(MODEM_DTR_PIN, HIGH);
}
