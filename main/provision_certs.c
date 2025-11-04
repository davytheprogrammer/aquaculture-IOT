#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "cert_manager.h"

static const char *TAG = "provision_certs";

static esp_err_t read_file_to_buffer(const char* filepath, char* buffer, size_t* length) {
    FILE* f = fopen(filepath, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file: %s", filepath);
        return ESP_FAIL;
    }

    size_t bytes_read = fread(buffer, 1, *length, f);
    fclose(f);

    if (bytes_read == 0) {
        ESP_LOGE(TAG, "Failed to read file or file is empty");
        return ESP_FAIL;
    }

    *length = bytes_read;
    buffer[bytes_read] = '\0';  // Null terminate the string
    return ESP_OK;
}

esp_err_t provision_certificates(void) {
    char cert_buffer[4096];  // Buffer for certificate data
    size_t cert_len;
    esp_err_t ret;

    // Initialize the certificate manager
    ret = init_cert_manager();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize certificate manager");
        return ret;
    }

    // Load and store server certificate
    cert_len = sizeof(cert_buffer);
    ret = read_file_to_buffer("../certificates/server_cert.pem", cert_buffer, &cert_len);
    if (ret == ESP_OK) {
        ret = store_certificate("server_cert", cert_buffer, cert_len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to store server certificate");
            return ret;
        }
    }

    // Load and store root certificate
    cert_len = sizeof(cert_buffer);
    ret = read_file_to_buffer("../certificates/isrg_root_x1.pem", cert_buffer, &cert_len);
    if (ret == ESP_OK) {
        ret = store_certificate("root_cert", cert_buffer, cert_len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to store root certificate");
            return ret;
        }
    }

    // Load and store Supabase certificate from cert.h
    cert_len = sizeof(cert_buffer);
    ret = read_file_to_buffer("../certificates/cert.h", cert_buffer, &cert_len);
    if (ret == ESP_OK) {
        // Extract the actual certificate content from the cert.h file
        char* cert_start = strstr(cert_buffer, "-----BEGIN CERTIFICATE-----");
        char* cert_end = strstr(cert_buffer, "-----END CERTIFICATE-----");
        if (cert_start && cert_end) {
            cert_end += strlen("-----END CERTIFICATE-----");
            size_t actual_cert_len = cert_end - cert_start;
            ret = store_certificate("supabase_cert", cert_start, actual_cert_len);
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to store Supabase certificate");
                return ret;
            }
        }
    }

    ESP_LOGI(TAG, "All certificates provisioned successfully");
    return ESP_OK;
}
