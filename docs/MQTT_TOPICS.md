# MQTT Topics Reference - Renault Zoe Telemetry Gateway

**Base Topic:** `vehicle/zoe/`

## Topic Directory

### Battery Management System (BMS)

| Topic | Type | Unit | Interval | Range | Notes |
|-------|------|------|----------|-------|-------|
| `battery/soc` | Float | % | 60s | 0-100 | State of Charge |
| `battery/soh` | Float | % | 300s | 0-100 | State of Health |
| `battery/real_soc` | Float | % | 60s | 0-100 | Real/usable SoC |
| `battery/voltage` | Float | V | 60s | 0-400V | Battery pack voltage |
| `battery/current` | Float | A | 60s | -∞ to +∞ | Positive: discharging, Negative: charging |
| `battery/power` | Float | kW | 60s | -400 to +400 | Positive: discharge, Negative: charge |
| `battery/energy_to_full` | Float | kWh | 60s | 0-52 | Energy needed to 100% |
| `battery/usable_capacity` | Float | kWh | 3600s | 20-50 | Usable battery capacity |
| `battery/max_capacity` | Float | kWh | 3600s | 22-52 | Maximum battery capacity (degradation indicator) |
| `battery/cell_voltage_min` | Float | V | 300s | 2.5-4.2 | Minimum cell voltage |
| `battery/cell_voltage_max` | Float | V | 300s | 2.5-4.2 | Maximum cell voltage |
| `battery/temp_min` | Float | °C | 300s | -40 to +100 | Minimum cell temperature |
| `battery/temp_max` | Float | °C | 300s | -40 to +100 | Maximum cell temperature |
| `battery/temp_avg` | Float | °C | 300s | -40 to +100 | Average cell temperature |
| `battery/full_cycles` | Integer | count | 3600s | 0-∞ | Total full charge cycles |

### Charging System

| Topic | Type | Unit | Interval | Range | Notes |
|-------|------|------|----------|-------|-------|
| `charging/plug_connected` | Boolean | - | 10s | 0/1 | 1 = plugged in |
| `charging/power` | Float | kW | 60s | 0-22 | Charging power |
| `charging/voltage` | Float | V | 60s | 0-400V | Charging voltage |
| `charging/current` | Float | A | 60s | 0-32 | Charging current |

### Motion & Speed

| Topic | Type | Unit | Interval | Range | Notes |
|-------|------|------|----------|-------|-------|
| `motion/speed` | Float | km/h | 10s | 0-200 | Vehicle speed |
| `motion/motor_rpm` | Integer | rpm | 10s | 0-15000 | Electric motor RPM |
| `motion/motor_torque` | Float | Nm | 10s | -200 to +200 | Motor torque |
| `motion/consumption_kwh_100km` | Float | kWh/100km | 60s | 10-25 | Current consumption |
| `motion/consumption_instant` | Float | kW | 10s | -50 to +50 | Instantaneous power |
| `motion/available_range` | Integer | km | 60s | 0-400 | Remaining range estimation |
| `motion/trip_distance` | Float | km | 60s | 0-∞ | Trip distance since start |

### Climate & HVAC

| Topic | Type | Unit | Interval | Range | Notes |
|-------|------|------|----------|-------|-------|
| `climate/interior_temp` | Float | °C | 300s | -40 to +80 | Cabin air temperature |
| `climate/heat_pump_pressure` | Float | bar | 300s | 0-30 | Heat pump system pressure |
| `climate/heat_pump_evap_temp` | Float | °C | 300s | -40 to +100 | Evaporator temperature |
| `climate/heat_pump_cond_temp` | Float | °C | 300s | -40 to +100 | Condenser temperature |

### Tire Pressure Monitoring System (TPMS)

| Topic | Type | Unit | Interval | Range | Notes |
|-------|------|------|----------|-------|-------|
| `tpms/tire_fl_pressure` | Float | bar | 300s | 1.5-3.5 | Front-Left tire pressure |
| `tpms/tire_fr_pressure` | Float | bar | 300s | 1.5-3.5 | Front-Right tire pressure |
| `tpms/tire_rl_pressure` | Float | bar | 300s | 1.5-3.5 | Rear-Left tire pressure |
| `tpms/tire_rr_pressure` | Float | bar | 300s | 1.5-3.5 | Rear-Right tire pressure |

### Power Systems

| Topic | Type | Unit | Interval | Range | Notes |
|-------|------|------|----------|-------|-------|
| `power/voltage_12v` | Float | V | 300s | 10-15 | 12V auxiliary voltage |
| `power/voltage_24v` | Float | V | 300s | 20-30 | 24V auxiliary voltage |
| `power/power_module_temp` | Float | °C | 300s | -40 to +100 | HV power electronics temperature |

### Recuperation (Regenerative Braking)

| Topic | Type | Unit | Interval | Range | Notes |
|-------|------|------|----------|-------|-------|
| `recuperation/max_power` | Float | kW | 60s | 0-80 | Maximum available recuperation |
| `recuperation/instant_power` | Float | kW | 10s | 0-80 | Current recuperation power |
| `recuperation/total_energy` | Float | kWh | 300s | 0-∞ | Total energy recovered this drive |

### GPS & Location

| Topic | Type | Unit | Interval | Range | Notes |
|-------|------|------|----------|-------|-------|
| `gps/latitude` | Float | degrees | 300s | -90 to +90 | GPS latitude |
| `gps/longitude` | Float | degrees | 300s | -180 to +180 | GPS longitude |
| `gps/accuracy` | Integer | meters | 300s | 0-100+ | GPS fix accuracy |
| `gps/satellites` | Integer | count | 300s | 0-30+ | Number of satellites |

### System Status

