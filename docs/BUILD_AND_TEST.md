# Build & Test Guide - Renault Zoe LTE Telemetry Gateway

## Prerequisites

### Software
- **PlatformIO**: `pip install platformio` (or VS Code extension)
- **Python 3.7+**: For PlatformIO CLI
- **USB Driver**: CH340/CP2102 (comes with ESP32 board)
- **MQTT Broker**: Mosquitto (local or test.mosquitto.org)
- **Home Assistant**: (optional) For full integration

### Hardware
- ESP32-WROOM-32 development board
- 2x SN65HVD230 CAN transceiver
- SIMCOM SIM7080G module
- Breadboard or PCB
- USB cable (for flashing)
- Micro-SIM card (Cat-M/NB-IoT enabled)

---

## Building the Firmware

### Step 1: Clone Repository

```bash
git clone https://github.com/yourusername/zoe-lte-telemetry-gateway.git
cd zoe-lte-telemetry-gateway
```

### Step 2: Install PlatformIO (if not already installed)

```bash
# Via pip
pip install platformio

# Or use VS Code extension: "PlatformIO IDE"
```

### Step 3: Configure Settings

Edit `src/config.h`:

```cpp
// MQTT Configuration
#define MQTT_BROKER_ADDR "192.168.1.100"  // YOUR MQTT BROKER
#define MQTT_USERNAME "homeassistant"
#define MQTT_PASSWORD "your_password_here"

// Sleep Configuration
#define SLEEP_TIMEOUT_IDLE 300000   // 5 minutes (ms)
#define RTC_WAKEUP_INTERVAL 21600  // 6 hours (seconds)

// Debug Output
#define ENABLE_DEBUG true  // Set to false after testing
```

### Step 4: Build Firmware

```bash
# Build for ESP32 development board
platformio run -e esp32dev

# Or build and immediately upload
platformio run -e esp32dev -t upload

# Clean build (if experiencing issues)
platformio run -e esp32dev -t clean
platformio run -e esp32dev
```

### Expected Output

```
Platform Manager: Installing espressif32 @ 6.7.0
LibraryManager: Installing PubSubClient @ 2.8
LibraryManager: Installing ArduinoJson @ 6.21.4
Building in release mode
Compiling .pio\build\esp32dev\src\main.cpp.o
Compiling .pio\build\esp32dev\src\can_handler.cpp.o
...
Linking .pio\build\esp32dev\firmware.elf
Building .pio\build\esp32dev\firmware.bin
Firmware Size: 650 KB
```

### Troubleshooting Build Issues

**Error: "twai.h not found"**
- Platform version mismatch
- Solution: `platformio platform update`

**Error: "PubSubClient.h not found"**
- Libraries not downloaded
- Solution: `platformio lib install`

**Error: Compilation warnings**
- These are suppressed in `platformio.ini`
- If new warnings appear, check format strings

---

## Flashing to ESP32

### Option 1: PlatformIO CLI

```bash
# Auto-detect port and upload
platformio run -e esp32dev -t upload

# Specify port explicitly
platformio run -e esp32dev -t upload --upload-port COM3

# Monitor after upload
platformio device monitor -e esp32dev -b 115200
```

### Option 2: VS Code PlatformIO Extension

1. Open VS Code
2. Click PlatformIO home icon
3. Select "esp32dev" environment
4. Click "Upload" button
5. Click "Serial Monitor" to watch output

### Option 3: esptool.py (Manual)

```bash
# Download esptool
pip install esptool

# Erase flash
esptool.py --chip esp32 --port COM3 erase_flash

# Flash firmware
esptool.py --chip esp32 --port COM3 --baud 921600 write_flash -z 0x1000 .pio/build/esp32dev/firmware.bin
```

### Flashing Issues

**"Failed to connect to ESP32"**
- Check USB cable
- Press BOOT button while connecting
- Try different USB port
- Check device manager for COM port

**"Timeout waiting for packet header"**
- Try lower baud rate: `--baud 460800`
- Use different USB cable
- Check power supply (need stable 5V)

---

## Initial Testing

### Test 1: Serial Monitor Connection

```bash
platformio device monitor -e esp32dev -b 115200
```

**Expected Output (within 5 seconds):**

