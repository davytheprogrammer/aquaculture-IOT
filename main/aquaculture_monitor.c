#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_adc/adc_cali.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_crt_bundle.h"
#include "esp_tls.h"

#define TAG "AQUA"

// ========== COMPACT CONFIG ==========
#define WIFI_SSID "TAMNET SYSTEMS"
#define WIFI_PASS "Tamnet123"
#define SUPABASE_URL "https://konuwipzeywfgroqszzz.supabase.co/rest/v1/sensor_data"
#define SUPABASE_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtvbnV3aXB6ZXl3Zmdyb3Fzenp6Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTk4MzY4MzQsImV4cCI6MjA3NTQxMjgzNH0.i-Ru1I5Y8QAve9MiD64pKQ3yereW0022sbRj-xlxjWY"

// ========== PIN CONFIG ==========
// Digital Sensors
#define DHT_PIN GPIO_NUM_4                    // DHT22 for air temp & humidity
#define WATER_TEMP_PIN GPIO_NUM_5             // DS18B20 water temperature sensor
#define PUMP_PIN GPIO_NUM_6                   // DC pump control

// Analog Sensors (ADC1)
#define PH_ADC_CH ADC_CHANNEL_5              // pH sensor on GPIO6
#define DO_ADC_CH ADC_CHANNEL_6              // Dissolved Oxygen sensor on GPIO7
#define TURBIDITY_ADC_CH ADC_CHANNEL_7       // Turbidity sensor on GPIO8
#define AMMONIA_ADC_CH ADC_CHANNEL_8         // Ammonia sensor on GPIO9

// Control Outputs
#define RELAY_PIN GPIO_NUM_10                 // pH control relay
#define PUMP_RELAY_PIN GPIO_NUM_11           // Pump relay
#define AERATOR_PIN GPIO_NUM_12              // Aerator control for DO
#define FILTER_PIN GPIO_NUM_13               // Filter control for turbidity

#define SAMPLE_DELAY_MS 10000 // 10 seconds between readings
#define WATCHDOG_FEED_INTERVAL 1000 // Feed watchdog every 1 second

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Test mode flag
#define TEST_MODE 0  // Set to 1 to enable test sequence

// Test sequence values
#define TEST_AIR_TEMP_NORMAL 25.0f
#define TEST_AIR_TEMP_HIGH 35.0f
#define TEST_WATER_TEMP_NORMAL 26.0f
#define TEST_WATER_TEMP_LOW 15.0f
#define TEST_DO_NORMAL 8.0f
#define TEST_DO_LOW 3.0f
#define TEST_PH_NORMAL 7.0f
#define TEST_PH_HIGH 9.0f
#define TEST_TURBIDITY_NORMAL 10.0f
#define TEST_TURBIDITY_HIGH 30.0f
#define TEST_AMMONIA_NORMAL 0.5f
#define TEST_AMMONIA_HIGH 2.0f

// ========== STATIC ADC (no heap) ==========
static esp_adc_cal_characteristics_t adc_chars;
static EventGroupHandle_t s_wifi_event_group;

// ========== IMPROVED WIFI ==========
static void wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, trying to reconnect...");
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) data;
        ESP_LOGI(TAG, "WiFi CONNECTED - IP: " IPSTR ", DNS1: 1.1.1.1, DNS2: 8.8.8.8", IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void wifi_init(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_LOGI(TAG, "Initializing NVS Flash...");
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI(TAG, "Connecting to WiFi: %s", WIFI_SSID);

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();

    // Configure DNS servers
    esp_netif_dns_info_t dns_info = {0};
    IP4_ADDR(&dns_info.ip.u_addr.ip4, 1, 1, 1, 1);  // Cloudflare DNS
    esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_MAIN, &dns_info);
    IP4_ADDR(&dns_info.ip.u_addr.ip4, 8, 8, 8, 8);  // Google DNS
    esp_netif_set_dns_info(sta_netif, ESP_NETIF_DNS_BACKUP, &dns_info);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
}

