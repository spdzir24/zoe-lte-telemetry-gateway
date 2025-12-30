# Quick Start: ESP32-S2 Mini

**Zoe LTE Telemetry Gateway** now supports the **ESP32-S2 Mini** with full multi-board compatibility.

## 🚀 Quick Setup

### 1. Build for ESP32-S2
```bash
# Compile firmware
pio run -e esp32s2

# Upload firmware to device
pio run -e esp32s2 --target upload --upload-port COM3

# Upload filesystem (settings.json)
pio run -e esp32s2 --target uploadfs --upload-port COM3

# Monitor output
pio device monitor -b 115200 -p COM3
```

### 2. GPIO Pin Mapping (ESP32-S2 Mini)

Connect your hardware like this:

**UART (Modem/SIM7080G):**
- RX ← GPIO 37 (from modem TX)
- TX → GPIO 36 (to modem RX)

**CAN Bus 1 (High-Speed 500kbps):**
- RX ← GPIO 9 (from CAN transceiver)
- TX → GPIO 8 (to CAN transceiver)

**CAN Bus 2 (Low-Speed 125kbps):**
- RX ← GPIO 7 (from CAN transceiver)
- TX → GPIO 6 (to CAN transceiver)

**Control Pins:**
- Modem EN (Power) → GPIO 4
- Modem DTR (Wake) → GPIO 5
- Modem NET (Status) ← GPIO 3
- LED Status → GPIO 15

**Power:**
- 3.3V ← Regulator
- GND ← Common Ground

## ⚙️ Features for ESP32-S2

### ✅ Supported
- MQTT over LTE (via SIM7080G modem)
- Dual CAN bus (500kbps + 125kbps)
- GPS positioning (via modem)
- Settings persistence (LittleFS)
- **Simulator mode** (for testing without CAN bus)
- Power management
- Debug output to serial

### ❌ Not Supported on S2
- Bluetooth Classic (ESP32-S2 has no BT)
- WiFi (disabled in firmware for power saving)
- Dual-core processing (S2 is single-core)

### ℹ️ Why ESP32-S2?
- **Single core @ 240 MHz** - Sufficient for this application
- **Low power** - Better efficiency than dual-core ESP32
- **Many GPIO pins** - 43 pins for flexibility
- **USB OTG** - Can charge devices if needed
- **Same price** as ESP32 but better optimized

## 📊 Settings (settings.json)

### Enable Simulator Mode (for testing)
```json
{
  "simulator": {
    "enabled": true,
    "update_interval_ms": 5000,
    "vary_values": true
  }
}
```

When enabled:
- ✅ No CAN bus needed
- ✅ Generates realistic vehicle data
- ✅ Updates every 5 seconds
- ✅ Good for development/testing

### Disable MQTT (optional)
If you want to test without MQTT broker:
1. Keep simulator enabled
2. MQTT is automatically skipped in simulator mode

## 🔧 Compilation Differences

The build system automatically handles:
- GPIO pin mapping
- Memory optimization
- Clock frequency (240 MHz on S2)
- LittleFS configuration
- Single-core task scheduling

## 📋 Build Flags for ESP32-S2

In `platformio.ini` under `[env:esp32s2]`:

```ini
build_flags =
    -DMODEM_RX_PIN=37
    -DMODEM_TX_PIN=36
    -DCAN1_RX_PIN=9
    -DCAN1_TX_PIN=8
    -DCAN2_RX_PIN=7
    -DCAN2_TX_PIN=6
    -DMODEM_DTR_PIN=5
    -DMODEM_EN_PIN=4
    -DMODEM_NET_PIN=3
    -DLED_PIN=15
```

**To change pins:**
1. Edit these build flags in `platformio.ini`
2. Rebuild and upload
3. No code changes needed!

## 🧪 Testing Steps

### 1. Verify Compilation
```bash
pio run -e esp32s2
```
Should compile without errors.

### 2. Flash Firmware
```bash
pio run -e esp32s2 --target upload --upload-port COM3
```
Watch for:
```
Writing at 0x00000000... (X %)
Firmware successfully written
```

### 3. Upload Filesystem
```bash
pio run -e esp32s2 --target uploadfs --upload-port COM3
```
Watch for:
```
Uploading /data/ directory to filesystem...
Firmware filesystem successfully written
```

### 4. Monitor Output
```bash
pio device monitor -b 115200 -p COM3
```

Expected startup output:
```
==================================
Zoe LTE Telemetry Gateway v1.0.0
==================================
[System] Initializing Settings Manager...
[System] Settings loaded successfully
[System] SIMULATOR MODE ENABLED!
[System] Running in normal mode (real CAN data)
[System] Initializing Modem Handler...
[System] Initializing Power Manager...
[System] Initializing Data Manager...
[System] Setup complete!

[Simulator] SOC: 80.0% | Temp: 25.0°C | Speed: 0.0 km/h
```

## 🐛 Troubleshooting

### "Device not found"
```bash
# Find available ports
pio device list

# Check if device shows USB serial
# Windows: Device Manager → Ports (COM and LPT)
# Linux: ls /dev/ttyUSB*
# macOS: ls /dev/cu.usbserial*
```

### Upload fails at 5%
- Try slower upload speed: change `upload_speed = 115200` in platformio.ini
- Check USB cable (must be data cable, not power-only)

### No serial output
- Check baud rate: must be 115200
- Verify USB cable is connected to DATA pins
- Try different USB port

### Settings not loading
```bash
# Verify filesystem upload
pio run -e esp32s2 --target uploadfs --upload-port COM3

# Check in serial output
# Should show: "[Settings] Files on LittleFS: settings.json (771 bytes)"
```

## 📚 More Info

- See `BOARD_CONFIG.md` for detailed board information
- See `PHASE_1_SUMMARY.md` for settings management
- See `README.md` for overall project information

## 🔗 Links

- [ESP32-S2 Datasheet](https://www.espressif.com/en/products/socs/esp32-s2/resources)
- [PlatformIO ESP32-S2 Support](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s2-saola-1.html)
- [SIM7080G Module Documentation](https://simcom.ee/documents/)

---

**Status:** ✅ Ready for Production  
**Last Updated:** December 30, 2025
