#include "cert_manager.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG = "cert_manager";

esp_err_t init_cert_manager(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    return ESP_OK;
}

esp_err_t store_certificate(const char* key, const char* cert_data, size_t cert_len) {
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ret = nvs_open("certificates", NVS_READWRITE, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle");
        return ret;
    }

    ret = nvs_set_blob(nvs_handle, key, cert_data, cert_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error storing certificate");
        nvs_close(nvs_handle);
        return ret;
    }

    ret = nvs_commit(nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error committing NVS");
    }

    nvs_close(nvs_handle);
    return ret;
}

esp_err_t get_certificate(const char* key, char* cert_data, size_t* cert_len) {
    nvs_handle_t nvs_handle;
    esp_err_t ret;

    ret = nvs_open("certificates", NVS_READONLY, &nvs_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle");
        return ret;
    }

    ret = nvs_get_blob(nvs_handle, key, cert_data, cert_len);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error reading certificate");
    }

    nvs_close(nvs_handle);
    return ret;
}
