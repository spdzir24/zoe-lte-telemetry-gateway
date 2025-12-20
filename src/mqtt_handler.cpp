#include "mqtt_handler.h"

MQTTHandler* MQTTHandler::instance = nullptr;

MQTTHandler::MQTTHandler()
    : is_connected(false),
      last_connection_attempt(0),
      connection_attempts(0),
      messages_published(0),
      last_error(0),
      batch_mode(false) {
    instance = this;
}

MQTTHandler::~MQTTHandler() {
    disconnect();
}

bool MQTTHandler::begin(const char* broker, uint16_t port, const char* client_id) {
    DEBUG_PRINTF("[MQTT] Configuring for broker: %s:%d\n", broker, port);
    client.setServer(broker, port);
    client.setCallback(messageCallback);
    client.setBufferSize(MQTT_MAX_PACKET_SIZE);
    return true;
}

bool MQTTHandler::connect(const char* username, const char* password) {
    if (client.connected()) {
        return true;
    }
    
    if ((millis() - last_connection_attempt) < MQTT_RECONNECT_INTERVAL) {
        return false;
    }
    
    last_connection_attempt = millis();
    connection_attempts++;
    
    DEBUG_PRINTF("[MQTT] Connection attempt #%d\n", connection_attempts);
    
    bool success = false;
    if (strlen(username) > 0 && strlen(password) > 0) {
        success = client.connect(MQTT_CLIENT_ID, username, password);
    } else {
        success = client.connect(MQTT_CLIENT_ID);
    }
    
    if (success) {
        is_connected = true;
        connection_attempts = 0;
        DEBUG_PRINTLN("[MQTT] Connected successfully");
        subscribe(MQTT_BASE_TOPIC "/control/#");
    } else {
        is_connected = false;
        last_error = client.state();
    }
    
    return success;
}

void MQTTHandler::disconnect() {
    if (client.connected()) {
        client.disconnect();
        is_connected = false;
        DEBUG_PRINTLN("[MQTT] Disconnected");
    }
}

bool MQTTHandler::isConnected() const {
    // Use mutable client for const method
    return const_cast<PubSubClient&>(client).connected();
}

void MQTTHandler::loop() {
    if (!client.connected()) {
        is_connected = false;
        handleReconnection();
    }
    client.loop();
}

bool MQTTHandler::publish(const char* topic, const char* payload, bool retain) {
    if (!client.connected()) {
        DEBUG_PRINTF("[MQTT] Not connected, cannot publish to %s\n", topic);
        return false;
    }
    
    if (client.publish(topic, payload, retain)) {
        messages_published++;
        DEBUG_PRINTF("[MQTT] Published to %s: %s\n", topic, payload);
        return true;
    }
    
    DEBUG_PRINTF("[MQTT] Publish failed to %s\n", topic);
    return false;
}

bool MQTTHandler::publish(const char* topic, float value, uint8_t precision, bool retain) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
    return publish(topic, buffer, retain);
}

bool MQTTHandler::publish(const char* topic, int32_t value, bool retain) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%ld", value);
    return publish(topic, buffer, retain);
}

bool MQTTHandler::publish(const char* topic, uint32_t value, bool retain) {
    char buffer[16];
    snprintf(buffer, sizeof(buffer), "%lu", value);
    return publish(topic, buffer, retain);
}

bool MQTTHandler::publishJSON(const char* topic, const char* json_payload, bool retain) {
    return publish(topic, json_payload, retain);
}

bool MQTTHandler::subscribe(const char* topic) {
    if (!client.connected()) {
        return false;
    }
    if (client.subscribe(topic)) {
        DEBUG_PRINTF("[MQTT] Subscribed to %s\n", topic);
        return true;
    }
    return false;
}

void MQTTHandler::startBatch() {
    batch_mode = true;
    batch_data = "";
    DEBUG_PRINTLN("[MQTT] Batch mode started");
}

void MQTTHandler::endBatch() {
    batch_mode = false;
    if (batch_data.length() > 0) {
        publishJSON(MQTT_BASE_TOPIC "/batch", batch_data.c_str(), false);
    }
    batch_data = "";
    DEBUG_PRINTLN("[MQTT] Batch mode ended");
}

void MQTTHandler::handleReconnection() {
    if (connection_attempts < 5) {
        connect(MQTT_USERNAME, MQTT_PASSWORD);
    } else {
        DEBUG_PRINTLN("[MQTT] Max reconnection attempts reached");
    }
}

void MQTTHandler::messageCallback(char* topic, byte* payload, unsigned int length) {
    if (instance) {
        DEBUG_PRINTF("[MQTT] Message received on %s\n", topic);
    }
}
