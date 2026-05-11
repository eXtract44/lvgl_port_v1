#include "wifi_user.h"

#include "esp_log.h"
#include <string.h>

#include "sensors.h"
#include "time_user.h"

// uint8_t wifi_ssid[32] = "WiFi"; /**< SSID of target AP. */
// uint8_t wifi_password[64] = "Lokomotive132";

// #define WIFI_SSID "WiFi"
// #define WIFI_PASS "Lokomotive132"

wifi_ap_record_t ap_info;
static const char *TAG = "WIFI";
static EventGroupHandle_t wifi_event_group;
static volatile bool s_scan_done = false;
static volatile bool s_scanning = false;
#define WIFI_CONNECTED_BIT BIT0

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
  if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
    ESP_LOGI(TAG, "WiFi started");
    esp_wifi_connect();
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
				 
    if (!s_scanning) {
        ESP_LOGI(TAG, "WiFi disconnected, reconnecting...");
        esp_wifi_connect();
    } else {
        ESP_LOGI(TAG, "WiFi disconnected during scan, skip reconnect");
    }
  } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

    ESP_LOGI(TAG, "WiFi connected");
    ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&event->ip_info.ip));
    // user_sntp_stop();
    // user_initialize_sntp();
    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
  } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE) {
    s_scanning  = false;
    s_scan_done = true;
  }
}

void wifi_print_info(void) {

  // if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK){
  //	current_weather_data.wifi_connected = true;
  // }else{current_weather_data.wifi_connected = false;}
#if DEBUG_INET

  if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
    ESP_LOGI(TAG, "Connected to SSID: %s", ap_info.ssid);
    ESP_LOGI(TAG, "RSSI: %d dBm", ap_info.rssi);
    ESP_LOGI(TAG, "Channel: %d", ap_info.primary);
  } else {
    ESP_LOGI(TAG, "Not connected to AP");
  }
#endif
}

void wifi_init_sta(void) {
  wifi_event_group = xEventGroupCreate();

  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();

  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);

  esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &wifi_event_handler, NULL, NULL);

  esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &wifi_event_handler, NULL, NULL);

  //  wifi_config_t wifi_config = {
  //      .sta =
  //          {
  //              .ssid = WIFI_SSID,
  //              .password = WIFI_PASS,
  //          },
  //
  //  };

  esp_wifi_set_mode(WIFI_MODE_STA);
  // esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_start();

  // Ждём подключения
  xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE,
                      5000);
}

void wifi_connect(const char *ssid, const char *pass) {
  wifi_config_t wifi_config = {0};

  strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid) - 1);

  strncpy((char *)wifi_config.sta.password, pass,
          sizeof(wifi_config.sta.password) - 1);

  esp_wifi_disconnect(); // если уже был коннект
  esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
  esp_wifi_connect();
}

// ---------------------------------------------------------------------------
// WiFi helpers (unchanged)
// ---------------------------------------------------------------------------
uint8_t get_wifi_status(void) {
  static uint8_t current_wifi_status = WIFI_DISCONNECTED;
#if SIMULATE_INET_VALUES
  current_wifi_status = WIFI_CONNECTED;
#else
  current_wifi_status = (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK)
                            ? WIFI_CONNECTED
                            : WIFI_DISCONNECTED;
#endif
  return current_wifi_status;
}

const char *get_wifi_ssid(void) {
  return (get_wifi_status() == WIFI_CONNECTED) ? (const char *)ap_info.ssid
                                               : NULL;
}

int16_t get_wifi_rssi(void) {
  return (get_wifi_status() == WIFI_CONNECTED) ? ap_info.rssi : 0;
}

esp_err_t wifi_scan_start(void) {
      s_scan_done = false;
    s_scanning  = true;
    wifi_scan_config_t cfg = {
        .ssid        = NULL,
        .bssid       = NULL,
        .channel     = 0,
        .show_hidden = false,
        .scan_type   = WIFI_SCAN_TYPE_PASSIVE,  // passive — слушаем дольше
        .scan_time.passive = 400,               // ms на канал
    };
    return esp_wifi_scan_start(&cfg, false);
}

bool wifi_scan_is_done(void) {
    return s_scan_done;
}

uint16_t wifi_scan_get_count(void) {
    uint16_t count = 0;
    esp_wifi_scan_get_ap_num(&count);
    return count;
}

esp_err_t wifi_scan_get_results(wifi_ap_record_t *list, uint16_t *count) {
    return esp_wifi_scan_get_ap_records(count, list);
}

void wifi_disconnect(void) {
    s_scanning = true;
    esp_wifi_disconnect();
}