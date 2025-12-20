#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>
#include "config.h"

typedef enum {
    POWER_STATE_ACTIVE,      // Normal operation
    POWER_STATE_IDLE,        // Waiting for activity
    POWER_STATE_SLEEP,       // Light sleep
    POWER_STATE_DEEP_SLEEP   // Deep sleep
} PowerState_t;

class PowerManager {
public:
    PowerManager();
    ~PowerManager();
    
    // Initialization
    bool begin();
    
    // Power state management
    bool goToDeepSleep(uint32_t sleep_duration_seconds = 0);
    bool wakeFromDeepSleep();
    bool goToLightSleep(uint32_t sleep_duration_ms = 0);
    
    // Activity monitoring
    void notifyActivity();
    uint32_t getIdleTime() const;
    bool shouldEnterSleep() const;
    
    // Battery monitoring
    float getBatteryVoltage() const;
    uint8_t estimateBatteryPercent() const;
    bool isBatteryLow() const;
    
    // Power state
    PowerState_t getCurrentPowerState() const { return current_state; }
    const char* getPowerStateName() const;
    
    // Sleep configuration
    void setSleepTimeout(uint32_t timeout_ms) { sleep_timeout = timeout_ms; }
    void setWakeupOnCAN(bool enable) { wakeup_on_can = enable; }
    void setWakeupOnGPS(bool enable) { wakeup_on_gps = enable; }
    
    // RTC Wake-up
    void setupRTCWakeup(uint32_t interval_seconds);
    bool isWakeupFromRTC() const { return wakeup_from_rtc; }
    
protected:
    PowerState_t current_state;
    uint32_t last_activity_time;
    uint32_t sleep_timeout;  // Milliseconds before entering sleep
    
    bool wakeup_on_can;
    bool wakeup_on_gps;
    bool wakeup_from_rtc;
    
private:
    void setupGPIOWakeup();
    void setupRTCTimer();
    void handleDeepSleep(uint32_t duration);
    void configureModemForSleep();
};

#endif // POWER_MANAGER_H
