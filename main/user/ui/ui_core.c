

#include "ui_core.h"

#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "ui_blocks.h"
#include "ui_geometry.h"
#include "ui_popup_settings.h"
#include "ui_theme.h"
#include "ui_time.h"
#include "ui_timers.h"
#include "ui_user_config.h"
#include "ui_weather_anim.h"
#include "ui_widgets.h"
#include "ui_wifi.h"
#include "esp_log.h"

#include "../periphery/sensors.h"
#include "../periphery/wifi_user.h"
#include "../periphery/open_meteo.h"
#include "../periphery/backlight.h"
#include "../periphery/nvs_user.h"
#include "../periphery/open_meteo.h"
#include "../periphery/backlight.h"
#include "../periphery/time_user.h"


extern const city_t cities_de[];
extern wifi_ap_record_t ap_info;


ui_main_menu_t ui = {0};

void set_visible(lv_obj_t *parent, bool visible) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR set_visible");
    return;
  }
  if (visible)
    lv_obj_clear_flag(parent, LV_OBJ_FLAG_HIDDEN); // on
  else
    lv_obj_add_flag(parent, LV_OBJ_FLAG_HIDDEN); // off
}

void print_value(const char *flags, float value, lv_obj_t *parent) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR print_value");
    return;
  }
  lv_label_set_text_fmt(parent, flags, value);
}

void hide_all_blocks(ui_main_menu_t *ui) {
  set_visible(ui->animation.screen, false);
  set_visible(ui->sensor.screen, false);
  set_visible(ui->weather.screen, false);
  set_visible(ui->time.screen, false);
  set_visible(ui->co2.meter, false);
  set_visible(ui->co2.chart, false);
  set_visible(ui->settings.btn_open, false);
}
void show_all_blocks(ui_main_menu_t *ui) {
  set_visible(ui->animation.screen, true);
  set_visible(ui->sensor.screen, true);
  set_visible(ui->weather.screen, true);
  set_visible(ui->time.screen, true);
  set_visible(ui->co2.meter, true);
  set_visible(ui->co2.chart, true);
  set_visible(ui->settings.btn_open, true);
}

void init_lv_objects() {
  ui.co2.co2_display = -1;
  ui.co2.co2_target = -1;
  ui.weather.settings_popup.cities_de = cities_de;
  ui.weather.settings_popup.city_count = CITY_COUNT;

main_settings_load(
      &ui.settings.switch_.display_mode, &ui.settings.switch_.backlight_manual_pct,
      &ui.settings.switch_.backlight_mode, &ui.settings.switch_.theme_mode,
      &ui.settings.switch_.co2_mode,
      &ui.settings.switch_.backlight_auto_min_pct,
      &ui.settings.switch_.backlight_auto_max_pct);
  backlight_set(ui.settings.switch_.backlight_manual_pct);
  ui.settings.switch_.theme_last_is_day = get_is_day(); // ← инициализируем кэш
  weather_settings_load(&ui.weather.settings_popup.saved_city);
  build_weather_url(ui.weather.settings_popup.saved_city);
  init_fonts(&ui);
  init_styles(&ui);
  create_menu(&ui);
  lv_scr_load(ui.screen);
  create_timers();
  
  // SGP30 калибровка
   ui.co2.calib_start_tick = xTaskGetTickCount();
   ui_create_sgp30_calib_popup(&ui); 	
   set_visible(ui.co2.calib_popup,true);  	
}
