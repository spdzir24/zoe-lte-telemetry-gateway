#include "modem_handler.h"
#include <HardwareSerial.h>

ModemHandler::ModemHandler()
    : initialized(false),
      network_connected(false),
      gps_enabled(false),
      mqtt_connected(false),
      uptime_ms(0),
      last_activity(0),
      last_error(0),
      last_gps_update(0),
      last_network_check(0) {}

ModemHandler::~ModemHandler() {
    end();
}

bool ModemHandler::begin() {
    DEBUG_PRINTLN("[Modem] Initializing SIM7080G...");
    
    setupSerial();
    setupPowerControl();
    setupDTRPin();
    
    // Power on the modem
    enableModemPower();
    delay(3000);  // Wait for module startup
    
    // Send initial AT command
    char response[256];
    if (!sendATCommand("AT", response, sizeof(response))) {
        DEBUG_PRINTLN("[Modem] Failed to communicate with SIM7080G");
        last_error = 2001;
        return false;
    }
    
    // Set network mode to LTE only
    if (!sendATCommand("AT+CNMP=38", response, sizeof(response))) {
        DEBUG_PRINTLN("[Modem] Failed to set network mode");
    }
    
    // Set preferred mode to CAT-M
    if (!sendATCommand("AT+CMNB=1", response, sizeof(response))) {
        DEBUG_PRINTLN("[Modem] Failed to set preferred mode");
    }
    
    initialized = true;
    DEBUG_PRINTLN("[Modem] Initialized successfully");
    
    return true;
}

bool ModemHandler::end() {
    if (!initialized) return true;
    
    if (gps_enabled) disableGPS();
    if (mqtt_connected) mqttDisconnect();
    if (network_connected) disconnect();
    
    // Power down
    disableModemPower();
    
    initialized = false;
    return true;
}

bool ModemHandler::wakeup() {
    // Wake up from sleep
    // DTR pin LOW triggers wake-up
    digitalWrite(MODEM_DTR_PIN, LOW);
    delay(100);
    
    // Send AT command to confirm awake
    char response[128];
    return sendATCommand("AT", response, sizeof(response), 2000);
}

bool ModemHandler::sleep() {
    // Enter sleep mode
    // DTR pin HIGH triggers sleep request
    digitalWrite(MODEM_DTR_PIN, HIGH);
    delay(100);
    
    // Optionally send AT command
    char response[128];
    return sendATCommand("AT+CSCLK=1", response, sizeof(response), 2000);
}

bool ModemHandler::connect() {
    if (network_connected) return true;
    
    DEBUG_PRINTLN("[Modem] Connecting to network...");
    
    char response[256];
    
    // Check if SIM is ready
    if (!sendATCommand("AT+CPIN?", response, sizeof(response))) {
        DEBUG_PRINTLN("[Modem] SIM check failed");
        last_error = 2002;
        return false;
    }
    
    // Wait for network registration (may take a while)
    uint32_t start_time = millis();
    while ((millis() - start_time) < 30000) {  // 30 second timeout
        if (sendATCommand("AT+CREG?", response, sizeof(response))) {
            // Check if registered (response contains +CREG: 0,1 or similar)
            if (strstr(response, ",1") || strstr(response, ",5")) {
                network_connected = true;
                DEBUG_PRINTLN("[Modem] Network connected");
                return true;
            }
        }
        delay(1000);
    }
    
    DEBUG_PRINTLN("[Modem] Network connection timeout");
    last_error = 2003;
    return false;
}

bool ModemHandler::disconnect() {
    network_connected = false;
    return true;
}

bool ModemHandler::isNetworkConnected() const {
    return network_connected;
}

NetworkStatus_t ModemHandler::getNetworkStatus() {
    NetworkStatus_t status;
    char response[256];
    
    // Get signal strength
    if (sendATCommand("AT+CSQ", response, sizeof(response))) {
        // Parse +CSQ: <rssi>,<ber>
        int rssi = 0;
        sscanf(response, "+CSQ: %d", &rssi);
        status.signal_strength = -113 + (2 * rssi);  // Convert to dBm
        status.signal_percent = (rssi * 100) / 31;   // Convert to percentage
    }
    
    // Get registration status
    if (sendATCommand("AT+CREG?", response, sizeof(response))) {
        status.is_connected = (network_connected);
    }
    
    status.network_type = "LTE Cat-M";
    
    return status;
}

bool ModemHandler::enableGPS() {
    DEBUG_PRINTLN("[Modem] Enabling GPS...");
    
    char response[256];
    
    // Enable GNSS (GPS)
    if (!sendATCommand("AT+CGNSPWR=1", response, sizeof(response))) {
        DEBUG_PRINTLN("[Modem] Failed to enable GNSS");
        return false;
    }
    
    gps_enabled = true;
    return true;
}

