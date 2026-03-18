
#include "../components/lvgl__lvgl/lvgl.h"
#include "user/menu/lvgl_menu.h"
#include "user/menu/lvgl_user_config.h"
#include "waveshare_rgb_lcd_port.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
//#include "nvs_flash.h"


#include "user/periphery/open_meteo.h"
#include "user/periphery/wifi.h"
#include "user/periphery/time.h"
#include "user/periphery/nvs_user.h"

//#include "user/periphery/sd_card.h"

extern SemaphoreHandle_t weather_mutex;
#if  ACTIVATE_BLOCK_BOT_RIGHT || ACTIVATE_BLOCK_BOT_LEFT
void weather_task(void *arg) {
  while (1) {
#if DEBUG_INET
    ESP_LOGI(TAG, "Fetching weather...");
#endif
    wifi_print_info();
    fetch_weather();
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}
 #endif 
void app_main() {
nvs_user_init();
  wifi_init_sta(); // ← ДО запуска weather_task
  #if ACTIVATE_BLOCK_TOP_RIGHT
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
   tzset();

    // 3. Инициализация NTP
   if(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        initialize_sntp();
    }
     start_ntp_time_task();
#endif
  // --- LCD + LVGL ---
  wifi_print_info();
  waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB
                                     // LCD
                                     	    // Initialize SD card 
//    if(waveshare_sd_card_init() == ESP_OK)
//    {
//        // Test SD card functionality 
//        waveshare_sd_card_test();
//    }

  //  Lock the mutex due to the LVGL APIs are not thread-safe
  if (lvgl_port_lock(-1)) {
    init_lv_objects();
    lvgl_port_unlock();
  }
#if  ACTIVATE_BLOCK_BOT_RIGHT || ACTIVATE_BLOCK_BOT_LEFT
  weather_mutex = xSemaphoreCreateMutex();

  xTaskCreate(weather_task, "weather_task", 16384, NULL, 5, NULL);
  #endif
}
