#include "config.h"
#include "settings.h"
#include <FS.h>
#include <LittleFS.h>

// Global instance
SettingsManager g_settings;

SettingsManager::SettingsManager() {
    // Constructor initializes with defaults
}

bool SettingsManager::begin() {
    DEBUG_PRINTLN("[Settings] Initializing LittleFS...");
    
    // Initialize LittleFS
    if (!LittleFS.begin()) {
        DEBUG_PRINTLN("[Settings] FAILED to mount LittleFS!");
        DEBUG_PRINTLN("[Settings] Formatting filesystem...");
        
        if (!LittleFS.format()) {
            DEBUG_PRINTLN("[Settings] FAILED to format LittleFS!");
            return false;
        }
        
        if (!LittleFS.begin()) {
            DEBUG_PRINTLN("[Settings] FAILED to mount LittleFS after format!");
            return false;
        }
    }
    
    DEBUG_PRINTLN("[Settings] LittleFS mounted successfully");
    
    // Print filesystem info
    uint32_t used, total;
    if (getFilesystemInfo(used, total)) {
        DEBUG_PRINTF("[Settings] Filesystem: %u / %u bytes\n", used, total);
    }
    
    // Try to load settings from file
    if (!load()) {
        DEBUG_PRINTLN("[Settings] No existing settings found, using defaults");
        // Create default settings file
        save();
    }
    
    initialized = true;
    return true;
}

bool SettingsManager::load() {
    DEBUG_PRINTF("[Settings] Loading from %s...\n", SETTINGS_FILE);
    
    if (!LittleFS.exists(SETTINGS_FILE)) {
        DEBUG_PRINTLN("[Settings] Settings file does not exist");
        return false;
    }
    
    File file = LittleFS.open(SETTINGS_FILE, "r");
    if (!file) {
        DEBUG_PRINTLN("[Settings] Failed to open settings file");
        return false;
    }
    
    StaticJsonDocument<4096> doc;
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        DEBUG_PRINTF("[Settings] JSON parsing failed: %s\n", error.c_str());
        return false;
    }
    
    // Parse MQTT settings
    if (doc["mqtt"].is<JsonObject>()) {
        auto mqtt = doc["mqtt"];
        if (mqtt["broker"]) strlcpy(settings.mqtt.broker, mqtt["broker"], sizeof(settings.mqtt.broker));
        if (mqtt["port"]) settings.mqtt.port = mqtt["port"];
        if (mqtt["username"]) strlcpy(settings.mqtt.username, mqtt["username"], sizeof(settings.mqtt.username));
        if (mqtt["password"]) strlcpy(settings.mqtt.password, mqtt["password"], sizeof(settings.mqtt.password));
        if (mqtt["keepalive"]) settings.mqtt.keepalive = mqtt["keepalive"];
        if (mqtt["base_topic"]) strlcpy(settings.mqtt.base_topic, mqtt["base_topic"], sizeof(settings.mqtt.base_topic));
        if (mqtt["publish_interval_fast"]) settings.mqtt.publish_interval_fast = mqtt["publish_interval_fast"];
        if (mqtt["publish_interval_mid"]) settings.mqtt.publish_interval_mid = mqtt["publish_interval_mid"];
        if (mqtt["publish_interval_slow"]) settings.mqtt.publish_interval_slow = mqtt["publish_interval_slow"];
        if (mqtt["reconnect_interval"]) settings.mqtt.reconnect_interval = mqtt["reconnect_interval"];
    }
    
    // Parse CAN settings
    if (doc["can"].is<JsonObject>()) {
        auto can = doc["can"];
        if (can["speed_high"]) settings.can.speed_high = can["speed_high"];
        if (can["speed_low"]) settings.can.speed_low = can["speed_low"];
        if (can["dual_can"]) settings.can.dual_can = can["dual_can"];
        if (can["rx_queue_size"]) settings.can.rx_queue_size = can["rx_queue_size"];
    }
    
    // Parse Modem settings
    if (doc["modem"].is<JsonObject>()) {
        auto modem = doc["modem"];
        if (modem["baudrate"]) settings.modem.baudrate = modem["baudrate"];
        if (modem["network_mode"]) settings.modem.network_mode = modem["network_mode"];
        if (modem["preferred_mode"]) settings.modem.preferred_mode = modem["preferred_mode"];
        if (modem["gps_interval"]) settings.modem.gps_interval = modem["gps_interval"];
        if (modem["gps_min_satellites"]) settings.modem.gps_min_satellites = modem["gps_min_satellites"];
    }
    
    // Parse Power settings
    if (doc["power"].is<JsonObject>()) {
        auto power = doc["power"];
        if (power["sleep_timeout_idle"]) settings.power.sleep_timeout_idle = power["sleep_timeout_idle"];
        if (power["sleep_timeout_parked"]) settings.power.sleep_timeout_parked = power["sleep_timeout_parked"];
        if (power["rtc_wakeup_interval"]) settings.power.rtc_wakeup_interval = power["rtc_wakeup_interval"];
        if (power["deep_sleep_enabled"]) settings.power.deep_sleep_enabled = power["deep_sleep_enabled"];
    }
    
    // Parse Debug settings
    if (doc["debug"].is<JsonObject>()) {
        auto debug = doc["debug"];
        if (debug["enabled"]) settings.debug.enabled = debug["enabled"];
        if (debug["log_level"]) settings.debug.log_level = debug["log_level"];
        if (debug["serial_output"]) settings.debug.serial_output = debug["serial_output"];
        if (debug["file_logging"]) settings.debug.file_logging = debug["file_logging"];
    }
    
    if (doc["version"]) settings.version = doc["version"];
    if (doc["last_modified"]) settings.last_modified = doc["last_modified"];
    
    DEBUG_PRINTLN("[Settings] Settings loaded successfully");
    return validate();
}

