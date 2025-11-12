#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_http_client.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "adc_handler.h"
#include "esp_timer.h"
#include "esp_task_wdt.h"
#include "esp_crt_bundle.h"
#include "esp_tls.h"

#define TAG "AQUA"

// Complete Supabase certificate chain
static const char supabase_cert_chain[] = 
// Server certificate
"-----BEGIN CERTIFICATE-----\n"
"MIIDpjCCA0ygAwIBAgIRAKK/z/J073aiE5pdWZEz2j0wCgYIKoZIzj0EAwIwOzEL\n"
"MAkGA1UEBhMCVVMxHjAcBgNVBAoTFUdvb2dsZSBUcnVzdCBTZXJ2aWNlczEMMAoG\n"
"A1UEAxMDV0UxMB4XDTI1MTEwNDA1NTU0MloXDTI2MDIwMjA2NTUzN1owFjEUMBIG\n"
"A1UEAxMLc3VwYWJhc2UuY28wWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAASTufZL\n"
"i4D0pF6nFPjnRZv9oRCUT4QAWCtv2NoypLipMex4NrkQHP5vUAObZ7Rc3OHG2pKn\n"
"cWeLfhC5y+P6+UsIo4ICVDCCAlAwDgYDVR0PAQH/BAQDAgeAMBMGA1UdJQQMMAoG\n"
"CCsGAQUFBwMBMAwGA1UdEwEB/wQCMAAwHQYDVR0OBBYEFJdLppI8x80ZJ/pvIMMC\n"
"OJo2wdihMB8GA1UdIwQYMBaAFJB3kjVnxP+ozKnme9mAeXvMk/k4MF4GCCsGAQUF\n"
"BwEBBFIwUDAnBggrBgEFBQcwAYYbaHR0cDovL28ucGtpLmdvb2cvcy93ZTEvb3I4\n"
"MCUGCCsGAQUFBzAChhlodHRwOi8vaS5wa2kuZ29vZy93ZTEuY3J0MCUGA1UdEQQe\n"
"MByCC3N1cGFiYXNlLmNvgg0qLnN1cGFiYXNlLmNvMBMGA1UdIAQMMAowCAYGZ4EM\n"
"AQIBMDYGA1UdHwQvMC0wK6ApoCeGJWh0dHA6Ly9jLnBraS5nb29nL3dlMS90cUFZ\n"
"YXFFeFJiTS5jcmwwggEFBgorBgEEAdZ5AgQCBIH2BIHzAPEAdgAWgy2r8KklDw/w\n"
"OqVF/8i/yCPQh0v2BCkn+OcfMxP1+gAAAZpNpmkbAAAEAwBHMEUCIFbq8cvd0ynK\n"
"d483G1FLzvf4DhlcY10+UE84RVC1fE5qAiEAwSCWumRl4NxMomC62aEha1uF2Qde\n"
"Y29OtVhCCXl0z2sAdwCWl2S/VViXrfdDh2g3CEJ36fA61fak8zZuRqQ/D8qpxgAA\n"
"AZpNpmkcAAAEAwBIMEYCIQDIACcx4k0sAd+eQAbQFUPpOj/PEV2gJZSKtygTPXw9\n"
"QAIhANcSU7EAUi6melXnAg8XDoDpu3mebmYYomL7JKcEq/fWMAoGCCqGSM49BAMC\n"
"A0gAMEUCIQCyZT67c4SDLY7zBVSGXwB5v/F1s+BD8Sic9WSUPA6jbgIgNroP7rZj\n"
"rr5/RNJpbO2mVKXcn47jGWnOPDDB9o3xo7c=\n"
"-----END CERTIFICATE-----\n"
// Intermediate certificate
"-----BEGIN CERTIFICATE-----\n"
"MIICnzCCAiWgAwIBAgIQf/MZd5csIkp2FV0TttaF4zAKBggqhkjOPQQDAzBHMQsw\n"
"CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n"
"MBIGA1UEAxMLR1RTIFJvb3QgUjQwHhcNMjMxMjEzMDkwMDAwWhcNMjkwMjIwMTQw\n"
"MDAwWjA7MQswCQYDVQQGEwJVUzEeMBwGA1UEChMVR29vZ2xlIFRydXN0IFNlcnZp\n"
"Y2VzMQwwCgYDVQQDEwNXRTEwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNCAARvzTr+\n"
"Z1dHTCEDhUDCR127WEcPQMFcF4XGGTfn1XzthkubgdnXGhOlCgP4mMTG6J7/EFmP\n"
"LCaY9eYmJbsPAvpWo4H+MIH7MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggr\n"
"BgEFBQcDAQYIKwYBBQUHAwIwEgYDVR0TAQH/BAgwBgEB/wIBADAdBgNVHQ4EFgQU\n"
"kHeSNWfE/6jMqeZ72YB5e8yT+TgwHwYDVR0jBBgwFoAUgEzW63T/STaj1dj8tT7F\n"
"avCUHYwwNAYIKwYBBQUHAQEEKDAmMCQGCCsGAQUFBzAChhhodHRwOi8vaS5wa2ku\n"
"Z29vZy9yNC5jcnQwKwYDVR0fBCQwIjAgoB6gHIYaaHR0cDovL2MucGtpLmdvb2cv\n"
"ci9yNC5jcmwwEwYDVR0gBAwwCjAIBgZngQwBAgEwCgYIKoZIzj0EAwMDaAAwZQIx\n"
"AOcCq1HW90OVznX+0RGU1cxAQXomvtgM8zItPZCuFQ8jSBJSjz5keROv9aYsAm5V\n"
"sQIwJonMaAFi54mrfhfoFNZEfuNMSQ6/bIBiNLiyoX46FohQvKeIoJ99cx7sUkFN\n"
"7uJW\n"
"-----END CERTIFICATE-----\n";

