#include "can_handler.h"
#include "driver/twai.h"  // ESP32 CAN (TWAI - Two Wire Automotive Interface)

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
    twai_stop();
    twai_driver_uninstall();
}

bool CANHandler::setupCAN1(uint32_t speed) {
    // CAN1 Configuration - High Speed (500kbps)
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
        (gpio_num_t)CAN1_TX_PIN,
        (gpio_num_t)CAN1_RX_PIN,
        TWAI_MODE_NORMAL
    );
    
    twai_timing_config_t t_config;
    
    // Configure timing for 500kbps
    switch(speed) {
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
        last_error = 1002;
        return false;
    }
    
    can1_initialized = true;
    DEBUG_PRINTLN("[CAN1] Initialized at 500kbps");
    return true;
}

bool CANHandler::setupCAN2(uint32_t speed) {
    // CAN2 using alternative UART/GPIO approach
    // For true dual CAN, you would need CAN2 controller (not available on ESP32)
    // This demonstrates single CAN implementation
    // To extend: use MCP2515 SPI module for second CAN bus
    
    can2_initialized = false;  // Placeholder
    DEBUG_PRINTLN("[CAN2] Note: Dual CAN requires external MCP2515 module on SPI");
    return true;
}

bool CANHandler::readCAN1(CANMessage_t& msg) {
    if (!can1_initialized) return false;
    
    twai_message_t rx_msg;
    
    if (twai_receive(&rx_msg, pdMS_TO_TICKS(0)) == ESP_OK) {
        msg.id = rx_msg.identifier;
        msg.dlc = rx_msg.data_length_code;
        msg.timestamp = millis();
        
        memcpy(msg.data, rx_msg.data, rx_msg.data_length_code);
        
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
    
    twai_message_t tx_msg;
    tx_msg.identifier = msg.id;
    tx_msg.data_length_code = msg.dlc;
    tx_msg.extd = (msg.id > 0x7FF) ? 1 : 0;  // Extended ID if > 11 bits
    tx_msg.rtr = 0;  // Not a remote request frame
    
    memcpy(tx_msg.data, msg.data, msg.dlc);
    
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
    uint64_t raw_value = 0;
    uint8_t start_bit = signal.start_bit;
    uint8_t bit_length = signal.bit_length;
    
    // Extract bits from data array
    for (int i = 0; i < bit_length; i++) {
        uint8_t byte_index = (start_bit + i) / 8;
        uint8_t bit_in_byte = (start_bit + i) % 8;
        
        if (byte_index < 8) {
            uint8_t bit_value = (msg.data[byte_index] >> bit_in_byte) & 1;
            raw_value |= ((uint64_t)bit_value << i);
        }
    }
    
    // Handle signed values (two's complement)
    if (bit_length < 64) {
        uint64_t sign_bit = 1ULL << (bit_length - 1);
        if (raw_value & sign_bit) {
            raw_value |= ~((1ULL << bit_length) - 1);
        }
    }
    
    // Apply scaling formula: physical_value = (raw_value * factor) + offset
    double physical_value = (double)(int64_t)raw_value * signal.factor + signal.offset;
    
    return physical_value;
}

void CANHandler::setupCANInterrupts1() {
    // Interrupt setup is handled by TWAI driver automatically
}

void CANHandler::setupCANInterrupts2() {
    // Placeholder
}

void CANHandler::IRAM_ATTR onCAN1Receive(void *ctx) {
    // ISR callback for CAN1
}

void CANHandler::IRAM_ATTR onCAN2Receive(void *ctx) {
    // ISR callback for CAN2
}
