#include <Arduino.h>
#include "config.h"
#include "can_handler.h"
#include "mqtt_handler.h"
#include "modem_handler.h"
#include "power_manager.h"
#include "data_manager.h"

// Global instances
CANHandler can_handler;
MQTTHandler mqtt_handler;
ModemHandler modem_handler;
PowerManager power_manager;
DataManager data_manager(&can_handler, &mqtt_handler, &modem_handler);

// Function prototypes
void handleMQTTConnection();
void checkSleepConditions();
void printSystemStatus();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_PRINTLN("\n\n==================================");
    DEBUG_PRINTLN("Zoe LTE Telemetry Gateway v1.0.0");
    DEBUG_PRINTF("Built: %s\n", __DATE__);
    DEBUG_PRINTLN("==================================");
    
    // Initialize all components
    can_handler.begin();
    modem_handler.begin();
    power_manager.begin();
    data_manager.begin();
    
    DEBUG_PRINTLN("[System] Setup complete!");
}

void loop() {
    // CAN processing
    CANMessage_t msg;
    if (can_handler.readCAN1(msg)) {
        data_manager.processCAN1Message(msg);
    }
    
    // MQTT handling
    handleMQTTConnection();
    mqtt_handler.loop();
    
    // Power management
    power_manager.loop();
    data_manager.loop();
    
    // GPS update every 5 minutes (when connected)
    static uint32_t last_gps_update = 0;
    if ((millis() - last_gps_update) > 300000UL && modem_handler.isConnected()) {
        float lat = 0.0f, lon = 0.0f;
        uint8_t sats = 0;
        if (modem_handler.getGPS(lat, lon, sats)) {
            DEBUG_PRINTF("[GPS] Lat: %.6f, Lon: %.6f, Sats: %d\n", lat, lon, sats);
            
            char gps_topic[128];
            snprintf(gps_topic, sizeof(gps_topic), "%s/gps/latitude", MQTT_BASE_TOPIC);
            mqtt_handler.publish(gps_topic, lat, 6);
            
            snprintf(gps_topic, sizeof(gps_topic), "%s/gps/longitude", MQTT_BASE_TOPIC);
            mqtt_handler.publish(gps_topic, lon, 6);
        }
        last_gps_update = millis();
    }
    
    // Check for sleep conditions
    checkSleepConditions();
    
    // Status print every 30 seconds
    static uint32_t last_status = 0;
    if ((millis() - last_status) > 30000UL) {
        printSystemStatus();
        last_status = millis();
    }
    
    delay(10);  // Small delay to prevent watchdog
}

void handleMQTTConnection() {
    if (!mqtt_handler.isConnected()) {
        static uint32_t last_attempt = 0;
        if ((millis() - last_attempt) > 10000UL) {
            DEBUG_PRINTLN("[MQTT] Attempting connection...");
            mqtt_handler.connect(MQTT_USERNAME, MQTT_PASSWORD);
            last_attempt = millis();
        }
    }
}

void checkSleepConditions() {
    // Check if we should enter sleep
    static uint32_t idle_start = millis();
    
    // This would check CAN activity and power state
    // For now, just monitor activity
}

void printSystemStatus() {
    DEBUG_PRINTLN("\n====== SYSTEM STATUS REPORT =======");
    DEBUG_PRINTF("Uptime: %lu seconds\n", millis() / 1000);
    DEBUG_PRINTF("Battery: %.2f V (%.1f%%)\n", power_manager.getBatteryVoltage(),
                power_manager.getBatteryPercent());
    DEBUG_PRINTF("CAN Messages: %lu\n", data_manager.getProcessedMessageCount());
    DEBUG_PRINTF("MQTT Published: %lu\n", data_manager.getPublishedMessageCount());
    DEBUG_PRINTF("MQTT Connected: %s\n", mqtt_handler.isConnected() ? "Yes" : "No");
    DEBUG_PRINTF("Modem Connected: %s\n", modem_handler.isConnected() ? "Yes" : "No");
    DEBUG_PRINTLN("===================================");
}
