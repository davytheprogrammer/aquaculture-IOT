# ESP32 Sensor Fix Checklist

## CRITICAL ISSUES TO FIX NOW:

### 1. DS18B20 Water Temperature Sensor (GPIO 5)
**Status: DISCONNECTED** ❌
```
DS18B20 → ESP32-S3
VCC     → 3.3V
GND     → GND  
DATA    → GPIO 5 + 4.7kΩ resistor to 3.3V
```

### 2. DHT22 Air Temp/Humidity (GPIO 4)  
**Status: DISCONNECTED** ❌
```
DHT22   → ESP32-S3
VCC     → 3.3V
GND     → GND
DATA    → GPIO 4
```

### 3. pH Sensor (ADC Channel 0)
**Status: DISCONNECTED** ❌
```
pH Sensor → ESP32-S3
VCC       → 3.3V
GND       → GND
SIGNAL    → A0 (ADC1_CH0)
```

## WORKING SENSORS:
✅ Turbidity: 150.00 NTU (connected to ADC)

## QUICK FIX STEPS:
1. Power off ESP32
2. Connect DS18B20 to GPIO 5 with pull-up resistor
3. Connect DHT22 to GPIO 4  
4. Connect pH sensor to A0
5. Power on and test

## TEST COMMAND:
```bash
tail -f /tmp/esp32_output.log | grep -E "Water Temp|pH|Humidity"
```
