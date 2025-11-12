#ifndef ADC_HANDLER_H
#define ADC_HANDLER_H

#include "adc_config.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

static const char *TAG = "ADC";
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle = NULL;
bool do_calibration = false;

esp_err_t init_adc() {
    // Delete any existing ADC unit first
    if (adc1_handle != NULL) {
        adc_oneshot_del_unit(adc1_handle);
        adc1_handle = NULL;
    }

    // Initialize ADC
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));

    // Configure ADC channels - match the channels defined in adc_config.h
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,  // Use DB_12 for ESP32-S3 (renamed from DB_11 in ESP-IDF v6.0)
    };

    // Configure all sensor ADC channels
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, PH_ADC_CH, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, DO_ADC_CH, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, TURBIDITY_ADC_CH, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, AMMONIA_ADC_CH, &config));

    // Skip ADC calibration for now - using raw values
    do_calibration = false;
    ESP_LOGW(TAG, "ADC Calibration Disabled - using raw values");

    return ESP_OK;
}

esp_err_t read_adc_voltage(int channel, int *voltage) {
    int adc_raw;
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &adc_raw));

    if (do_calibration) {
        ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, voltage));
    } else {
        *voltage = adc_raw;
    }

    return ESP_OK;
}

#endif // ADC_HANDLER_H
