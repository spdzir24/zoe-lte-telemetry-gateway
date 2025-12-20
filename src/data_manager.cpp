#include "data_manager.h"

DataManager::DataManager(CANHandler* can, MQTTHandler* mqtt, ModemHandler* modem)
    : can_handler(can),
      mqtt_handler(mqtt),
      modem_handler(modem),
      processed_messages(0),
      published_messages(0) {}

DataManager::~DataManager() {}

bool DataManager::begin() {
    DEBUG_PRINTLN("[DataMgr] Data manager started");
    return true;
}

void DataManager::loop() {
    // Called from main loop for periodic tasks
}

void DataManager::registerSignal(const char* signal_name, uint32_t can_id, const CANSignal_t& signal,
                                  uint32_t publish_interval, double tolerance) {
    ManagedSignal_t managed_signal;
    managed_signal.name = signal_name;
    managed_signal.can_id = can_id;
    managed_signal.signal = signal;
    managed_signal.publish_interval = publish_interval;
    managed_signal.last_published = 0;
    managed_signal.last_value = 0.0;
    managed_signal.value_tolerance = tolerance;
    
    signal_map[can_id].push_back(managed_signal);
    
    DEBUG_PRINTF("[DataMgr] Registered signal: %s (CAN ID: 0x%03X, topic: %s)\n",
                 signal_name, can_id, signal.mqtt_topic);
}

