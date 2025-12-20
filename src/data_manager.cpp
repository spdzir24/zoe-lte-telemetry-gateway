#include "data_manager.h"

DataManager::DataManager(CANHandler* can, MQTTHandler* mqtt, ModemHandler* modem)
    : can_handler(can), mqtt_handler(mqtt), modem_handler(modem),
      processed_messages(0), published_messages(0) {}

DataManager::~DataManager() {}

bool DataManager::begin() {
    DEBUG_PRINTLN("[DataMgr] Data manager started");
    registerAllZoeSignals();
    return true;
}

void DataManager::loop() {
    // This would be called periodically to process messages
}

void DataManager::registerSignal(const char* signal_name, uint32_t can_id,
                                  const CANSignal_t& signal,
                                  uint32_t publish_interval, double tolerance) {
    ManagedSignal_t managed_signal = {};
    managed_signal.name = signal_name;
    managed_signal.can_id = can_id;
    managed_signal.signal = signal;
    managed_signal.publish_interval = publish_interval;
    managed_signal.last_published = 0;
    managed_signal.last_value = 0;
    managed_signal.value_tolerance = tolerance;
    
    signal_map[can_id].push_back(managed_signal);
    
    DEBUG_PRINTF("[DataMgr] Registered signal: %s (CAN ID: 0x%03X, topic: %s)\n",
                signal_name, can_id, signal.mqtt_topic);
}

void DataManager::registerAllZoeSignals() {
    DEBUG_PRINTLN("[DataMgr] Registering Renault Zoe PH2 signals...");
    
    // Battery signals
    registerSignal("SoC", BatteryMessages::MSG_BATTERY_STATUS,
                   BatteryMessages::SIG_SOC, 60000UL, 0.1);
    registerSignal("SoH", BatteryMessages::MSG_BATTERY_STATUS,
                   BatteryMessages::SIG_SOH, 300000UL, 0.1);
    registerSignal("Voltage", BatteryMessages::MSG_BATTERY_POWER,
                   BatteryMessages::SIG_BATTERY_VOLTAGE, 60000UL, 0.5);
    registerSignal("Current", BatteryMessages::MSG_BATTERY_POWER,
                   BatteryMessages::SIG_BATTERY_CURRENT, 60000UL, 0.1);
    
    // Charging signals
    registerSignal("PlugConnected", ChargingMessages::MSG_CHARGE_STATUS,
                   ChargingMessages::SIG_PLUG_CONNECTED, 10000UL, 0.0);
    registerSignal("ChargePower", ChargingMessages::MSG_CHARGE_STATUS,
                   ChargingMessages::SIG_CHARGE_POWER, 60000UL, 0.5);
    
    // Motion signals
    registerSignal("Speed", MotionMessages::MSG_SPEED,
                   MotionMessages::SIG_VEHICLE_SPEED, 10000UL, 0.5);
    registerSignal("Consumption", MotionMessages::MSG_CONSUMPTION,
                   MotionMessages::SIG_CONSUMPTION_KWH, 60000UL, 0.1);
    
    // Climate signals
    registerSignal("InteriorTemp", ClimateMessages::MSG_INTERIOR_TEMP,
                   ClimateMessages::SIG_INTERIOR_TEMP, 300000UL, 0.5);
    
    // TPMS signals
    registerSignal("TireFL_Pressure", TPMSMessages::MSG_TPMS,
                   TPMSMessages::SIG_TIRE_FL_PRESSURE, 300000UL, 0.1);
    registerSignal("TireFR_Pressure", TPMSMessages::MSG_TPMS,
                   TPMSMessages::SIG_TIRE_FR_PRESSURE, 300000UL, 0.1);
    registerSignal("TireRL_Pressure", TPMSMessages::MSG_TPMS,
                   TPMSMessages::SIG_TIRE_RL_PRESSURE, 300000UL, 0.1);
    registerSignal("TireRR_Pressure", TPMSMessages::MSG_TPMS,
                   TPMSMessages::SIG_TIRE_RR_PRESSURE, 300000UL, 0.1);
    
    // Power signals
    registerSignal("Voltage12V", PowerMessages::MSG_AUX_VOLTAGE,
                   PowerMessages::SIG_12V_VOLTAGE, 300000UL, 0.1);
    
    DEBUG_PRINTF("[DataMgr] Total CAN message types registered: %zu\n", signal_map.size());
}

void DataManager::processCAN1Message(const CANMessage_t& msg) {
    if (signal_map.find(msg.id) == signal_map.end()) {
        return;  // No signals registered for this CAN ID
    }
    
    processed_messages++;
    
    auto& signals = signal_map[msg.id];
    for (auto& signal : signals) {
        double value = can_handler->extractSignal(msg, signal.signal);
        if (shouldPublish(signal, value)) {
            publishSignal(signal, value);
            published_messages++;
        }
    }
}

void DataManager::processCAN2Message(const CANMessage_t& msg) {
    // CAN2 not implemented yet
}

bool DataManager::shouldPublish(ManagedSignal_t& signal, double new_value) {
    uint32_t now = millis();
    
    if ((now - signal.last_published) >= signal.publish_interval) {
        double change = fabs(new_value - signal.last_value);
        if (change >= signal.value_tolerance) {
            signal.last_value = new_value;
            signal.last_published = now;
            return true;
        }
    }
    return false;
}

void DataManager::publishSignal(const ManagedSignal_t& signal, double value) {
    char topic_buffer[128];
    snprintf(topic_buffer, sizeof(topic_buffer), "%s/%s", MQTT_BASE_TOPIC, signal.signal.mqtt_topic);
    mqtt_handler->publish(topic_buffer, value, 2, true);
}

void DataManager::publishAllData() {
    DEBUG_PRINTLN("[DataMgr] Force publishing all signals...");
    // This would iterate through all signals and force publish
}

void DataManager::printStatus() {
    DEBUG_PRINTF("[DataMgr] Processed: %lu, Published: %lu, Registered signals: %zu\n",
                processed_messages, published_messages, signal_map.size());
}
