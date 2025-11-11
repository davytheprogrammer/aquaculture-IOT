#ifndef ADC_CONFIG_H
#define ADC_CONFIG_H

// ADC channel definitions for ESP32-S3 - Updated for connected sensors
#define ADC1_CHAN0          ADC_CHANNEL_5  // pH sensor on GPIO6
#define ADC1_CHAN1          ADC_CHANNEL_7  // Turbidity sensor on GPIO8
#define ADC1_CHAN2          ADC_CHANNEL_2  // DO sensor on GPIO3 (not connected yet)
#define ADC1_CHAN3          ADC_CHANNEL_0  // Ammonia sensor on GPIO1 (not connected yet)

// ADC channel aliases
#define PH_ADC_CH          ADC1_CHAN0
#define TURBIDITY_ADC_CH   ADC1_CHAN1
#define DO_ADC_CH          ADC1_CHAN2
#define AMMONIA_ADC_CH     ADC1_CHAN3

#endif // ADC_CONFIG_H
