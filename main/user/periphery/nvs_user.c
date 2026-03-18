
#include "nvs_user.h"
#include "nvs_flash.h"
#include "nvs.h"

void nvs_user_init(){
	esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }
}

void settings_save(bool standby, bool theme) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("settings", NVS_READWRITE, &handle);
    if (err != ESP_OK) return;

    nvs_set_u8(handle, "standby", (uint8_t)standby);
    nvs_set_u8(handle, "theme",   (uint8_t)theme);
    nvs_commit(handle);
    nvs_close(handle);
}

void settings_load(bool *standby, bool *theme) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open("settings", NVS_READONLY, &handle);
    if (err != ESP_OK) {
        // Первый запуск — дефолтные значения
        *standby = false;
        *theme   = false;
        return;
    }

    uint8_t val = 0;
    nvs_get_u8(handle, "standby", &val); *standby = (bool)val;
    nvs_get_u8(handle, "theme",   &val); *theme   = (bool)val;
    nvs_close(handle);
}