// ========== DHT22 (improved reliability) ==========
static esp_err_t dht22_read(float* hum, float* temp) {
    uint8_t data[5] = {0};
    uint64_t start_time;

    gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20));  // Hold low for 20ms
    gpio_set_level(DHT_PIN, 1);
    ets_delay_us(40);
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);

    // Wait for DHT22 to pull low (start signal)
    start_time = esp_timer_get_time();
    while (gpio_get_level(DHT_PIN) == 1) {
        if (esp_timer_get_time() - start_time > 200) {
            return ESP_ERR_TIMEOUT;
        }
    }

    // Wait for DHT22 to pull high
    start_time = esp_timer_get_time();
    while (gpio_get_level(DHT_PIN) == 0) {
        if (esp_timer_get_time() - start_time > 200) {
            return ESP_ERR_TIMEOUT;
        }
    }

    // Wait for DHT22 to pull low
    start_time = esp_timer_get_time();
    while (gpio_get_level(DHT_PIN) == 1) {
        if (esp_timer_get_time() - start_time > 200) {
            return ESP_ERR_TIMEOUT;
        }
    }

    // Read 40 bits (5 bytes)
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 8; j++) {
            // Wait for rising edge
            start_time = esp_timer_get_time();
            while (gpio_get_level(DHT_PIN) == 0) {
                if (esp_timer_get_time() - start_time > 200) {
                    return ESP_ERR_TIMEOUT;
                }
            }

            ets_delay_us(40); // Wait and sample after 40us
            if (gpio_get_level(DHT_PIN) == 1) {
                data[i] |= (1 << (7 - j));

                // Wait for falling edge
                start_time = esp_timer_get_time();
                while (gpio_get_level(DHT_PIN) == 1) {
                    if (esp_timer_get_time() - start_time > 200) {
                        return ESP_ERR_TIMEOUT;
                    }
                }
            }
        }
    }

    // Verify checksum
    if (data[4] != ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        return ESP_ERR_INVALID_CRC;
    }

    *hum = ((data[0] << 8) + data[1]) / 10.0f;
    *temp = ((data[2] << 8) + data[3]) / 10.0f;

    // Validate readings
    if (*temp < -40.0f || *temp > 80.0f || *hum < 0.0f || *hum > 100.0f) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    ESP_LOGI(TAG, "DHT22 Raw data: %d %d %d %d %d",
             data[0], data[1], data[2], data[3], data[4]);
    return ESP_OK;
}

// ========== SENSOR READINGS ==========

// pH Sensor
static float read_ph(void) {
    const int SAMPLES = 10;
    int sum = 0;

    for (int i = 0; i < SAMPLES; i++) {
        sum += adc1_get_raw(PH_ADC_CH);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int raw = sum / SAMPLES;
    uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);

    // pH calculation (adjust these values based on calibration)
    float ph = 7.0f + ((2500.0f - mv) / 180.0f);
    return (ph >= 0.0f && ph <= 14.0f) ? ph : -1.0f;
}

// Dissolved Oxygen Sensor
static float read_do(void) {
    const int SAMPLES = 10;
    int sum = 0;

    for (int i = 0; i < SAMPLES; i++) {
        sum += adc1_get_raw(DO_ADC_CH);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int raw = sum / SAMPLES;
    uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);

    // DO calculation (adjust calibration values)
    float do_value = mv * 0.2f; // Convert mV to mg/L (adjust factor based on calibration)
    return (do_value >= 0.0f && do_value <= 20.0f) ? do_value : -1.0f;
}

// Turbidity Sensor
static float read_turbidity(void) {
    const int SAMPLES = 10;
    int sum = 0;

    for (int i = 0; i < SAMPLES; i++) {
        sum += adc1_get_raw(TURBIDITY_ADC_CH);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int raw = sum / SAMPLES;
    uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);

    // Turbidity calculation (adjust calibration values)
    float ntu = mv * 0.5f; // Convert mV to NTU (adjust factor based on calibration)
    return (ntu >= 0.0f && ntu <= 1000.0f) ? ntu : -1.0f;
}

