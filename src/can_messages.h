#ifndef CAN_MESSAGES_H
#define CAN_MESSAGES_H

#include <Arduino.h>
#include <map>
#include <vector>

// ============================================================================
// CAN MESSAGE STRUCTURE & DEFINITIONS
// ============================================================================

typedef struct {
    uint32_t id;           // CAN ID (11-bit or 29-bit extended)
    uint8_t dlc;           // Data Length Code (0-8)
    uint8_t data[8];       // CAN data bytes
    uint32_t timestamp;    // Milliseconds when message received
} CANMessage_t;

typedef struct {
    const char* name;      // Signal name (e.g., "SOC")
    uint8_t start_bit;     // Start bit in CAN frame
    uint8_t bit_length;    // Number of bits
    float factor;          // Scaling factor
    float offset;          // Offset value
    const char* unit;      // Unit string
    const char* mqtt_topic;// MQTT topic suffix
    uint32_t update_interval; // Update interval in ms (must be uint32_t for values > 65535)
} CANSignal_t;

// ============================================================================
// RENAULT ZOE PH2 CAN MESSAGE DEFINITIONS
// Based on CanZE database: ZOE_Ph2 CSV files
// ============================================================================

// Battery Management System (High-Speed CAN - 500kbps)
namespace BatteryMessages {
    // 0x042F - Battery Status & SoC
    const uint32_t MSG_BATTERY_STATUS = 0x042F;
    const CANSignal_t SIG_SOC = {
        "SoC", 0, 8, 0.5, 0, "%", "battery/soc", 60000UL
    };
    const CANSignal_t SIG_SOH = {
        "SoH", 8, 8, 0.5, 0, "%", "battery/soh", 300000UL
    };
    const CANSignal_t SIG_REAL_SOC = {
        "RealSOC", 16, 8, 0.5, 0, "%", "battery/real_soc", 60000UL
    };

    // 0x0637 - Cell Voltages & Temps
    const uint32_t MSG_CELL_VOLTAGES = 0x0637;
    const CANSignal_t SIG_CELL_VOLTAGE_MIN = {
        "CellVoltMin", 0, 16, 0.001, 0, "V", "battery/cell_voltage_min", 300000UL
    };
    const CANSignal_t SIG_CELL_VOLTAGE_MAX = {
        "CellVoltMax", 16, 16, 0.001, 0, "V", "battery/cell_voltage_max", 300000UL
    };

    // 0x0639 - Battery Temperature
    const uint32_t MSG_BATTERY_TEMP = 0x0639;
    const CANSignal_t SIG_TEMP_MIN = {
        "TempMin", 0, 8, 1, -40, "°C", "battery/temp_min", 300000UL
    };
    const CANSignal_t SIG_TEMP_MAX = {
        "TempMax", 8, 8, 1, -40, "°C", "battery/temp_max", 300000UL
    };
    const CANSignal_t SIG_TEMP_AVG = {
        "TempAvg", 16, 8, 1, -40, "°C", "battery/temp_avg", 300000UL
    };

    // 0x0645 - Battery Current & Voltage
    const uint32_t MSG_BATTERY_POWER = 0x0645;
    const CANSignal_t SIG_BATTERY_VOLTAGE = {
        "BatteryVolt", 0, 16, 0.1, 0, "V", "battery/voltage", 60000UL
    };
    const CANSignal_t SIG_BATTERY_CURRENT = {
        "BatteryCurrent", 16, 16, 0.1, -1638.4, "A", "battery/current", 60000UL
    };
    const CANSignal_t SIG_BATTERY_POWER = {
        "BatteryPower", 32, 16, 0.1, -3276.8, "kW", "battery/power", 60000UL
    };