void DataManager::registerAllZoeSignals() {
    DEBUG_PRINTLN("[DataMgr] Registering Renault Zoe PH2 signals...");
    
    // Battery Management Signals
    registerSignal("SoC", BatteryMessages::MSG_BATTERY_STATUS, BatteryMessages::SIG_SOC,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.5);  // Publish if change > 0.5%
    registerSignal("SoH", BatteryMessages::MSG_BATTERY_STATUS, BatteryMessages::SIG_SOH,
                   MQTT_PUBLISH_INTERVAL_MID, 1.0);
    registerSignal("RealSOC", BatteryMessages::MSG_BATTERY_STATUS, BatteryMessages::SIG_REAL_SOC,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.5);
    
    registerSignal("CellVoltMin", BatteryMessages::MSG_CELL_VOLTAGES, BatteryMessages::SIG_CELL_VOLTAGE_MIN,
                   MQTT_PUBLISH_INTERVAL_MID, 0.01);
    registerSignal("CellVoltMax", BatteryMessages::MSG_CELL_VOLTAGES, BatteryMessages::SIG_CELL_VOLTAGE_MAX,
                   MQTT_PUBLISH_INTERVAL_MID, 0.01);
    
    registerSignal("BatteryTempMin", BatteryMessages::MSG_BATTERY_TEMP, BatteryMessages::SIG_TEMP_MIN,
                   MQTT_PUBLISH_INTERVAL_MID, 1.0);
    registerSignal("BatteryTempMax", BatteryMessages::MSG_BATTERY_TEMP, BatteryMessages::SIG_TEMP_MAX,
                   MQTT_PUBLISH_INTERVAL_MID, 1.0);
    registerSignal("BatteryTempAvg", BatteryMessages::MSG_BATTERY_TEMP, BatteryMessages::SIG_TEMP_AVG,
                   MQTT_PUBLISH_INTERVAL_MID, 1.0);
    
    registerSignal("BatteryVoltage", BatteryMessages::MSG_BATTERY_POWER, BatteryMessages::SIG_BATTERY_VOLTAGE,
                   MQTT_PUBLISH_INTERVAL_FAST, 1.0);
    registerSignal("BatteryCurrent", BatteryMessages::MSG_BATTERY_POWER, BatteryMessages::SIG_BATTERY_CURRENT,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.5);
    registerSignal("BatteryPower", BatteryMessages::MSG_BATTERY_POWER, BatteryMessages::SIG_BATTERY_POWER,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.5);
    
    registerSignal("UsableCapacity", BatteryMessages::MSG_BATTERY_CAPACITY, BatteryMessages::SIG_USABLE_CAPACITY,
                   MQTT_PUBLISH_INTERVAL_SLOW, 0.1);
    registerSignal("MaxCapacity", BatteryMessages::MSG_BATTERY_CAPACITY, BatteryMessages::SIG_MAX_CAPACITY,
                   MQTT_PUBLISH_INTERVAL_SLOW, 0.1);
    registerSignal("EnergyToFull", BatteryMessages::MSG_BATTERY_CAPACITY, BatteryMessages::SIG_ENERGY_TO_FULL,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.5);
    
    registerSignal("FullCycles", BatteryMessages::MSG_CHARGE_CYCLES, BatteryMessages::SIG_FULL_CYCLES,
                   MQTT_PUBLISH_INTERVAL_SLOW, 1.0);
    
    // Charging Signals
    registerSignal("PlugConnected", ChargingMessages::MSG_CHARGE_STATUS, ChargingMessages::SIG_PLUG_CONNECTED,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.0);
    registerSignal("ChargePower", ChargingMessages::MSG_CHARGE_STATUS, ChargingMessages::SIG_CHARGE_POWER,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.5);
    registerSignal("ChargeVoltage", ChargingMessages::MSG_CHARGE_STATUS, ChargingMessages::SIG_CHARGE_VOLTAGE,
                   MQTT_PUBLISH_INTERVAL_FAST, 1.0);
    registerSignal("ChargeCurrent", ChargingMessages::MSG_CHARGE_STATUS, ChargingMessages::SIG_CHARGE_CURRENT,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.5);
    
    // Motion/Speed Signals
    registerSignal("Speed", MotionMessages::MSG_SPEED, MotionMessages::SIG_VEHICLE_SPEED,
                   MQTT_PUBLISH_INTERVAL_FAST, 1.0);
    registerSignal("Consumption", MotionMessages::MSG_CONSUMPTION, MotionMessages::SIG_CONSUMPTION_KWH,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.5);
    registerSignal("AvailableRange", MotionMessages::MSG_RANGE, MotionMessages::SIG_AVAILABLE_RANGE,
                   MQTT_PUBLISH_INTERVAL_FAST, 1.0);
    
    // Climate Signals
    registerSignal("InteriorTemp", ClimateMessages::MSG_INTERIOR_TEMP, ClimateMessages::SIG_INTERIOR_TEMP,
                   MQTT_PUBLISH_INTERVAL_MID, 1.0);
    registerSignal("HeatPumpPressure", ClimateMessages::MSG_HEAT_PUMP, ClimateMessages::SIG_HP_PRESSURE,
                   MQTT_PUBLISH_INTERVAL_MID, 0.5);
    registerSignal("HeatPumpEvapTemp", ClimateMessages::MSG_HEAT_PUMP, ClimateMessages::SIG_HP_EVAP_TEMP,
                   MQTT_PUBLISH_INTERVAL_MID, 1.0);
    registerSignal("HeatPumpCondTemp", ClimateMessages::MSG_HEAT_PUMP, ClimateMessages::SIG_HP_COND_TEMP,
                   MQTT_PUBLISH_INTERVAL_MID, 1.0);
    
    // Power Signals
    registerSignal("Voltage12V", PowerMessages::MSG_AUX_VOLTAGE, PowerMessages::SIG_12V_VOLTAGE,
                   MQTT_PUBLISH_INTERVAL_MID, 0.5);
    registerSignal("Voltage24V", PowerMessages::MSG_AUX_VOLTAGE, PowerMessages::SIG_24V_VOLTAGE,
                   MQTT_PUBLISH_INTERVAL_MID, 0.5);
    registerSignal("PowerModuleTemp", PowerMessages::MSG_POWER_MODULE_TEMP, PowerMessages::SIG_POWER_MODULE_TEMP,
                   MQTT_PUBLISH_INTERVAL_MID, 1.0);
    
    // Recuperation Signals
    registerSignal("MaxRecupPower", RecuperationMessages::MSG_RECUPERATION, RecuperationMessages::SIG_MAX_RECUP,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.5);
    registerSignal("InstantRecup", RecuperationMessages::MSG_RECUPERATION, RecuperationMessages::SIG_INSTANT_RECUP,
                   MQTT_PUBLISH_INTERVAL_FAST, 0.1);
    registerSignal("TotalRecup", RecuperationMessages::MSG_RECUPERATION, RecuperationMessages::SIG_TOTAL_RECUP,
                   MQTT_PUBLISH_INTERVAL_MID, 0.5);
    
    // TPMS Signals
    registerSignal("TireFL_Pressure", TPMSMessages::MSG_TPMS, TPMSMessages::SIG_TIRE_FL_PRESSURE,
                   MQTT_PUBLISH_INTERVAL_MID, 0.1);
    registerSignal("TireFR_Pressure", TPMSMessages::MSG_TPMS, TPMSMessages::SIG_TIRE_FR_PRESSURE,
                   MQTT_PUBLISH_INTERVAL_MID, 0.1);
    registerSignal("TireRL_Pressure", TPMSMessages::MSG_TPMS, TPMSMessages::SIG_TIRE_RL_PRESSURE,
                   MQTT_PUBLISH_INTERVAL_MID, 0.1);
    registerSignal("TireRR_Pressure", TPMSMessages::MSG_TPMS, TPMSMessages::SIG_TIRE_RR_PRESSURE,
                   MQTT_PUBLISH_INTERVAL_MID, 0.1);
    
    DEBUG_PRINTF("[DataMgr] Total CAN message types registered: %zu\n", signal_map.size());
}

