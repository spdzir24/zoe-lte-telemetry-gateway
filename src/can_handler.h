#ifndef CAN_HANDLER_H
#define CAN_HANDLER_H

#include <Arduino.h>
#include <queue>
#include "config.h"
#include "can_messages.h"

class CANHandler {
public:
    CANHandler();
    ~CANHandler();
    
    // Initialization
    bool begin();
    void end();
    
    // CAN Bus Operations
    bool setupCAN1(uint32_t speed = CAN_SPEED_HIGH);
    bool setupCAN2(uint32_t speed = CAN_SPEED_LOW);
    
    // Message receiving
    bool readCAN1(CANMessage_t& msg);
    bool readCAN2(CANMessage_t& msg);
    
    // Message sending (for diagnostic purposes)
    bool sendCAN1(const CANMessage_t& msg);
    bool sendCAN2(const CANMessage_t& msg);
    
    // Signal extraction and parsing
    double extractSignal(const CANMessage_t& msg, const CANSignal_t& signal);
    
    // Status
    bool isConnected1() const { return can1_initialized; }
    bool isConnected2() const { return can2_initialized; }
    uint32_t getMessagesReceived1() const { return msg_count1; }
    uint32_t getMessagesReceived2() const { return msg_count2; }
    
    // Error handling
    uint32_t getLastError() const { return last_error; }
    void clearError() { last_error = 0; }
    
protected:
    bool can1_initialized;
    bool can2_initialized;
    
    uint32_t msg_count1;  // Counter for CAN1 messages
    uint32_t msg_count2;  // Counter for CAN2 messages
    uint32_t last_error;
    
    // Message queues
    std::queue<CANMessage_t> can1_queue;
    std::queue<CANMessage_t> can2_queue;
    
    // RX activity tracking for sleep management
    uint32_t last_can_activity;
    
private:
    // Hardware setup helpers
    void setupCANInterrupts1();
    void setupCANInterrupts2();
    
    // ISR callbacks (static)
    static void IRAM_ATTR onCAN1Receive(void *ctx);
    static void IRAM_ATTR onCAN2Receive(void *ctx);
};

#endif // CAN_HANDLER_H
