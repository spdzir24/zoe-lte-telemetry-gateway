#include "power_manager.h"
#include "modem_handler.h"
#include "can_handler.h"

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
    
    // Disable WiFi and Bluetooth
    WiFi.mode(WIFI_OFF);
    btStop();
    
    // Disable peripherals
    adc_power_off();
    
    // Handle deep sleep
    handleDeepSleep(sleep_duration_seconds);
    
    // This will not return immediately; device wakes from sleep
    return true;
}

bool PowerManager::wakeFromDeepSleep() {
    DEBUG_PRINTLN("[Power] Waking from deep sleep...");
    
    // Re-enable ADC
    adc_power_on();
    
    current_state = POWER_STATE_ACTIVE;
    last_activity_time = millis();
    wakeup_from_rtc = esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER;
    
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
    float voltage = (adc_value / 4095.0) * 3.3 * BAT_MON_MULTIPLIER;
    
    return voltage;
}

uint8_t PowerManager::estimateBatteryPercent() const {
    float voltage = getBatteryVoltage();
    
    // Simple linear approximation for 12V system
    // Typical: 12V = 100%, 10.5V = 0%
    uint8_t percent = constrain((voltage - 10.5) / (12.0 - 10.5) * 100, 0, 100);
    
    return percent;
}

bool PowerManager::isBatteryLow() const {
    // Consider battery low if below 10.8V (for 12V system)
    return getBatteryVoltage() < 10.8;
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
    // Setup GPIO wake-up (e.g., from CAN activity)
    // This is typically done via external interrupt
    DEBUG_PRINTLN("[Power] GPIO wake-up configured");
}

void PowerManager::setupRTCTimer() {
    // Setup RTC timer for periodic wake-ups (e.g., GPS update every 6 hours)
    DEBUG_PRINTLN("[Power] RTC timer configured");
}

void PowerManager::handleDeepSleep(uint32_t duration) {
    if (duration > 0) {
        // Wake up after specified duration
        esp_sleep_enable_timer_wakeup(duration * 1000000);  // Convert seconds to microseconds
        DEBUG_PRINTF("[Power] Deep sleep for %lu seconds\n", duration);
    } else {
        // Wake up on external interrupt (CAN activity)
        if (wakeup_on_can) {
            // Setup CAN interrupt as wake-up source
            DEBUG_PRINTLN("[Power] Wake on CAN activity enabled");
        }
    }
    
    // Enter deep sleep
    esp_deep_sleep_start();  // This function does not return
}

void PowerManager::configureModemForSleep() {
    // Configure SIM7080G for low-power mode
    // This would be done through ModemHandler
    DEBUG_PRINTLN("[Power] Configuring modem for sleep mode");
    
    // Example: Set DTR pin HIGH to request sleep
    // digitalWrite(MODEM_DTR_PIN, HIGH);
}
