#include "data_simulator.h"
#include "config.h"
#include <Arduino.h>
#include <cmath>
#include <ctime>

// Default simulation configuration
const DataSimulator::SimulationConfig DataSimulator::DEFAULT_CONFIG = {
    .enabled = false,                  // Disabled by default
    .update_interval_ms = 5000,        // Update every 5 seconds
    .vary_values = true,               // Randomize values
    .soc_variation = 0.5f,             // SOC change 0-2% per interval
    .temp_variation = 2.0f,            // Temperature ±2°C
    .speed_variation = 1.0f,           // Random speeds
    .current_variation = 1.0f          // Random currents
};

// Global instance
DataSimulator g_simulator;

// Singleton instance
DataSimulator& DataSimulator::getInstance() {
    static DataSimulator instance;
    return instance;
}

DataSimulator::DataSimulator()
    : sim_config(DEFAULT_CONFIG),
      last_update_ms(0),
      simulated_soc(80.0f),
      simulated_temp(25.0f),
      simulated_speed(0.0f),
      simulated_current(0.0f),
      rng(std::time(nullptr)) {
    // Initialize with default vehicle data
    memset(&current_data, 0, sizeof(current_data));
    current_data.timestamp_ms = millis();
    current_data.soc_percent = 80.0f;
    current_data.battery_temp_c = 25.0f;
    current_data.dc_voltage = 400.0f;
    current_data.dc_current_a = 0.0f;
}

bool DataSimulator::begin() {
    if (!sim_config.enabled) {
        DEBUG_PRINTLN("[Simulator] Disabled in configuration");
        return false;
    }

    DEBUG_PRINTLN("[Simulator] Initializing data simulator...");
    DEBUG_PRINTF("[Simulator] Update interval: %u ms\n", sim_config.update_interval_ms);
    DEBUG_PRINTF("[Simulator] Value variation: %s\n", sim_config.vary_values ? "ENABLED" : "DISABLED");

    last_update_ms = millis();
    reset();

    DEBUG_PRINTLN("[Simulator] Data simulator initialized successfully");
    return true;
}

bool DataSimulator::update() {
    if (!sim_config.enabled) {
        return false;
    }

    uint32_t now_ms = millis();
    if ((now_ms - last_update_ms) < sim_config.update_interval_ms) {
        return false;  // Not time to update yet
    }

    last_update_ms = now_ms;

    if (sim_config.vary_values) {
        generateRandomData();
    }

    // Update timestamp
    current_data.timestamp_ms = now_ms;

    return true;  // New data available
}

void DataSimulator::generateRandomData() {
    // SOC gradually decreases with some random variation
    float soc_delta = getRandomFloat(-sim_config.soc_variation, sim_config.soc_variation);
    simulated_soc += soc_delta;
    simulated_soc = fmax(0.0f, fmin(100.0f, simulated_soc));  // Clamp 0-100%
    current_data.soc_percent = simulated_soc;

    // Temperature oscillates around average
    float temp_delta = getRandomFloat(-sim_config.temp_variation, sim_config.temp_variation);
    simulated_temp = 25.0f + temp_delta;
    simulated_temp = fmax(-20.0f, fmin(60.0f, simulated_temp));  // Realistic range
    current_data.battery_temp_c = simulated_temp;

    // Speed: 50% idle, 50% driving
    if (getRandomInt(0, 100) < 50) {
        simulated_speed = 0.0f;  // Idle
    } else {
        simulated_speed = getRandomFloat(10.0f, 120.0f);  // 10-120 km/h
    }
    current_data.speed_kmh = simulated_speed;

    // Current correlates with speed and SOC
    if (simulated_speed > 10.0f) {
        // Driving: discharge current
        simulated_current = getRandomFloat(-40.0f, -10.0f);  // Discharging
    } else if (getRandomInt(0, 100) < 30) {
        // Sometimes charging
        simulated_current = getRandomFloat(5.0f, 30.0f);  // Charging
    } else {
        // Idle drain
        simulated_current = getRandomFloat(-5.0f, 0.5f);
    }
    current_data.dc_current_a = simulated_current;

    // Voltage correlates with SOC and current
    float base_voltage = 350.0f + (simulated_soc * 0.5f);  // 350-400V range
    float voltage_variation = getRandomFloat(-2.0f, 2.0f);
    current_data.dc_voltage = base_voltage + voltage_variation;

    // Power calculation
    current_data.power_kw = (current_data.dc_voltage * current_data.dc_current_a) / 1000.0f;

    // Odometer: increase by simulated distance
    float distance_delta = (simulated_speed / 3.6f) * (sim_config.update_interval_ms / 1000.0f) / 1000.0f;  // km
    current_data.odometer_km += distance_delta;

    // Cabin temperature: random fluctuation
    current_data.cabin_temp_c = 20.0f + getRandomFloat(-5.0f, 5.0f);

    // Motor temperature: correlates with power
    float motor_base = 25.0f;
    if (fabs(current_data.power_kw) > 20.0f) {
        motor_base += 30.0f;  // Gets hot under load
    } else if (fabs(current_data.power_kw) > 10.0f) {
        motor_base += 15.0f;
    }
    current_data.motor_temp_c = motor_base + getRandomFloat(-3.0f, 3.0f);

    // Motor speed: 0 when idle
    if (simulated_speed > 5.0f) {
        current_data.motor_rpm = simulated_speed * 100.0f;  // Rough estimate
    } else {
        current_data.motor_rpm = 0;
    }

    // GPS data: simulate movement in small area
    // Default location: Lauf an der Pegnitz
    static float lat = 49.5154f;
    static float lon = 11.5023f;

    if (simulated_speed > 10.0f) {
        // Generate random walk
        lat += getRandomFloat(-0.001f, 0.001f);
        lon += getRandomFloat(-0.001f, 0.001f);
    }
    current_data.gps_latitude = lat;
    current_data.gps_longitude = lon;
    current_data.gps_satellites = getRandomInt(8, 14);

    // Door locks: sometimes random changes
    if (getRandomInt(0, 100) < 5) {
        current_data.doors_locked = !current_data.doors_locked;
    }

    // Charging status
    if (simulated_current > 5.0f) {
        current_data.charging = true;
    } else if (simulated_current < -5.0f) {
        current_data.charging = false;
    }
}