// ========== COMPACT CONFIG ==========
// WiFi credentials array
typedef struct {
    const char *ssid;
    const char *password;
} wifi_cred_t;

// Structure for available network scan results
typedef struct {
    wifi_cred_t *network;
    wifi_ap_record_t ap_info;
} available_network_t;

wifi_cred_t wifi_networks[] = {
    {"TAMNET SYSTEMS", "Tamnet123"},
    {"ztech", "112345678"}
};

#define WIFI_NETWORKS_COUNT (sizeof(wifi_networks) / sizeof(wifi_cred_t))
#define WIFI_TIMEOUT_MS 15000  // 15 seconds to connect to each network
#define SUPABASE_URL "https://konuwipzeywfgroqszzz.supabase.co/rest/v1/sensor_data"
#define SUPABASE_KEY "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6ImtvbnV3aXB6ZXl3Zmdyb3Fzenp6Iiwicm9sZSI6ImFub24iLCJpYXQiOjE3NTk4MzY4MzQsImV4cCI6MjA3NTQxMjgzNH0.i-Ru1I5Y8QAve9MiD64pKQ3yereW0022sbRj-xlxjWY"

// ========== PIN CONFIG ==========
// Digital Sensors
#define DHT_PIN GPIO_NUM_4                    // DHT22 for air temp & humidity
#define WATER_TEMP_PIN GPIO_NUM_5             // DS18B20 water temperature sensor
#define PUMP_PIN GPIO_NUM_14                  // DC pump control

// Note: Analog sensor channels are defined in adc_config.h

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