bool SettingsManager::save() {
    DEBUG_PRINTF("[Settings] Saving to %s...\n", SETTINGS_FILE);
    
    StaticJsonDocument<4096> doc;
    
    // Build MQTT section
    doc["mqtt"]["broker"] = settings.mqtt.broker;
    doc["mqtt"]["port"] = settings.mqtt.port;
    doc["mqtt"]["username"] = settings.mqtt.username;
    doc["mqtt"]["password"] = settings.mqtt.password;
    doc["mqtt"]["keepalive"] = settings.mqtt.keepalive;
    doc["mqtt"]["base_topic"] = settings.mqtt.base_topic;
    doc["mqtt"]["publish_interval_fast"] = settings.mqtt.publish_interval_fast;
    doc["mqtt"]["publish_interval_mid"] = settings.mqtt.publish_interval_mid;
    doc["mqtt"]["publish_interval_slow"] = settings.mqtt.publish_interval_slow;
    doc["mqtt"]["reconnect_interval"] = settings.mqtt.reconnect_interval;
    
    // Build CAN section
    doc["can"]["speed_high"] = settings.can.speed_high;
    doc["can"]["speed_low"] = settings.can.speed_low;
    doc["can"]["dual_can"] = settings.can.dual_can;
    doc["can"]["rx_queue_size"] = settings.can.rx_queue_size;
    
    // Build Modem section
    doc["modem"]["baudrate"] = settings.modem.baudrate;
    doc["modem"]["network_mode"] = settings.modem.network_mode;
    doc["modem"]["preferred_mode"] = settings.modem.preferred_mode;
    doc["modem"]["gps_interval"] = settings.modem.gps_interval;
    doc["modem"]["gps_min_satellites"] = settings.modem.gps_min_satellites;
    
    // Build Power section
    doc["power"]["sleep_timeout_idle"] = settings.power.sleep_timeout_idle;
    doc["power"]["sleep_timeout_parked"] = settings.power.sleep_timeout_parked;
    doc["power"]["rtc_wakeup_interval"] = settings.power.rtc_wakeup_interval;
    doc["power"]["deep_sleep_enabled"] = settings.power.deep_sleep_enabled;
    
    // Build Debug section
    doc["debug"]["enabled"] = settings.debug.enabled;
    doc["debug"]["log_level"] = settings.debug.log_level;
    doc["debug"]["serial_output"] = settings.debug.serial_output;
    doc["debug"]["file_logging"] = settings.debug.file_logging;
    
    // Metadata
    doc["version"] = settings.version;
    doc["last_modified"] = millis();
    
    File file = LittleFS.open(SETTINGS_FILE, "w");
    if (!file) {
        DEBUG_PRINTLN("[Settings] Failed to open settings file for writing");
        return false;
    }
    
    if (serializeJson(doc, file) == 0) {
        DEBUG_PRINTLN("[Settings] Failed to write JSON to file");
        file.close();
        return false;
    }
    
    file.close();
    DEBUG_PRINTLN("[Settings] Settings saved successfully");
    return true;
}

