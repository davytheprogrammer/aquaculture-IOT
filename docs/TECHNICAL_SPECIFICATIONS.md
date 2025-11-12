# 📋 ESP32-S3 Aquaculture IoT System - Technical Specifications

## 🔧 Hardware Specifications

### Microcontroller Unit (MCU)

| Specification | Value | Notes |
|---------------|-------|-------|
| **Processor** | Dual-core Xtensa LX7 | 32-bit RISC architecture |
| **Clock Speed** | 240 MHz | Configurable, power-optimized |
| **Architecture** | Harvard architecture | Separate instruction/data buses |
| **Instruction Set** | Xtensa ISA | RISC with DSP extensions |
| **Cache** | 16KB I-Cache + 8KB D-Cache | L1 cache per core |

### Memory Architecture

| Component | Capacity | Type | Usage |
|-----------|----------|------|-------|
| **SRAM** | 512 KB | Internal SRAM | Program execution, variables |
| **ROM** | 384 KB | Internal ROM | Boot loader, system libraries |
| **Flash** | 4 MB | External SPI Flash | Program storage, certificates |
| **RTC Memory** | 16 KB | Ultra-low power | Deep sleep retention |
| **eFuse** | 1792 bits | One-time programmable | Security keys, calibration |

### Wireless Connectivity

| Feature | Specification | Performance |
|---------|---------------|-------------|
| **WiFi Standard** | IEEE 802.11 b/g/n | 2.4 GHz band only |
| **WiFi Security** | WPA/WPA2/WPA3 | Enterprise & Personal |
| **Data Rate** | Up to 150 Mbps | 802.11n HT40 |
| **Range** | ~100m outdoor | Depends on antenna/environment |
| **Power Consumption** | 240mA (TX), 80mA (RX) | At 3.3V supply |

### Analog-to-Digital Converter (ADC)

| Parameter | ADC1 | ADC2 | Notes |
|-----------|------|------|-------|
| **Resolution** | 12-bit | 12-bit | Configurable 9-12 bit |
| **Channels** | 10 channels | 10 channels | GPIO 1-10, 11-20 |
| **Sample Rate** | 83.33 kSPS | 83.33 kSPS | Maximum theoretical |
| **Input Range** | 0-3.3V | 0-3.3V | With 12dB attenuation |
| **Accuracy** | ±2 LSB | ±2 LSB | After calibration |
| **Reference** | Internal 1.1V | Internal 1.1V | Temperature compensated |

### GPIO Configuration

| Pin | Function | Type | Sensor Connection |
|-----|----------|------|-------------------|
| **GPIO 1** | ADC1_CH0 | Analog Input | pH Sensor |
| **GPIO 2** | ADC1_CH1 | Analog Input | Turbidity Sensor |
| **GPIO 3** | ADC1_CH2 | Analog Input | Dissolved Oxygen |
| **GPIO 4** | Digital I/O | OneWire/DHT | DHT22 (Air Temp/Humidity) |
| **GPIO 5** | Digital I/O | OneWire | DS18B20 (Water Temperature) |
| **GPIO 6** | ADC1_CH3 | Analog Input | Ammonia Sensor |
| **GPIO 14** | Digital Output | PWM/GPIO | DC Pump Control |

### Power Management

| Mode | Current Consumption | Voltage | Duration |
|------|-------------------|---------|----------|
| **Active (WiFi TX)** | 240 mA | 3.3V | During transmission |
| **Active (WiFi RX)** | 80 mA | 3.3V | During reception |
| **Modem Sleep** | 20 mA | 3.3V | CPU active, WiFi off |
| **Light Sleep** | 0.8 mA | 3.3V | CPU suspended |
| **Deep Sleep** | 10 µA | 3.3V | RTC timer only |
| **Hibernation** | 2.5 µA | 3.3V | External wakeup only |

## 🖥️ Software Architecture

### Operating System

