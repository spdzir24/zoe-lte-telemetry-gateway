#include <Arduino.h>
#include "config.h"
#include "can_handler.h"
#include "mqtt_handler.h"
#include "modem_handler.h"
#include "power_manager.h"
#include "data_manager.h"

// Global object instances
CANHandler can_handler;
MQTTHandler mqtt_handler;
ModemHandler modem_handler;
PowerManager power_manager;
DataManager* data_manager = nullptr;

// Timing variables
uint32_t last_status_print = 0;
uint32_t last_mqtt_connection_attempt = 0;
const uint32_t STATUS_PRINT_INTERVAL = 30000;  // 30 seconds
const uint32_t MQTT_CHECK_INTERVAL = 10000;    // 10 seconds

// Forward declarations
void setup();
void loop();
void handleCAN1Messages();
void handleMQTTConnection();
void checkSleepConditions();
void printSystemStatus();

// ============================================================================
// SETUP FUNCTION
// ============================================================================
void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_PRINTLN("\n\n==================================");
    DEBUG_PRINTLN("   ZOE LTE TELEMETRY GATEWAY");
    DEBUG_PRINTLN("   Firmware Version: " FIRMWARE_VERSION);
    DEBUG_PRINTLN("   Build Date: " BUILD_DATE);
    DEBUG_PRINTLN("==================================");
    
    // Setup LED pin for status indication
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    
    // Initialize Power Manager
    DEBUG_PRINTLN("[SETUP] Initializing power manager...");
    if (!power_manager.begin()) {
        DEBUG_PRINTLN("[ERROR] Power manager initialization failed");
    }
    
    // Initialize CAN bus
    DEBUG_PRINTLN("[SETUP] Initializing CAN bus...");
    if (!can_handler.begin()) {
        DEBUG_PRINTLN("[ERROR] CAN handler initialization failed");
    }
    
    // Initialize SIM7080G Modem
    DEBUG_PRINTLN("[SETUP] Initializing modem...");
    if (!modem_handler.begin()) {
        DEBUG_PRINTLN("[ERROR] Modem initialization failed");
        // Continue anyway - might recover later
    }
    
    // Wait for modem to connect to network
    DEBUG_PRINTLN("[SETUP] Connecting to network...");
    uint32_t start_connect = millis();
    while (!modem_handler.isNetworkConnected()) {
        if ((millis() - start_connect) > 60000) {  // 60 second timeout
            DEBUG_PRINTLN("[WARN] Network connection timeout");
            break;
        }
        delay(100);
    }
    
    if (modem_handler.isNetworkConnected()) {
        DEBUG_PRINTLN("[SETUP] Network connected!");
        
        // Try to enable GPS
        modem_handler.enableGPS();
    }
    
    // Initialize MQTT Handler
    DEBUG_PRINTLN("[SETUP] Initializing MQTT...");
    if (!mqtt_handler.begin(MQTT_BROKER_ADDR, MQTT_BROKER_PORT, MQTT_CLIENT_ID)) {
        DEBUG_PRINTLN("[ERROR] MQTT initialization failed");
    }
    
    // Initialize Data Manager
    DEBUG_PRINTLN("[SETUP] Initializing data manager...");
    data_manager = new DataManager(&can_handler, &mqtt_handler, &modem_handler);
    if (!data_manager->begin()) {
        DEBUG_PRINTLN("[ERROR] Data manager initialization failed");
        delete data_manager;
        data_manager = nullptr;
    }
    
    // Register all Renault Zoe specific signals
    DEBUG_PRINTLN("[SETUP] Registering vehicle signals...");
    data_manager->registerAllZoeSignals();
    
    // LED indication: ready
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    
    DEBUG_PRINTLN("[SETUP] Initialization complete! System ready.");
}