// Ammonia Sensor
static float read_ammonia(void) {
    const int SAMPLES = 10;
    int sum = 0;

    for (int i = 0; i < SAMPLES; i++) {
        sum += adc1_get_raw(AMMONIA_ADC_CH);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int raw = sum / SAMPLES;
    uint32_t mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);

    // Ammonia calculation (adjust calibration values)
    float nh3 = mv * 0.1f; // Convert mV to mg/L (adjust factor based on calibration)
    return (nh3 >= 0.0f && nh3 <= 10.0f) ? nh3 : -1.0f;
}

// Water Temperature (DS18B20)
static float read_water_temp(void) {
    // Note: This is a simplified implementation. For production,
    // implement full DS18B20 1-wire protocol
    float temp = 25.0f; // Default value
    // TODO: Implement DS18B20 reading protocol
    return temp;
}

// Global HTTP client for reuse
static esp_http_client_handle_t supabase_client = NULL;

// Initialize HTTP client once during startup
static void init_supabase_client(void) {
    esp_http_client_config_t config = {
        .url = SUPABASE_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 20000,            // Increased timeout
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .cert_pem = supabase_root_cert,
        .skip_cert_common_name_check = true,
        .crt_bundle_attach = NULL,      // Don't use the bundle
        .keep_alive_enable = true,      // Enable keep-alive
        .buffer_size = 2048,            // Increased buffer size
        .buffer_size_tx = 1024,         // Increased TX buffer
        .disable_auto_redirect = true,   // Disable redirects
        .max_authorization_retries = 0   // No retries on auth
    };

    if (supabase_client) {
        esp_http_client_cleanup(supabase_client);
    }

    supabase_client = esp_http_client_init(&config);
    if (!supabase_client) {
        ESP_LOGE(TAG, "[SUPABASE] Failed to initialize HTTP client");
        return;
    }

    esp_http_client_set_header(supabase_client, "Content-Type", "application/json");
    esp_http_client_set_header(supabase_client, "apikey", SUPABASE_KEY);
    esp_http_client_set_header(supabase_client, "Authorization", "Bearer " SUPABASE_KEY);
}

// Cleanup HTTP client
static void cleanup_supabase_client(void) {
    if (supabase_client) {
        esp_http_client_cleanup(supabase_client);
        supabase_client = NULL;
    }
}

