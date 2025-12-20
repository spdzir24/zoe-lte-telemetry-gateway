#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// GPIO PIN CONFIGURATION
// ============================================================================
#define MODEM_RX_PIN 16
#define MODEM_TX_PIN 17
#define MODEM_DTR_PIN 4   // LOW = Wake-up, HIGH = Sleep request
#define MODEM_EN_PIN 27   // Power control (PWM or GPIO)
#define MODEM_NET_PIN 26  // Network status output

#define CAN1_RX_PIN 22    // High-speed CAN (500kbps) - RX from transceiver
#define CAN1_TX_PIN 21    // High-speed CAN (500kbps) - TX to transceiver
#define CAN2_RX_PIN 35    // Low-speed CAN (125kbps) - RX from transceiver
#define CAN2_TX_PIN 33    // Low-speed CAN (125kbps) - TX to transceiver

#define LED_PIN 2         // Status LED

// ============================================================================
// CAN BUS CONFIGURATION
// ============================================================================
#define CAN_SPEED_HIGH 500000  // High-speed CAN (vehicle telemetry)
#define CAN_SPEED_LOW  125000  // Low-speed CAN (comfort features)

// CAN message queue size
#define CAN_RX_QUEUE_SIZE 256

// ============================================================================
// MQTT CONFIGURATION
// ============================================================================
#define MQTT_BROKER_ADDR "192.168.1.100"
#define MQTT_BROKER_PORT 1883
#define MQTT_CLIENT_ID "zoe-telemetry-gateway"
#define MQTT_USERNAME "homeassistant"
#define MQTT_PASSWORD "your_password_here"  // Change this!

// Base topic for all published messages
#define MQTT_BASE_TOPIC "vehicle/zoe"

// MQTT publish intervals (milliseconds)
#define MQTT_PUBLISH_INTERVAL_FAST 60000   // 60 seconds (battery, SoC, speed)
#define MQTT_PUBLISH_INTERVAL_MID  300000  // 5 minutes (temperatures, voltages)
#define MQTT_PUBLISH_INTERVAL_SLOW 3600000 // 60 minutes (statistics, history)

#define MQTT_RECONNECT_INTERVAL 10000      // Retry connection every 10s
#define MQTT_KEEPALIVE 60                   // MQTT keep-alive in seconds

// ============================================================================
// SIM7080G MODEM CONFIGURATION
// ============================================================================
#define MODEM_BAUDRATE 115200
#define MODEM_NETWORK_MODE 38  // 38 = LTE only
#define MODEM_PREFERRED_MODE 1 // 1 = CAT-M, 2 = NB-IoT, 3 = Both

// GPS Configuration
#define GPS_UPDATE_INTERVAL 300000  // Update GPS every 5 minutes
#define GPS_REQUIRED_SATELLITES 4   // Minimum satellites for fix

// ============================================================================
// POWER MANAGEMENT
// ============================================================================
// Sleep timeout after no CAN activity (milliseconds)
#define SLEEP_TIMEOUT_IDLE 300000   // 5 minutes = 300 seconds
#define SLEEP_TIMEOUT_PARKED 600000 // 10 minutes for parked vehicle

// Deep sleep RTC wake-up interval (seconds)
#define RTC_WAKEUP_INTERVAL 21600  // 6 hours for GPS update

// ADC pin for battery voltage monitoring (optional)
#define BAT_MON_PIN 34
#define BAT_MON_MULTIPLIER 3.0  // Voltage divider ratio

// ============================================================================
// DATA BUFFER & TIMING
// ============================================================================
#define BUFFER_SIZE 8192
#define MAX_MQTT_PAYLOAD 4096

// CAN message filter settings
#define CAN_FILTER_MODE ACCEPT_CUSTOM  // Filter specific CAN IDs
#define CAN_FILTER_ID1 0x100  // Example: VIN-like messages
#define CAN_FILTER_ID2 0x200  // Example: Battery status

// ============================================================================
// FEATURE FLAGS
// ============================================================================
#define ENABLE_WIFI false       // Disable WiFi to save power
#define ENABLE_BLUETOOTH false  // Disable Bluetooth
#define ENABLE_GPS true         // Enable GPS (SIM7080G internal)
#define ENABLE_DEEP_SLEEP true  // Enable deep sleep after timeout
#define ENABLE_DEBUG false      // Enable serial debug output
#define ENABLE_WEB_SERVER false // Optional: web interface on GPIO 80

// ============================================================================
// DEVICE IDENTIFICATION
// ============================================================================
#define DEVICE_NAME "ZoE-LTE-Gateway-001"
#define DEVICE_TYPE "Renault-ZoE-PH2"
#define FIRMWARE_VERSION "1.0.0"
#define BUILD_DATE __DATE__

// ============================================================================
// DEBUG MACROS
// ============================================================================
#if ENABLE_DEBUG
  #define DEBUG_PRINT(x) Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(fmt, ...)
#endif

#endif // CONFIG_H