// ========== Event Groups ==========
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

    ESP_LOGI(TAG, "Scanning for available WiFi networks...");

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
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    // Set WiFi mode and start
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Wait for WiFi to start properly
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Scan for available networks first
    uint16_t ap_count = 0;

    // Perform WiFi scan with retries
    wifi_scan_config_t scan_config = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = {
                .min = 100,
                .max = 500
            }
        }
    };

    // Try scanning multiple times
    for (int retry = 0; retry < 3; retry++) {
        ESP_LOGI(TAG, "WiFi scan attempt %d/3", retry + 1);
        esp_err_t scan_result = esp_wifi_scan_start(&scan_config, true);
        if (scan_result == ESP_OK) {
            esp_wifi_scan_get_ap_num(&ap_count);
            ESP_LOGI(TAG, "Found %d access points", ap_count);
            if (ap_count > 0) break;
        } else {
            ESP_LOGE(TAG, "WiFi scan failed: %s", esp_err_to_name(scan_result));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    if (ap_count == 0) {
        ESP_LOGW(TAG, "No WiFi networks found after 3 attempts");
        ESP_LOGW(TAG, "Continuing without WiFi connection");
    } else {
        wifi_ap_record_t ap_records[ap_count];
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));

        // Log all found networks for debugging
        ESP_LOGI(TAG, "Available WiFi networks:");
        for (int i = 0; i < ap_count && i < 10; i++) {
            ESP_LOGI(TAG, "  %d: %s (RSSI: %d)", i+1, ap_records[i].ssid, ap_records[i].rssi);
        }

        // Find our configured networks among the scan results
        available_network_t available_networks[WIFI_NETWORKS_COUNT];
        int available_count = 0;

        for (int i = 0; i < ap_count; i++) {
            for (int j = 0; j < WIFI_NETWORKS_COUNT; j++) {
                if (strcmp((char*)ap_records[i].ssid, wifi_networks[j].ssid) == 0) {
                    // Found one of our networks
                    available_networks[available_count].network = &wifi_networks[j];
                    memcpy(&available_networks[available_count].ap_info, &ap_records[i], sizeof(wifi_ap_record_t));
                    available_count++;
                    ESP_LOGI(TAG, "Network '%s' found with RSSI: %d", ap_records[i].ssid, ap_records[i].rssi);
                    break;
                }
            }
        }

        ESP_LOGI(TAG, "Found %d of our configured networks available", available_count);

        // Prioritize TAMNET SYSTEMS if available, use strongest signal for others
        if (available_count > 1) {
            // Check if TAMNET SYSTEMS is available
            for (int i = 0; i < available_count; i++) {
                if (strcmp(available_networks[i].network->ssid, "TAMNET SYSTEMS") == 0) {
                    // Move TAMNET SYSTEMS to front (highest priority)
                    if (i > 0) {
                        available_network_t temp = available_networks[0];
                        available_networks[0] = available_networks[i];
                        available_networks[i] = temp;
                    }
                    break;
                }
            }
        }

        // Try connecting to networks in order (TAMNET SYSTEMS first, then by signal strength)
        for (int i = 0; i < available_count; i++) {
            ESP_LOGI(TAG, "Trying network '%s' (RSSI: %d dBm)",
                     available_networks[i].network->ssid, available_networks[i].ap_info.rssi);

            wifi_config_t wifi_config = {
                .sta = {
                    .ssid = {},
                    .password = {},
                    .threshold.authmode = WIFI_AUTH_WPA2_PSK,
                    .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
                },
            };
            strcpy((char*)wifi_config.sta.ssid, available_networks[i].network->ssid);
            strcpy((char*)wifi_config.sta.password, available_networks[i].network->password);

            esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
            esp_wifi_connect();

            // Wait for connection with timeout
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                    pdTRUE, pdFALSE, pdMS_TO_TICKS(WIFI_TIMEOUT_MS));

            if (bits & WIFI_CONNECTED_BIT) {
                ESP_LOGI(TAG, "Successfully connected to '%s' (RSSI: %d dBm)",
                         available_networks[i].network->ssid, available_networks[i].ap_info.rssi);
                return;
            } else {
                ESP_LOGW(TAG, "Failed to connect to '%s', trying next network...",
                         available_networks[i].network->ssid);
            }
        }

        ESP_LOGE(TAG, "Failed to connect to any available network");
    }

    ESP_LOGE(TAG, "WiFi initialization failed - no networks available");
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
    int sum_mv = 0;

    for (int i = 0; i < SAMPLES; i++) {
        int mv;
        ESP_ERROR_CHECK(read_adc_voltage(PH_ADC_CH, &mv));
        sum_mv += mv;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int avg_mv = sum_mv / SAMPLES;

    // pH calculation (adjust these values based on calibration)
    float ph = 7.0f + ((2500.0f - avg_mv) / 180.0f);
    return (ph >= 0.0f && ph <= 14.0f) ? ph : -1.0f;
}