// ========== IMPROVED HTTP UPLOAD ==========
static bool send_to_supabase(float air_temp, float water_temp, float hum, float ph,
                        float do_level, float turbidity, float ammonia,
                        bool ph_relay, bool aerator, bool filter, bool pump) {
    const int MAX_RETRIES = 3;
    int retry_count = 0;
    int delay_ms = 1000; // Start with 1 second delay

    // Wait for WiFi connection
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            pdMS_TO_TICKS(1000));  // Add timeout

    if (!(bits & WIFI_CONNECTED_BIT)) {
        ESP_LOGE(TAG, "WiFi not connected");
        return false;
    }

    // Validate all sensor values
    if (air_temp < -40 || air_temp > 80 ||
        water_temp < -40 || water_temp > 80 ||
        hum < 0 || hum > 100 ||
        ph < 0 || ph > 14 ||
        do_level < 0 || do_level > 20 ||
        turbidity < 0 || turbidity > 1000 ||
        ammonia < 0 || ammonia > 10) {
        ESP_LOGE(TAG, "Invalid sensor values");
        return false;
    }

    // Create JSON in smaller chunks to reduce stack usage
    char json[384];  // Reduced buffer size
    char temp[64];  // Temporary buffer

    strcpy(json, "{");

    // Temperature and humidity
    snprintf(temp, sizeof(temp), "\"temperature\":%.2f,\"humidity\":%.2f,", air_temp, hum);
    strcat(json, temp);

    // Water temperature and pH
    snprintf(temp, sizeof(temp), "\"water_temperature\":%.2f,\"ph\":%.2f,", water_temp, ph);
    strcat(json, temp);

    // DO and turbidity
    snprintf(temp, sizeof(temp), "\"dissolved_oxygen\":%.2f,\"turbidity\":%.2f,", do_level, turbidity);
    strcat(json, temp);

    // Ammonia
    snprintf(temp, sizeof(temp), "\"ammonia\":%.2f,", ammonia);
    strcat(json, temp);

    // Control states
    snprintf(temp, sizeof(temp),
        "\"ph_relay\":%s,\"aerator\":%s,\"filter\":%s,\"pump\":%s,\"created_at\":\"NOW()\"}",
        ph_relay ? "true" : "false",
        aerator ? "true" : "false",
        filter ? "true" : "false",
        pump ? "true" : "false");
    strcat(json, temp);

    ESP_LOGI(TAG, "[SUPABASE] Preparing HTTP request...");
    ESP_LOGI(TAG, "[SUPABASE] URL: %s", SUPABASE_URL);
    ESP_LOGI(TAG, "[SUPABASE] Method: POST");
    ESP_LOGI(TAG, "[SUPABASE] Headers: Content-Type=application/json, apikey=*****, Authorization=Bearer *****");
    ESP_LOGI(TAG, "[SUPABASE] Payload: %s", json);

    while (retry_count < MAX_RETRIES) {
        // Cleanup and reinitialize for each attempt
        cleanup_supabase_client();
        init_supabase_client();

        if (!supabase_client) {
            ESP_LOGE(TAG, "[SUPABASE] Failed to initialize client");
            return false;
        }

        // Set headers
        esp_http_client_set_header(supabase_client, "Content-Type", "application/json");
        esp_http_client_set_header(supabase_client, "apikey", SUPABASE_KEY);
        esp_http_client_set_header(supabase_client, "Authorization", "Bearer " SUPABASE_KEY);

        // Set post data
        esp_http_client_set_post_field(supabase_client, json, strlen(json));

        // Perform request
        esp_err_t err = esp_http_client_perform(supabase_client);
        int status_code = esp_http_client_get_status_code(supabase_client);

        if (err == ESP_OK && (status_code == 200 || status_code == 201)) {
            ESP_LOGI(TAG, "[SUPABASE] Data sent successfully (Status: %d)", status_code);
            cleanup_supabase_client();
            return true;
        }

        ESP_LOGW(TAG, "[SUPABASE] Attempt %d failed. Status: %d, Error: %s",
                 retry_count + 1, status_code, esp_err_to_name(err));

        cleanup_supabase_client();

        if (retry_count < MAX_RETRIES - 1) {
            ESP_LOGI(TAG, "[SUPABASE] Retrying in %d ms...", delay_ms);
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
            delay_ms *= 2; // Exponential backoff
        }

        retry_count++;
    }

    ESP_LOGI(TAG, "[SUPABASE] Performing HTTP request...");
    esp_err_t err = esp_http_client_perform(supabase_client);
    int status = esp_http_client_get_status_code(supabase_client);

    ESP_LOGI(TAG, "[SUPABASE] HTTP Response Status: %d", status);
    ESP_LOGI(TAG, "[SUPABASE] ESP-IDF Error Code: %s", esp_err_to_name(err));
    ESP_LOGI(TAG, "[SUPABASE] ESP-IDF Error Value: %d", err);

    bool success = (err == ESP_OK && status >= 200 && status < 300);
    if (!success) {
        // Reinitialize client on error
        init_supabase_client();
    }

    return success;
}

// Alert thresholds
#define TEMP_MIN 20.0f
#define TEMP_MAX 30.0f
#define DO_MIN 5.0f
#define PH_MIN 6.5f
#define PH_MAX 8.5f
#define AMMONIA_MAX 1.0f
#define TURBIDITY_MAX 20.0f