    // 0x0643 - Battery Capacity Info
    const uint32_t MSG_BATTERY_CAPACITY = 0x0643;
    const CANSignal_t SIG_USABLE_CAPACITY = {
        "UsableCapacity", 0, 16, 0.1, 0, "kWh", "battery/usable_capacity", 3600000UL
    };
    const CANSignal_t SIG_MAX_CAPACITY = {
        "MaxCapacity", 16, 16, 0.1, 0, "kWh", "battery/max_capacity", 3600000UL
    };
    const CANSignal_t SIG_ENERGY_TO_FULL = {
        "EnergyToFull", 32, 16, 0.1, 0, "kWh", "battery/energy_to_full", 60000UL
    };

    // 0x0655 - Charge Cycles
    const uint32_t MSG_CHARGE_CYCLES = 0x0655;
    const CANSignal_t SIG_FULL_CYCLES = {
        "FullCycles", 0, 16, 1, 0, "count", "battery/full_cycles", 3600000UL
    };
}

// Charging System (High-Speed CAN)
namespace ChargingMessages {
    // 0x1F8 - Plug Status & Charging Power
    const uint32_t MSG_CHARGE_STATUS = 0x1F8;
    const CANSignal_t SIG_PLUG_CONNECTED = {
        "PlugConnected", 0, 1, 1, 0, "bool", "charging/plug_connected", 10000UL
    };
    const CANSignal_t SIG_CHARGE_POWER = {
        "ChargePower", 8, 16, 0.1, 0, "kW", "charging/power", 60000UL
    };
    const CANSignal_t SIG_CHARGE_VOLTAGE = {
        "ChargeVoltage", 24, 16, 0.1, 0, "V", "charging/voltage", 60000UL
    };
    const CANSignal_t SIG_CHARGE_CURRENT = {
        "ChargeCurrent", 40, 16, 0.1, 0, "A", "charging/current", 60000UL
    };
}

// Vehicle Motion & Status (Low-Speed CAN - 125kbps)
namespace MotionMessages {
    // 0x140 - Vehicle Speed
    const uint32_t MSG_SPEED = 0x140;
    const CANSignal_t SIG_VEHICLE_SPEED = {
        "Speed", 0, 16, 0.01, 0, "km/h", "motion/speed", 10000UL
    };
    const CANSignal_t SIG_BRAKE_PRESSURE = {
        "BrakePressure", 16, 16, 0.01, 0, "bar", "motion/brake_pressure", 10000UL
    };

    // 0x0154 - Motor RPM & Power
    const uint32_t MSG_MOTOR_STATUS = 0x0154;
    const CANSignal_t SIG_MOTOR_RPM = {
        "MotorRPM", 0, 16, 1, 0, "rpm", "motion/motor_rpm", 10000UL
    };
    const CANSignal_t SIG_MOTOR_TORQUE = {
        "MotorTorque", 16, 16, 0.1, -3276.8, "Nm", "motion/motor_torque", 10000UL
    };

    // 0x119 - Reconstruction (consumption)
    const uint32_t MSG_CONSUMPTION = 0x119;
    const CANSignal_t SIG_CONSUMPTION_KWH = {
        "ConsumptionKWh", 0, 16, 0.01, 0, "kWh/100km", "motion/consumption_kwh_100km", 60000UL
    };
    const CANSignal_t SIG_CONSUMPTION_INSTANT = {
        "InstantConsumption", 16, 16, 0.01, 0, "kW", "motion/consumption_instant", 10000UL
    };

    // 0x100 - Distances & Ranges
    const uint32_t MSG_RANGE = 0x100;
    const CANSignal_t SIG_AVAILABLE_RANGE = {
        "AvailableRange", 0, 16, 1, 0, "km", "motion/available_range", 60000UL
    };
    const CANSignal_t SIG_TRIP_DISTANCE = {
        "TripDistance", 16, 32, 0.01, 0, "km", "motion/trip_distance", 60000UL
    };
}

// Climate & Environmental
namespace ClimateMessages {
    // 0x55B - Interior Temperature
    const uint32_t MSG_INTERIOR_TEMP = 0x55B;
    const CANSignal_t SIG_INTERIOR_TEMP = {
        "InteriorTemp", 0, 8, 0.5, -40, "°C", "climate/interior_temp", 300000UL
    };

