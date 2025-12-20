#ifndef MODEM_HANDLER_H
#define MODEM_HANDLER_H

#include <Arduino.h>
#include "config.h"

typedef struct {
    float latitude;
    float longitude;
    float accuracy;  // meters
    uint8_t satellites;
    uint32_t timestamp;
    bool has_fix;
} GPSData_t;

typedef struct {
    int signal_strength;  // -1 to -120 dBm
    uint8_t signal_percent;  // 0-100%
    const char* network_type;  // "LTE", "NB-IoT", etc.
    bool is_connected;
} NetworkStatus_t;

class ModemHandler {
public:
    ModemHandler();
    ~ModemHandler();
    
    // Initialization and power management
    bool begin();
    bool end();
    bool wakeup();
    bool sleep();
    
    // Network operations
    bool connect();
    bool disconnect();
    bool isNetworkConnected() const;
    NetworkStatus_t getNetworkStatus();
    
    // GPS operations
    bool enableGPS();
    bool disableGPS();
    bool getGPS(GPSData_t& gps_data);
    
    // AT Command Interface
    bool sendATCommand(const char* cmd, char* response, uint16_t response_size, uint32_t timeout = 5000);
    
    // Status
    uint32_t getUptime() const { return uptime_ms; }
    bool isInitialized() const { return initialized; }
    uint32_t getLastError() const { return last_error; }
    
    // MQTT over LTE (internal MQTT support in SIM7080G)
    bool mqttConnect(const char* broker, uint16_t port, const char* client_id);
    bool mqttPublish(const char* topic, const char* payload);
    bool mqttSubscribe(const char* topic);
    bool mqttDisconnect();
    
protected:
    bool initialized;
    bool network_connected;
    bool gps_enabled;
    bool mqtt_connected;
    
    uint32_t uptime_ms;
    uint32_t last_activity;
    uint32_t last_error;
    
    // GPS cache
    GPSData_t cached_gps;
    uint32_t last_gps_update;
    
    // Network status cache
    NetworkStatus_t cached_network_status;
    uint32_t last_network_check;
    
private:
    // Hardware initialization
    void setupSerial();
    void setupPowerControl();
    void setupDTRPin();
    
    // AT Command helpers
    bool waitForResponse(const char* expected, uint32_t timeout = 5000);
    void flushSerial();
    
    // Power management
    void enableModemPower();
    void disableModemPower();
};

#endif // MODEM_HANDLER_H
