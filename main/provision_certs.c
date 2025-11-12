#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "cert_manager.h"

static const char *TAG = "provision_certs";

esp_err_t provision_certificates(void) {
    ESP_LOGI(TAG, "Using embedded certificates");
    
    // Declare external embedded certificate data
    extern const uint8_t server_cert_pem_start[] asm("_binary_server_cert_pem_start");
    extern const uint8_t server_cert_pem_end[]   asm("_binary_server_cert_pem_end");
    extern const uint8_t isrg_root_x1_pem_start[] asm("_binary_isrg_root_x1_pem_start");
    extern const uint8_t isrg_root_x1_pem_end[]   asm("_binary_isrg_root_x1_pem_end");
    
    // Initialize the certificate manager
    esp_err_t ret = init_cert_manager();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize certificate manager");
        return ret;
    }
    
    // Store server certificate
    size_t server_cert_len = server_cert_pem_end - server_cert_pem_start;
    ret = store_certificate("server_cert", (const char*)server_cert_pem_start, server_cert_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store server certificate");
        return ret;
    }
    
    // Store root certificate
    size_t root_cert_len = isrg_root_x1_pem_end - isrg_root_x1_pem_start;
    ret = store_certificate("root_cert", (const char*)isrg_root_x1_pem_start, root_cert_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store root certificate");
        return ret;
    }
    
    ESP_LOGI(TAG, "All certificates provisioned successfully");
    return ESP_OK;
}
