# Board Configuration Guide

Zoe LTE Telemetry Gateway supports multiple ESP32 variants. This guide explains the GPIO pin mapping and how to build for each board.

## 📋 Supported Boards

| Board | Processor | RAM | Flash | Cores | Clock | Status |
|-------|-----------|-----|-------|-------|-------|--------|
| **ESP32 Dev** | Xtensa 32-bit | 320 KB | 4 MB | 2 | 240 MHz | ✅ Fully Supported |
| **ESP32-C3** | RISC-V 32-bit | 400 KB | 4 MB | 1 | 160 MHz | ✅ Fully Supported |
| **ESP32-S2 Mini** | Xtensa 32-bit | 320 KB | 4 MB | 1 | 240 MHz | ✅ Fully Supported |

## 🔧 GPIO Pin Mapping

### ESP32 Dev (esp32dev)
Default configuration for standard ESP32 dev boards.

```
MODEM Interface (UART2):
  RX  → GPIO 16
  TX  → GPIO 17
  DTR → GPIO 4   (Wake-up control)
  EN  → GPIO 27  (Power control)
  NET → GPIO 26  (Network status)

CAN Bus:
  CAN1 (500 kbps - High Speed):
    RX → GPIO 22
    TX → GPIO 21
  CAN2 (125 kbps - Low Speed):
    RX → GPIO 35
    TX → GPIO 33

Other:
  LED → GPIO 2
```

**Compile:**
```bash
pio run -e esp32dev
pio run -e esp32dev --target upload --upload-port COM3
pio run -e esp32dev --target uploadfs --upload-port COM3
```

---

### ESP32-C3 (esp32c3) - LILYGO T-OI Plus
For RISC-V based ESP32-C3 boards (e.g., LILYGO T-OI Plus).

**Key Differences:**
- RISC-V 32-bit processor (not Xtensa)
- Single core @ 160 MHz
- 22 GPIO pins total (fewer than ESP32)
- No Bluetooth Classic (only BLE)

```
MODEM Interface (UART0):
  RX  → GPIO 20
  TX  → GPIO 21
  DTR → GPIO 5   (Wake-up control)
  EN  → GPIO 6   (Power control)
  NET → GPIO 7   (Network status)

CAN Bus:
  CAN1 (500 kbps - High Speed):
    RX → GPIO 8
    TX → GPIO 9
  CAN2 (125 kbps - Low Speed):
    RX → GPIO 3
    TX → GPIO 4

Other:
  LED → GPIO 10
```

**Compile:**
```bash
pio run -e esp32c3
pio run -e esp32c3 --target upload --upload-port COM3
pio run -e esp32c3 --target uploadfs --upload-port COM3
```

**Hardware Notes:**
- T-OI Plus has built-in 16340 battery holder (optional)
- Grove connector compatible with D1 shields
- CH340K USB-UART chip

---

### ESP32-S2 Mini (esp32s2)
For single-core Xtensa ESP32-S2 boards.

**Key Differences:**
- Single core Xtensa @ 240 MHz
- No Bluetooth (all variants)
- USB OTG support
- 43 GPIO pins
- Lower power than dual-core ESP32

```
MODEM Interface (UART1):
  RX  → GPIO 37
  TX  → GPIO 36
  DTR → GPIO 5   (Wake-up control)
  EN  → GPIO 4   (Power control)
  NET → GPIO 3   (Network status)

CAN Bus:
  CAN1 (500 kbps - High Speed):
    RX → GPIO 9
    TX → GPIO 8
  CAN2 (125 kbps - Low Speed):
    RX → GPIO 7
    TX → GPIO 6

Other:
  LED → GPIO 15
```

**Compile:**
```bash
pio run -e esp32s2
pio run -e esp32s2 --target upload --upload-port COM3
pio run -e esp32s2 --target uploadfs --upload-port COM3
```

**Hardware Notes:**
- Single core is sufficient for this application
- Good CPU performance at lower power
- USB port can be used for power and programming
- Better memory efficiency than dual-core ESP32

---

## 🛠️ Build & Upload Commands