    // 0x65F - Heat Pump Data
    const uint32_t MSG_HEAT_PUMP = 0x65F;
    const CANSignal_t SIG_HP_PRESSURE = {
        "HPPressure", 0, 16, 0.1, 0, "bar", "climate/heat_pump_pressure", 300000UL
    };
    const CANSignal_t SIG_HP_EVAP_TEMP = {
        "HPEvapTemp", 16, 8, 1, -40, "°C", "climate/heat_pump_evap_temp", 300000UL
    };
    const CANSignal_t SIG_HP_COND_TEMP = {
        "HPCondTemp", 24, 8, 1, -40, "°C", "climate/heat_pump_cond_temp", 300000UL
    };
}

// Door & Light Status
namespace StatusMessages {
    // 0x060 - Door Status
    const uint32_t MSG_DOOR_STATUS = 0x060;
    // Bit fields: 0=FL_Open, 1=FR_Open, 2=RL_Open, 3=RR_Open, 4=Trunk_Open

    // 0x061 - Light Status  
    const uint32_t MSG_LIGHT_STATUS = 0x061;
    // Bit fields: various light indicators
}

// TPMS - Tire Pressure Monitoring
namespace TPMSMessages {
    // 0x354 - TPMS Data
    const uint32_t MSG_TPMS = 0x354;
    const CANSignal_t SIG_TIRE_FL_PRESSURE = {
        "TireFL_Pressure", 0, 8, 0.5, 0, "bar", "tpms/tire_fl_pressure", 300000UL
    };
    const CANSignal_t SIG_TIRE_FR_PRESSURE = {
        "TireFR_Pressure", 8, 8, 0.5, 0, "bar", "tpms/tire_fr_pressure", 300000UL
    };
    const CANSignal_t SIG_TIRE_RL_PRESSURE = {
        "TireRL_Pressure", 16, 8, 0.5, 0, "bar", "tpms/tire_rl_pressure", 300000UL
    };
    const CANSignal_t SIG_TIRE_RR_PRESSURE = {
        "TireRR_Pressure", 24, 8, 0.5, 0, "bar", "tpms/tire_rr_pressure", 300000UL
    };
}

// Voltage & Power Systems
namespace PowerMessages {
    // 0x35E - 12V & 24V Auxiliary
    const uint32_t MSG_AUX_VOLTAGE = 0x35E;
    const CANSignal_t SIG_12V_VOLTAGE = {
        "Voltage12V", 0, 16, 0.01, 0, "V", "power/voltage_12v", 300000UL
    };
    const CANSignal_t SIG_24V_VOLTAGE = {
        "Voltage24V", 16, 16, 0.01, 0, "V", "power/voltage_24v", 300000UL
    };

    // 0x35F - HV Power Module Temp
    const uint32_t MSG_POWER_MODULE_TEMP = 0x35F;
    const CANSignal_t SIG_POWER_MODULE_TEMP = {
        "PowerModuleTemp", 0, 8, 1, -40, "°C", "power/power_module_temp", 300000UL
    };
}

// Recuperation Energy
namespace RecuperationMessages {
    // 0x0634 - Recuperation Data
    const uint32_t MSG_RECUPERATION = 0x0634;
    const CANSignal_t SIG_MAX_RECUP = {
        "MaxRecupPower", 0, 16, 0.1, 0, "kW", "recuperation/max_power", 60000UL
    };
    const CANSignal_t SIG_INSTANT_RECUP = {
        "InstantRecup", 16, 16, 0.1, 0, "kW", "recuperation/instant_power", 10000UL
    };
    const CANSignal_t SIG_TOTAL_RECUP = {
        "TotalRecup", 32, 32, 0.01, 0, "kWh", "recuperation/total_energy", 300000UL
    };
}

#endif // CAN_MESSAGES_H
