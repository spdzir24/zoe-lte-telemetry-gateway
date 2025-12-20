#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <map>
#include <functional>
#include "config.h"

class MQTTHandler {
public:
    MQTTHandler();
    ~MQTTHandler();
    
    // Initialization and connection
    bool begin(const char* broker, uint16_t port, const char* client_id);
    void loop();
    bool connect(const char* username = "", const char* password = "");
    void disconnect();
    
    // Connection status
    bool isConnected() const;
    uint32_t getConnectionAttempts() const { return connection_attempts; }
    uint32_t getMessagesPublished() const { return messages_published; }
    
    // Publish methods
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool publish(const char* topic, float value, uint8_t precision = 2, bool retain = false);
    bool publish(const char* topic, int32_t value, bool retain = false);
    bool publish(const char* topic, uint32_t value, bool retain = false);
    bool publishJSON(const char* topic, const char* json_payload, bool retain = false);
    
    // Subscribe methods
    bool subscribe(const char* topic);
    void setMessageCallback(std::function<void(const char*, const byte*, unsigned int)> callback);
    
    // Batch publishing (for efficient data sending)
    void startBatch();
    void endBatch();
    
    // Reconnection handler
    void handleReconnection();
    
    // Last error
    uint32_t getLastError() const { return last_error; }
    void clearError() { last_error = 0; }
    
protected:
    PubSubClient client;
    
    bool is_connected;
    uint32_t last_connection_attempt;
    uint32_t connection_attempts;
    uint32_t messages_published;
    uint32_t last_error;
    
    // Batch mode
    bool batch_mode;
    String batch_data;
    
private:
    static MQTTHandler* instance;  // For callback routing
    static void messageCallback(char* topic, byte* payload, unsigned int length);
};

#endif // MQTT_HANDLER_H
