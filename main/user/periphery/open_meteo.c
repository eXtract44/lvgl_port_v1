/*
 * open_meteo.c
 *
 *  Created on: 22.02.2026
 *      Author: toose
 */
 #include "wifi.h"
 #include "cJSON.h"
 
 #include "open_meteo.h"
 #include "user/periphery/periphery.h"
 #include "esp_event.h"
 #include "esp_http_client.h"
 #include "esp_crt_bundle.h"
 #include "esp_log.h"
#include <stdint.h>
 
 static const char *TAG = "WEATHER_APP";
 
 //#define BUFFER_SIZE 1024
//uint8_t buffer[BUFFER_SIZE];
 
typedef struct {
  uint8_t *buffer;
  size_t buffer_len;
} http_response_t;

extern current_weather_t current_weather_data;

SemaphoreHandle_t weather_mutex;

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    http_response_t *response = (http_response_t *)evt->user_data;

    switch (evt->event_id) {

    case HTTP_EVENT_ON_DATA:
        if (evt->data_len > 0) {

            size_t new_size = response->buffer_len + evt->data_len + 1;

uint8_t *new_buf = heap_caps_realloc(response->buffer,
                                     new_size,
                                     MALLOC_CAP_DEFAULT);

if (!new_buf) return ESP_FAIL;

response->buffer = new_buf;

memcpy(response->buffer + response->buffer_len,
       evt->data,
       evt->data_len);

response->buffer_len += evt->data_len;
response->buffer[response->buffer_len] = 0;
        }
        break;

    default:
        break;
    }

    return ESP_OK;
}

void fetch_weather(void) {
  static const char *WEATHER_URL_CURRENT =
      "https://api.open-meteo.com/v1/forecast?"
      "latitude=51.5136&longitude=7.4653&"
      "current=temperature_2m,relative_humidity_2m,"
      "cloud_cover,wind_speed_10m,rain,snowfall&"
      "timeformat=unixtime&timezone=auto";

  http_response_t response = {0};

  esp_http_client_config_t config = {
      .url = WEATHER_URL_CURRENT,
      .event_handler = _http_event_handler,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .user_data = &response,
      .disable_auto_redirect = true,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  
//ESP_LOGI(TAG, "Free 8bit heap: %d",
//         heap_caps_get_free_size(MALLOC_CAP_8BIT));
  esp_err_t err = esp_http_client_perform(client);
  if (err == ESP_OK) {
#if DEBUG_INET
    ESP_LOGI(TAG, "HTTP Status = %d", esp_http_client_get_status_code(client));
#endif

    if (response.buffer && response.buffer_len > 0) {
#if DEBUG_INET
      ESP_LOGI(TAG, "RAW JSON: %s", response.buffer);
#endif

      cJSON *json = cJSON_Parse((char *)response.buffer);
      if (json) {
        cJSON *current = cJSON_GetObjectItem(json, "current");
        if (current) {
          cJSON *item = NULL;

          item = cJSON_GetObjectItem(current, "temperature_2m");
          if (cJSON_IsNumber(item))
            current_weather_data.temperature_2m = item->valuedouble;

          item = cJSON_GetObjectItem(current, "relative_humidity_2m");
          if (cJSON_IsNumber(item))
            current_weather_data.relative_humidity_2m = item->valueint;

          item = cJSON_GetObjectItem(current, "cloud_cover");
          if (cJSON_IsNumber(item))
            current_weather_data.cloud_cover = item->valueint;

          item = cJSON_GetObjectItem(current, "wind_speed_10m");
          if (cJSON_IsNumber(item)){
			current_weather_data.wind_speed_10m = item->valuedouble;
            current_weather_data.wind_speed_10m = current_weather_data.wind_speed_10m/3.60;  
		  }
            

          item = cJSON_GetObjectItem(current, "rain");
          current_weather_data.rain =
              (cJSON_IsNumber(item)) ? item->valuedouble : 0.0;

          item = cJSON_GetObjectItem(current, "snowfall");
          current_weather_data.snow =
              (cJSON_IsNumber(item)) ? item->valuedouble : 0.0;
#if DEBUG_INET
          ESP_LOGI(TAG,
                   "Weather updated: Temp=%.1f C Humidity=%d%% Clouds=%d%% "
                   "Wind=%.1f m/s",
                   current_weather_data.temperature_2m,
                   current_weather_data.relative_humidity_2m,
                   current_weather_data.cloud_cover,
                   current_weather_data.wind_speed_10m);
          ESP_LOGI(TAG, "Rain=%.2f mm Snow=%.2f mm", current_weather_data.rain,
                   current_weather_data.snow);
#endif
        }
        cJSON_Delete(json);
      } else {
        const char *error_ptr = cJSON_GetErrorPtr();
        ESP_LOGE(TAG, "JSON parse error: %s",
                 error_ptr ? error_ptr : "unknown");
      }
    } else {
      ESP_LOGE(TAG, "No data received from server");
    }

    if (response.buffer) {
      heap_caps_free(response.buffer);
      response.buffer = NULL;
      response.buffer_len = 0;
    }

  } else {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
  }
esp_http_client_cleanup(client);
 //esp_http_client_close(client);
}