void SettingsManager::restoreDefaults() {
    DEBUG_PRINTLN("[Settings] Restoring factory defaults");
    
    // Reset to hardcoded defaults
    settings = Settings();
    save();
}

bool SettingsManager::validate() {
    // Basic validation checks
    if (settings.mqtt.port == 0) {
        DEBUG_PRINTLN("[Settings] Invalid MQTT port");
        return false;
    }
    
    if (settings.can.speed_high == 0) {
        DEBUG_PRINTLN("[Settings] Invalid CAN speed");
        return false;
    }
    
    if (settings.modem.baudrate == 0) {
        DEBUG_PRINTLN("[Settings] Invalid modem baudrate");
        return false;
    }
    
    return true;
}

const SettingsManager::Settings& SettingsManager::getSettings() const {
    return settings;
}

SettingsManager::Settings& SettingsManager::getMutableSettings() {
    return settings;
}

void SettingsManager::setMQTTSettings(const MQTTSettings& mqtt) {
    settings.mqtt = mqtt;
    save();
}

void SettingsManager::setCANSettings(const CANSettings& can) {
    settings.can = can;
    save();
}

void SettingsManager::setModemSettings(const ModemSettings& modem) {
    settings.modem = modem;
    save();
}

void SettingsManager::setPowerSettings(const PowerSettings& power) {
    settings.power = power;
    save();
}

void SettingsManager::setDebugSettings(const DebugSettings& debug) {
    settings.debug = debug;
    save();
}

void SettingsManager::exportToJSON(JsonDocument& doc) const {
    doc["mqtt"]["broker"] = settings.mqtt.broker;
    doc["mqtt"]["port"] = settings.mqtt.port;
    doc["can"]["speed_high"] = settings.can.speed_high;
    doc["modem"]["baudrate"] = settings.modem.baudrate;
    doc["power"]["sleep_timeout_idle"] = settings.power.sleep_timeout_idle;
}

bool SettingsManager::importFromJSON(const JsonDocument& doc) {
    if (doc["mqtt"]["broker"]) strlcpy(settings.mqtt.broker, doc["mqtt"]["broker"], sizeof(settings.mqtt.broker));
    if (doc["mqtt"]["port"]) settings.mqtt.port = doc["mqtt"]["port"];
    
    return validate() && save();
}

size_t SettingsManager::toJSONString(char* buffer, size_t buffer_size) const {
    StaticJsonDocument<4096> doc;
    exportToJSON(doc);
    return serializeJson(doc, buffer, buffer_size);
}

void SettingsManager::debugPrint() const {
    DEBUG_PRINTLN("\n=== SETTINGS DEBUG INFO ===");
    DEBUG_PRINTF("MQTT Broker: %s:%d\n", settings.mqtt.broker, settings.mqtt.port);
    DEBUG_PRINTF("MQTT Base Topic: %s\n", settings.mqtt.base_topic);
    DEBUG_PRINTF("CAN Speed: %u bps\n", settings.can.speed_high);
    DEBUG_PRINTF("Modem Baudrate: %u\n", settings.modem.baudrate);
    DEBUG_PRINTF("Sleep Timeout: %u ms\n", settings.power.sleep_timeout_idle);
    DEBUG_PRINTF("Debug Enabled: %s\n", settings.debug.enabled ? "YES" : "NO");
    DEBUG_PRINTLN("============================\n");
}

bool SettingsManager::getFilesystemInfo(uint32_t& used_bytes, uint32_t& total_bytes) {
    // LittleFS doesn't have info() method like SPIFFS, use different approach
    // Calculate used space by counting files
    used_bytes = 0;
    total_bytes = 1048576;  // 1 MB LittleFS partition
    
    File root = LittleFS.open("/");
    if (!root) return false;
    
    File file = root.openNextFile();
    while (file) {
        used_bytes += file.size();
        file = root.openNextFile();
    }
    
    return true;
}

bool SettingsManager::formatFilesystem() {
    DEBUG_PRINTLN("[Settings] WARNING: Formatting filesystem...");
    return LittleFS.format();
}

void SettingsManager::listFiles() {
    DEBUG_PRINTLN("[Settings] Files on LittleFS:");
    File root = LittleFS.open("/");
    if (!root) return;
    
    File file = root.openNextFile();
    while (file) {
        DEBUG_PRINTF("  %s (%u bytes)\n", file.name(), file.size());
        file = root.openNextFile();
    }
}
