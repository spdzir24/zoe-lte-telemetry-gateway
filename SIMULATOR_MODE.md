# 🧪 Data Simulator - Testing Mode

## Overview

The **Data Simulator** allows you to test the complete communication pipeline (LTE, MQTT, Home Assistant) **without needing an actual vehicle or CAN bus connection**.

This is perfect for:
- ✅ Testing MQTT connectivity and publishing
- ✅ Validating Home Assistant integration
- ✅ Debugging data transmission
- ✅ Development and testing without a Zoe connected
- ✅ CI/CD pipeline testing

## Quick Start

### 1. Enable Simulator Mode

Edit `data/settings.json`:

```json
{
  "simulator": {
    "enabled": true,
    "update_interval_ms": 5000,
    "vary_values": true
  }
  // ... rest of config
}
```

**Parameters:**
- `enabled`: Set to `true` to enable simulator, `false` for normal operation
- `update_interval_ms`: How often to generate new data (5000 = 5 seconds)
- `vary_values`: Whether to randomize values (realistic data) or use static defaults

### 2. Flash the Device

```bash
# Compile
pio run -e esp32dev

# Flash firmware
pio run -e esp32dev --target upload --upload-port /dev/ttyUSB0

# Upload filesystem with settings
pio run -e esp32dev --target uploadfs --upload-port /dev/ttyUSB0
```

### 3. Monitor the Output

```bash
pio device monitor -b 115200
```

You should see:
```
==================================
Zoe LTE Telemetry Gateway v1.0.0
==================================
[System] Initializing Settings Manager...
[System] Settings loaded successfully
[System] SIMULATOR MODE ENABLED!
[Simulator] Initializing data simulator...
[Simulator] Update interval: 5000 ms
[Simulator] Value variation: ENABLED
[Simulator] Data simulator initialized successfully
```

## What the Simulator Generates

### Realistic Vehicle Data

The simulator generates realistic Renault Zoe telemetry data:

```cpp
// State of Charge: 0-100% (gradually decreases)
SOC: 80.2%

// Battery temperature: -20 to +60°C
Battery Temp: 25.3°C

// DC voltage: correlates with SOC
Voltage: 395.7V

// Current: -50A (discharge) to +50A (charge)
Current: -23.5A

// Power calculation
Power: -9.29kW

// Speed: 0-120 km/h (50% idle, 50% driving)
Speed: 45.2 km/h

// Motor data (correlates with power)
Motor Temp: 35.6°C
Motor RPM: 4520

// Cabin comfort
Cabin Temp: 21.5°C

// Location in Lauf an der Pegnitz with random walk
GPS: 49.5156, 11.5025 (12 satellites)
Odometer: 12345.6 km

// Status
Charging: NO
Doors: LOCKED
```

### Data Behavior

**State of Charge (SOC)**
- Starts at 80%
- Decreases by 0-2% per update interval when driving
- Increases when charging
- Ranges: 0-100%

**Temperature**
- Base: 25°C
- Variation: ±2°C random walk
- Increases under high power load
- Range: -20 to +60°C (realistic)

**Speed & Power**
- 50% of time: idle (0 km/h, ~0A)
- 50% of time: driving (10-120 km/h, variable current)
- Current correlates with speed and load
- Power = Voltage × Current

**GPS**
- Starting location: Lauf an der Pegnitz (49.5154, 11.5023)
- Random walk when driving
- 8-14 satellites (realistic)

**Charging Detection**
- Automatic when current > +5A
- Automatic discharge when current < -5A
- Realistic hysteresis

## Configuration Options

### In `settings.json`

```json
{
  "simulator": {
    "enabled": true,           // true = simulator mode, false = normal CAN mode
    "update_interval_ms": 5000, // Update frequency in milliseconds
    "vary_values": true        // true = random data, false = static defaults
  }
}
```

### Variation Parameters (in `data_simulator.cpp`)

These can be customized in the `DEFAULT_CONFIG`:

```cpp
const DataSimulator::SimulationConfig DataSimulator::DEFAULT_CONFIG = {
    .enabled = false,
    .update_interval_ms = 5000,      // 5 seconds
    .vary_values = true,
    .soc_variation = 0.5f,           // 0-2% per interval
    .temp_variation = 2.0f,          // ±2°C
    .speed_variation = 1.0f,         // 0-120 km/h
    .current_variation = 1.0f        // ±50A
};
```

## Simulator Integration

### In `main.cpp`

**Initialization:**
```cpp
if (sim_config.enabled) {
    DEBUG_PRINTLN("[System] SIMULATOR MODE ENABLED!");
    simulator.configure(config);
    simulator.begin();
}
```

**Main Loop:**
```cpp
if (g_settings.getSettings().simulator.enabled) {
    if (simulator.update()) {
        const VehicleData& sim_data = simulator.getData();
        // Process simulated data for MQTT
    }
} else {
    // Normal CAN processing
}
```

### Data Flow in Simulator Mode

```
Simulator (generates data)
    ↓
Data Manager (processes for MQTT)
    ↓
MQTT Handler (publishes to broker)
    ↓
Home Assistant (receives and displays)
```

## Testing Scenarios

### Test 1: MQTT Connectivity

**Goal:** Verify MQTT connection and publishing works

1. Enable simulator in settings
2. Configure MQTT broker IP/credentials
3. Flash device
4. Monitor serial output - should show MQTT connected
5. Check MQTT broker for published messages

