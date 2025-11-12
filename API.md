# ESP32-S3 Aquaculture IoT System - API Documentation

## Supabase REST API

### Endpoint Configuration
```
POST https://konuwipzeywfgroqszzz.supabase.co/rest/v1/sensor_data
```

### Authentication Headers
```http
Content-Type: application/json
apikey: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
Authorization: Bearer eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...
```

### Request Payload
```json
{
  "air_temperature": 25.6,
  "humidity": 65.2,
  "water_temperature": 24.8,
  "ph": 7.2,
  "turbidity": 15.3,
  "dissolved_oxygen": 8.5,
  "ammonia": 0.2,
  "ph_relay": false,
  "aerator": true,
  "filter": true,
  "pump": false
}
```

### Response Format
```json
{
  "id": 12345,
  "created_at": "2024-11-12T08:30:00.000Z",
  "air_temperature": 25.6,
  "humidity": 65.2,
  "water_temperature": 24.8,
  "ph": 7.2,
  "turbidity": 15.3,
  "dissolved_oxygen": 8.5,
  "ammonia": 0.2,
  "ph_relay": false,
  "aerator": true,
  "filter": true,
  "pump": false,
  "user_id": null
}
```

## Database Queries

### Latest Readings
```sql
SELECT * FROM sensor_data 
ORDER BY created_at DESC 
LIMIT 10;
```

### Hourly Averages
```sql
SELECT 
  AVG(water_temperature) as avg_water_temp,
  AVG(ph) as avg_ph,
  AVG(turbidity) as avg_turbidity
FROM sensor_data 
WHERE created_at > NOW() - INTERVAL '1 hour';
```

### 24-Hour Trends
```sql
SELECT 
  DATE_TRUNC('hour', created_at) as hour,
  AVG(water_temperature) as water_temp,
  AVG(ph) as ph_level,
  COUNT(*) as readings_count
FROM sensor_data 
WHERE created_at > NOW() - INTERVAL '24 hours'
GROUP BY DATE_TRUNC('hour', created_at)
ORDER BY hour;
```

## HTTP Client Implementation

### Configuration
```c
esp_http_client_config_t config = {
    .url = SUPABASE_URL,
    .method = HTTP_METHOD_POST,
    .cert_pem = supabase_cert_chain,
    .cert_len = strlen(supabase_cert_chain) + 1,
    .timeout_ms = 10000,
    .keep_alive_enable = true,
    .skip_cert_common_name_check = false,
};
```

### Request Function
```c
bool send_to_supabase(const char* json_data) {
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) return false;
    
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "apikey", SUPABASE_KEY);
    esp_http_client_set_header(client, "Authorization", "Bearer " SUPABASE_KEY);
    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    
    esp_err_t err = esp_http_client_perform(client);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    
    return (err == ESP_OK && status_code == 201);
}
```