### For all boards:

```bash
# Compile only
pio run -e <environment>

# Compile and upload firmware
pio run -e <environment> --target upload --upload-port COM3

# Upload filesystem (settings.json)
pio run -e <environment> --target uploadfs --upload-port COM3

# Monitor serial output
pio device monitor -b 115200 -p COM3

# Clean build
pio run -e <environment> --target clean
```

### Replace:
- `<environment>` with: `esp32dev`, `esp32c3`, or `esp32s2`
- `COM3` with your actual port (Windows: COM3, Linux/Mac: /dev/ttyUSB0)

---

## 📦 Hardware Components

For all boards, you'll need:

### Modem
- **SIM7080G** (LTE/GPS module)
- 3.7V LiPo battery for modem backup
- USB-to-UART adapter if needed

### CAN Interface
- **2x MCP2562** or similar CAN transceiver
- For CAN1 (500 kbps): Direct to vehicle High-Speed CAN
- For CAN2 (125 kbps): Direct to vehicle Low-Speed CAN

### Power Supply
- 12V from vehicle (with buck converter to 5V)
- Or USB power for development

### Wiring Example (ESP32 Dev)
```
Modem (UART2):
  RX  → GPIO 16 ← Modem TX
  TX  → GPIO 17 → Modem RX
  GND → Common Ground
  VCC → 3.3V

CAN1 (MCP2562-1):
  RX  → GPIO 22 ← Transceiver RX
  TX  → GPIO 21 → Transceiver TX
  GND → Common Ground
  VCC → 3.3V

CAN2 (MCP2562-2):
  RX  → GPIO 35 ← Transceiver RX
  TX  → GPIO 33 → Transceiver TX
  GND → Common Ground
  VCC → 3.3V
```

---

## 🚨 Troubleshooting

### "Device not found" during upload
```bash
# List available ports
pio device list

# On Windows, check Device Manager
# On Linux, check: ls /dev/ttyUSB*
# On Mac, check: ls /dev/cu.usbserial*
```

### Upload speed too slow
- Try reducing `upload_speed` in platformio.ini (default 921600)
- Some boards work better at 115200

### "Load access fault" on ESP32-C3
- Make sure simulator is enabled in settings.json
- MQTT may not work yet on C3 without proper WiFi/Modem
- Use simulator mode for testing

### Settings not persisting
- Ensure LittleFS upload: `pio run -e <env> --target uploadfs`
- Check filesystem with: `g_settings.listFiles()`

---

## 📊 Performance Comparison

| Metric | ESP32 Dev | ESP32-C3 | ESP32-S2 |
|--------|-----------|----------|----------|
| **CPU Performance** | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ |
| **Power Efficiency** | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ |
| **RAM Available** | 320 KB | 400 KB | 320 KB |
| **GPIO Pins** | 39 | 22 | 43 |
| **Cost** | $ | $$ | $ |
| **Best For** | Production | Low-power IoT | Optimized |

---

## 📝 Customizing GPIO Pins

To change GPIO pins for your specific hardware:

1. Edit `platformio.ini` under your environment section
2. Modify the `-D` flags:
   ```ini
   build_flags =
       -DMODEM_RX_PIN=16
       -DMODEM_TX_PIN=17
       -DCAN1_RX_PIN=22
       # ... etc
   ```

3. Rebuild and upload

Alternatively, modify `src/config.h` to change defaults for all environments.

---

## 🔗 Resources

- [ESP32 Pinout](https://randomnerdtutorials.com/esp32-pinout-reference/)
- [ESP32-C3 Pinout](https://www.espressif.com/en/products/socs/esp32-c3/resources)
- [ESP32-S2 Pinout](https://www.espressif.com/en/products/socs/esp32-s2/resources)
- [LILYGO T-OI Plus](https://github.com/Xinyuan-LilyGO/T-OI-PLUS)
- [SIM7080G Documentation](https://simcom.ee/documents/)
- [MCP2562 CAN Transceiver](https://www.microchip.com/en-us/product/MCP2562)

---

*Last Updated: December 30, 2025*
