#!/bin/bash

echo "🔧 ESP32 Sensor Connection Test"
echo "================================"

# Kill existing screen session
screen -S esp32 -X quit 2>/dev/null

# Start fresh monitoring
echo "📡 Starting fresh monitor session..."
screen -dmS esp32 -L -Logfile /tmp/esp32_test.log /dev/ttyACM0 115200

sleep 3

echo "🔍 Checking sensor readings..."
timeout 30 tail -f /tmp/esp32_test.log | while read line; do
    if echo "$line" | grep -q "Water Temp:"; then
        temp=$(echo "$line" | grep -o '\-\?[0-9]\+\.[0-9]\+')
        if [ "$temp" != "-999.0" ]; then
            echo "✅ Water Temperature: ${temp}°C - WORKING!"
        else
            echo "❌ Water Temperature: DISCONNECTED"
        fi
    fi
    
    if echo "$line" | grep -q "pH:"; then
        ph=$(echo "$line" | grep -o '[0-9]\+\.[0-9]\+')
        if [ "$ph" != "-999.0" ]; then
            echo "✅ pH Sensor: ${ph} - WORKING!"
        else
            echo "❌ pH Sensor: DISCONNECTED"
        fi
    fi
    
    if echo "$line" | grep -q "Humidity:"; then
        humidity=$(echo "$line" | grep -o '[0-9]\+\.[0-9]\+')
        if [ "$humidity" != "-999.0" ]; then
            echo "✅ Humidity: ${humidity}% - WORKING!"
        else
            echo "❌ Humidity: DISCONNECTED"
        fi
    fi
done