// Structure to hold alert states
typedef struct {
    bool high_temp;
    bool low_temp;
    bool low_do;
    bool high_ph;
    bool low_ph;
    bool high_ammonia;
    bool high_turbidity;
} alert_states_t;

static alert_states_t last_alerts = {0};

// Global HTTP client for alerts
static esp_http_client_handle_t alert_client = NULL;

// Initialize alert HTTP client once during startup
static void init_alert_client(void) {
    esp_http_client_config_t config = {
        .url = SUPABASE_URL "/alerts",
        .method = HTTP_METHOD_POST,
        .timeout_ms = 5000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .skip_cert_common_name_check = true,
        .buffer_size = 1024,
        .buffer_size_tx = 512,
        .keep_alive_enable = false
    };

    if (alert_client) {
        esp_http_client_cleanup(alert_client);
    }

    alert_client = esp_http_client_init(&config);
    if (!alert_client) {
        ESP_LOGE(TAG, "[ALERTS] Failed to initialize HTTP client");
        return;
    }

    esp_http_client_set_header(alert_client, "Content-Type", "application/json");
    esp_http_client_set_header(alert_client, "apikey", SUPABASE_KEY);
    esp_http_client_set_header(alert_client, "Authorization", "Bearer " SUPABASE_KEY);
}

// Cleanup alert HTTP client
static void cleanup_alert_client(void) {
    if (alert_client) {
        esp_http_client_cleanup(alert_client);
        alert_client = NULL;
    }
}

// Check sensor values and send alerts if needed
static void check_and_send_alerts(float water_temp, float do_level, float ph,
                                float ammonia, float turbidity) {
    alert_states_t current_alerts = {0};
    char json[384];  // Reduced buffer size
    char temp[64];   // Temporary buffer

    // Check each parameter
    current_alerts.high_temp = water_temp > TEMP_MAX;
    current_alerts.low_temp = water_temp < TEMP_MIN;
    current_alerts.low_do = do_level < DO_MIN;
    current_alerts.high_ph = ph > PH_MAX;
    current_alerts.low_ph = ph < PH_MIN;
    current_alerts.high_ammonia = ammonia > AMMONIA_MAX;
    current_alerts.high_turbidity = turbidity > TURBIDITY_MAX;

    // Compare with last state to avoid duplicate alerts
    if (memcmp(&last_alerts, &current_alerts, sizeof(alert_states_t)) != 0) {
        strcpy(json, "{\"type\":\"alert\",\"alerts\":{");

        // Alert flags
        snprintf(temp, sizeof(temp),
                "\"high_temperature\":%s,\"low_temperature\":%s,",
                current_alerts.high_temp ? "true" : "false",
                current_alerts.low_temp ? "true" : "false");
        strcat(json, temp);

        snprintf(temp, sizeof(temp),
                "\"low_dissolved_oxygen\":%s,\"high_ph\":%s,\"low_ph\":%s,",
                current_alerts.low_do ? "true" : "false",
                current_alerts.high_ph ? "true" : "false",
                current_alerts.low_ph ? "true" : "false");
        strcat(json, temp);

        snprintf(temp, sizeof(temp),
                "\"high_ammonia\":%s,\"high_turbidity\":%s,",
                current_alerts.high_ammonia ? "true" : "false",
                current_alerts.high_turbidity ? "true" : "false");
        strcat(json, temp);

        // Sensor values
        snprintf(temp, sizeof(temp),
                "\"water_temp\":%.2f,\"do_level\":%.2f,\"ph\":%.2f,"
                "\"ammonia\":%.2f,\"turbidity\":%.2f}}",
                water_temp, do_level, ph, ammonia, turbidity);
        strcat(json, temp);

        if (!alert_client) {
            init_alert_client();
            if (!alert_client) {
                return;
            }
        }

        esp_http_client_set_post_field(alert_client, json, strlen(json));
        esp_err_t err = esp_http_client_perform(alert_client);

        if (err == ESP_OK) {
            int status = esp_http_client_get_status_code(alert_client);
            if (status >= 200 && status < 300) {
                // Update last alert state only if successfully sent
                memcpy(&last_alerts, &current_alerts, sizeof(alert_states_t));
                ESP_LOGI(TAG, "Alert notification sent successfully");
            }
        } else {
            ESP_LOGE(TAG, "Failed to send alert notification");
            // Reinitialize client on error
            init_alert_client();
        }
    }
}

