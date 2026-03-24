
#include "../components/lvgl__lvgl/lvgl.h"
#include "user/menu/lvgl_menu.h"
#include "user/menu/lvgl_user_config.h"
#include "waveshare_rgb_lcd_port.h"

#include "esp_event.h"
#include "esp_netif.h"
#include "freertos/event_groups.h"
#include "user/periphery/i2c_bus.h"
//#include "nvs_flash.h"


#include "user/periphery/open_meteo.h"
#include "user/periphery/wifi.h"
#include "user/periphery/nvs_user.h"
#include "user/periphery/periphery.h"
#include "user/periphery/time_user.h"

//#include "user/periphery/sd_card.h"

static void sensors_task(void *arg) {
     vTaskDelay(pdMS_TO_TICKS(1000));
    
    if (lvgl_port_lock(2000)) {  // берём мьютекс на время инита
        //sht31_init();
        sgp30_init();
        lvgl_port_unlock();
    }
    
    while (1) {
        if (lvgl_port_lock(100)) {
            read_sensors();
            lvgl_port_unlock();
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

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
 void i2c_scan() {
    ESP_LOGI("I2C_SCAN", "Scanning...");
    for (uint8_t addr = 1; addr < 127; addr++) {
        uint8_t buf;
        esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, addr,
                            &buf, 1, pdMS_TO_TICKS(10));
        if (ret == ESP_OK) {
            ESP_LOGI("I2C_SCAN", "Found device at 0x%02X", addr);
        }
    }
    ESP_LOGI("I2C_SCAN", "Done.");
}
void app_main() {
	i2c_bus_mutex_init();
	
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
//  wifi_print_info();
  waveshare_esp32_s3_rgb_lcd_init(); // Initialize the Waveshare ESP32-S3 RGB
  //aht10_init(); 
  //i2c_scan();                    
  if (lvgl_port_lock(-1)) {
    init_lv_objects();
    lvgl_port_unlock();
  }
  xTaskCreate(sensors_task, "sensors", 4096, NULL, 2, NULL);
#if  ACTIVATE_BLOCK_BOT_RIGHT || ACTIVATE_BLOCK_BOT_LEFT
  weather_mutex = xSemaphoreCreateMutex();

  xTaskCreate(weather_task, "weather_task", 16384, NULL, 5, NULL);
  #endif
}