// Dissolved Oxygen Sensor
static float read_do(void) {
    const int SAMPLES = 10;
    int sum_mv = 0;

    for (int i = 0; i < SAMPLES; i++) {
        int mv;
        ESP_ERROR_CHECK(read_adc_voltage(DO_ADC_CH, &mv));
        sum_mv += mv;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int avg_mv = sum_mv / SAMPLES;

    // DO calculation (adjust calibration values)
    float do_value = avg_mv * 0.2f; // Convert mV to mg/L (adjust factor based on calibration)
    return (do_value >= 0.0f && do_value <= 20.0f) ? do_value : -1.0f;
}

// Turbidity Sensor
static float read_turbidity(void) {
    const int SAMPLES = 10;
    int sum_mv = 0;

    for (int i = 0; i < SAMPLES; i++) {
        int mv;
        ESP_ERROR_CHECK(read_adc_voltage(TURBIDITY_ADC_CH, &mv));
        sum_mv += mv;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    int avg_mv = sum_mv / SAMPLES;

    // Turbidity calculation (adjust calibration values)
    float ntu = avg_mv * 0.5f; // Convert mV to NTU (adjust factor based on calibration)
    return (ntu >= 0.0f && ntu <= 1000.0f) ? ntu : -1.0f;
}

// Ammonia Sensor
static float read_ammonia(void) {
    const int SAMPLES = 10;
    int sum_mv = 0;

    for (int i = 0; i < SAMPLES; i++) {
        int mv;
        ESP_ERROR_CHECK(read_adc_voltage(AMMONIA_ADC_CH, &mv));
        sum_mv += mv;
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    int avg_mv = sum_mv / SAMPLES;

    // Ammonia calculation (adjust calibration values)
    float nh3 = avg_mv * 0.1f; // Convert mV to mg/L (adjust factor based on calibration)
    return (nh3 >= 0.0f && nh3 <= 10.0f) ? nh3 : -1.0f;
}

// DS18B20 timing constants (in microseconds)
#define DS18B20_RESET_PULSE 480
#define DS18B20_PRESENCE_WAIT 60
#define DS18B20_PRESENCE_PULSE 240
#define DS18B20_WRITE_0 60
#define DS18B20_WRITE_1 10
#define DS18B20_READ_SLOT 15
#define DS18B20_RECOVERY_TIME 1

// DS18B20 CRC lookup table
static const uint8_t ds18b20_crc_table[256] = {
    0x00, 0x5E, 0xBC, 0xE2, 0x61, 0x3F, 0xDD, 0x83,
    0xC2, 0x9C, 0x7E, 0x20, 0xA3, 0xFD, 0x1F, 0x41,
    0x9D, 0xC3, 0x21, 0x7F, 0xFC, 0xA2, 0x40, 0x1E,
    0x5F, 0x01, 0xE3, 0xBD, 0x3E, 0x60, 0x82, 0xDC,
    0x23, 0x7D, 0x9F, 0xC1, 0x42, 0x1C, 0xFE, 0xA0,
    0xE1, 0xBF, 0x5D, 0x03, 0x80, 0xDE, 0x3C, 0x62,
    0xBE, 0xE0, 0x02, 0x5C, 0xDF, 0x81, 0x63, 0x3D,
    0x7C, 0x22, 0xC0, 0x9E, 0x1D, 0x43, 0xA1, 0xFF,
    0x46, 0x18, 0xFA, 0xA4, 0x27, 0x79, 0x9B, 0xC5,
    0x84, 0xDA, 0x38, 0x66, 0xE5, 0xBB, 0x59, 0x07,
    0xDB, 0x85, 0x67, 0x39, 0xBA, 0xE4, 0x06, 0x58,
    0x19, 0x47, 0xA5, 0xFB, 0x78, 0x26, 0xC4, 0x9A,
    0x65, 0x3B, 0xD9, 0x87, 0x04, 0x5A, 0xB8, 0xE6,
    0xA7, 0xF9, 0x1B, 0x45, 0xC6, 0x98, 0x7A, 0x24,
    0xF8, 0xA6, 0x44, 0x1A, 0x99, 0xC7, 0x25, 0x7B,
    0x3A, 0x64, 0x86, 0xD8, 0x5B, 0x05, 0xE7, 0xB9,
    0x8C, 0xD2, 0x30, 0x6E, 0xED, 0xB3, 0x51, 0x0F,
    0x4E, 0x10, 0xF2, 0xAC, 0x2F, 0x71, 0x93, 0xCD,
    0x11, 0x4F, 0xAD, 0xF3, 0x70, 0x2E, 0xCC, 0x92,
    0xD3, 0x8D, 0x6F, 0x31, 0xB2, 0xEC, 0x0E, 0x50,
    0xAF, 0xF1, 0x13, 0x4D, 0xCE, 0x90, 0x72, 0x2C,
    0x6D, 0x33, 0xD1, 0x8F, 0x0C, 0x52, 0xB0, 0xEE,
    0x32, 0x6C, 0x8E, 0xD0, 0x53, 0x0D, 0xEF, 0xB1,
    0xF0, 0xAE, 0x4C, 0x12, 0x91, 0xCF, 0x2D, 0x73,
    0xCA, 0x94, 0x76, 0x28, 0xAB, 0xF5, 0x17, 0x49,
    0x08, 0x56, 0xB4, 0xEA, 0x69, 0x37, 0xD5, 0x8B,
    0x57, 0x09, 0xEB, 0xB5, 0x36, 0x68, 0x8A, 0xD4,
    0x95, 0xCB, 0x29, 0x77, 0xF4, 0xAA, 0x48, 0x16,
    0xE9, 0xB7, 0x55, 0x0B, 0x88, 0xD6, 0x34, 0x6A,
    0x2B, 0x75, 0x97, 0xC9, 0x4A, 0x14, 0xF6, 0xA8,
    0x74, 0x2A, 0xC8, 0x96, 0x15, 0x4B, 0xA9, 0xF7,
    0xB6, 0xE8, 0x0A, 0x54, 0xD7, 0x89, 0x6B, 0x35
};

// Helper functions for DS18B20 1-wire protocol
static void write_ds18b20_bit(int gpio_pin, int bit) {
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_pin, 0);
    ets_delay_us(bit ? DS18B20_WRITE_1 : DS18B20_WRITE_0);
    gpio_set_level(gpio_pin, 1);
    ets_delay_us(DS18B20_RECOVERY_TIME);
}

static int read_ds18b20_bit(int gpio_pin) {
    gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
    gpio_set_level(gpio_pin, 0);
    ets_delay_us(DS18B20_READ_SLOT);
    gpio_set_level(gpio_pin, 1);
    gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
    ets_delay_us(DS18B20_READ_SLOT / 2);
    int bit = gpio_get_level(gpio_pin);
    ets_delay_us(DS18B20_READ_SLOT / 2);
    return bit;
}

// Water Temperature (DS18B20) - 1-Wire Protocol Implementation
static float read_water_temp(void) {
    uint8_t data[9] = {0};
    int64_t start_time;

    // Initialize GPIO for 1-wire
    gpio_set_direction(WATER_TEMP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(WATER_TEMP_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10)); // Stabilization delay

    // Reset pulse
    gpio_set_level(WATER_TEMP_PIN, 0);
    ets_delay_us(DS18B20_RESET_PULSE);
    gpio_set_level(WATER_TEMP_PIN, 1);
    gpio_set_direction(WATER_TEMP_PIN, GPIO_MODE_INPUT);

    // Wait for presence pulse
    start_time = esp_timer_get_time();
    while (gpio_get_level(WATER_TEMP_PIN) == 1) {
        if (esp_timer_get_time() - start_time > DS18B20_PRESENCE_WAIT) {
            ESP_LOGE(TAG, "DS18B20: No presence pulse detected on GPIO %d", WATER_TEMP_PIN);
            return -999.0f;
        }
    }

    start_time = esp_timer_get_time();
    while (gpio_get_level(WATER_TEMP_PIN) == 0) {
        if (esp_timer_get_time() - start_time > DS18B20_PRESENCE_PULSE) {
            ESP_LOGE(TAG, "DS18B20: Presence pulse timeout on GPIO %d", WATER_TEMP_PIN);
            return -999.0f;
        }
    }

    // Send SKIP ROM command (0xCC)
    for (int i = 0; i < 8; i++) {
        write_ds18b20_bit(WATER_TEMP_PIN, (0xCC >> i) & 1);
    }

    // Send CONVERT T command (0x44)
    for (int i = 0; i < 8; i++) {
        write_ds18b20_bit(WATER_TEMP_PIN, (0x44 >> i) & 1);
    }

    // Wait for conversion (750ms max)
    vTaskDelay(pdMS_TO_TICKS(750));

    // Reset and presence again
    gpio_set_direction(WATER_TEMP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(WATER_TEMP_PIN, 0);
    ets_delay_us(DS18B20_RESET_PULSE);
    gpio_set_level(WATER_TEMP_PIN, 1);
    gpio_set_direction(WATER_TEMP_PIN, GPIO_MODE_INPUT);

    start_time = esp_timer_get_time();
    while (gpio_get_level(WATER_TEMP_PIN) == 1) {
        if (esp_timer_get_time() - start_time > DS18B20_PRESENCE_WAIT) {
            ESP_LOGE(TAG, "DS18B20: No presence pulse after conversion on GPIO %d", WATER_TEMP_PIN);
            return -999.0f;
        }
    }

    // Send SKIP ROM again
    for (int i = 0; i < 8; i++) {
        write_ds18b20_bit(WATER_TEMP_PIN, (0xCC >> i) & 1);
    }

    // Send READ SCRATCHPAD command (0xBE)
    for (int i = 0; i < 8; i++) {
        write_ds18b20_bit(WATER_TEMP_PIN, (0xBE >> i) & 1);
    }

    // Read 9 bytes of scratchpad
    for (int byte = 0; byte < 9; byte++) {
        for (int bit = 0; bit < 8; bit++) {
            data[byte] |= (read_ds18b20_bit(WATER_TEMP_PIN) << bit);
        }
    }

    // Verify CRC
    uint8_t crc = 0;
    for (int i = 0; i < 8; i++) {
        crc = ds18b20_crc_table[crc ^ data[i]];
    }
    if (crc != data[8]) {
        ESP_LOGE(TAG, "DS18B20: CRC check failed on GPIO %d", WATER_TEMP_PIN);
        return -999.0f;
    }

    // Convert temperature
    int16_t raw_temp = (data[1] << 8) | data[0];
    float temperature = raw_temp / 16.0f;

    if (temperature < -55.0f || temperature > 125.0f) {
        ESP_LOGE(TAG, "DS18B20: Invalid temperature reading: %.2f°C on GPIO %d", temperature, WATER_TEMP_PIN);
        return -999.0f;
    }

    ESP_LOGI(TAG, "DS18B20: Raw data: %02X %02X %02X %02X %02X %02X %02X %02X %02X",
             data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8]);
    return temperature;
}

// Global HTTP client for reuse
static esp_http_client_handle_t supabase_client = NULL;

// Initialize HTTP client once during startup
static void init_supabase_client(void) {
    esp_http_client_config_t config = {
        .url = SUPABASE_URL,
        .method = HTTP_METHOD_POST,
        .timeout_ms = 15000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .cert_pem = supabase_cert_chain,
        .skip_cert_common_name_check = false,
        .buffer_size = 2048,
        .buffer_size_tx = 1024
    };

    if (supabase_client) {
        esp_http_client_cleanup(supabase_client);
    }

    supabase_client = esp_http_client_init(&config);
    if (!supabase_client) {
        ESP_LOGE(TAG, "[SUPABASE] Failed to initialize HTTP client");
        return;
    }

    // Use the global CA store - no need to load additional certificates
    ESP_LOGI(TAG, "[SUPABASE] HTTP client initialized with certificate bundle");

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

    // Check WiFi connection status directly
    wifi_ap_record_t ap_info;
    esp_err_t wifi_status = esp_wifi_sta_get_ap_info(&ap_info);
    
    if (wifi_status != ESP_OK) {
        ESP_LOGW(TAG, "WiFi not connected, attempting reconnection...");
        
        // Try to reconnect
        for (int attempt = 0; attempt < 3; attempt++) {
            esp_wifi_connect();
            vTaskDelay(pdMS_TO_TICKS(5000));
            
            wifi_status = esp_wifi_sta_get_ap_info(&ap_info);
            if (wifi_status == ESP_OK) {
                ESP_LOGI(TAG, "WiFi reconnected successfully");
                break;
            }
            ESP_LOGW(TAG, "Reconnection attempt %d/3 failed", attempt + 1);
        }
        
        if (wifi_status != ESP_OK) {
            ESP_LOGE(TAG, "WiFi connection failed after 3 attempts");
            return false;
        }
    }

    // Validate all sensor values (allow -999.0f for sensor errors)
    if ((air_temp < -40 && air_temp != -999.0f) || air_temp > 80 ||
        (water_temp < -40 && water_temp != -999.0f) || water_temp > 80 ||
        (hum < 0 && hum != -999.0f) || hum > 100 ||
        (ph < 0 && ph != -999.0f) || ph > 14 ||
        (do_level < 0 && do_level != -999.0f) || do_level > 20 ||
        (turbidity < 0 && turbidity != -999.0f) || turbidity > 1000 ||
        (ammonia < 0 && ammonia != -999.0f) || ammonia > 10) {
        ESP_LOGE(TAG, "Invalid sensor values detected");
        return false;
    }

    // Create JSON in smaller chunks to reduce stack usage
    char json[512];  // Increased buffer size for conditional fields
    char temp[128];  // Larger temporary buffer

    strcpy(json, "{");

    // Always include air temperature and humidity (DHT22 is connected)
    snprintf(temp, sizeof(temp), "\"air_temperature\":%.2f,\"humidity\":%.2f", air_temp, hum);
    strcat(json, temp);

    // Add water temperature only if sensor is connected
    if (water_temp != -999.0f) {
        snprintf(temp, sizeof(temp), ",\"water_temperature\":%.2f", water_temp);
        strcat(json, temp);
    } else {
        ESP_LOGE(TAG, "🔥🔥🔥 CRITICAL: DS18B20 WATER TEMPERATURE SENSOR NOT CONNECTED! 🔥🔥🔥");
        ESP_LOGE(TAG, "💀💀💀 FIX THIS IMMEDIATELY! CONNECT DS18B20 TO GPIO 5! 💀💀💀");
        ESP_LOGE(TAG, "⚠️⚠️⚠️ AQUACULTURE SYSTEM INCOMPLETE WITHOUT WATER TEMP! ⚠️⚠️⚠️");
    }

    // Add pH only if sensor is connected
    if (ph != -999.0f) {
        snprintf(temp, sizeof(temp), ",\"ph\":%.2f", ph);
        strcat(json, temp);
    } else {
        ESP_LOGE(TAG, "🔥🔥🔥 CRITICAL: pH SENSOR NOT CONNECTED! 🔥🔥🔥");
        ESP_LOGE(TAG, "💀💀💀 FIX THIS IMMEDIATELY! CONNECT pH SENSOR TO GPIO 6! 💀💀💀");
        ESP_LOGE(TAG, "⚠️⚠️⚠️ pH MONITORING IS ESSENTIAL FOR FISH SURVIVAL! ⚠️⚠️⚠️");
    }

    // Add dissolved oxygen only if sensor is connected (but don't count as critical)
    if (do_level != -999.0f) {
        snprintf(temp, sizeof(temp), ",\"dissolved_oxygen\":%.2f", do_level);
        strcat(json, temp);
    }

    // Add turbidity only if sensor is connected
    if (turbidity != -999.0f) {
        snprintf(temp, sizeof(temp), ",\"turbidity\":%.2f", turbidity);
        strcat(json, temp);
    } else {
        ESP_LOGE(TAG, "🔥🔥🔥 CRITICAL: TURBIDITY SENSOR NOT CONNECTED! 🔥🔥🔥");
        ESP_LOGE(TAG, "💀💀💀 FIX THIS IMMEDIATELY! CONNECT TURBIDITY SENSOR TO GPIO 8! 💀💀💀");
        ESP_LOGE(TAG, "⚠️⚠️⚠️ WATER QUALITY MONITORING IS MANDATORY! ⚠️⚠️⚠️");
    }

    // Add ammonia only if sensor is connected (but don't count in critical checklist)
    if (ammonia != -999.0f) {
        snprintf(temp, sizeof(temp), ",\"ammonia\":%.2f", ammonia);
        strcat(json, temp);
    }

    // Always include control states
    snprintf(temp, sizeof(temp),
        ",\"ph_relay\":%s,\"aerator\":%s,\"filter\":%s,\"pump\":%s}",
        ph_relay ? "true" : "false",
        aerator ? "true" : "false",
        filter ? "true" : "false",
        pump ? "true" : "false");
    strcat(json, temp);

    // Count missing CRITICAL sensors (excluding ammonia and dissolved oxygen)
    int missing_sensors = 0;
    if (water_temp == -999.0f) missing_sensors++;
    if (ph == -999.0f) missing_sensors++;
    if (turbidity == -999.0f) missing_sensors++;

    if (missing_sensors > 0) {
        ESP_LOGE(TAG, "");
        ESP_LOGE(TAG, "🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨");
        ESP_LOGE(TAG, "💥💥💥 SYSTEM FAILURE: %d OUT OF 3 CRITICAL SENSORS MISSING! 💥💥💥", missing_sensors);
        ESP_LOGE(TAG, "🔥🔥🔥 THIS IS NOT A COMPLETE AQUACULTURE SYSTEM! 🔥🔥🔥");
        ESP_LOGE(TAG, "💀💀💀 FISH WILL DIE! GET TO WORK AND FIX THIS NOW! 💀💀💀");
        ESP_LOGE(TAG, "⚡⚡⚡ NO EXCUSES! NO DELAYS! FIX IT IMMEDIATELY! ⚡⚡⚡");
        ESP_LOGE(TAG, "🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨🚨");
        ESP_LOGE(TAG, "");
    }

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

        // Log response for debugging
        if (status_code == 400) {
            int content_length = esp_http_client_get_content_length(supabase_client);
            if (content_length > 0 && content_length < 512) {
                char response_buffer[512];
                int data_read = esp_http_client_read_response(supabase_client, response_buffer, sizeof(response_buffer) - 1);
                if (data_read > 0) {
                    response_buffer[data_read] = '\0';
                    ESP_LOGE(TAG, "[SUPABASE] 400 Error Response: %s", response_buffer);
                }
            }
        }

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

    // All retries failed
    ESP_LOGE(TAG, "[SUPABASE] All %d attempts failed", MAX_RETRIES);
    return false;
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
        .timeout_ms = 10000,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,
        .skip_cert_common_name_check = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .buffer_size = 2048,
        .buffer_size_tx = 1024,
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
        // Split sensor values formatting to avoid buffer overflow
        snprintf(temp, sizeof(temp),
                "\"water_temp\":%.2f,\"do_level\":%.2f,\"ph\":%.2f}",
                water_temp, do_level, ph);
        strcat(json, temp);

        snprintf(temp, sizeof(temp),
                ",\"ammonia\":%.2f,\"turbidity\":%.2f}}",
                ammonia, turbidity);
        strcat(json, temp);

        if (!alert_client) {
            init_alert_client();
            if (!alert_client) {
                return;
            }
        }

        if (alert_client) {
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

    // Initialize ADC
    ESP_LOGI(TAG, "Initializing ADC...");
    ESP_ERROR_CHECK(init_adc());

    // Configure watchdog timer
    ESP_LOGI(TAG, "Configuring watchdog timer...");

    // Delete the old watchdog timer first
    esp_task_wdt_deinit();

    // Configure new watchdog timer
    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 30000,              // 30 second timeout
        .trigger_panic = false            // Don't panic on timeout
    };
    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));    // Add idle task

    ESP_LOGI(TAG, "Initializing global CA store...");
    esp_tls_init_global_ca_store();

    // Connect to WiFi
    ESP_LOGI(TAG, "Connecting to WiFi (trying %d networks)...", WIFI_NETWORKS_COUNT);
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
    ESP_ERROR_CHECK(init_adc());

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
            ESP_LOGE(TAG, "DHT22 READ FAILED - Sensor not responding (GPIO %d)", DHT_PIN);
            ESP_LOGE(TAG, "Error: %s", esp_err_to_name(dht_result));
            air_temp = -999.0f; // Error indicator
            hum = -999.0f;      // Error indicator
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
            ESP_LOGE(TAG, "pH sensor error - ADC channel %d (GPIO %d) reading failed", PH_ADC_CH, 6);
            ESP_LOGE(TAG, "Check sensor connection, power supply, and calibration");
            ph = -999.0f; // Error indicator
        } else {
            ESP_LOGI(TAG, "pH: %.2f (connected and working)", ph);
        }
        esp_task_wdt_reset();

        // Read Dissolved Oxygen
        ESP_LOGI(TAG, "Reading dissolved oxygen...");
        do_level = read_do();
        if (do_level < 0) {
            ESP_LOGE(TAG, "DO sensor error - ADC channel %d (GPIO %d) reading failed", DO_ADC_CH, 3);
            ESP_LOGE(TAG, "Sensor not connected yet - will be available when DO sensor is added");
            do_level = -999.0f; // Error indicator
        } else {
            ESP_LOGI(TAG, "DO: %.2f mg/L (connected and working)", do_level);
        }
        esp_task_wdt_reset();

        // Read Turbidity
        ESP_LOGI(TAG, "Reading turbidity...");
        turbidity = read_turbidity();
        if (turbidity < 0) {
            ESP_LOGE(TAG, "Turbidity sensor error - ADC channel %d (GPIO %d) reading failed", TURBIDITY_ADC_CH, 8);
            ESP_LOGE(TAG, "Check sensor connection, power supply, and calibration");
            turbidity = -999.0f; // Error indicator
        } else {
            ESP_LOGI(TAG, "Turbidity: %.2f NTU (connected and working)", turbidity);
        }
        esp_task_wdt_reset();

        // Read Ammonia
        ESP_LOGI(TAG, "Reading ammonia...");
        ammonia = read_ammonia();
        if (ammonia < 0) {
            ESP_LOGE(TAG, "Ammonia sensor error - ADC channel %d (GPIO %d) reading failed", AMMONIA_ADC_CH, 1);
            ESP_LOGE(TAG, "Sensor not connected yet - will be available when ammonia sensor is added");
            ammonia = -999.0f; // Error indicator
        } else {
            ESP_LOGI(TAG, "Ammonia: %.2f mg/L (connected and working)", ammonia);
        }
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
        // Skip alert checking - focus only on data sending

        esp_task_wdt_reset(); // Feed the watchdog after Supabase upload

        ESP_LOGI(TAG, "Cycle #%d complete. Sleeping %d seconds...",
                 cycle_count, SAMPLE_DELAY_MS/1000);

        // Use the watchdog-friendly delay instead of a single long vTaskDelay
        watchdog_friendly_delay(SAMPLE_DELAY_MS);
    }
}