| Component | Version | Description |
|-----------|---------|-------------|
| **FreeRTOS** | v10.4.3 | Real-time operating system |
| **ESP-IDF** | v5.x | Espressif IoT Development Framework |
| **Toolchain** | GCC 11.2.0 | Cross-compilation toolchain |
| **Build System** | CMake 3.16+ | Modern build system |

### Memory Layout

```
Flash Memory Layout (4MB):
┌─────────────────────────────────────┐ 0x000000
│ Bootloader (64KB)                   │
├─────────────────────────────────────┤ 0x010000
│ Partition Table (4KB)               │
├─────────────────────────────────────┤ 0x011000
│ NVS Storage (20KB)                  │
├─────────────────────────────────────┤ 0x016000
│ PHY Init Data (4KB)                 │
├─────────────────────────────────────┤ 0x017000
│ Application (1.2MB)                 │
├─────────────────────────────────────┤ 0x150000
│ Certificate Storage (64KB)          │
├─────────────────────────────────────┤ 0x160000
│ OTA Update Partition (1.2MB)        │
├─────────────────────────────────────┤ 0x290000
│ SPIFFS File System (1.5MB)          │
└─────────────────────────────────────┘ 0x400000

SRAM Layout (512KB):
┌─────────────────────────────────────┐ 0x3FC88000
│ ROM Functions (32KB)                │
├─────────────────────────────────────┤ 0x3FC90000
│ Application Heap (180KB)            │
├─────────────────────────────────────┤ 0x3FCBD000
│ FreeRTOS Stacks (64KB)              │
├─────────────────────────────────────┤ 0x3FCCD000
│ WiFi/BT Stack (128KB)               │
├─────────────────────────────────────┤ 0x3FCED000
│ System Reserved (108KB)             │
└─────────────────────────────────────┘ 0x3FD00000
```

### Task Architecture

| Task Name | Priority | Stack Size | Core | Function |
|-----------|----------|------------|------|----------|
| **main_task** | 5 | 8192 bytes | Core 0 | Main application logic |
| **wifi_task** | 4 | 4096 bytes | Core 0 | WiFi management |
| **sensor_task** | 3 | 4096 bytes | Core 1 | Sensor reading |
| **http_task** | 3 | 8192 bytes | Core 0 | HTTP communications |
| **idle_task** | 0 | 1024 bytes | Both | System idle processing |

### Interrupt Handling

| Interrupt Source | Priority | Handler | Latency |
|------------------|----------|---------|---------|
| **WiFi Events** | Level 1 | wifi_event_handler | <10µs |
| **Timer Events** | Level 2 | timer_isr | <5µs |
| **GPIO Events** | Level 3 | gpio_isr_handler | <3µs |
| **ADC Complete** | Level 2 | adc_isr | <5µs |

## 🌐 Network Protocols

### WiFi Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| **SSID Storage** | Multiple networks | Automatic selection |
| **Security** | WPA2-PSK | Pre-shared key authentication |
| **Channel** | Auto-select | 2.4GHz channels 1-13 |
| **Power Save** | WIFI_PS_MIN_MODEM | Minimum power save mode |
| **Connection Timeout** | 15 seconds | Per network attempt |
| **Retry Logic** | Exponential backoff | 1s, 2s, 4s, 8s intervals |

### HTTP/HTTPS Protocol Stack

| Layer | Protocol | Implementation | Purpose |
|-------|----------|----------------|---------|
| **Application** | HTTP/1.1 | esp_http_client | REST API calls |
| **Security** | TLS 1.2/1.3 | mbedTLS | Encryption & authentication |
| **Transport** | TCP | lwIP stack | Reliable data transport |
| **Network** | IPv4 | lwIP stack | Internet protocol |
| **Data Link** | WiFi 802.11 | ESP32 WiFi driver | Wireless communication |

### TLS/SSL Configuration

| Parameter | Value | Notes |
|-----------|-------|-------|
| **TLS Version** | 1.2 minimum | 1.3 preferred |
| **Cipher Suites** | ECDHE-RSA-AES128-GCM-SHA256 | High security |
| **Certificate Validation** | Full chain | Server + Intermediate + Root |
| **Certificate Storage** | Embedded in firmware | Complete chain included |
| **Handshake Timeout** | 10 seconds | Configurable |
| **Session Resumption** | Enabled | Performance optimization |

