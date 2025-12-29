#include <Arduino.h>
#include "config.h"
#include "settings.h"
#include "can_handler.h"
#include "mqtt_handler.h"
#include "modem_handler.h"
#include "power_manager.h"
#include "data_manager.h"
#include "data_simulator.h"

// Global instances
CANHandler can_handler;
MQTTHandler mqtt_handler;
ModemHandler modem_handler;
PowerManager power_manager;
DataManager data_manager(&can_handler, &mqtt_handler, &modem_handler);
DataSimulator& simulator = DataSimulator::getInstance();

// Function prototypes
void handleMQTTConnection();
void checkSleepConditions();
void printSystemStatus();
void initializeFromSettings();

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    DEBUG_PRINTLN("\n\n==================================");
    DEBUG_PRINTLN("Zoe LTE Telemetry Gateway v1.0.0");
    DEBUG_PRINTF("Built: %s\n", __DATE__);
    DEBUG_PRINTLN("==================================");
    
    // Initialize Settings Manager FIRST
    DEBUG_PRINTLN("[System] Initializing Settings Manager...");
    if (!g_settings.begin()) {
        DEBUG_PRINTLN("[System] CRITICAL: Settings initialization failed!");
        // Continue anyway with defaults
    } else {
        DEBUG_PRINTLN("[System] Settings loaded successfully");
        g_settings.debugPrint();
    }
    
    // Initialize configuration from settings
    initializeFromSettings();
    
    // Initialize simulator if enabled in config
    const auto& sim_config = g_settings.getSettings().simulator;
    if (sim_config.enabled) {
        DEBUG_PRINTLN("[System] SIMULATOR MODE ENABLED!");
        DataSimulator::SimulationConfig config = {
            .enabled = true,
            .update_interval_ms = sim_config.update_interval_ms,
            .vary_values = sim_config.vary_values,
            .soc_variation = 0.5f,
            .temp_variation = 2.0f,
            .speed_variation = 1.0f,
            .current_variation = 1.0f
        };
        simulator.configure(config);
        if (simulator.begin()) {
            simulator.debugPrint();
        }
    } else {
        DEBUG_PRINTLN("[System] Running in normal mode (real CAN data)");
    }
    
    // Initialize all components with settings
    if (!g_settings.getSettings().simulator.enabled) {
        DEBUG_PRINTLN("[System] Initializing CAN Handler...");
        can_handler.begin();
    }
    
    DEBUG_PRINTLN("[System] Initializing Modem Handler...");
    modem_handler.begin();
    
    DEBUG_PRINTLN("[System] Initializing Power Manager...");
    power_manager.begin();
    
    DEBUG_PRINTLN("[System] Initializing Data Manager...");
    data_manager.begin();
    
    // List files on LittleFS (debug)
    g_settings.listFiles();
    
    // Get filesystem info
    uint32_t used, total;
    if (g_settings.getFilesystemInfo(used, total)) {
        DEBUG_PRINTF("[System] Filesystem: %u / %u bytes (%.1f%% used)\n", 
                    used, total, (float)used / total * 100);
    }
    
    DEBUG_PRINTLN("[System] Setup complete!");
}

void loop() {
    // Handle simulator mode
    if (g_settings.getSettings().simulator.enabled) {
        // Use simulated data instead of CAN
        if (simulator.update()) {
            const VehicleData& sim_data = simulator.getData();
            DEBUG_PRINTF("[Simulator] SOC: %.1f%% | Temp: %.1f°C | Speed: %.1f km/h\n",
                        sim_data.soc_percent, sim_data.battery_temp_c, sim_data.speed_kmh);
            // Data would be processed here for MQTT publishing
        }
    } else {
        // Normal CAN processing
        CANMessage_t msg;
        if (can_handler.readCAN1(msg)) {
            power_manager.notifyActivity();
            data_manager.processCAN1Message(msg);
        }
    }
    
    // MQTT handling - SKIP in simulator mode
    if (!g_settings.getSettings().simulator.enabled) {
        handleMQTTConnection();
        mqtt_handler.loop();
    }
    
    // Power management
    data_manager.loop();
    
    // GPS update interval from settings (also skip in simulator)
    if (!g_settings.getSettings().simulator.enabled) {
        static uint32_t last_gps_update = 0;
        uint32_t gps_interval = g_settings.getSettings().modem.gps_interval;
        
        if ((millis() - last_gps_update) > gps_interval && modem_handler.isNetworkConnected()) {
            GPSData_t gps;
            if (modem_handler.getGPS(gps)) {
                DEBUG_PRINTF("[GPS] Lat: %.6f, Lon: %.6f, Sats: %d\n", 
                            gps.latitude, gps.longitude, gps.satellites);
                
                // Get base topic from settings
                const char* base_topic = g_settings.getSettings().mqtt.base_topic;
                
                char gps_topic[128];
                snprintf(gps_topic, sizeof(gps_topic), "%s/gps/latitude", base_topic);
                mqtt_handler.publish(gps_topic, gps.latitude, 6);
                
                snprintf(gps_topic, sizeof(gps_topic), "%s/gps/longitude", base_topic);
                mqtt_handler.publish(gps_topic, gps.longitude, 6);
            }
            last_gps_update = millis();
        }
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
        uint32_t reconnect_interval = g_settings.getSettings().mqtt.reconnect_interval;
        
        if ((millis() - last_attempt) > reconnect_interval) {
            DEBUG_PRINTLN("[MQTT] Attempting connection...");
            const auto& mqtt_settings = g_settings.getSettings().mqtt;
            mqtt_handler.connect(mqtt_settings.username, mqtt_settings.password);
            last_attempt = millis();
        }
    }
}

