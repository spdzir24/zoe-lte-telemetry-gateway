#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

/**
 * Settings Manager - Handles JSON-based configuration
 * Supports persistent storage on LittleFS filesystem
 */

class SettingsManager {
public:
    // MQTT Settings
    struct MQTTSettings {
        char broker[256] = "192.168.1.100";
        uint16_t port = 1883;
        char username[128] = "homeassistant";
        char password[128] = "your_password_here";
        uint16_t keepalive = 60;
        char base_topic[128] = "vehicle/zoe";
        uint32_t publish_interval_fast = 60000UL;     // 60 seconds
        uint32_t publish_interval_mid = 300000UL;     // 5 minutes
        uint32_t publish_interval_slow = 3600000UL;   // 60 minutes
        uint32_t reconnect_interval = 10000UL;         // 10 seconds
    };

    // CAN Bus Settings
    struct CANSettings {
        uint32_t speed_high = 500000;  // 500 kbps
        uint32_t speed_low = 125000;   // 125 kbps
        bool dual_can = true;
        uint16_t rx_queue_size = 256;
    };

    // Modem Settings
    struct ModemSettings {
        uint32_t baudrate = 115200;
        uint8_t network_mode = 38;      // 38 = LTE only
        uint8_t preferred_mode = 1;     // 1 = CAT-M, 2 = NB-IoT, 3 = Both
        uint32_t gps_interval = 300000UL; // 5 minutes
        uint8_t gps_min_satellites = 4;
    };

    // Power Management Settings
    struct PowerSettings {
        uint32_t sleep_timeout_idle = 300000UL;    // 5 minutes
        uint32_t sleep_timeout_parked = 600000UL;  // 10 minutes
        uint32_t rtc_wakeup_interval = 21600UL;    // 6 hours
        bool deep_sleep_enabled = true;
    };

    // Debug Settings
    struct DebugSettings {
        bool enabled = true;
        uint8_t log_level = 1;  // 0=DEBUG, 1=INFO, 2=WARN, 3=ERROR
        bool serial_output = true;
        bool file_logging = false;
    };

    // Complete Settings Structure
    struct Settings {
        MQTTSettings mqtt;
        CANSettings can;
        ModemSettings modem;
        PowerSettings power;
        DebugSettings debug;
        uint32_t version = 1;
        uint32_t last_modified = 0;
    };

private:
    Settings settings;
    const char* SETTINGS_FILE = "/settings.json";
    bool initialized = false;

public:
    SettingsManager();
    
    /**
     * Initialize settings manager and LittleFS
     * @return true if successful
     */
    bool begin();

    /**
     * Load settings from JSON file on LittleFS
     * Falls back to defaults if file doesn't exist
     * @return true if successful
     */
    bool load();

    /**
     * Save current settings to JSON file
     * @return true if successful
     */
    bool save();

    /**
     * Restore factory defaults
     */
    void restoreDefaults();

    /**
     * Validate settings for consistency
     * @return true if all settings are valid
     */
    bool validate();

    /**
     * Get settings structure (read-only)
     * @return const reference to settings
     */
    const Settings& getSettings() const;

    /**
     * Get mutable settings (use with caution)
     * @return mutable reference to settings
     */
    Settings& getMutableSettings();

    /**
     * Update MQTT settings
     */
    void setMQTTSettings(const MQTTSettings& mqtt);

    /**
     * Update CAN settings
     */
    void setCANSettings(const CANSettings& can);

    /**
     * Update Modem settings
     */
    void setModemSettings(const ModemSettings& modem);

    /**
     * Update Power settings
     */
    void setPowerSettings(const PowerSettings& power);

    /**
     * Update Debug settings
     */
    void setDebugSettings(const DebugSettings& debug);

    /**
     * Export settings as JSON document
     * @param doc ArduinoJson document to fill
     */
    void exportToJSON(JsonDocument& doc) const;

    /**
     * Import settings from JSON document
     * @param doc ArduinoJson document with settings
     * @return true if successful
     */
    bool importFromJSON(const JsonDocument& doc);

    /**
     * Get JSON string representation (for debugging)
     * @param buffer output buffer
     * @param buffer_size buffer size
     * @return number of bytes written
     */
    size_t toJSONString(char* buffer, size_t buffer_size) const;

    /**
     * Print settings to Serial (debug)
     */
    void debugPrint() const;

    /**
     * Check if settings have been initialized
     * @return true if initialized
     */
    bool isInitialized() const { return initialized; }

    /**
     * Get filesystem info
     * @param used_bytes out parameter for used space
     * @param total_bytes out parameter for total space
     * @return true if successful
     */
    static bool getFilesystemInfo(uint32_t& used_bytes, uint32_t& total_bytes);

    /**
     * Format LittleFS filesystem (WARNING: destructive!)
     * @return true if successful
     */
    static bool formatFilesystem();

    /**
     * List all files on filesystem (debug)
     */
    static void listFiles();
};

// Global settings manager instance
extern SettingsManager g_settings;

#endif // SETTINGS_H