bool ModemHandler::disableGPS() {
    if (!gps_enabled) return true;
    
    DEBUG_PRINTLN("[Modem] Disabling GPS...");
    
    char response[256];
    sendATCommand("AT+CGNSPWR=0", response, sizeof(response));
    
    gps_enabled = false;
    return true;
}

bool ModemHandler::getGPS(GPSData_t& gps_data) {
    if (!gps_enabled) return false;
    
    char response[512];
    if (!sendATCommand("AT+CGNSINF", response, sizeof(response))) {
        return false;
    }
    
    // Parse +CGNSINF: <gnss_run>,<fix_stat>,<utc_date>,<utc_time>,<lat>,<lon>,<altitude>,<speed>,<course>,<pdop>,<hdop>,<vdop>,<sat_view>,<sat_use>,<glonass_used>
    int gnss_run = 0, fix_stat = 0;
    float lat = 0.0, lon = 0.0;
    int sat_use = 0;
    
    int parsed = sscanf(response, "+CGNSINF: %d,%d,,,%.6f,%.6f,,,,,,,%d",
                        &gnss_run, &fix_stat, &lat, &lon, &sat_use);
    
    if (parsed >= 5 && fix_stat > 0) {
        gps_data.latitude = lat;
        gps_data.longitude = lon;
        gps_data.satellites = sat_use;
        gps_data.has_fix = true;
        gps_data.timestamp = millis();
        
        last_gps_update = millis();
        return true;
    }
    
    return false;
}

bool ModemHandler::sendATCommand(const char* cmd, char* response, uint16_t response_size, uint32_t timeout) {
    if (!initialized) return false;
    
    Serial2.println(cmd);  // UART2 for SIM7080G
    
    uint32_t start_time = millis();
    uint16_t response_len = 0;
    
    while ((millis() - start_time) < timeout) {
        if (Serial2.available()) {
            char c = Serial2.read();
            if (response_len < (response_size - 1)) {
                response[response_len++] = c;
            }
            
            // Check for OK or ERROR
            if (response_len >= 4) {
                if (strstr(response, "OK") || strstr(response, "ERROR")) {
                    response[response_len] = '\0';
                    DEBUG_PRINTF("[Modem] Response: %s\n", response);
                    return (strstr(response, "OK") != nullptr);
                }
            }
        }
    }
    
    response[response_len] = '\0';
    DEBUG_PRINTF("[Modem] Timeout for command: %s\n", cmd);
    return false;
}

bool ModemHandler::mqttConnect(const char* broker, uint16_t port, const char* client_id) {
    // Note: SIM7080G has internal MQTT support
    // This is a simplified implementation
    
    char cmd[256];
    char response[256];
    
    // Example: AT+SMCONF="SERVER","test.mosquitto.org",1883
    snprintf(cmd, sizeof(cmd), "AT+SMCONF=\"SERVER\",\"%s\",%d", broker, port);
    
    if (!sendATCommand(cmd, response, sizeof(response))) {
        return false;
    }
    
    mqtt_connected = true;
    return true;
}

bool ModemHandler::mqttPublish(const char* topic, const char* payload) {
    if (!mqtt_connected) return false;
    
    // AT+SMPUB="topic",length,0,0,"payload"
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "AT+SMPUB=\"%s\",%d,0,0,\"%s\"",
             topic, (int)strlen(payload), payload);
    
    char response[256];
    return sendATCommand(cmd, response, sizeof(response));
}

bool ModemHandler::mqttSubscribe(const char* topic) {
    if (!mqtt_connected) return false;
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "AT+SMSUB=\"%s\",1", topic);
    
    char response[256];
    return sendATCommand(cmd, response, sizeof(response));
}

bool ModemHandler::mqttDisconnect() {
    char response[256];
    sendATCommand("AT+SMDISC", response, sizeof(response));
    mqtt_connected = false;
    return true;
}

void ModemHandler::setupSerial() {
    Serial2.begin(MODEM_BAUDRATE, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);
    delay(100);
    DEBUG_PRINTLN("[Modem] UART2 initialized");
}

void ModemHandler::setupPowerControl() {
    pinMode(MODEM_EN_PIN, OUTPUT);
    digitalWrite(MODEM_EN_PIN, LOW);  // Initially off
}

void ModemHandler::setupDTRPin() {
    pinMode(MODEM_DTR_PIN, OUTPUT);
    digitalWrite(MODEM_DTR_PIN, LOW);  // Initially wake (LOW)
}

void ModemHandler::enableModemPower() {
    digitalWrite(MODEM_EN_PIN, HIGH);
    DEBUG_PRINTLN("[Modem] Power enabled");
}

void ModemHandler::disableModemPower() {
    digitalWrite(MODEM_EN_PIN, LOW);
    DEBUG_PRINTLN("[Modem] Power disabled");
}

void ModemHandler::flushSerial() {
    while (Serial2.available()) {
        Serial2.read();
    }
}
