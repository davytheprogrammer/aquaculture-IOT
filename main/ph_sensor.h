#ifndef PH_SENSOR_H
#define PH_SENSOR_H

#include "adc_handler.h"

static float read_ph(void) {
    int voltage;
    esp_err_t ret = read_adc_voltage(PH_ADC_CH, &voltage);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error reading pH ADC: %d", ret);
        return -1.0f;
    }

    // Convert voltage to pH value
    // Assuming pH sensor gives 59.16mV per pH unit change from pH 7
    // pH 7 corresponds to approximately 2.5V
    float ph = 7.0f + ((2500 - voltage) / 59.16f);

    // Validate pH range
    return (ph >= 0.0f && ph <= 14.0f) ? ph : -1.0f;
}

#endif // PH_SENSOR_H