## 📊 Sensor Specifications

### Digital Sensors

#### DHT22 (Air Temperature & Humidity)
| Parameter | Specification | Accuracy | Notes |
|-----------|---------------|----------|-------|
| **Temperature Range** | -40°C to +80°C | ±0.5°C | Operating range |
| **Humidity Range** | 0% to 100% RH | ±2% RH | Relative humidity |
| **Resolution** | 0.1°C, 0.1% RH | 16-bit | Digital output |
| **Response Time** | 2 seconds | Typical | 63% of step change |
| **Power Supply** | 3.3V to 5.5V | 1.5mA | During measurement |
| **Interface** | Single-wire digital | Custom protocol | 40-bit data |

#### DS18B20 (Water Temperature)
| Parameter | Specification | Accuracy | Notes |
|-----------|---------------|----------|-------|
| **Temperature Range** | -55°C to +125°C | ±0.5°C | Wide operating range |
| **Resolution** | 9 to 12-bit | Configurable | 0.0625°C at 12-bit |
| **Conversion Time** | 750ms (12-bit) | Maximum | Temperature dependent |
| **Power Supply** | 3.0V to 5.5V | 1.5mA | During conversion |
| **Interface** | 1-Wire digital | Dallas protocol | Unique 64-bit address |
| **Waterproofing** | IP68 rated | Stainless steel probe | Submersible |

### Analog Sensors

#### pH Sensor
| Parameter | Specification | Accuracy | Notes |
|-----------|---------------|----------|-------|
| **Measurement Range** | 0 to 14 pH | ±0.1 pH | Full pH scale |
| **Output Voltage** | 0 to 3.3V | Linear | Proportional to pH |
| **Response Time** | 5-10 seconds | 95% response | Temperature dependent |
| **Temperature Range** | 0°C to 60°C | Operating range | Compensation required |
| **Calibration** | 2-point minimum | pH 4.0 and 7.0 | 3-point recommended |
| **Maintenance** | Monthly cleaning | Electrode care | Storage solution |

#### Turbidity Sensor
| Parameter | Specification | Accuracy | Notes |
|-----------|---------------|----------|-------|
| **Measurement Range** | 0 to 1000 NTU | ±2% or ±2 NTU | Nephelometric units |
| **Output Voltage** | 0 to 3.3V | Analog | Inverse relationship |
| **Light Source** | 860nm IR LED | Infrared | Reduces color interference |
| **Detection Angle** | 90 degrees | Scattered light | ISO 7027 compliant |
| **Response Time** | <1 second | Instantaneous | Real-time measurement |
| **Cleaning** | Weekly recommended | Optical surfaces | Affects accuracy |

#### Dissolved Oxygen Sensor
| Parameter | Specification | Accuracy | Notes |
|-----------|---------------|----------|-------|
| **Measurement Range** | 0 to 20 mg/L | ±0.1 mg/L | Dissolved oxygen |
| **Output Voltage** | 0 to 3.3V | Linear | Proportional to DO |
| **Response Time** | 30 seconds | 90% response | Membrane dependent |
| **Temperature Range** | 0°C to 50°C | Operating range | Compensation required |
| **Pressure Range** | 0.5 to 2.0 atm | Atmospheric | Altitude compensation |
| **Calibration** | Air saturation | 100% DO at temperature | Regular required |

#### Ammonia Sensor
| Parameter | Specification | Accuracy | Notes |
|-----------|---------------|----------|-------|
| **Measurement Range** | 0 to 100 ppm | ±1 ppm | NH3/NH4+ total |
| **Output Voltage** | 0 to 3.3V | Logarithmic | Ion-selective electrode |
| **Response Time** | 60 seconds | 95% response | Ion diffusion limited |
| **pH Dependency** | 6.0 to 8.5 pH | Optimal range | Affects NH3/NH4+ ratio |
| **Temperature Range** | 5°C to 40°C | Operating range | Compensation required |
| **Calibration** | Standard solutions | 1, 10, 100 ppm | Multi-point curve |