| Topic | Type | Unit | Interval | Range | Notes |
|-------|------|------|----------|-------|-------|
| `status` | String | - | Always | online / sleeping | Gateway online status |
| `info/firmware` | String | - | Once | semver | Firmware version (e.g., "1.0.0") |
| `info/device` | String | - | Once | name | Device identifier |

---

## Publishing Intervals

### Fast (60 seconds)
Real-time driving data, changing frequently:
- SoC, Speed, Power, Consumption, Range
- Recuperation, Energy to full
- Motor RPM/Torque, Brake pressure

### Medium (300 seconds = 5 minutes)
Values changing slowly:
- SoH, Temperatures (battery, climate, HV module)
- Cell voltages, TPMS, Auxiliary voltages
- Heat pump data, Trip distance

### Slow (3600 seconds = 60 minutes)
Statistics and long-term trends:
- Battery capacity (degradation)
- Full charge cycles
- Charging history totals

### Event-Based (When Changed)
- Plug connected status (immediate on change)
- System status (online/sleeping)
- GPS data (best available frequency)

---

## Value Tolerance (Skip Publish If Change < X)

| Category | Tolerance | Reason |
|----------|-----------|--------|
| SoC | ±0.5% | Reduce noise, relevant changes are >1% |
| Speed | ±1 km/h | Filter sensor noise |
| Power | ±0.5 kW | Smooth power fluctuations |
| Temperature | ±1°C | Reduce frequency for stable temps |
| Voltage | ±0.1 V | Filter ADC noise |
| TPMS | ±0.1 bar | Normal pressure variance |
| GPS | Continuous | No filtering (raw data) |
| Boolean | 0 (any change) | Plug status, door status |

---

## MQTT Message Format

### Simple Value
```
Topic: vehicle/zoe/battery/soc
Payload: 75.5
QoS: 1
Retain: false (immediate data)
```

### Retain Messages (State)
```
Topic: vehicle/zoe/status
Payload: "online"
QoS: 1
Retain: true (persists after disconnect)
```

### JSON Batch (Future Enhancement)
```json
{
  "timestamp": 1703084160,
  "battery": {
    "soc": 75.5,
    "voltage": 360.2,
    "current": 15.3,
    "power": 5.4,
    "temp_avg": 22.1
  },
  "motion": {
    "speed": 65,
    "consumption": 15.2,
    "range": 285
  }
}
```

---

## Integration with Home Assistant

### MQTT Discovery
If using Home Assistant's MQTT Discovery, entities are created with:
```
homeassistant/sensor/zoe/soc/config
homeassistant/binary_sensor/zoe/plug_connected/config
homeassistant/device_tracker/zoe/location/config
```

### Manual Entity Definition
Add to `configuration.yaml`:
```yaml
sensor:
  - platform: mqtt
    name: "Zoe SoC"
    state_topic: "vehicle/zoe/battery/soc"
    unit_of_measurement: "%"
    device_class: "battery"
```

---

## Testing MQTT Topics

### Subscribe to All Topics
```bash
mosquitto_sub -h 192.168.1.100 -u homeassistant -P password -t "vehicle/zoe/#"
```

### Monitor Battery Specifically
```bash
mosquitto_sub -h 192.168.1.100 -u homeassistant -P password -t "vehicle/zoe/battery/#"
```

### Publish Test Value
```bash
mosquitto_pub -h 192.168.1.100 -u homeassistant -P password -t "vehicle/zoe/battery/soc" -m "75.5"
```

---

## Example: Charging Session Data Flow

```
Time    Event                          Topic                    Payload
─────────────────────────────────────────────────────────────────────────
10:00   Plug connected                 charging/plug_connected  1
10:00   Charging starts               charging/power           5.5 kW
10:01   Battery temp rising            battery/temp_avg         22.5°C
10:05   Update GPS every 5 min         gps/latitude/longitude   52.5234, 13.4234
10:15   SoC increased 5%               battery/soc              50.0%
11:00   SoC at 85%, charging slower    battery/soc              85.0%
11:30   Charging complete              charging/plug_connected  0
11:30   Status back to online          status                   online
```

---

## Retention Policy

### Non-Retained (Immediate Data)
- All real-time sensor values (power, speed, temps)
- Status changes during operation
- GPS coordinates

### Retained (State)
- `status` (online/sleeping)
- `info/firmware`, `info/device`
- Charging plug status (for UI state persistence)

---

## Bandwidth Estimation

**Assumptions:**
- 50 signals publishing
- Average payload: 10 bytes
- Fast interval: 60s, Medium: 300s, Slow: 3600s

**Daily Estimate:**
- Fast signals: 50 msgs/min × 10 bytes × 1440 min = 720 KB/day
- Medium signals: 10 msgs/min × 10 bytes × 1440 min = 144 KB/day
- Slow signals: 1 msg/min × 10 bytes × 1440 min = 14.4 KB/day
- **Total: ~880 KB/day** (assuming idle time)
- **Monthly: ~26 MB** (typical with driving patterns)

---

## Troubleshooting

### No Messages Appearing
1. Check MQTT broker connectivity: `mosquitto_sub -h <broker>`
2. Verify credentials in config.h
3. Check modem network registration: `AT+CREG?`
4. Monitor serial output for errors

### Intermittent Messages
1. Check CAN bus termination (120Ω resistors)
2. Verify MQTT QoS setting (should be 1)
3. Check vehicle CAN message frequency
4. Look for publish_interval violations

### Incorrect Values
1. Verify CAN ID and signal definitions
2. Check bit positions and scaling factors
3. Validate against CanZE database
4. Test with known message (OBD PID)

---

*Last Updated: 2025-12-20*
*Reference: CanZE Zoe PH2 Database*