```
==================================
   ZOE LTE TELEMETRY GATEWAY
   Firmware Version: 1.0.0
   Build Date: Dec 20 2025
==================================
[SETUP] Initializing power manager...
[SETUP] Initializing CAN bus...
[SETUP] Initializing modem...
[SETUP] Initializing MQTT...
[SETUP] Registering vehicle signals...
[SETUP] Initialization complete! System ready.
=====================================
```

If you see errors:
- Check config.h settings
- Verify all hardware connections
- Check modem power supply

### Test 2: CAN Bus Detection

Add debug code to verify CAN is working (optional):

```cpp
// In main loop, print CAN status every 10 seconds
static uint32_t last_can_check = 0;
if ((millis() - last_can_check) > 10000) {
    DEBUG_PRINTF("[TEST] CAN1 messages: %lu\n", can_handler.getMessagesReceived1());
    last_can_check = millis();
}
```

**Expected:**
- Messages increasing if vehicle CAN is connected
- 0 if no CAN connection

### Test 3: MQTT Connection

Monitor MQTT broker:

```bash
# In another terminal, subscribe to all topics
mosquitto_sub -h 192.168.1.100 -u homeassistant -P password -t "vehicle/zoe/#"
```

**Expected Output (after gateway connects):**

```
vehicle/zoe/status
online
vehicle/zoe/info/firmware
1.0.0
vehicle/zoe/info/device
ZoE-LTE-Gateway-001
vehicle/zoe/battery/soc
75.5
vehicle/zoe/battery/voltage
360.2
...
```

If no messages:
1. Check MQTT broker is running
2. Verify network connectivity
3. Check modem has LTE signal
4. Look at serial output for errors

### Test 4: GPS Functionality

GPS data should appear every 5 minutes (if module has fix):

```bash
mosquitto_sub -h 192.168.1.100 -t "vehicle/zoe/gps/#"
```

**Expected after 30-60 seconds (cold start):**

```
vehicle/zoe/gps/latitude
52.5234567
vehicle/zoe/gps/longitude
13.4123456
vehicle/zoe/gps/accuracy
8
vehicle/zoe/gps/satellites
12
```

If GPS not working:
- Ensure LTE connection established first
- Check GPS antenna connection
- Wait 60+ seconds for cold start
- Try outdoor location

---

## Home Assistant Integration

### Step 1: Install MQTT Integration

1. Home Assistant > Settings > Devices & Services
2. Create integration > "MQTT"
3. Configure broker:
   - Broker: `192.168.1.100`
   - Port: `1883`
   - Username: `homeassistant`
   - Password: (from config.h)

### Step 2: Add Sensors (Manual Method)

Edit `configuration.yaml`:

```yaml
mqtt:
  broker: 192.168.1.100
  port: 1883
  username: homeassistant
  password: your_password_here

sensor:
  - platform: mqtt
    name: "Zoe SoC"
    unique_id: zoe_soc
    state_topic: "vehicle/zoe/battery/soc"
    unit_of_measurement: "%"
    device_class: battery
    icon: mdi:battery

  - platform: mqtt
    name: "Zoe Speed"
    unique_id: zoe_speed
    state_topic: "vehicle/zoe/motion/speed"
    unit_of_measurement: "km/h"
    device_class: speed
    icon: mdi:speedometer

  # Add more sensors from docs/home_assistant_config.yaml
```

Restart Home Assistant to load new sensors.

### Step 3: Create Lovelace Dashboard

Example card:

```yaml
type: entities
title: Renault Zoe
entities:
  - entity: sensor.zoe_soc
    name: "State of Charge"
  - entity: sensor.zoe_battery_voltage
    name: "Battery Voltage"
  - entity: sensor.zoe_speed
    name: "Speed"
  - entity: binary_sensor.zoe_plug_connected
    name: "Charging"
  - entity: device_tracker.zoe_location
    name: "Location"
```

---

## Stress Testing

### Test 5: CAN Message Processing

**Setup:**
1. Park vehicle with engine running
2. Leave gateway connected for 30 minutes
3. Monitor serial output for errors

**Metrics to Check:**
- CAN message count increases
- No "TWAI alerts" or bus errors
- MQTT messages posting regularly
- Memory usage stable

### Test 6: Long-term Idle

**Setup:**
1. Turn off vehicle
2. Let gateway sit idle for 10+ minutes
3. Expect deep sleep after 5 minutes

