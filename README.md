# Renault Zoe LTE Telemetry Gateway

A professional-grade CAN-to-MQTT telemetry system for monitoring Renault Zoe PH2 vehicles via LTE (Cat-M/NB-IoT) with Home Assistant integration.

## Features

✅ **CAN Bus Integration**
- Dual CAN bus support (High-speed 500kbps + Low-speed 125kbps)
- Comprehensive Renault Zoe PH2 signal decoding
- Real-time vehicle data extraction

✅ **LTE Connectivity**
- SIM7080G modem support (Cat-M + NB-IoT)
- Integrated GPS/GNSS receiver
- Remote monitoring from anywhere

✅ **MQTT Publishing**
- Home Assistant MQTT integration
- Intelligent publish intervals (60s to 60min)
- Value-based filtering to reduce bandwidth
- Batch publishing support

✅ **Power Management**
- Deep sleep mode after vehicle inactivity
- Automatic wake-up on CAN activity
- RTC-based periodic wake-ups for GPS updates
- Battery voltage monitoring
- ~5mA consumption in deep sleep

✅ **Vehicle Data Monitored**
- Battery Management: SoC, SoH, cell voltages, temperatures, cycles
- Charging System: plug status, power, voltage, current
- Motion: speed, consumption, range, recuperation
- Climate: interior temperature, heat pump data
- Power Systems: 12V/24V auxiliary voltages
- TPMS: tire pressure and temperature
- Motor/Drivetrain: RPM, torque, power

## Hardware Requirements

### Microcontroller
- **ESP32-WROOM-32** (Dual-core, 240MHz)

### Modem
- **SIMCOM SIM7080G** module
  - Cat-M/NB-IoT LTE
  - Integrated GNSS (GPS/GLONASS/Galileo/BDS)
  - MQTT stack
  - Power consumption: ~1.8A peak, <5mA sleep

### CAN Transceivers
- **2x SN65HVD230D** (TI CAN transceiver)
  - 3.3V logic supply
  - Suitable for 500kbps and 125kbps

### Power Management
- **5V/3A DCDC converter** (12V vehicle to 5V)
- **AMS1117-3.3** (5V to 3.3V LDO) x2
- **N-Channel MOSFET** (SIM power control)
- 12V vehicle battery monitoring

### Connectors
- OBD-II socket or direct CAN harness
- Micro SIM card slot
- SMA connectors (LTE + GPS antennas)

## Pin Configuration

```
ESP32 Pin | Function         | Peripheral
────────────────────────────────────────────
22        | CAN1 RX          | SN65HVD230 #1
21        | CAN1 TX          | SN65HVD230 #1
35        | CAN2 RX          | SN65HVD230 #2
33        | CAN2 TX          | SN65HVD230 #2
16        | SIM RX (UART2)   | SIM7080G
17        | SIM TX (UART2)   | SIM7080G
4         | DTR Pin          | SIM7080G (LOW=Wake)
27        | EN Pin           | SIM7080G (Power)
26        | NET Pin          | SIM7080G (Status)
2         | Status LED       | System indicator
34        | Battery ADC      | Voltage monitoring
```

## Software Stack

### Build System
- **PlatformIO** (Arduino framework)
- ESP-IDF 4.4+

### Libraries
- **PubSubClient** - MQTT client
- **TinyGSM** - Generic modem support
- **ArduinoJson** - JSON serialization
- **CAN_BUS_Shield** - CAN interface

### Architecture

```
CAN Bus → CANHandler → DataManager → MQTTHandler → SIM7080G → MQTT Broker
   ↓          ↓           ↓              ↓            ↓
  CAN1/2   Parse/Filter  Register    Publish    LTE Network
  Messages   Signals    Intervals    Topics
                  ↓
           PowerManager ← Activity Monitor
                  ↓
           Deep Sleep Control
```

## MQTT Topic Structure

