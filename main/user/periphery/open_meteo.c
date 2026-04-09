/*
 * open_meteo.c
 *
 *  Created on: 22.02.2026
 *      Author: toose
 */
#include "cJSON.h"
#include "wifi.h"

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "open_meteo.h"
#include "user/periphery/periphery.h"
#include "user/menu/lvgl_menu.h"
#include <stdint.h>

static const char *TAG = "WEATHER_APP";

extern ui_main_menu_t ui;

const city_t cities_de[CITY_COUNT] = {
    {"Berlin", 52.5200, 13.4050},       {"Hamburg", 53.5511, 9.9937},
    {"Munich", 48.1351, 11.5820},       {"Cologne", 50.9375, 6.9603},
    {"Frankfurt", 50.1109, 8.6821},     {"Stuttgart", 48.7758, 9.1829},
    {"Duesseldorf", 51.2277, 6.7735},   {"Leipzig", 51.3397, 12.3731},
    {"Dortmund", 51.5136, 7.4653},      {"Essen", 51.4556, 7.0116},
    {"Bremen", 53.0793, 8.8017},        {"Dresden", 51.0504, 13.7373},
    {"Hannover", 52.3759, 9.7320},      {"Nuremberg", 49.4521, 11.0767},
    {"Duisburg", 51.4344, 6.7623},      {"Bochum", 51.4818, 7.2162},
    {"Wuppertal", 51.2562, 7.1508},     {"Bielefeld", 52.0302, 8.5325},
    {"Bonn", 50.7374, 7.0982},          {"Muenster", 51.9607, 7.6261},
    {"Karlsruhe", 49.0069, 8.4037},     {"Mannheim", 49.4875, 8.4660},
    {"Augsburg", 48.3705, 10.8978},     {"Wiesbaden", 50.0782, 8.2398},
    {"Gelsenkirchen", 51.5177, 7.0857}, {"Moenchengladbach", 51.1805, 6.4428},
    {"Braunschweig", 52.2689, 10.5268}, {"Chemnitz", 50.8278, 12.9214},
    {"Kiel", 54.3233, 10.1228},         {"Aachen", 50.7753, 6.0839},
    {"Halle", 51.4828, 11.9699},        {"Magdeburg", 52.1205, 11.6276},
    {"Freiburg", 47.9990, 7.8421},      {"Krefeld", 51.3388, 6.5853},
    {"Luebeck", 53.8655, 10.6866},      {"Oberhausen", 51.4963, 6.8638},
    {"Erfurt", 50.9848, 11.0299},       {"Mainz", 49.9929, 8.2473},
    {"Rostock", 54.0924, 12.0991},      {"Kassel", 51.3127, 9.4797}};

char url[256];

typedef struct {
  uint8_t *buffer;
  size_t buffer_len;
} http_response_t;

extern current_weather_t current_weather_data;

SemaphoreHandle_t weather_mutex;

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
  http_response_t *response = (http_response_t *)evt->user_data;

  switch (evt->event_id) {

  case HTTP_EVENT_ON_DATA:
    if (evt->data_len > 0) {

      size_t new_size = response->buffer_len + evt->data_len + 1;

      uint8_t *new_buf =
          heap_caps_realloc(response->buffer, new_size, MALLOC_CAP_DEFAULT);

      if (!new_buf)
        return ESP_FAIL;

      response->buffer = new_buf;

      memcpy(response->buffer + response->buffer_len, evt->data, evt->data_len);

      response->buffer_len += evt->data_len;
      response->buffer[response->buffer_len] = 0;
    }
    break;

  default:
    break;
  }

  return ESP_OK;
}

void build_weather_url(int city_index) {

  if (city_index < 0 || city_index >= CITY_COUNT) {
    return;
  }

  snprintf(url, sizeof(url),

           "https://api.open-meteo.com/v1/forecast?"
           "latitude=%.4f&longitude=%.4f&"
           "current=temperature_2m,relative_humidity_2m,"
           "cloud_cover,wind_speed_10m,rain,snowfall,is_day&&"
           //"current_weather=true&"
           "timeformat=unixtime&timezone=auto",
           cities_de[city_index].lat, cities_de[city_index].lon);

  // ESP_LOGI("WEATHER","lat=%f lon=%f",
  // cities_de[city_index].lat,
  // cities_de[city_index].lon);

  // fetch_weather();
}

void fetch_weather(void) {
  build_weather_url(ui.weather.settings_popup.saved_city);
  // ESP_LOGI(TAG, "build_weather_url: %s",url);
  //   static const char *WEATHER_URL_CURRENT =
  //      "https://api.open-meteo.com/v1/forecast?"
  //      "latitude=51.5136&longitude=7.4653&"
  //      "current=temperature_2m,relative_humidity_2m,"
  //      "cloud_cover,wind_speed_10m,rain,snowfall&"
  //      "timeformat=unixtime&timezone=auto";
  // ESP_LOGI(TAG, "WEATHER_URL_CURRENT : %s",WEATHER_URL_CURRENT );
  http_response_t response = {0};

  esp_http_client_config_t config = {
      .url = url,
      .event_handler = _http_event_handler,
      .crt_bundle_attach = esp_crt_bundle_attach,
      .user_data = &response,
      .disable_auto_redirect = true,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);

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
          if (cJSON_IsNumber(item)) {
            current_weather_data.wind_speed_10m = item->valuedouble;
            current_weather_data.wind_speed_10m =
                current_weather_data.wind_speed_10m / 3.60;
          }

          item = cJSON_GetObjectItem(current, "rain");
          current_weather_data.rain =
              (cJSON_IsNumber(item)) ? item->valuedouble : 0.0;

          item = cJSON_GetObjectItem(current, "snowfall");
          current_weather_data.snow =
              (cJSON_IsNumber(item)) ? item->valuedouble : 0.0;
          item = cJSON_GetObjectItem(current, "is_day");
          if (cJSON_IsNumber(item))
            current_weather_data.is_day = item->valueint;
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
}