// Watchdog-friendly delay function
static void watchdog_friendly_delay(int delay_ms) {
    int intervals = delay_ms / WATCHDOG_FEED_INTERVAL;
    int remainder = delay_ms % WATCHDOG_FEED_INTERVAL;

    // Feed watchdog periodically during the delay
    for(int i = 0; i < intervals; i++) {
        vTaskDelay(pdMS_TO_TICKS(WATCHDOG_FEED_INTERVAL));
        esp_task_wdt_reset(); // Feed the watchdog
    }

    // Handle any remaining time
    if(remainder > 0) {
        vTaskDelay(pdMS_TO_TICKS(remainder));
        esp_task_wdt_reset(); // Feed the watchdog one final time
    }
}

// Test sequence function
static void run_test_sequence(void) {
    ESP_LOGI(TAG, "\n=== STARTING TEST SEQUENCE ===\n");

    // Test 1: Normal conditions
    ESP_LOGI(TAG, "Test 1: Normal conditions");
    if (!send_to_supabase(
        TEST_AIR_TEMP_NORMAL,    // Air temp normal
        TEST_WATER_TEMP_NORMAL,  // Water temp normal
        65.0f,                   // Humidity normal
        TEST_PH_NORMAL,          // pH normal
        TEST_DO_NORMAL,          // DO normal
        TEST_TURBIDITY_NORMAL,   // Turbidity normal
        TEST_AMMONIA_NORMAL,     // Ammonia normal
        false,                   // pH relay off
        false,                   // Aerator off
        false,                   // Filter off
        false                    // Pump off
    )) {
        ESP_LOGE(TAG, "Test 1 failed!");
    }
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Test 2: High temperature alert
    ESP_LOGI(TAG, "Test 2: High temperature condition");
    if (!send_to_supabase(
        TEST_AIR_TEMP_HIGH,      // Air temp high
        TEST_WATER_TEMP_NORMAL,
        65.0f,
        TEST_PH_NORMAL,
        TEST_DO_NORMAL,
        TEST_TURBIDITY_NORMAL,
        TEST_AMMONIA_NORMAL,
        false, false, false, false
    )) {
        ESP_LOGE(TAG, "Test 2 failed!");
    }
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Test 3: Low dissolved oxygen and high turbidity
    ESP_LOGI(TAG, "Test 3: Low DO and high turbidity condition");
    if (!send_to_supabase(
        TEST_AIR_TEMP_NORMAL,
        TEST_WATER_TEMP_NORMAL,
        65.0f,
        TEST_PH_NORMAL,
        TEST_DO_LOW,            // DO low
        TEST_TURBIDITY_HIGH,    // Turbidity high
        TEST_AMMONIA_NORMAL,
        false,
        true,                   // Aerator on
        true,                   // Filter on
        false
    )) {
        ESP_LOGE(TAG, "Test 3 failed!");
    }
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Test 4: High pH and ammonia
    ESP_LOGI(TAG, "Test 4: High pH and ammonia condition");
    if (!send_to_supabase(
        TEST_AIR_TEMP_NORMAL,
        TEST_WATER_TEMP_NORMAL,
        65.0f,
        TEST_PH_HIGH,           // pH high
        TEST_DO_NORMAL,
        TEST_TURBIDITY_NORMAL,
        TEST_AMMONIA_HIGH,      // Ammonia high
        true,                   // pH relay on
        false,
        false,
        true                    // Pump on
    )) {
        ESP_LOGE(TAG, "Test 4 failed!");
    }
    vTaskDelay(pdMS_TO_TICKS(5000));

    ESP_LOGI(TAG, "\n=== TEST SEQUENCE COMPLETE ===\n");
}

