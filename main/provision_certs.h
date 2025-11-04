#ifndef PROVISION_CERTS_H
#define PROVISION_CERTS_H

#include "esp_err.h"

/**
 * @brief Load certificates from files and store them in secure storage
 *
 * This function reads certificates from the ../certificates directory
 * and stores them securely using the certificate manager.
 *
 * @return ESP_OK on success
 */
esp_err_t provision_certificates(void);

#endif // PROVISION_CERTS_H
