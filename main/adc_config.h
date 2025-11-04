#ifndef ADC_CONFIG_H
#define ADC_CONFIG_H

// ADC channel definitions for ESP32-S3
#define ADC1_CHAN0          ADC_CHANNEL_5  // pH sensor on GPIO6
#define ADC1_CHAN1          ADC_CHANNEL_6  // DO sensor on GPIO7
#define ADC1_CHAN2          ADC_CHANNEL_7  // Turbidity sensor on GPIO8
#define ADC1_CHAN3          ADC_CHANNEL_8  // Ammonia sensor on GPIO9

// ADC channel aliases
#define PH_ADC_CH          ADC1_CHAN0
#define DO_ADC_CH          ADC1_CHAN1
#define TURBIDITY_ADC_CH   ADC1_CHAN2
#define AMMONIA_ADC_CH     ADC1_CHAN3

#endif // ADC_CONFIG_H