// ========== MAIN APPLICATION ==========
void app_main(void) {
    printf("\n========================================\n");
    printf("    AQUACULTURE MONITOR v4.0 - TASK-BASED\n");
    printf("========================================\n");

    // Initialize components
    ESP_LOGI(TAG, "Initializing NVS Flash...");
    ESP_ERROR_CHECK(nvs_flash_init());

    // Configure watchdog timer
    ESP_LOGI(TAG, "Configuring watchdog timer...");
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 30000,              // 30 second timeout
        .trigger_panic = false            // Don't panic on timeout
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));    // Add idle task

    ESP_LOGI(TAG, "Initializing global CA store...");
    esp_tls_init_global_ca_store();

    // Connect to WiFi
    ESP_LOGI(TAG, "Connecting to WiFi: %s", WIFI_SSID);
    wifi_init();

    // Initialize HTTP clients
    init_supabase_client();
    init_alert_client();

    // Initialize all GPIO pins
    ESP_LOGI(TAG, "Configuring GPIO pins...");

    // Configure digital sensor pins
    gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(WATER_TEMP_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(PUMP_PIN, GPIO_MODE_OUTPUT);

    // Configure control output pins
    gpio_set_direction(RELAY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(PUMP_RELAY_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(AERATOR_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(FILTER_PIN, GPIO_MODE_OUTPUT);

    // Set initial states for outputs
    gpio_set_level(RELAY_PIN, 0);
    gpio_set_level(PUMP_RELAY_PIN, 0);
    gpio_set_level(AERATOR_PIN, 0);
    gpio_set_level(FILTER_PIN, 0);
    gpio_set_level(PUMP_PIN, 0);

    // Configure ADC for analog sensors
    ESP_LOGI(TAG, "Setting up ADC for sensors...");
    adc1_config_width(ADC_WIDTH_BIT_12);

    // Configure ADC channels with 11dB attenuation for higher voltage range
    adc1_config_channel_atten(PH_ADC_CH, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(DO_ADC_CH, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(TURBIDITY_ADC_CH, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(AMMONIA_ADC_CH, ADC_ATTEN_DB_11);

    // Characterize ADC
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);

    // Sensor variables
    float air_temp = 0, hum = 0, water_temp = 0;
    float ph = 0, do_level = 0, turbidity = 0, ammonia = 0;

    // Control states
    bool ph_relay_on = false;
    bool pump_on = false;
    bool aerator_on = false;
    bool filter_on = false;

    int cycle_count = 0;

    ESP_LOGI(TAG, "Starting enhanced sensor monitoring loop...");
    ESP_LOGI(TAG, "Reading cycles every %d seconds", SAMPLE_DELAY_MS/1000);

    // Run test sequence if in test mode
    #if TEST_MODE
    ESP_LOGI(TAG, "Running in TEST MODE");
    run_test_sequence();
    ESP_LOGI(TAG, "Test sequence completed. Continuing with normal operation...\n");
    #endif

    while (1) {
        cycle_count++;
        esp_task_wdt_reset(); // Feed the watchdog at the start of each cycle
        ESP_LOGI(TAG, "\n========== CYCLE #%d ==========", cycle_count);

        // Read Air Temperature and Humidity (DHT22)
        ESP_LOGI(TAG, "Reading DHT22 sensor...");
        esp_err_t dht_result = dht22_read(&hum, &air_temp);
        if (dht_result != ESP_OK) {
            ESP_LOGW(TAG, "DHT22 READ FAILED - Using dummy values");
            air_temp = 26.2f + ((float)esp_random() / UINT32_MAX - 0.5f);
            hum = 72.9f + ((float)esp_random() / UINT32_MAX - 0.5f);
        }
        ESP_LOGI(TAG, "Air Temp: %.1f°C, Humidity: %.1f%%", air_temp, hum);
        esp_task_wdt_reset();

        // Read Water Temperature
        ESP_LOGI(TAG, "Reading water temperature...");
        water_temp = read_water_temp();
        ESP_LOGI(TAG, "Water Temp: %.1f°C", water_temp);
        esp_task_wdt_reset();

        // Read pH
        ESP_LOGI(TAG, "Reading pH...");
        ph = read_ph();
        if (ph < 0) {
            ESP_LOGW(TAG, "pH sensor error - Using default");
            ph = 7.0f;
        }
        ESP_LOGI(TAG, "pH: %.2f", ph);
        esp_task_wdt_reset();

        // Read Dissolved Oxygen
        ESP_LOGI(TAG, "Reading dissolved oxygen...");
        do_level = read_do();
        if (do_level < 0) {
            ESP_LOGW(TAG, "DO sensor error - Using default");
            do_level = 8.0f;
        }
        ESP_LOGI(TAG, "DO: %.2f mg/L", do_level);
        esp_task_wdt_reset();

        // Read Turbidity
        ESP_LOGI(TAG, "Reading turbidity...");
        turbidity = read_turbidity();
        if (turbidity < 0) {
            ESP_LOGW(TAG, "Turbidity sensor error - Using default");
            turbidity = 10.0f;
        }
        ESP_LOGI(TAG, "Turbidity: %.2f NTU", turbidity);
        esp_task_wdt_reset();

        // Read Ammonia
        ESP_LOGI(TAG, "Reading ammonia...");
        ammonia = read_ammonia();
        if (ammonia < 0) {
            ESP_LOGW(TAG, "Ammonia sensor error - Using default");
            ammonia = 0.5f;
        }
        ESP_LOGI(TAG, "Ammonia: %.2f mg/L", ammonia);
        esp_task_wdt_reset();

        // Control System Logic
        ph_relay_on = (ph < 6.5f);                  // Activate if pH is too low
        aerator_on = (do_level < 5.0f);            // Activate if DO is too low
        filter_on = (turbidity > 20.0f);           // Activate if water is too turbid
        pump_on = (ammonia > 1.0f);                // Activate if ammonia is too high

        // Update control outputs
        gpio_set_level(RELAY_PIN, ph_relay_on);
        gpio_set_level(AERATOR_PIN, aerator_on);
        gpio_set_level(FILTER_PIN, filter_on);
        gpio_set_level(PUMP_PIN, pump_on);

        ESP_LOGI(TAG, "Control States - pH Relay: %s, Aerator: %s, Filter: %s, Pump: %s",
                ph_relay_on ? "ON" : "OFF",
                aerator_on ? "ON" : "OFF",
                filter_on ? "ON" : "OFF",
                pump_on ? "ON" : "OFF");

        // Log final readings
        ESP_LOGI(TAG, "FINAL READINGS: Temperature=%.1f°C, Humidity=%.1f%%, pH=%.2f, Relay=%s",
                 air_temp, hum, ph, ph_relay_on ? "ON" : "OFF");

        // Send data to Supabase
        ESP_LOGI(TAG, "Sending data to Supabase...");
        if (!send_to_supabase(air_temp, water_temp, hum, ph, do_level, turbidity, ammonia,
                             ph_relay_on, aerator_on, filter_on, pump_on)) {
            ESP_LOGW(TAG, "UPLOAD FAILED");
        }

        // Check conditions and send alerts if needed
        ESP_LOGI(TAG, "Checking alert conditions...");
        check_and_send_alerts(water_temp, do_level, ph, ammonia, turbidity);

        esp_task_wdt_reset(); // Feed the watchdog after Supabase upload

        ESP_LOGI(TAG, "Cycle #%d complete. Sleeping %d seconds...",
                 cycle_count, SAMPLE_DELAY_MS/1000);

        // Use the watchdog-friendly delay instead of a single long vTaskDelay
        watchdog_friendly_delay(SAMPLE_DELAY_MS);
    }
}
