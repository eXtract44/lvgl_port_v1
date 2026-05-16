/*
 * open_meteo.c
 *
 *  Created on: 22.02.2026
 *      Author: toose
 */
#include "cJSON.h"
#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "open_meteo.h"
#include <stdint.h>

#include "../menu/ui_core.h"
#include "sensors.h"
#include "wifi_user.h"

//static const char *TAG = "WEATHER_APP";
forecast_data_t forecast_data;
current_weather_t current_weather_data = {0};

extern ui_main_menu_t ui;

const city_t cities_de[CITY_COUNT] = {
    // Крупные города
    {"Berlin", 52.5200, 13.4050},
    {"Hamburg", 53.5511, 9.9937},
    {"Munich", 48.1351, 11.5820},
    {"Cologne", 50.9375, 6.9603},
    {"Frankfurt", 50.1109, 8.6821},
    {"Stuttgart", 48.7758, 9.1829},
    {"Duesseldorf", 51.2277, 6.7735},
    {"Leipzig", 51.3397, 12.3731},
    {"Essen", 51.4556, 7.0116},
    {"Bremen", 53.0793, 8.8017},
    {"Dresden", 51.0504, 13.7373},
    {"Hannover", 52.3759, 9.7320},
    {"Nuremberg", 49.4521, 11.0767},
    {"Duisburg", 51.4344, 6.7623},
    {"Bochum", 51.4818, 7.2162},
    {"Wuppertal", 51.2562, 7.1508},
    {"Bielefeld", 52.0302, 8.5325},
    {"Bonn", 50.7374, 7.0982},
    {"Muenster", 51.9607, 7.6261},
    {"Karlsruhe", 49.0069, 8.4037},
    {"Mannheim", 49.4875, 8.4660},
    {"Augsburg", 48.3705, 10.8978},
    {"Wiesbaden", 50.0782, 8.2398},
    {"Gelsenkirchen", 51.5177, 7.0857},
    {"Moenchengladbach", 51.1805, 6.4428},
    {"Braunschweig", 52.2689, 10.5268},
    {"Chemnitz", 50.8278, 12.9214},
    {"Kiel", 54.3233, 10.1228},
    {"Aachen", 50.7753, 6.0839},
    {"Halle", 51.4828, 11.9699},
    {"Magdeburg", 52.1205, 11.6276},
    {"Freiburg", 47.9990, 7.8421},
    {"Krefeld", 51.3388, 6.5853},
    {"Luebeck", 53.8655, 10.6866},
    {"Oberhausen", 51.4963, 6.8638},
    {"Erfurt", 50.9848, 11.0299},
    {"Mainz", 49.9929, 8.2473},
    {"Rostock", 54.0924, 12.0991},
    {"Kassel", 51.3127, 9.4797},

    // Дортмунд — районы
    {"DO-Mitte", 51.5136, 7.4653},
    {"DO-Eving", 51.5541, 7.4602},
    {"DO-Scharnhorst", 51.5573, 7.5200},
    {"DO-Brackel", 51.5100, 7.5500},
    {"DO-Aplerbeck", 51.4833, 7.5500},
    {"DO-Hoerde", 51.4833, 7.5000},
    {"DO-Hombruch", 51.4833, 7.4333},
    {"DO-Luetgendortmund", 51.5167, 7.3833},
    {"DO-Huckarde", 51.5500, 7.3833},
    {"DO-Mengede", 51.5667, 7.3833},
    {"DO-Innenstadt-W", 51.5136, 7.4500},
    {"DO-Innenstadt-O", 51.5136, 7.4800},
       // Dortmund — ключевые Stadtteile
    {"DO-Innenstadt-N",    51.5250, 7.4600},
    {"DO-Borsigplatz",     51.5350, 7.4850},
    {"DO-Dorstfeld",       51.5100, 7.4100},
    {"DO-Marten",          51.5000, 7.3800},
    {"DO-Kirchlinde",      51.5200, 7.3600},
    {"DO-Derne",           51.5700, 7.5100},
    {"DO-Grevel",          51.5600, 7.5400},
    {"DO-Alt-Scharnhorst", 51.5500, 7.5450},
    {"DO-Asseln",          51.5050, 7.5800},
    {"DO-Wambel",          51.5150, 7.5200},
    {"DO-Wickede",         51.5000, 7.6000},
    {"DO-Berghofen",       51.4700, 7.5200},
    {"DO-Kirchhörde",      51.4600, 7.4600},
    {"DO-Barop",           51.4750, 7.4100},
    {"DO-Eichlinghofen",   51.4800, 7.4000},
    {"DO-Bodelschwingh",   51.5600, 7.3500},
    {"DO-Brechten",        51.5750, 7.4400},
    {"DO-Lindenhorst",     51.5650, 7.4200},
    {"DO-Husen",           51.5450, 7.5300},
    {"DO-Körne",           51.5050, 7.5050},
    {"DO-Oespel",          51.5000, 7.4000},
};