## 🔒 Security Architecture

### Certificate Management

```
Certificate Chain Structure:
┌─────────────────────────────────────┐
│ Root CA Certificate                 │
│ (GTS Root R4)                       │
│ - RSA 4096-bit key                  │
│ - Valid until 2036                  │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│ Intermediate CA Certificate         │
│ (WE1 - Google Trust Services)       │
│ - ECDSA P-256 key                   │
│ - Valid until 2029                  │
└─────────────────────────────────────┘
              │
              ▼
┌─────────────────────────────────────┐
│ Server Certificate                  │
│ (*.supabase.co)                     │
│ - ECDSA P-256 key                   │
│ - Valid until 2026                  │
└─────────────────────────────────────┘
```

### Cryptographic Algorithms

| Algorithm | Key Size | Usage | Security Level |
|-----------|----------|-------|----------------|
| **ECDSA** | P-256 (256-bit) | Certificate signatures | 128-bit equivalent |
| **ECDHE** | P-256 (256-bit) | Key exchange | Perfect forward secrecy |
| **AES-GCM** | 128-bit | Symmetric encryption | AEAD cipher |
| **SHA-256** | 256-bit | Hash functions | Collision resistant |
| **HMAC-SHA256** | 256-bit | Message authentication | Integrity protection |

### API Security

| Security Layer | Implementation | Purpose |
|----------------|----------------|---------|
| **Transport Security** | TLS 1.2/1.3 | Encryption in transit |
| **API Authentication** | Bearer token | Service authentication |
| **Request Signing** | HMAC-SHA256 | Request integrity |
| **Rate Limiting** | Supabase built-in | DoS protection |
| **Input Validation** | JSON schema | Data sanitization |

## 📈 Performance Characteristics

### Timing Analysis

| Operation | Typical Time | Maximum Time | Notes |
|-----------|--------------|--------------|-------|
| **System Boot** | 2.5 seconds | 5 seconds | Cold start |
| **WiFi Scan** | 3 seconds | 10 seconds | Network dependent |
| **WiFi Connect** | 5 seconds | 15 seconds | Per network |
| **TLS Handshake** | 1.5 seconds | 3 seconds | Certificate validation |
| **HTTP Request** | 2 seconds | 5 seconds | Including response |
| **Sensor Reading** | 100ms | 2 seconds | Per sensor |
| **ADC Conversion** | 12µs | 50µs | Single sample |

### Memory Usage Analysis

| Component | RAM Usage | Flash Usage | Notes |
|-----------|-----------|-------------|-------|
| **FreeRTOS Kernel** | 32 KB | 64 KB | Operating system |
| **WiFi Stack** | 64 KB | 128 KB | Network protocols |
| **TLS Library** | 48 KB | 256 KB | mbedTLS implementation |
| **HTTP Client** | 16 KB | 32 KB | Application protocol |
| **Application Code** | 20 KB | 128 KB | Main application |
| **Certificates** | 4 KB | 8 KB | Embedded certificates |
| **Total Used** | 184 KB | 616 KB | Approximate |
| **Available** | 328 KB | 3.4 MB | Remaining capacity |

### Power Consumption Profile

```
Power Consumption Over Time (30-second cycle):
┌─────────────────────────────────────┐
│ 250mA ┤                             │
│       │  ██                         │
│ 200mA ┤  ██                         │
│       │  ██                         │
│ 150mA ┤  ██                         │
│       │  ██                         │
│ 100mA ┤  ██  ████████████████████   │
│       │  ██  ████████████████████   │
│  50mA ┤  ██  ████████████████████   │
│       │  ██  ████████████████████   │
│   0mA └──██──████████████████████───┘
│        0s  2s                   30s │
│        │   │                     │  │
│        │   └─ HTTP Transmission  │  │
│        └─ WiFi Connection       └─ Sleep
└─────────────────────────────────────┘
```

## 🔧 Calibration Procedures

### ADC Calibration