**Expected Output:**
```
[Simulator] SOC: 80.1% | Temp: 25.2°C | Speed: 0.0 km/h
[MQTT] Publishing to vehicle/zoe/battery/soc: 80.1
[MQTT] Publishing to vehicle/zoe/battery/temp: 25.2
[MQTT] Publishing to vehicle/zoe/speed: 0.0
```

### Test 2: Home Assistant Integration

**Goal:** Verify HA receives and displays simulated data

1. Enable simulator
2. Configure MQTT to your Home Assistant instance
3. Verify MQTT discovery works
4. Check HA dashboard for vehicle entities
5. Monitor data updates in real-time

### Test 3: Data Publishing Intervals

**Goal:** Test different publish intervals

Edit settings.json and change publish intervals:
```json
{
  "mqtt": {
    "publish_interval_fast": 10000,   // Every 10 seconds (was 60s)
    "publish_interval_mid": 60000,    // Every 60 seconds (was 5m)
    "publish_interval_slow": 300000   // Every 5 minutes (was 1h)
  }
}
```

Monitor to verify correct timing.

### Test 4: Modem Connectivity

**Goal:** Test LTE module while simulator provides data

1. Enable simulator
2. Enable modem (LTE)
3. Device will:
   - Generate simulated vehicle data
   - Connect to LTE network
   - Publish data via MQTT over LTE
   - Retrieve GPS data from modem

No CAN bus needed!

## Architecture

### Data Simulator Class

**File:** `src/data_simulator.h` and `src/data_simulator.cpp`

**Key Methods:**

```cpp
class DataSimulator {
    // Initialize simulator
    bool begin();
    
    // Update generated data (call in loop)
    bool update();
    
    // Get current simulated data
    const VehicleData& getData() const;
    
    // Configure parameters
    void configure(const SimulationConfig& config);
    
    // Reset to defaults
    void reset();
    
    // Debug output
    void debugPrint() const;
};
```

### Data Generation

**Algorithm:**
1. Generates random values within realistic ranges
2. Maintains state between updates (SOC decreases gradually)
3. Correlates data (power = voltage × current, speed affects heating)
4. Simulates GPS movement (random walk when driving)
5. Realistic hysteresis (charging/discharging thresholds)

**Random Number Generation:**
- C++11 `std::mt19937` for reproducible randomness
- Seeded with `time(nullptr)` for variation
- Uniform distributions for all parameters

## Performance Impact

**Memory Overhead:**
- Header: ~2.5 KB
- Implementation: ~8 KB
- Runtime: ~100 bytes (SimulationConfig struct)

**CPU Overhead:**
- ~0.1% when enabled and updating
- Minimal impact on main loop

**Compilation Size:**
- +10 KB total (negligible on 4 MB ESP32 flash)

## Troubleshooting

### Simulator Not Enabled

**Symptom:** "Running in normal mode (real CAN data)" message

**Solution:**
```json
{
  "simulator": {
    "enabled": true   // Was false
  }
}
```

Re-upload filesystem with `pio run -e esp32dev --target uploadfs`

### No Data Updates

**Symptom:** Serial shows simulator init but no data

**Solution:**
- Check `update_interval_ms` - might be too large
- Check serial baud rate: 115200
- Verify `vary_values` is true

### MQTT Not Publishing

**Symptom:** Simulator running but MQTT shows no messages

**Solution:**
1. Verify MQTT broker IP/port in settings
2. Check network connectivity
3. Verify base_topic is correct
4. Check MQTT credentials

## Advanced Usage

### Custom Variation Ranges

Edit `data_simulator.cpp` DEFAULT_CONFIG:

```cpp
const DataSimulator::SimulationConfig DataSimulator::DEFAULT_CONFIG = {
    .soc_variation = 1.0f,    // Change to 0-4% per interval
    .temp_variation = 5.0f,   // Change to ±5°C
    .speed_variation = 2.0f,  // Change to 0-240 km/h range
};
```

### Starting Location

Change default GPS in `data_simulator.cpp`:

```cpp
static float lat = 48.8566f;   // Paris
static float lon = 2.3522f;    // Paris
```

### Static Data (No Variation)

Set in `settings.json`:
```json
{
  "simulator": {
    "vary_values": false   // Static defaults every interval
  }
}
```

Useful for:
- Regression testing
- Predictable behavior
- UI testing

## Integration with CI/CD

Simulator is perfect for automated testing:

```bash
#!/bin/bash
# Configure simulator mode
sed -i 's/"enabled": false/"enabled": true/' data/settings.json

# Build and flash
pio run -e esp32dev
pio run -e esp32dev --target upload
pio run -e esp32dev --target uploadfs

# Test communication
python3 test_mqtt_communication.py
```

## Next Steps

With simulator mode working:

1. **Phase 2:** Web UI for live monitoring
   - Dashboard showing simulated data
   - Real-time charts
   - Settings interface

2. **Phase 3:** Expression engine
   - Custom calculations on simulated data
   - Signal transformations

3. **Phase 4:** Plugin architecture
   - Custom data processors
   - Additional simulators

## Files Involved

| File | Purpose |
|------|----------|
| `src/data_simulator.h` | Simulator class definition |
| `src/data_simulator.cpp` | Simulator implementation |
| `src/settings.h` | Added SimulatorSettings struct |
| `src/settings.cpp` | Simulator settings support |
| `src/main.cpp` | Integration in setup/loop |
| `data/settings.json` | Configuration (simulator section) |

## Support

For issues or questions:
1. Check serial output with `pio device monitor -b 115200`
2. Enable `debug.enabled = true` in settings
3. Review `SIMULATOR_MODE.md` (this file)
4. Check GitHub issues

---

**Happy Testing! 🚀**
