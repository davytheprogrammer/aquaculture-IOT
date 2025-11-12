# 🔧 ESP32-S3 Aquaculture IoT System - Troubleshooting Guide

## 🚨 Critical Issues Resolution

### System Crashes & Reboots

#### Guru Meditation Errors

**Error Pattern:**
```
Guru Meditation Error: Core 0 panic'ed (LoadProhibited). Exception was unhandled.
Core 0 register dump:
PC      : 0x400d1234  PS      : 0x00060030  A0      : 0x800d5678
```

**Root Causes & Solutions:**

1. **Null Pointer Dereference**
   ```c
   // PROBLEMATIC CODE:
   esp_http_client_handle_t client = NULL;
   esp_http_client_perform(client); // CRASH!
   
   // FIXED CODE:
   if (client != NULL) {
       esp_err_t err = esp_http_client_perform(client);
       if (err != ESP_OK) {
           ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
       }
   }
   ```

2. **Stack Overflow**
   ```c
   // Increase stack size in menuconfig or task creation
   xTaskCreate(task_function, "task_name", 8192, NULL, 5, NULL); // Increased from 4096
   ```

3. **Memory Corruption**
   ```c
   // Enable heap poisoning in menuconfig:
   // Component config -> Heap memory debugging -> Heap poisoning
   ```

**Recovery Steps:**
1. Press RESET button on ESP32-S3
2. Check serial monitor for crash details
3. Use `addr2line` to decode crash addresses:
   ```bash
   xtensa-esp32s3-elf-addr2line -pfiaC -e build/aquaculture_monitor.elf 0x400d1234
   ```

#### Continuous Reboot Loop

**Symptoms:**
```
ets Jul 29 2019 12:21:46
rst:0x1 (POWERON_RESET),boot:0x8 (SPI_FAST_FLASH_BOOT)
configsip: 0, SPIWP:0xee
clk_drv:0x00,q_drv:0x00,d_drv:0x00,cs0_drv:0x00,hd_drv:0x00,wp_drv:0x00
mode:DIO, clock div:1
load:0x3fff0030,len:7104
```

**Solutions:**
1. **Erase Flash Completely**
   ```bash
   idf.py -p /dev/ttyUSB0 erase-flash
   idf.py -p /dev/ttyUSB0 flash
   ```

2. **Check Power Supply**
   - Ensure stable 3.3V supply
   - Check current capacity (minimum 500mA)
   - Use quality USB cable

3. **Bootloader Issues**
   ```bash
   idf.py -p /dev/ttyUSB0 bootloader-flash
   ```

### WiFi Connection Failures

#### Network Not Found

**Error Messages:**
```
E (1234) AQUA: ❌ WiFi connection failed for: TAMNET SYSTEMS
E (1245) AQUA: 🔄 Trying next network...
W (1256) wifi: No AP found
```

**Diagnostic Steps:**

1. **Check Network Availability**
   ```bash
   # On Linux/Mac, scan for networks
   sudo iwlist scan | grep -i "tamnet"
   
   # On Windows
   netsh wlan show profiles
   ```

2. **Verify Credentials**
   ```c
   // In main/aquaculture_monitor.c, check:
   wifi_cred_t wifi_networks[] = {
       {"TAMNET SYSTEMS", "Tamnet123"},  // Exact SSID match required
       {"ztech", "112345678"}
   };
   ```

3. **Signal Strength Issues**
   ```c
   // Add RSSI logging in WiFi scan
   ESP_LOGI(TAG, "Found network: %s (RSSI: %d)", 
            (char*)ap_info[i].ssid, ap_info[i].rssi);
   ```

**Solutions:**

