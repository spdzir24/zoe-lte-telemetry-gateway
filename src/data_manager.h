#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include <Arduino.h>
#include <map>
#include <ArduinoJson.h>
#include "config.h"
#include "can_messages.h"
#include "can_handler.h"
#include "mqtt_handler.h"
#include "modem_handler.h"

// Vehicle telemetry data structure
// Used for both real CAN data and simulated data
typedef struct {
    uint32_t timestamp_ms;              // Timestamp in milliseconds
    
    // Battery
    float soc_percent;                  // State of charge 0-100%
    float battery_temp_c;               // Battery temperature in Celsius
    float dc_voltage;                   // DC voltage in volts
    float dc_current_a;                 // DC current in amps (negative = discharge)
    float power_kw;                     // Calculated power in kilowatts
    
    // Motor
    float motor_rpm;                    // Motor revolutions per minute
    float motor_temp_c;                 // Motor temperature in Celsius
    
    // Cabin
    float cabin_temp_c;                 // Cabin temperature in Celsius
    
    // Speed and distance
    float speed_kmh;                    // Current speed in km/h
    float odometer_km;                  // Total distance in km
    
    // GPS
    float gps_latitude;                 // GPS latitude
    float gps_longitude;                // GPS longitude
    uint8_t gps_satellites;             // Number of satellites
    
    // Status
    bool charging;                      // Is charging
    bool doors_locked;                  // Are doors locked
} VehicleData;

typedef struct {
    const char* name;
    uint32_t can_id;
    CANSignal_t signal;  // Removed const to allow initialization
    uint32_t publish_interval;  // ms
    uint32_t last_published;
    double last_value;
    double value_tolerance;  // Skip publish if change < tolerance
} ManagedSignal_t;

class DataManager {
public:
    DataManager(CANHandler* can, MQTTHandler* mqtt, ModemHandler* modem);
    ~DataManager();
    
    // Initialization
    bool begin();
    void loop();
    
    // Signal management
    void registerSignal(const char* signal_name, uint32_t can_id, const CANSignal_t& signal,
                        uint32_t publish_interval, double tolerance = 0.0);
    void registerAllZoeSignals();  // Pre-configured Zoe signals
    
    // Process CAN messages
    void processCAN1Message(const CANMessage_t& msg);
    void processCAN2Message(const CANMessage_t& msg);
    
    // Force publish all data
    void publishAllData();
    
    // Statistics
    uint32_t getProcessedMessageCount() const { return processed_messages; }
    uint32_t getPublishedMessageCount() const { return published_messages; }
    
    // Status
    void printStatus();
    
protected:
    CANHandler* can_handler;
    MQTTHandler* mqtt_handler;
    ModemHandler* modem_handler;
    
    std::map<uint32_t, std::vector<ManagedSignal_t>> signal_map;  // CAN ID -> Signals
    
    uint32_t processed_messages;
    uint32_t published_messages;
    
    // JSON document for batching
    StaticJsonDocument<4096> json_document;
    
private:
    // Helper methods
    bool shouldPublish(ManagedSignal_t& signal, double new_value);
    void publishSignal(const ManagedSignal_t& signal, double value);
    void publishSignalWithUnit(const char* mqtt_topic, double value, const char* unit);
};

#endif // DATA_MANAGER_H
