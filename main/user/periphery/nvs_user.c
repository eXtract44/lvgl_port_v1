
#include "nvs_user.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "open_meteo.h"
#include "esp_log.h"

void nvs_user_init(){
	esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

void weather_settings_save(uint16_t city) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wea_settings", NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        //ESP_LOGE("NVS", "open failed: %s", esp_err_to_name(err));
        return;
    }

    err = nvs_set_u16(handle, "city", city);
    //ESP_LOGI("NVS", "set city=%d, err=%s", city, esp_err_to_name(err));

    err = nvs_commit(handle);
   // ESP_LOGI("NVS", "commit err=%s", esp_err_to_name(err));

    nvs_close(handle);
}

void main_settings_save(uint8_t  standby_mode, bool theme) {
      nvs_handle_t handle;
    esp_err_t err = nvs_open("main_settings", NVS_READWRITE, &handle);
    if (err != ESP_OK) return;

    nvs_set_u8(handle, "standby", standby_mode);  // uint8_t напрямую
    nvs_set_u8(handle, "theme",   (uint8_t)theme);
    nvs_commit(handle);
    nvs_close(handle);
}

void main_settings_load(uint8_t  *standby_mode, bool *theme) {
  nvs_handle_t handle;
    esp_err_t err = nvs_open("main_settings", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        *standby_mode = 0;  // дефолт — выключен
        *theme        = false;
        return;
    }

    uint8_t val = 0;
    nvs_get_u8(handle, "standby", &val); *standby_mode = val;
    nvs_get_u8(handle, "theme",   &val); *theme = (bool)val;
    nvs_close(handle);
}

void weather_settings_load(uint16_t *city) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("wea_settings", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        // Первый запуск — дефолтные значения
        *city = 0;
        return;
    }

    uint16_t val = 0;
    nvs_get_u16(handle, "city", &val); *city = val;
    nvs_close(handle);
    *city = (val < CITY_COUNT) ? val : 0;
}