1. **Network Configuration**
   - Ensure 2.4GHz network (ESP32 doesn't support 5GHz)
   - Check for hidden networks
   - Verify WPA2-PSK security (not WPA3 Enterprise)

2. **Antenna Optimization**
   - Use external antenna if available
   - Position device for better signal
   - Avoid metal enclosures

3. **Advanced WiFi Settings**
   ```c
   wifi_config_t wifi_config = {
       .sta = {
           .ssid = "NETWORK_NAME",
           .password = "PASSWORD",
           .scan_method = WIFI_FAST_SCAN,
           .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
           .threshold.rssi = -80,  // Minimum signal strength
           .threshold.authmode = WIFI_AUTH_WPA2_PSK,
       },
   };
   ```

#### Authentication Failures

**Error Messages:**
```
W (5678) wifi: reason: 4way handshake timeout(15)
E (5679) AQUA: WiFi connection failed: ESP_ERR_WIFI_CONN
```

**Common Causes:**
1. **Incorrect Password**
2. **MAC Address Filtering**
3. **Network Congestion**
4. **Router Compatibility Issues**

**Solutions:**
1. **Verify Password**
   ```c
   // Enable WiFi debug logging
   esp_log_level_set("wifi", ESP_LOG_VERBOSE);
   ```

2. **Router Settings**
   - Disable MAC filtering temporarily
   - Check maximum client connections
   - Update router firmware

3. **Channel Selection**
   ```c
   wifi_config_t wifi_config = {
       .sta = {
           .channel = 6,  // Force specific channel
           .listen_interval = 3,
       },
   };
   ```

### SSL/TLS Handshake Failures

#### Certificate Verification Errors

**Error Codes & Solutions:**

| Error Code | mbedTLS Error | Cause | Solution |
|------------|---------------|-------|----------|
| `-0x7780` | `SSL_CONN_EOF` | Connection terminated | Check network stability |
| `-0x2700` | `SSL_INVALID_RECORD` | HTTP/HTTPS mismatch | Verify URL scheme |
| `-0x3000` | `X509_CERT_VERIFY_FAILED` | Certificate invalid | Update certificate chain |
| `-0x7200` | `SSL_TIMEOUT` | Handshake timeout | Increase timeout values |

**Certificate Chain Validation:**

1. **Verify Certificate Chain**
   ```bash
   # Test certificate chain manually
   openssl s_client -connect konuwipzeywfgroqszzz.supabase.co:443 -showcerts
   ```

2. **Update Embedded Certificates**
   ```c
   // Ensure complete chain in aquaculture_monitor.c
   static const char supabase_cert_chain[] = 
   "-----BEGIN CERTIFICATE-----\n"
   // Server certificate
   "MIIDpjCCA0ygAwIBAgIRAKK/z/J073aiE5pdWZEz2j0wCgYIKoZIzj0EAwIwOzEL\n"
   // ... complete certificate
   "-----END CERTIFICATE-----\n"
   "-----BEGIN CERTIFICATE-----\n"
   // Intermediate certificate
   "MIICnzCCAiWgAwIBAgIQf/MZd5csIkp2FV0TttaF4zAKBggqhkjOPQQDAzBHMQsw\n"
   // ... complete certificate
   "-----END CERTIFICATE-----\n";
   ```

3. **HTTP Client Configuration**
   ```c
   esp_http_client_config_t config = {
       .url = SUPABASE_URL,
       .cert_pem = supabase_cert_chain,
       .cert_len = strlen(supabase_cert_chain) + 1,
       .timeout_ms = 15000,  // Increased timeout
       .skip_cert_common_name_check = false,  // Enable CN check
       .use_global_ca_store = false,  // Use embedded certs
   };
   ```

#### Time Synchronization Issues

**Problem:** TLS requires accurate system time for certificate validation.

**Solution:**
```c
// Add NTP synchronization before HTTPS requests
void initialize_sntp(void) {
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();
    
    // Wait for time synchronization
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    
    time(&now);
    localtime_r(&now, &timeinfo);
    ESP_LOGI(TAG, "Time synchronized: %s", asctime(&timeinfo));
}
```

### Database Connection Issues

#### HTTP 400 Bad Request

**Error Message:**
```
E (6789) AQUA: ❌ HTTP POST failed: 400 Bad Request
Response: {"code":"PGRST116","details":null,"hint":null,"message":"Unknown field 'temperature'"}
```

**Root Cause:** Field name mismatch between JSON payload and database schema.

**Solution:**
```c
// INCORRECT - causes 400 error:
cJSON_AddNumberToObject(json, "temperature", air_temp);

// CORRECT - matches database schema:
cJSON_AddNumberToObject(json, "air_temperature", air_temp);
```

**Database Schema Verification:**
```sql
-- Check actual column names in Supabase
SELECT column_name, data_type 
FROM information_schema.columns 
WHERE table_name = 'sensor_data';
```

#### HTTP 401 Unauthorized

**Error Message:**
```
E (7890) AQUA: ❌ HTTP POST failed: 401 Unauthorized
Response: {"message":"Invalid API key"}
```

**Solutions:**

1. **Verify API Key**
   ```c
   // Check Supabase dashboard for correct anon key
   #define SUPABASE_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9..."
   ```

2. **Check Headers**
   ```c
   esp_http_client_set_header(client, "apikey", SUPABASE_KEY);
   esp_http_client_set_header(client, "Authorization", "Bearer " SUPABASE_KEY);
   esp_http_client_set_header(client, "Content-Type", "application/json");
   ```

3. **Row Level Security (RLS)**
   ```sql
   -- Disable RLS for testing (Supabase dashboard)
   ALTER TABLE sensor_data DISABLE ROW LEVEL SECURITY;
   ```

### Sensor Reading Errors

#### ADC Reading Issues

**Symptoms:**
```
W (2345) AQUA: ⚠️ pH sensor disconnected (reading: -999.0)
W (2346) AQUA: ⚠️ Invalid ADC reading: 4095 (max scale)
```

**Diagnostic Steps:**

1. **Check Wiring**
   ```
   Sensor → ESP32-S3
   VCC    → 3.3V
   GND    → GND
   OUT    → GPIO (ADC channel)
   ```

2. **Measure Voltages**
   ```bash
   # Use multimeter to verify:
   # - Power supply: 3.3V ±0.1V
   # - Sensor output: 0-3.3V range
   # - Ground continuity
   ```

3. **ADC Calibration Check**
   ```c
   // Verify ADC calibration
   adc_cali_handle_t adc1_cali_handle = NULL;
   bool do_calibration = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
   if (!do_calibration) {
       ESP_LOGW(TAG, "ADC calibration failed, using raw values");
   }
   ```

**Solutions:**

1. **Hardware Fixes**
   - Check loose connections
   - Verify sensor power requirements
   - Add pull-up/pull-down resistors if needed
   - Use shielded cables for analog signals

2. **Software Filtering**
   ```c
   // Implement moving average filter
   #define FILTER_SIZE 5
   static float filter_buffer[FILTER_SIZE] = {0};
   static int filter_index = 0;
   
   float apply_moving_average(float new_value) {
       filter_buffer[filter_index] = new_value;
       filter_index = (filter_index + 1) % FILTER_SIZE;
       
       float sum = 0;
       for (int i = 0; i < FILTER_SIZE; i++) {
           sum += filter_buffer[i];
       }
       return sum / FILTER_SIZE;
   }
   ```

3. **Sensor-Specific Calibration**
   ```c
   // pH sensor calibration
   float calibrate_ph_sensor(int adc_raw) {
       // Two-point calibration: pH 4.0 and pH 7.0
       const float ph4_voltage = 2.5;   // Measured at pH 4.0
       const float ph7_voltage = 2.0;   // Measured at pH 7.0
       
       float voltage = (float)adc_raw / 4095.0 * 3.3;
       float ph = 7.0 + (voltage - ph7_voltage) / (ph4_voltage - ph7_voltage) * 3.0;
       
       return ph;
   }
   ```

#### Digital Sensor Communication

**DHT22 Reading Failures:**
```
E (3456) DHT: DHT22 read error: timeout
E (3457) AQUA: Failed to read DHT22 sensor
```

**Solutions:**

1. **Timing Issues**
   ```c
   // Ensure proper timing delays
   gpio_set_level(DHT_PIN, 0);
   vTaskDelay(pdMS_TO_TICKS(18));  // Start signal: 18ms low
   gpio_set_level(DHT_PIN, 1);
   ets_delay_us(40);               // 40µs high
   ```

2. **Power Supply Stability**
   ```c
   // Add power-on delay
   gpio_set_level(DHT_POWER_PIN, 1);
   vTaskDelay(pdMS_TO_TICKS(2000));  // 2 second power-up delay
   ```

**DS18B20 OneWire Issues:**
```
E (4567) DS18B20: No devices found on OneWire bus
E (4568) AQUA: Water temperature sensor not responding
```

**Solutions:**

1. **Pull-up Resistor**
   ```
   DS18B20 Data Pin → 4.7kΩ resistor → 3.3V
   ```

2. **OneWire Search**
   ```c
   // Implement device search
   uint8_t rom_codes[8][8];
   int device_count = onewire_search_devices(DHT_PIN, rom_codes, 8);
   ESP_LOGI(TAG, "Found %d OneWire devices", device_count);
   ```

## 🔍 Diagnostic Tools & Commands

### Serial Monitor Analysis

**Enable Verbose Logging:**
```bash
# In menuconfig
idf.py menuconfig
# Component config → Log output → Default log verbosity → Verbose
```

**Log Level Control:**
```c
// Set specific component log levels
esp_log_level_set("wifi", ESP_LOG_VERBOSE);
esp_log_level_set("esp-tls", ESP_LOG_DEBUG);
esp_log_level_set("HTTP_CLIENT", ESP_LOG_DEBUG);
```

### Memory Debugging

**Heap Monitoring:**
```c
// Add to main loop
size_t free_heap = esp_get_free_heap_size();
size_t min_free_heap = esp_get_minimum_free_heap_size();
ESP_LOGI(TAG, "Free heap: %d bytes, Min free: %d bytes", free_heap, min_free_heap);

// Enable heap tracing
#include "esp_heap_trace.h"
heap_trace_start(HEAP_TRACE_LEAKS);
// ... application code ...
heap_trace_stop();
heap_trace_dump();
```

**Stack Usage:**
```c
// Check task stack usage
UBaseType_t stack_high_water = uxTaskGetStackHighWaterMark(NULL);
ESP_LOGI(TAG, "Stack high water mark: %d bytes", stack_high_water * sizeof(StackType_t));
```

### Network Diagnostics

**WiFi Signal Analysis:**
```c
wifi_ap_record_t ap_info;
esp_wifi_sta_get_ap_info(&ap_info);
ESP_LOGI(TAG, "Connected to: %s, RSSI: %d, Channel: %d", 
         ap_info.ssid, ap_info.rssi, ap_info.primary);
```

**HTTP Response Analysis:**
```c
// Log complete HTTP response
char *response_buffer = malloc(1024);
int content_length = esp_http_client_read(client, response_buffer, 1024);
ESP_LOGI(TAG, "HTTP Response (%d bytes): %.*s", content_length, content_length, response_buffer);
free(response_buffer);
```

### Hardware Testing

**GPIO Test Sequence:**
```c
// Test all GPIO pins
void test_gpio_pins(void) {
    gpio_num_t test_pins[] = {GPIO_NUM_4, GPIO_NUM_5, GPIO_NUM_14};
    
    for (int i = 0; i < sizeof(test_pins)/sizeof(gpio_num_t); i++) {
        gpio_set_direction(test_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(test_pins[i], 1);
        vTaskDelay(pdMS_TO_TICKS(500));
        gpio_set_level(test_pins[i], 0);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```

**ADC Calibration Test:**
```c
// Test ADC linearity
void test_adc_calibration(void) {
    for (int i = 0; i < 10; i++) {
        int adc_raw = 0;
        adc_oneshot_read(adc1_handle, ADC_CHANNEL_0, &adc_raw);
        
        int voltage_mv = 0;
        adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, &voltage_mv);
        
        ESP_LOGI(TAG, "ADC Raw: %d, Voltage: %d mV", adc_raw, voltage_mv);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

## 🛠️ Recovery Procedures

### Factory Reset

**Complete System Reset:**
```bash
# Erase everything and reflash
idf.py -p /dev/ttyUSB0 erase-flash
idf.py -p /dev/ttyUSB0 flash
```

**Preserve Configuration:**
```bash
# Erase only application partition
esptool.py --port /dev/ttyUSB0 erase_region 0x10000 0x140000
idf.py -p /dev/ttyUSB0 app-flash
```

### Configuration Reset

**Clear NVS Storage:**
```c
// In application code
nvs_flash_erase();
nvs_flash_init();
ESP_LOGI(TAG, "NVS storage cleared, rebooting...");
esp_restart();
```

### Emergency Recovery Mode

**Enter Download Mode:**
1. Hold BOOT button
2. Press and release RESET button
3. Release BOOT button
4. Device enters download mode

**Manual Flash Recovery:**
```bash
# Use esptool directly
esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 write_flash --flash_mode dio --flash_freq 80m --flash_size 4MB 0x0 bootloader.bin 0x10000 app.bin 0x8000 partition-table.bin
```

## 📞 Support Resources

### Log Collection

**Comprehensive Log Capture:**
```bash
# Capture logs with timestamps
idf.py -p /dev/ttyUSB0 monitor | tee -a system_logs_$(date +%Y%m%d_%H%M%S).txt
```

**System Information:**
```c
// Add to startup logs
esp_chip_info_t chip_info;
esp_chip_info(&chip_info);
ESP_LOGI(TAG, "ESP32-S3 with %d CPU cores, WiFi%s%s, revision %d",
         chip_info.cores,
         (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
         (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "",
         chip_info.revision);

ESP_LOGI(TAG, "Flash: %dMB %s", spi_flash_get_chip_size() / (1024 * 1024),
         (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
```

### Issue Reporting Template

When reporting issues, include:

1. **Hardware Information**
   - ESP32-S3 board model
   - Sensor types and connections
   - Power supply specifications

2. **Software Information**
   - ESP-IDF version
   - Application version/commit hash
   - Build configuration

3. **Error Details**
   - Complete error logs
   - Steps to reproduce
   - Expected vs actual behavior

4. **Environment**
   - Network configuration
   - Physical setup
   - Environmental conditions

---

*This troubleshooting guide covers the most common issues and their solutions. For additional support, refer to the ESP-IDF documentation and community forums.*
