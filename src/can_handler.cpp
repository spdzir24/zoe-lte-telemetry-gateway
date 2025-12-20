#include "can_handler.h"
#include <driver/twai.h>  // Correct include for TWAI
#include <cstring>  // For memcpy

CANHandler::CANHandler()
    : can1_initialized(false),
      can2_initialized(false),
      msg_count1(0),
      msg_count2(0),
      last_error(0),
      last_can_activity(0) {}

CANHandler::~CANHandler() {
    end();
}

bool CANHandler::begin() {
    return setupCAN1() && setupCAN2();
}

void CANHandler::end() {
    if (can1_initialized) {
        twai_stop();
        twai_driver_uninstall();
        can1_initialized = false;
    }
}

bool CANHandler::setupCAN1(uint32_t speed) {
    // CAN1 Configuration - High Speed (500kbps)
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN1_TX_PIN,
        (gpio_num_t)CAN1_RX_PIN,
        TWAI_MODE_NORMAL
    );
    
    // Silence GCC warnings about missing field initializers
    g_config.rx_queue_len = 32;
    g_config.tx_queue_len = 16;
    
    twai_timing_config_t t_config;
    
    // Configure timing based on speed
    switch(speed) {
        case 1000000:
            t_config = TWAI_TIMING_CONFIG_1MBPS();
            break;
        case 500000:
            t_config = TWAI_TIMING_CONFIG_500KBPS();
            break;
        case 250000:
            t_config = TWAI_TIMING_CONFIG_250KBPS();
            break;
        case 125000:
            t_config = TWAI_TIMING_CONFIG_125KBPS();
            break;
        default:
            t_config = TWAI_TIMING_CONFIG_500KBPS();
    }
    
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    
    // Install TWAI driver
    if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
        DEBUG_PRINTLN("[CAN1] Driver install failed");
        last_error = 1001;
        return false;
    }
    
    // Start TWAI driver
    if (twai_start() != ESP_OK) {
        DEBUG_PRINTLN("[CAN1] Driver start failed");
        twai_driver_uninstall();
        last_error = 1002;
        return false;
    }
    
    // Configure alerts to enable RX data detection
    uint32_t alerts_to_enable = TWAI_ALERT_RX_DATA | TWAI_ALERT_ERR_PASS | TWAI_ALERT_BUS_ERROR;
    if (twai_reconfigure_alerts(alerts_to_enable, NULL) != ESP_OK) {
        DEBUG_PRINTLN("[CAN1] Alert configuration failed");
    }
    
    can1_initialized = true;
    DEBUG_PRINTF("[CAN1] Initialized successfully at %lu bps\n", speed);
    return true;
}

bool CANHandler::setupCAN2(uint32_t speed) {
    // CAN2 Note: ESP32 has only one hardware CAN module (TWAI)
    // For true dual CAN, external MCP2515 module would be needed on SPI
    // This is kept as placeholder for future expansion
    DEBUG_PRINTLN("[CAN2] Note: Dual CAN requires external MCP2515 module. Currently disabled.");
    can2_initialized = false;
    return true;
}

bool CANHandler::readCAN1(CANMessage_t& msg) {
    if (!can1_initialized) return false;
    
    twai_message_t rx_msg;
    
    // Use non-blocking read
    if (twai_receive(&rx_msg, 0) == ESP_OK) {
        msg.id = rx_msg.identifier;
        msg.dlc = rx_msg.data_length_code;
        msg.timestamp = millis();
        
        // Safe copy of data
        if (rx_msg.data_length_code <= 8) {
            memcpy(msg.data, rx_msg.data, rx_msg.data_length_code);
        }
        
        msg_count1++;
        last_can_activity = millis();
        
        return true;
    }
    
    return false;
}

bool CANHandler::readCAN2(CANMessage_t& msg) {
    // Placeholder for CAN2 (would use MCP2515 or similar)
    return false;
}

bool CANHandler::sendCAN1(const CANMessage_t& msg) {
    if (!can1_initialized) return false;
    
    twai_message_t tx_msg = {};
    tx_msg.identifier = msg.id;
    tx_msg.data_length_code = msg.dlc;
    tx_msg.extd = (msg.id > 0x7FF) ? 1 : 0;  // Extended ID if > 11 bits
    tx_msg.rtr = 0;  // Not a remote request frame
    tx_msg.ss = 0;   // Standard frame
    
    if (msg.dlc <= 8) {
        memcpy(tx_msg.data, msg.data, msg.dlc);
    }
    
    if (twai_transmit(&tx_msg, pdMS_TO_TICKS(1000)) == ESP_OK) {
        DEBUG_PRINTF("[CAN1] Sent ID: 0x%03X\n", msg.id);
        return true;
    }
    
    DEBUG_PRINTF("[CAN1] Transmit failed for ID: 0x%03X\n", msg.id);
    last_error = 1003;
    return false;
}

bool CANHandler::sendCAN2(const CANMessage_t& msg) {
    return false;  // CAN2 not implemented
}

double CANHandler::extractSignal(const CANMessage_t& msg, const CANSignal_t& signal) {
    // Extract signal from CAN message using bit position and length
    // This implementation supports little-endian (Intel format)
    
    if (msg.dlc == 0) return signal.offset;  // No data
    
    uint64_t raw_value = 0;
    uint8_t start_bit = signal.start_bit;
    uint8_t bit_length = signal.bit_length;
    
    // Extract bits from data array (little-endian)
    for (uint8_t i = 0; i < bit_length; i++) {
        uint8_t byte_index = (start_bit + i) / 8;
        uint8_t bit_in_byte = (start_bit + i) % 8;
        
        if (byte_index < msg.dlc && byte_index < 8) {
            uint8_t bit_value = (msg.data[byte_index] >> bit_in_byte) & 1;
            raw_value |= ((uint64_t)bit_value << i);
        }
    }
    
    // Handle signed values (two's complement)
    if (bit_length < 64) {
        uint64_t sign_bit = 1ULL << (bit_length - 1);
        if (raw_value & sign_bit) {
            // Sign extend
            raw_value |= (0xFFFFFFFFFFFFFFFFULL << bit_length);
        }
    }
    
    // Apply scaling formula: physical_value = (raw_value * factor) + offset
    int64_t signed_value = (int64_t)raw_value;
    double physical_value = (double)signed_value * signal.factor + signal.offset;
    
    return physical_value;
}

void CANHandler::setupCANInterrupts1() {
    // Interrupt setup is handled by TWAI driver with alert system
}

void CANHandler::setupCANInterrupts2() {
    // Placeholder
}

void CANHandler::IRAM_ATTR onCAN1Receive(void *ctx) {
    // ISR callback for CAN1 - handled via twai_read_alerts
}

void CANHandler::IRAM_ATTR onCAN2Receive(void *ctx) {
    // ISR callback for CAN2
}
