
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

void ota_last_update_save(uint32_t timestamp) {
    nvs_handle_t handle;
    if (nvs_open("ota_settings", NVS_READWRITE, &handle) != ESP_OK) return;
    nvs_set_u32(handle, "last_update", timestamp);
    nvs_commit(handle);
    nvs_close(handle);
}

uint32_t ota_last_update_load(void) {
    nvs_handle_t handle;
    if (nvs_open("ota_settings", NVS_READONLY, &handle) != ESP_OK) return 0;
    uint32_t val = 0;
    nvs_get_u32(handle, "last_update", &val);
    nvs_close(handle);
    return val;
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

void main_settings_save(ui_main_menu_t *input) {
    nvs_handle_t handle;
    if (nvs_open("main_settings", NVS_READWRITE, &handle) != ESP_OK) return;
    nvs_set_u8(handle, "standby",    input->settings.switch_.display_mode);
    nvs_set_u8(handle, "bl_pct",     input->settings.switch_.backlight_manual_pct);
    nvs_set_u8(handle, "bl_mode",    input->settings.switch_.backlight_mode);
    nvs_set_u8(handle, "theme_mode", input->settings.switch_.theme_mode);
    nvs_set_u8(handle, "co2_mode",   input->settings.switch_.co2_mode);
    nvs_set_u8(handle, "bl_amin",    input->settings.switch_.backlight_auto_min_pct);
    nvs_set_u8(handle, "bl_amax",    input->settings.switch_.backlight_auto_max_pct);
    nvs_commit(handle);
    nvs_close(handle);
}

void main_settings_load(ui_main_menu_t *output) {
    nvs_handle_t handle;
    if (nvs_open("main_settings", NVS_READONLY, &handle) != ESP_OK) {
		
        output->settings.switch_.display_mode   = 0;
        output->settings.switch_.backlight_manual_pct  = 80;
        output->settings.switch_.backlight_mode = 0;
        output->settings.switch_.theme_mode     = 0;
        output->settings.switch_.co2_mode       = 0;
        output->settings.switch_.backlight_auto_min_pct    = 5;
        output->settings.switch_.backlight_auto_max_pct    = 100;
        return;
    }
    uint8_t val;
    val = 0;   nvs_get_u8(handle, "standby",    &val); output->settings.switch_.display_mode   = val;
    val = 80;  nvs_get_u8(handle, "bl_pct",     &val); output->settings.switch_.backlight_manual_pct  = val;
    val = 0;   nvs_get_u8(handle, "bl_mode",    &val); output->settings.switch_.backlight_mode = val;
    val = 0;   nvs_get_u8(handle, "theme_mode", &val); output->settings.switch_.theme_mode     = val;
    val = 0;   nvs_get_u8(handle, "co2_mode",   &val); output->settings.switch_.co2_mode       = val;
    val = 5;   nvs_get_u8(handle, "bl_amin",    &val); output->settings.switch_.backlight_auto_min_pct    = val;
    val = 100; nvs_get_u8(handle, "bl_amax",    &val); output->settings.switch_.backlight_auto_max_pct    = val;
    nvs_close(handle);

    // защита от битой пары в NVS
    if (output->settings.switch_.backlight_auto_min_pct > output->settings.switch_.backlight_auto_max_pct) output->settings.switch_.backlight_auto_min_pct = output->settings.switch_.backlight_auto_max_pct;
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