// ============================================================================
// MAIN LOOP
// ============================================================================
void loop() {
    // Check for MQTT connection and reconnect if needed
    handleMQTTConnection();
    
    // Process CAN messages
    handleCAN1Messages();
    
    // Let MQTT client process incoming messages
    mqtt_handler.loop();
    
    // Get GPS data periodically
    static uint32_t last_gps_read = 0;
    if ((millis() - last_gps_read) > GPS_UPDATE_INTERVAL) {
        GPSData_t gps_data;
        if (modem_handler.getGPS(gps_data)) {
            // Publish GPS data to MQTT
            mqtt_handler.publish(MQTT_BASE_TOPIC "/gps/latitude", gps_data.latitude, 6);
            mqtt_handler.publish(MQTT_BASE_TOPIC "/gps/longitude", gps_data.longitude, 6);
            mqtt_handler.publish(MQTT_BASE_TOPIC "/gps/accuracy", (int32_t)gps_data.accuracy);
            mqtt_handler.publish(MQTT_BASE_TOPIC "/gps/satellites", (uint32_t)gps_data.satellites);
            
            DEBUG_PRINTF("[GPS] Lat: %.6f, Lon: %.6f, Sats: %d\n",
                         gps_data.latitude, gps_data.longitude, gps_data.satellites);
        }
        last_gps_read = millis();
    }
    
    // Print system status periodically
    if ((millis() - last_status_print) > STATUS_PRINT_INTERVAL) {
        printSystemStatus();
        last_status_print = millis();
    }
    
    // Check sleep conditions
    checkSleepConditions();
    
    // Small delay to prevent watchdog timeout
    delay(10);
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

void handleCAN1Messages() {
    CANMessage_t msg;
    static uint32_t last_can_activity = 0;
    
    // Read all available CAN messages
    while (can_handler.readCAN1(msg)) {
        power_manager.notifyActivity();  // Reset sleep timeout
        last_can_activity = millis();
        
        // Process message through data manager
        if (data_manager) {
            data_manager->processCAN1Message(msg);
        }
        
        // Blink LED on CAN activity
        digitalWrite(LED_PIN, HIGH);
        delayMicroseconds(50);
        digitalWrite(LED_PIN, LOW);
    }
}

void handleMQTTConnection() {
    if (!mqtt_handler.isConnected()) {
        // Try to reconnect periodically
        if ((millis() - last_mqtt_connection_attempt) > MQTT_CHECK_INTERVAL) {
            DEBUG_PRINTLN("[MQTT] Attempting connection...");
            if (mqtt_handler.connect(MQTT_USERNAME, MQTT_PASSWORD)) {
                DEBUG_PRINTLN("[MQTT] Connected successfully!");
                
                // Publish online status
                mqtt_handler.publish(MQTT_BASE_TOPIC "/status", "online", true);
                
                // Publish system info
                mqtt_handler.publish(MQTT_BASE_TOPIC "/info/firmware", FIRMWARE_VERSION);
                mqtt_handler.publish(MQTT_BASE_TOPIC "/info/device", DEVICE_NAME);
            }
            last_mqtt_connection_attempt = millis();
        }
    }
}

void checkSleepConditions() {
    // Check if vehicle is idle and should enter deep sleep
    if (power_manager.shouldEnterSleep() && ENABLE_DEEP_SLEEP) {
        DEBUG_PRINTLN("[Sleep] Vehicle idle, entering deep sleep...");
        
        // Publish offline status before sleeping
        mqtt_handler.publish(MQTT_BASE_TOPIC "/status", "sleeping", true);
        delay(500);
        
        // Disconnect from network
        modem_handler.disconnect();
        mqtt_handler.disconnect();
        
        // Enter deep sleep (wakes on CAN activity or RTC timer)
        power_manager.goToDeepSleep(RTC_WAKEUP_INTERVAL);
        
        // After waking from deep sleep, this code continues
        power_manager.wakeFromDeepSleep();
        DEBUG_PRINTLN("[Sleep] Woke from deep sleep!");
        
        // Reconnect to network
        modem_handler.begin();
    }
}

void printSystemStatus() {
    DEBUG_PRINTLN("\n=== SYSTEM STATUS ===");
    DEBUG_PRINTF("Uptime: %lu seconds\n", millis() / 1000);
    DEBUG_PRINTF("CAN1 messages: %lu\n", can_handler.getMessagesReceived1());
    DEBUG_PRINTF("MQTT messages published: %lu\n", mqtt_handler.getMessagesPublished());
    
    if (data_manager) {
        DEBUG_PRINTF("Data processed: %lu\n", data_manager->getProcessedMessageCount());
    }
    
    DEBUG_PRINTF("Power state: %s\n", power_manager.getPowerStateName());
    DEBUG_PRINTF("Idle time: %lu ms\n", power_manager.getIdleTime());
    DEBUG_PRINTF("Battery: %.2fV (%.0f%%)\n",
                 power_manager.getBatteryVoltage(),
                 (float)power_manager.estimateBatteryPercent());
    
    NetworkStatus_t net_status = modem_handler.getNetworkStatus();
    DEBUG_PRINTF("Network: %s (%d dBm)\n",
                 net_status.is_connected ? "Connected" : "Disconnected",
                 net_status.signal_strength);
    
    DEBUG_PRINTF("MQTT: %s (attempts: %lu)\n",
                 mqtt_handler.isConnected() ? "Connected" : "Disconnected",
                 mqtt_handler.getConnectionAttempts());
    
    DEBUG_PRINTLN("=====================\n");
}