float DataSimulator::getRandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

int32_t DataSimulator::getRandomInt(int32_t min, int32_t max) {
    std::uniform_int_distribution<int32_t> dist(min, max);
    return dist(rng);
}

void DataSimulator::reset() {
    current_data = {};
    current_data.timestamp_ms = millis();
    current_data.soc_percent = 80.0f;
    current_data.battery_temp_c = 25.0f;
    current_data.dc_voltage = 400.0f;
    current_data.dc_current_a = 0.0f;
    current_data.cabin_temp_c = 20.0f;
    current_data.motor_temp_c = 25.0f;
    current_data.gps_latitude = 49.5154f;   // Lauf an der Pegnitz
    current_data.gps_longitude = 11.5023f;
    current_data.gps_satellites = 12;
    current_data.doors_locked = true;
    current_data.charging = false;
    current_data.odometer_km = 0.0f;

    simulated_soc = 80.0f;
    simulated_temp = 25.0f;
    simulated_speed = 0.0f;
    simulated_current = 0.0f;

    DEBUG_PRINTLN("[Simulator] Reset to default state");
}

void DataSimulator::debugPrint() const {
    if (!sim_config.enabled) {
        DEBUG_PRINTLN("[Simulator] DISABLED");
        return;
    }

    DEBUG_PRINTLN("\n=== SIMULATOR DATA ===");
    DEBUG_PRINTF("SOC: %.1f%%\n", current_data.soc_percent);
    DEBUG_PRINTF("Battery Temp: %.1f°C\n", current_data.battery_temp_c);
    DEBUG_PRINTF("Voltage: %.1fV\n", current_data.dc_voltage);
    DEBUG_PRINTF("Current: %.1fA\n", current_data.dc_current_a);
    DEBUG_PRINTF("Power: %.2fkW\n", current_data.power_kw);
    DEBUG_PRINTF("Speed: %.1f km/h\n", current_data.speed_kmh);
    DEBUG_PRINTF("Motor Temp: %.1f°C\n", current_data.motor_temp_c);
    DEBUG_PRINTF("Motor RPM: %.0f\n", current_data.motor_rpm);
    DEBUG_PRINTF("Cabin Temp: %.1f°C\n", current_data.cabin_temp_c);
    DEBUG_PRINTF("GPS: %.4f, %.4f (%d sats)\n",
                 current_data.gps_latitude,
                 current_data.gps_longitude,
                 current_data.gps_satellites);
    DEBUG_PRINTF("Odometer: %.1f km\n", current_data.odometer_km);
    DEBUG_PRINTF("Charging: %s | Doors: %s\n",
                 current_data.charging ? "YES" : "NO",
                 current_data.doors_locked ? "LOCKED" : "UNLOCKED");
    DEBUG_PRINTLN("====================\n");
}
