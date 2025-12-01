#!/bin/bash

LOG_FILE="/tmp/esp32_output.log"
ALERT_FILE="/tmp/esp32_alerts.log"

echo "🔍 ESP32 Aquaculture Monitor Started - $(date)"
echo "📊 Monitoring: $LOG_FILE"
echo "🚨 Alerts: $ALERT_FILE"
echo "----------------------------------------"

# Monitor log file for critical issues
tail -f "$LOG_FILE" | while read line; do
    timestamp=$(date '+%H:%M:%S')
    
    # Critical sensor failures
    if echo "$line" | grep -q "CRITICAL SENSORS MISSING"; then
        echo "🚨 [$timestamp] CRITICAL: Multiple sensors offline!" | tee -a "$ALERT_FILE"
    fi
    
    # Individual sensor failures
    if echo "$line" | grep -q "Water temperature sensor missing"; then
        echo "🌡️ [$timestamp] ALERT: Water temp sensor offline!" | tee -a "$ALERT_FILE"
    fi
    
    if echo "$line" | grep -q "pH sensor disconnected"; then
        echo "🧪 [$timestamp] ALERT: pH sensor offline!" | tee -a "$ALERT_FILE"
    fi
    
    # Successful data transmission
    if echo "$line" | grep -q "Data sent successfully"; then
        echo "✅ [$timestamp] Data transmitted to Supabase"
    fi
    
    # WiFi issues
    if echo "$line" | grep -q "WiFi connection failed"; then
        echo "📡 [$timestamp] WARNING: WiFi connection failed!" | tee -a "$ALERT_FILE"
    fi
    
    # Show sensor readings
    if echo "$line" | grep -q "Turbidity:"; then
        turbidity=$(echo "$line" | grep -o '[0-9]\+\.[0-9]\+')
        echo "💧 [$timestamp] Turbidity: ${turbidity} NTU"
    fi
    
    # Show current status
    echo "[$timestamp] $line"
done
