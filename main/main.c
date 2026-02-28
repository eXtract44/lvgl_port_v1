
#include "../components/lvgl__lvgl/lvgl.h"
#include "user/menu/lvgl_menu.h"
#include "waveshare_rgb_lcd_port.h"



#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"


#include "user/periphery/open_meteo.h"
#include "user/periphery/wifi.h"
#include "user/periphery/time.h"

#include "user/periphery/sd_card.h"

extern SemaphoreHandle_t weather_mutex;

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

void app_main() {
  nvs_flash_init();

  wifi_init_sta(); // ← ДО запуска weather_task
  
  setenv("TZ", "CET-1CEST,M3.5.0/2,M10.5.0/3", 1);
   tzset();

    // 3. Инициализация NTP
   if(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
        initialize_sntp();
    }
     start_ntp_time_task();
  // --- LCD + LVGL ---
  waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB
                                     // LCD
                                     	    // Initialize SD card 
//    if(waveshare_sd_card_init() == ESP_OK)
//    {
//        // Test SD card functionality 
//        waveshare_sd_card_test();
//    }
  // wavesahre_rgb_lcd_bl_on();  //Turn on the screen backlight
  // wavesahre_rgb_lcd_bl_off(); //Turn off the screen backlight
  // ESP_LOGI(TAG, "Display LVGL demos");
  //  Lock the mutex due to the LVGL APIs are not thread-safe
  if (lvgl_port_lock(-1)) {
    init_lv_objects();
    lvgl_port_unlock();
  }

  weather_mutex = xSemaphoreCreateMutex();

  xTaskCreate(weather_task, "weather_task", 16384, NULL, 5, NULL);
}