```c
// ADC Calibration Configuration
adc_cali_curve_fitting_config_t cali_config = {
    .unit_id = ADC_UNIT_1,
    .atten = ADC_ATTEN_DB_12,
    .bitwidth = ADC_BITWIDTH_DEFAULT,
};

// Two-point calibration
// Point 1: 0V input = 0 ADC counts
// Point 2: 3.3V input = 4095 ADC counts
// Linear interpolation for intermediate values
```

### Sensor Calibration Matrix

| Sensor | Calibration Points | Frequency | Method |
|--------|-------------------|-----------|---------|
| **pH Sensor** | pH 4.0, 7.0, 10.0 | Monthly | Buffer solutions |
| **Turbidity** | 0, 100, 800 NTU | Quarterly | Formazin standards |
| **Dissolved O2** | 0%, 100% saturation | Bi-weekly | Air/nitrogen calibration |
| **Ammonia** | 1, 10, 100 ppm | Monthly | Standard solutions |
| **Temperature** | Ice point, boiling point | Annually | Physical constants |

## 🚨 Error Handling & Recovery

### Error Classification

| Error Level | Response | Recovery Action | Logging |
|-------------|----------|-----------------|---------|
| **CRITICAL** | System halt | Manual intervention | Flash storage |
| **ERROR** | Function retry | Automatic recovery | Serial + flash |
| **WARNING** | Continue operation | Graceful degradation | Serial output |
| **INFO** | Normal operation | No action required | Serial output |
| **DEBUG** | Development only | No action required | Serial output |

### Watchdog Configuration

| Watchdog Type | Timeout | Action | Purpose |
|---------------|---------|--------|---------|
| **Task WDT** | 5 seconds | Reset system | Detect task hangs |
| **Interrupt WDT** | 300ms | Panic handler | Detect ISR hangs |
| **RTC WDT** | 9 seconds | Reset system | Deep sleep protection |

### Fault Recovery Mechanisms

1. **Automatic WiFi Reconnection**
   - Exponential backoff: 1s, 2s, 4s, 8s
   - Network switching on failure
   - Connection health monitoring

2. **HTTP Request Retry Logic**
   - Maximum 3 retry attempts
   - Increasing timeout: 5s, 10s, 15s
   - Circuit breaker pattern

3. **Sensor Error Handling**
   - Invalid reading detection (-999.0)
   - Sensor disconnection alerts
   - Graceful degradation mode

4. **Memory Management**
   - Heap monitoring and alerts
   - Automatic garbage collection
   - Stack overflow protection

## 📋 Compliance & Standards

### Regulatory Compliance

| Standard | Compliance | Certification | Notes |
|----------|------------|---------------|-------|
| **FCC Part 15** | Class B | ESP32-S3 module | Unintentional radiators |
| **IC RSS-247** | Compliant | ESP32-S3 module | Canadian regulations |
| **CE RED 2014/53/EU** | Compliant | ESP32-S3 module | European conformity |
| **RoHS 2011/65/EU** | Compliant | All components | Hazardous substances |

### Environmental Standards

| Parameter | Rating | Test Standard | Notes |
|-----------|--------|---------------|-------|
| **Operating Temperature** | -10°C to +60°C | IEC 60068-2-1/2 | Extended range |
| **Storage Temperature** | -40°C to +85°C | IEC 60068-2-1/2 | Non-operating |
| **Humidity** | 10% to 90% RH | IEC 60068-2-78 | Non-condensing |
| **Vibration** | 2G, 10-500Hz | IEC 60068-2-6 | Operational |
| **Shock** | 15G, 11ms | IEC 60068-2-27 | Survival |

### Software Quality Standards

| Standard | Compliance Level | Implementation |
|----------|------------------|----------------|
| **MISRA C:2012** | Advisory | Static analysis |
| **ISO/IEC 25010** | Quality model | Code reviews |
| **OWASP Top 10** | Security guidelines | Threat modeling |
| **IEEE 829** | Test documentation | Test procedures |

---

*This technical specification document provides comprehensive details for system implementation, maintenance, and compliance verification.*