char url[512];

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
         "cloud_cover,wind_speed_10m,rain,snowfall,is_day,"
         "apparent_temperature,uv_index,"
         "precipitation_probability,surface_pressure&"
         "daily=temperature_2m_max,temperature_2m_min,"
         "relative_humidity_2m_max,weathercode,sunrise,sunset&"
         "forecast_days=3&"
         "timeformat=unixtime&timezone=auto",
         cities_de[city_index].lat, cities_de[city_index].lon);
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
          // --- новое ---
          item = cJSON_GetObjectItem(current, "apparent_temperature");
          current_weather_data.apparent_temperature =
              cJSON_IsNumber(item) ? item->valuedouble
                                   : current_weather_data.temperature_2m;

          item = cJSON_GetObjectItem(current, "uv_index");
          current_weather_data.uv_index =
              cJSON_IsNumber(item) ? item->valuedouble : 0.0;

          item = cJSON_GetObjectItem(current, "precipitation_probability");
          current_weather_data.precipitation_probability =
              cJSON_IsNumber(item) ? item->valuedouble : 0.0;

          item = cJSON_GetObjectItem(current, "surface_pressure");
          current_weather_data.surface_pressure =
              cJSON_IsNumber(item) ? item->valuedouble : 1013.0;

          // --- daily forecast ---
          cJSON *daily = cJSON_GetObjectItem(json, "daily");
          if (daily) {
            cJSON *temp_max_arr =
                cJSON_GetObjectItem(daily, "temperature_2m_max");
            cJSON *temp_min_arr =
                cJSON_GetObjectItem(daily, "temperature_2m_min");
            cJSON *hum_max_arr =
                cJSON_GetObjectItem(daily, "relative_humidity_2m_max");
            cJSON *wcode_arr = cJSON_GetObjectItem(daily, "weathercode");
            cJSON *sunrise_arr = cJSON_GetObjectItem(daily, "sunrise");
            cJSON *sunset_arr = cJSON_GetObjectItem(daily, "sunset");

            for (int i = 0; i < FORECAST_DAYS; i++) {
              cJSON *item;

              item = cJSON_GetArrayItem(temp_max_arr, i);
              if (cJSON_IsNumber(item))
                forecast_data.day[i].temp_max = (int8_t)item->valuedouble;

              item = cJSON_GetArrayItem(temp_min_arr, i);
              if (cJSON_IsNumber(item))
                forecast_data.day[i].temp_min = (int8_t)item->valuedouble;

              item = cJSON_GetArrayItem(hum_max_arr, i);
              if (cJSON_IsNumber(item))
                forecast_data.day[i].humidity_max = (uint8_t)item->valueint;

              item = cJSON_GetArrayItem(wcode_arr, i);
              if (cJSON_IsNumber(item))
                forecast_data.day[i].weathercode = (uint8_t)item->valueint;

              item = cJSON_GetArrayItem(sunrise_arr, i);
              if (cJSON_IsNumber(item))
                forecast_data.day[i].sunrise = (uint32_t)item->valuedouble;

              item = cJSON_GetArrayItem(sunset_arr, i);
              if (cJSON_IsNumber(item))
                forecast_data.day[i].sunset = (uint32_t)item->valuedouble;
            }
          }
          forecast_data.valid = true;