```
vehicle/zoe/
├── battery/
│   ├── soc (% - fast 60s)
│   ├── soh (% - medium 5min)
│   ├── cell_voltage_min (V - medium)
│   ├── cell_voltage_max (V - medium)
│   ├── temp_min (°C - medium)
│   ├── temp_max (°C - medium)
│   ├── voltage (V - fast)
│   ├── current (A - fast)
│   ├── power (kW - fast)
│   ├── energy_to_full (kWh - fast)
│   └── full_cycles (count - slow)
├── charging/
│   ├── plug_connected (bool - fast)
│   ├── power (kW - fast)
│   ├── voltage (V - fast)
│   └── current (A - fast)
├── motion/
│   ├── speed (km/h - fast)
│   ├── consumption_kwh_100km (kWh/100km - fast)
│   ├── available_range (km - fast)
│   └── trip_distance (km - medium)
├── climate/
│   ├── interior_temp (°C - medium)
│   ├── heat_pump_pressure (bar - medium)
│   ├── heat_pump_evap_temp (°C - medium)
│   └── heat_pump_cond_temp (°C - medium)
├── tpms/
│   ├── tire_fl_pressure (bar - medium)
│   ├── tire_fr_pressure (bar - medium)
│   ├── tire_rl_pressure (bar - medium)
│   └── tire_rr_pressure (bar - medium)
├── power/
│   ├── voltage_12v (V - medium)
│   ├── voltage_24v (V - medium)
│   └── power_module_temp (°C - medium)
├── recuperation/
│   ├── max_power (kW - fast)
│   ├── instant_power (kW - fast)
│   └── total_energy (kWh - medium)
├── gps/
│   ├── latitude (float - 5min)
│   ├── longitude (float - 5min)
│   ├── accuracy (m - 5min)
│   └── satellites (count - 5min)
├── status (online/sleeping - always)
└── info/
    ├── firmware (version)
    └── device (name)
```

## Installation

### 1. Hardware Assembly

Refer to `docs/HARDWARE.md` for detailed schematics and assembly instructions.

### 2. Clone Repository

```bash
git clone https://github.com/yourusername/zoe-lte-telemetry-gateway.git
cd zoe-lte-telemetry-gateway
```

### 3. Install PlatformIO

```bash
pip install platformio
```

### 4. Configure Settings

Edit `src/config.h`:
- MQTT broker address and credentials
- SIM PIN (if required)
- Sleep timeout duration
- GPS update interval

### 5. Build and Upload

```bash
platformio run -e esp32dev -t upload
```

Monitor serial output:
```bash
platformio device monitor -e esp32dev
```

## Home Assistant Integration

Add to `configuration.yaml`:

```yaml
mqtt:
  broker: 192.168.1.100
  port: 1883
  username: homeassistant
  password: your_password

# See docs/HOMEASSISTANT.md for full entity definitions
```

Or use MQTT Discovery (automatic entity creation).

## Operating Modes

### Active Mode
- CAN bus listening enabled
- MQTT publishing: 60-300 seconds (signal dependent)
- Modem connected
- GPS updates every 5 minutes
- Power consumption: ~500mA average

### Idle Mode
- Vehicle parked, no CAN activity
- All peripherals disabled
- Reduced MQTT publishing
- Power consumption: <50mA

### Deep Sleep
- After 5+ minutes inactivity
- CAN bus monitoring disabled
- Modem in PSM (Power Saving Mode)
- GPS disabled
- Periodic RTC wake-up every 6 hours
- **Power consumption: <5mA**
- Wake-up triggers: CAN activity, RTC timer

## Troubleshooting

### No MQTT messages
- Check LTE connection: `AT+CREG?` should return `0,1` or `0,5`
- Verify MQTT broker address and port
- Check credentials in `config.h`
- Monitor serial output for errors

### High power consumption
- Check if modem is in sleep mode: `AT+CSCLK?`
- Verify deep sleep is enabled: `ENABLE_DEEP_SLEEP = true`
- Check for spurious CAN activity

### CAN signals not decoding
- Verify CAN ID in `can_messages.h` matches CanZE database
- Check signal bit positions and scaling factors
- Monitor `[DataMgr]` output for registered signals

### GPS not acquiring fix
- Ensure LTE connection is established first
- Wait 30-60 seconds for cold start
- Check GPS antenna connection
- Verify `enableGPS()` was called in firmware

## Performance Metrics

| Metric | Value |
|--------|-------|
| CAN message latency | <100ms to MQTT |
| MQTT message size | 10-100 bytes avg |
| Network bandwidth | ~5-10 MB/month avg |
| Battery draw (active) | ~500mA @ 12V |
| Battery draw (sleep) | <5mA @ 12V |
| GPS accuracy | ~5-10 meters |
| Sleep wake-up time | <5 seconds |

## Future Enhancements

- [ ] OTA firmware updates over LTE
- [ ] Charging history database (SQLite on SD card)
- [ ] Predictive analytics for range/charging time
- [ ] Multi-vehicle support
- [ ] Web dashboard with real-time data
- [ ] Advanced diagnostics DTC reading
- [ ] Two-way control (door lock, climate presets)

## License

MIT License - See LICENSE file

## Support

For issues, questions, or contributions:
- GitHub Issues: Report bugs and feature requests
- Discussions: Ask questions and share ideas
- Documentation: Check `docs/` directory

## References

- **CanZE Database**: https://github.com/fesch/CanZE/tree/master/app/src/main/assets/ZOE_Ph2
- **CanSee Project**: https://gitlab.com/jeroenmeijer/cansee
- **SIM7080G Datasheet**: SIMCOM official documentation
- **Home Assistant**: https://www.home-assistant.io/
