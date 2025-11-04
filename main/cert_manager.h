#ifndef CERT_MANAGER_H
#define CERT_MANAGER_H

#include "esp_err.h"

/**
 * @brief Initialize the certificate manager
 * @return ESP_OK on success
 */
esp_err_t init_cert_manager(void);

/**
 * @brief Store a certificate in secure storage
 * @param key Unique identifier for the certificate
 * @param cert_data Certificate data
 * @param cert_len Length of certificate data
 * @return ESP_OK on success
 */
esp_err_t store_certificate(const char* key, const char* cert_data, size_t cert_len);

/**
 * @brief Retrieve a certificate from secure storage
 * @param key Unique identifier for the certificate
 * @param cert_data Buffer to store certificate data
 * @param cert_len On input, size of buffer; on output, actual size of certificate
 * @return ESP_OK on success
 */
esp_err_t get_certificate(const char* key, char* cert_data, size_t* cert_len);

#endif // CERT_MANAGER_H
