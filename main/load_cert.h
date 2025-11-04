#include "esp_http_client.h"
#include "esp_log.h"

static const char *CERT_TAG = "cert_loader";

static esp_err_t load_cert_to_client(esp_http_client_handle_t client) {
    char cert_buffer[4096];
    size_t cert_len = sizeof(cert_buffer);

    esp_err_t ret = get_certificate("supabase_cert", cert_buffer, &cert_len);
    if (ret != ESP_OK) {
        ESP_LOGE(CERT_TAG, "Failed to load certificate from secure storage");
        return ret;
    }

    ret = esp_tls_set_global_ca_store((unsigned char*)cert_buffer, cert_len);
    if (ret != ESP_OK) {
        ESP_LOGE(CERT_TAG, "Failed to set global CA store");
        return ret;
    }

    return ESP_OK;

    return ESP_OK;
}