void checkSleepConditions() {
    // Check if we should enter sleep
    if (power_manager.shouldEnterSleep()) {
        uint32_t sleep_timeout = g_settings.getSettings().power.sleep_timeout_idle;
        if (power_manager.getIdleTime() > sleep_timeout) {
            // Would enter sleep here
            // power_manager.goToLightSleep(5000);
        }
    }
}

void initializeFromSettings() {
    const auto& settings = g_settings.getSettings();
    
    DEBUG_PRINTLN("\n[Settings] Applying configuration:");
    
    // MQTT Settings
    DEBUG_PRINTF("  MQTT: %s:%d\n", settings.mqtt.broker, settings.mqtt.port);
    DEBUG_PRINTF("  Topic: %s\n", settings.mqtt.base_topic);
    DEBUG_PRINTF("  Keepalive: %d seconds\n", settings.mqtt.keepalive);
    
    // CAN Settings
    if (!settings.simulator.enabled) {
        DEBUG_PRINTF("  CAN High Speed: %u bps\n", settings.can.speed_high);
        DEBUG_PRINTF("  CAN Low Speed: %u bps\n", settings.can.speed_low);
        DEBUG_PRINTF("  Dual CAN: %s\n", settings.can.dual_can ? "YES" : "NO");
    }
    
    // Modem Settings
    DEBUG_PRINTF("  Modem Baudrate: %u\n", settings.modem.baudrate);
    DEBUG_PRINTF("  GPS Interval: %u ms\n", settings.modem.gps_interval);
    DEBUG_PRINTF("  Min Satellites: %d\n", settings.modem.gps_min_satellites);
    
    // Power Settings
    DEBUG_PRINTF("  Sleep Timeout: %u ms\n", settings.power.sleep_timeout_idle);
    DEBUG_PRINTF("  Deep Sleep: %s\n", settings.power.deep_sleep_enabled ? "ENABLED" : "DISABLED");
    
    // Debug Settings
    DEBUG_PRINTF("  Debug: %s\n", settings.debug.enabled ? "ENABLED" : "DISABLED");
    DEBUG_PRINTF("  Log Level: %d\n", settings.debug.log_level);
    
    // Simulator Settings
    DEBUG_PRINTF("  Simulator: %s\n", settings.simulator.enabled ? "ENABLED" : "DISABLED");
    if (settings.simulator.enabled) {
        DEBUG_PRINTF("  Simulator Update Interval: %u ms\n", settings.simulator.update_interval_ms);
        DEBUG_PRINTF("  Simulator Vary Values: %s\n", settings.simulator.vary_values ? "YES" : "NO");
    }
    
    DEBUG_PRINTLN();
}

void printSystemStatus() {
    const auto& settings = g_settings.getSettings();
    
    DEBUG_PRINTLN("\n====== SYSTEM STATUS REPORT =======");
    DEBUG_PRINTF("Uptime: %lu seconds\n", millis() / 1000);
    
    if (settings.simulator.enabled) {
        DEBUG_PRINTLN("Mode: SIMULATOR (testing without CAN bus)");
        simulator.debugPrint();
    } else {
        DEBUG_PRINTF("Battery: %.2f V (%u%%)\n", power_manager.getBatteryVoltage(),
                    power_manager.estimateBatteryPercent());
        DEBUG_PRINTF("CAN Messages: %lu\n", data_manager.getProcessedMessageCount());
    }
    
    DEBUG_PRINTF("MQTT Published: %lu\n", data_manager.getPublishedMessageCount());
    if (!settings.simulator.enabled) {
        DEBUG_PRINTF("MQTT Connected: %s\n", mqtt_handler.isConnected() ? "Yes" : "No");
    }
    DEBUG_PRINTF("Modem Connected: %s\n", modem_handler.isNetworkConnected() ? "Yes" : "No");
    DEBUG_PRINTF("Power State: %s\n", power_manager.getPowerStateName());
    DEBUG_PRINTF("Idle Time: %lu ms\n", power_manager.getIdleTime());
    
    // Settings info
    uint32_t used, total;
    if (g_settings.getFilesystemInfo(used, total)) {
        DEBUG_PRINTF("Filesystem: %u / %u bytes\n", used, total);
    }
    
    DEBUG_PRINTLN("====================================\n");
}