#if DEBUG_INET
          ESP_LOGI(TAG, "Forecast day0: max=%d min=%d wcode=%d",
                   forecast_data.day[0].temp_max, forecast_data.day[0].temp_min,
                   forecast_data.day[0].weathercode);
          ESP_LOGI(TAG, "Forecast day1: max=%d min=%d wcode=%d",
                   forecast_data.day[1].temp_max, forecast_data.day[1].temp_min,
                   forecast_data.day[1].weathercode);
          ESP_LOGI(TAG, "Forecast day2: max=%d min=%d wcode=%d",
                   forecast_data.day[2].temp_max, forecast_data.day[2].temp_min,
                   forecast_data.day[2].weathercode);

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

const char *weathercode_to_text(uint8_t code) {
  if (code == 0)
    return "Klar";
  if (code <= 2)
    return "Leicht bewoelkt";
  if (code == 3)
    return "Bewoelkt";
  if (code <= 49)
    return "Nebel";
  if (code <= 55)
    return "Nieseln";
  if (code <= 65)
    return "Regen";
  if (code <= 75)
    return "Schnee";
  if (code <= 82)
    return "Schauer";
  if (code <= 86)
    return "Schneeschauer";
  if (code <= 99)
    return "Gewitter";
  return "Unbekannt";
}

float get_weather_temperature() {
	static float current_temperature_outside = 0;
#if SIMULATE_INET_VALUES
	current_temperature_outside += 0.05;
	if (current_temperature_outside > 35) {
		current_temperature_outside = -20.0;
	}
#else
	current_temperature_outside = current_weather_data.temperature_2m;
#endif
	return current_temperature_outside;
}

uint8_t get_weather_humidity() {
	static uint8_t current_humidity_outside = 0;
#if SIMULATE_INET_VALUES
	current_humidity_outside++;
	if (current_humidity_outside > 99) {
		current_humidity_outside = 0;
	}
#else
	current_humidity_outside = current_weather_data.relative_humidity_2m;
#endif
	return current_humidity_outside;
}

uint8_t get_weather_wind() {
	static uint8_t current_wind_outside = 0;
#if SIMULATE_INET_VALUES
	current_wind_outside += 1;
	if (current_wind_outside > 50) {
		current_wind_outside = 0;
	}
#else
	current_wind_outside = current_weather_data.wind_speed_10m;
#endif
	return current_wind_outside;
}

uint8_t get_weather_clouds() {
	static uint8_t current_clouds = 0;
#if SIMULATE_INET_VALUES
	current_clouds += 1;
	if (current_clouds > 100) {
		current_clouds = 0;
	}
#else
	current_clouds = current_weather_data.cloud_cover;
#endif
	return current_clouds;
}

float get_weather_rain() {
	static float current_rain = 0;
#if SIMULATE_INET_VALUES
	current_rain += 1;
	if (current_rain > 5) {
		current_rain = 0;
	}
#else
	current_rain = current_weather_data.rain;
#endif
	return current_rain;
}

float get_weather_snow() {
	static float current_snow = 0;
#if SIMULATE_INET_VALUES
	current_snow += 1;
	if (current_snow > 5) {
		current_snow = 0;
	}
#else
	current_snow = current_weather_data.snow;
#endif
	return current_snow;
}

uint8_t get_is_day() { return current_weather_data.is_day; }

double get_apparent_temperature(void) {
    return current_weather_data.apparent_temperature; }

double get_uv_index(void) {
    return current_weather_data.uv_index; }

double get_precipitation_probability(void) {
    return current_weather_data.precipitation_probability; }

double get_surface_pressure(void) {
    return current_weather_data.surface_pressure; }