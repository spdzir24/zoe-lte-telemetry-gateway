#pragma once

#include "config.h"
#include "data_manager.h"
#include <cstdint>
#include <random>

/**
 * @class DataSimulator
 * @brief Simulates vehicle data for testing communication without CAN bus
 * 
 * Generates realistic vehicle telemetry data for testing:
 * - MQTT/LTE connectivity
 * - HomeAssistant integration
 * - Data transmission pipeline
 * 
 * No actual CAN bus connection needed!
 */
class DataSimulator {
public:
    struct SimulationConfig {
        bool enabled;                      // Enable/disable simulation
        uint32_t update_interval_ms;       // Update frequency (default: 5000ms)
        bool vary_values;                  // Randomize values (vs static defaults)
        float soc_variation;               // SOC change rate (-2 to +2 % per interval)
        float temp_variation;              // Temperature variation (±2°C)
        float speed_variation;             // Random speed 0-120 km/h
        float current_variation;           // Random current ±50A
    };

    static DataSimulator& getInstance();

    /**
     * Initialize simulator with config
     * Call this in setup() if simulation enabled
     */
    bool begin();

    /**
     * Update simulated data
     * Call this in loop() at regular intervals
     * @return true if new data available
     */
    bool update();

    /**
     * Get current simulated vehicle data
     */
    const VehicleData& getData() const { return current_data; }

    /**
     * Reset simulation to default state
     */
    void reset();

    /**
     * Configure simulator
     */
    void configure(const SimulationConfig& config) {
        sim_config = config;
    }

    /**
     * Get current configuration
     */
    const SimulationConfig& getConfig() const { return sim_config; }

    /**
     * Print simulation status
     */
    void debugPrint() const;

private:
    // Simulated data generator
    void generateRandomData();
    float getRandomFloat(float min, float max);
    int32_t getRandomInt(int32_t min, int32_t max);

    // Simulation parameters
    SimulationConfig sim_config;
    VehicleData current_data;

    // State tracking
    uint32_t last_update_ms;
    float simulated_soc;
    float simulated_temp;
    float simulated_speed;
    float simulated_current;

    // Random number generator
    std::mt19937 rng;

    // Default configuration
    static const SimulationConfig DEFAULT_CONFIG;
    
    // Private constructor for singleton
    DataSimulator();
};

// Global singleton accessor
extern DataSimulator& g_simulator;