**Expected Behavior:**
```
[Sleep] Vehicle idle, preparing for deep sleep...
[Sleep] Entering deep sleep for 21600 seconds
[Power] RTC timer set for 21600 seconds
[Power] Deep sleep starting now...

(No output for 5-10 minutes)

[Power] Waking from deep sleep...
[Sleep] Woke from deep sleep! Reconnecting...
```

### Test 7: Charging Session

**Setup:**
1. Connect vehicle to charger
2. Monitor MQTT topics during charge
3. Record metrics

**Metrics:**
- `battery/soc` increases every 1-2 minutes
- `charging/power` shows actual charging power
- `battery/temp_*` increases gradually
- `battery/energy_to_full` decreases

---

## Performance Benchmarks

### Firmware Size
```
Flash: 650-750 KB
SPIFFS: 0 KB (not used)
Free: 3.25 MB
```

### RAM Usage
```
Initialization: ~80 KB
Running (idle): ~120 KB
With MQTT: ~150 KB
Free RAM: ~150 KB
```

### CAN Bus Performance
```
Message Rate: 50-100 msgs/sec (depends on vehicle)
Processing Latency: <10 ms
Miss Rate: <1% (good termination)
```

### MQTT Performance
```
Connction Time: 2-5 seconds
Publish Rate: 50-100 msgs/min (depends on intervals)
Average Payload: 10-20 bytes
Monthly Data: ~25-30 MB
```

### Power Consumption
```
Active Mode: ~500 mA @ 12V
Idle Mode: ~50 mA @ 12V
Sleep Mode: <5 mA @ 12V
```

---

## Debugging Commands

### Monitor All MQTT Topics

```bash
mosquitto_sub -h 192.168.1.100 -u homeassistant -P password -t "vehicle/zoe/#" -v
```

### Check Specific Signal

```bash
mosquitto_sub -h 192.168.1.100 -u homeassistant -P password -t "vehicle/zoe/battery/soc"
```

### Test MQTT Publish

```bash
mosquitto_pub -h 192.168.1.100 -u homeassistant -P password \
  -t "vehicle/zoe/battery/soc" -m "75.5"
```

### Monitor Serial Output with Timestamps

```bash
platformio device monitor -e esp32dev -b 115200 --filter=time
```

### Enable Maximum Debug Output

Edit `src/config.h`:

```cpp
#define ENABLE_DEBUG true
#define CORE_DEBUG_LEVEL 3  // 0=None, 1=Error, 2=Warning, 3=Info
```

Rebuild and upload.

---

## Common Issues & Solutions

### Issue: No CAN Messages

**Check:**
1. CAN transceiver power (3.3V, GND)
2. CAN wiring to vehicle
3. Termination resistors (120Ω)
4. GPIO pin numbers match config.h
5. CAN bus not powered off

**Debug:**
```cpp
if (!can_handler.isConnected1()) {
    DEBUG_PRINTLN("CAN1 not connected!");
}
```

### Issue: MQTT Not Connecting

**Check:**
1. Broker IP/port in config.h
2. Modem has network signal (check `AT+CREG?`)
3. Credentials correct (username/password)
4. Broker is running and accessible

### Issue: High Power Consumption

**Check:**
1. Deep sleep enabled: `ENABLE_DEEP_SLEEP = true`
2. Sleep timeout not too long: `SLEEP_TIMEOUT_IDLE = 300000`
3. WiFi disabled (not used in LTE mode)
4. Modem in PSM or sleep mode

### Issue: GPS Not Working

**Check:**
1. LTE network connected first (GPS needs network)
2. `AT+CGNSPWR=1` sent successfully
3. Antenna connected and positioned
4. Cold start (wait 30-60 seconds)
5. Check `AT+CGNSINF` response

---

## Production Deployment

### Pre-Deployment Checklist

- [ ] All sensors verified in Home Assistant
- [ ] Deep sleep tested and working
- [ ] MQTT connectivity stable for 24h+
- [ ] GPS updates consistent
- [ ] Battery voltage reading accurate
- [ ] CAN message rate matches expected
- [ ] No memory leaks observed
- [ ] Power consumption within spec
- [ ] Firmware version documented
- [ ] Backup configuration saved

### Firmware Update Procedure

1. Note current firmware version
2. Build new version
3. Test on development board first
4. Flash to vehicle (with USB)
5. Verify all sensors still working
6. Monitor for 30 minutes

---

*Last Updated: 2025-12-20*
*Maintained by: Your Name*