void DataManager::processCAN1Message(const CANMessage_t& msg) {
    processed_messages++;
    
    // Find signals registered for this CAN ID
    auto it = signal_map.find(msg.id);
    if (it != signal_map.end()) {
        // Process each signal in this message
        for (ManagedSignal_t& signal : it->second) {
            double value = can_handler.extractSignal(msg, signal.signal);
            
            if (shouldPublish(signal, value)) {
                publishSignal(signal, value);
                signal.last_value = value;
                signal.last_published = millis();
                published_messages++;
            }
        }
    }
}

void DataManager::processCAN2Message(const CANMessage_t& msg) {
    // Same as CAN1 for now
    processCAN1Message(msg);
}

bool DataManager::shouldPublish(ManagedSignal_t& signal, double new_value) {
    uint32_t elapsed = millis() - signal.last_published;
    
    // Always publish if interval has elapsed
    if (elapsed >= signal.publish_interval) {
        return true;
    }
    
    // Publish if value changed beyond tolerance
    if (signal.value_tolerance > 0.0) {
        if (fabs(new_value - signal.last_value) >= signal.value_tolerance) {
            return true;
        }
    }
    
    return false;
}

void DataManager::publishSignal(const ManagedSignal_t& signal, double value) {
    // Create full MQTT topic
    char full_topic[256];
    snprintf(full_topic, sizeof(full_topic), "%s/%s", MQTT_BASE_TOPIC, signal.signal.mqtt_topic);
    
    // Publish with appropriate precision
    uint8_t precision = 2;
    if (strstr(signal.signal.unit, "%") != nullptr || strstr(signal.signal.unit, "count") != nullptr) {
        precision = 0;  // Integer for percentages and counts
    } else if (strstr(signal.signal.unit, "V") != nullptr) {
        precision = 2;  // 2 decimals for voltage
    } else if (strstr(signal.signal.unit, "A") != nullptr) {
        precision = 1;  // 1 decimal for current
    }
    
    mqtt_handler->publish(full_topic, value, precision);
}

void DataManager::publishSignalWithUnit(const char* mqtt_topic, double value, const char* unit) {
    char payload[64];
    snprintf(payload, sizeof(payload), "%.2f %s", value, unit);
    mqtt_handler->publish(mqtt_topic, payload);
}

void DataManager::publishAllData() {
    DEBUG_PRINTLN("[DataMgr] Force publishing all signals...");
    
    uint32_t old_timeout = 0;
    for (auto& pair : signal_map) {
        for (ManagedSignal_t& signal : pair.second) {
            signal.last_published = 0;  // Force publish
        }
    }
}

void DataManager::printStatus() {
    DEBUG_PRINTF("[DataMgr] Processed: %lu, Published: %lu, Registered signals: %zu\n",
                 processed_messages, published_messages, signal_map.size());
}
