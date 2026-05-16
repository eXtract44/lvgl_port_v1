
#include "ui_timers.h"
#include "backlight.h"
#include "ui_core.h"
#include "ui_theme.h"
#include "user/menu/ui_user_config.h"
#include "wifi_user.h"
#include "ui_weather_anim.h"
#include "ui_time.h"
#include "user/periphery/time_user.h"
#include "ui_blocks.h"

extern ui_main_menu_t ui;

volatile int standby_touched = 0; // callback for extern touch driver

extern sgp30_data_t sgp30_data;
extern sht41_data_t sht41_data;

void timer_10000(lv_timer_t *timer) {
  LV_UNUSED(timer);
#if ACTIVATE_BLOCK_TOP_RIGHT
  update_block_top_right(&ui);
#endif

  // --- авто-тема ---
  if (ui.settings.switch_.theme_mode == 0) { // 0 = Auto
    bool is_day = get_is_day();
    if (is_day != ui.settings.switch_.theme_last_is_day) {
      ui.settings.switch_.theme_last_is_day = is_day;
      is_day ? apply_theme_light(&ui) : apply_theme_dark(&ui);
      lv_obj_report_style_change(&ui.style.main);
      lv_obj_report_style_change(&ui.style.popup);
      lv_obj_report_style_change(&ui.style.top_left);
      lv_obj_report_style_change(&ui.style.bot_left);
      lv_obj_report_style_change(&ui.style.top_right);
      lv_obj_report_style_change(&ui.style.bot_right);
      lv_obj_report_style_change(&ui.font.very_small_20);
      lv_obj_report_style_change(&ui.font.small_24);
      lv_obj_report_style_change(&ui.font.medium_32);
      lv_obj_report_style_change(&ui.font.large_48);
      lv_obj_report_style_change(&ui.font.nav_btn);
      lv_obj_report_style_change(&ui.font.time);
      lv_obj_report_style_change(&ui.font.icon);
      lv_obj_report_style_change(&ui.style.category_btn);
    }
  }
}

static void timer_1000(lv_timer_t *timer) {
  LV_UNUSED(timer);
  if (!ui.settings.switch_.standby_screen_off) {
    switch (ui.settings.switch_.backlight_mode) {
    case 1:
      backlight_set(backlight_get_auto_pct());
      break;
    case 2:
      backlight_set(backlight_get_zeitplan_pct());
      break;
    default:
      break;
    }
  }
#if ACTIVATE_BLOCK_TOP_LEFT
  update_block_top_left(&ui);
#endif
#if ACTIVATE_BLOCK_BOT_LEFT
  update_block_bot_left(&ui);
#endif
#if ACTIVATE_BLOCK_TOP_MID
  update_block_top_middle(&ui);
#endif
#if ACTIVATE_BLOCK_BOT_MID
  update_block_bot_middle(&ui);
#endif
#if ACTIVATE_BLOCK_BOT_RIGHT
  update_block_bot_right(&ui);
#endif
}


static void timer_200(lv_timer_t *timer) {
  LV_UNUSED(timer);
  display_standby_handle(&ui);
}

void co2_meter_anim_cb(lv_timer_t *timer) {
  ui_main_menu_t *ui = timer->user_data;

  if (ui->co2.co2_target < 0)
    return;

  if (ui->co2.co2_display < 0) {
    ui->co2.co2_display = ui->co2.co2_target;
  }

  int32_t diff = ui->co2.co2_target - ui->co2.co2_display;
  if (diff == 0)
    return;

  int32_t step = diff / 8;
  if (step == 0)
    step = (diff > 0) ? 1 : -1;

  ui->co2.co2_display += step;
  uint16_t val = (uint16_t)ui->co2.co2_display;

  // обновляем иглу
  lv_meter_set_indicator_value(ui->co2.meter, ui->co2.indicator, val);

  // обновляем цветной arc от начала до текущего значения
  lv_meter_set_indicator_start_value(ui->co2.meter, ui->co2.needle_arc,
                                     MIN_VALUE_CO2);
  lv_meter_set_indicator_end_value(ui->co2.meter, ui->co2.needle_arc, val);

  // меняем цвет arc в зависимости от значения
  lv_meter_indicator_t *arc = ui->co2.needle_arc;
  arc->type_data.arc.color = calc_co2_color(val);
  lv_obj_invalidate(ui->co2.meter);
}

void display_standby_handle(ui_main_menu_t *ui) {
  static uint32_t timer_standby_sec = 0;

  uint8_t mode = ui->settings.switch_.standby_mode;

  bool standby_active = false;
  if (mode == 1) {
    standby_active = true;
  } else if (mode == 2) {
    standby_active = !get_is_day();
  }

  if (!standby_active) {
    timer_standby_sec = 0;
    ui->settings.switch_.standby_screen_off = false;
    set_visible(ui->standby.background, false);
    return;
  }

  if (is_screen_pressed()) {
    timer_standby_sec = 0;
    ui->settings.switch_.standby_screen_off = false;
    set_visible(ui->standby.background, false);
    backlight_set(backlight_get_current_pct(ui)); // восстанавливаем яркость
  } else {
    timer_standby_sec++;
    if (timer_standby_sec > MAX_STANDBY_TIME * 5) {
      ui->settings.switch_.standby_screen_off = true;
      backlight_set(0); // гасим подсветку
      lv_obj_move_foreground(ui->standby.background);
      set_visible(ui->standby.background, true);
      timer_standby_sec = MAX_STANDBY_TIME * 5;
    }
  }
}

uint8_t backlight_get_current_pct(ui_main_menu_t *ui) {
  switch (ui->settings.switch_.backlight_mode) {
  case 1:
    return backlight_get_auto_pct();
  case 2:
    return backlight_get_zeitplan_pct();
  default:
    return ui->settings.switch_.backlight_pct;
  }
}

bool is_screen_pressed(void) { return standby_touched; }

void sgp30_calib_timer_cb(lv_timer_t *t) {
  ui_main_menu_t *ui = t->user_data;
  if ((xTaskGetTickCount() - ui->co2.calib_start_tick) >= CO2_CALIB_TIME) {
    lv_obj_add_flag(ui->co2.calib_icon_label, LV_OBJ_FLAG_HIDDEN);
    lv_timer_del(t);
  }
}

void draw_symbol_wifi(ui_main_menu_t *ui) {
  //  static uint8_t status_wifi_old = 254;
  //  uint8_t current_status = get_wifi_status();
  //  if (current_status == status_wifi_old)
  //    return;
  //
  //  if (!ui->time.wifi_status_icon)
  //    return;
  //
  //  switch (current_status) {
  //  case WIFI_RECONNECT:
  //    lv_label_set_text(ui->time.wifi_status_icon, LV_SYMBOL_REFRESH);
  //    break;
  //  case WIFI_CONNECTED:
  //    lv_label_set_text(ui->time.wifi_status_icon, LV_SYMBOL_WIFI);
  //    break;
  //  case WIFI_DISCONNECTED:
  //  default:
  //    lv_label_set_text(ui->time.wifi_status_icon, LV_SYMBOL_WARNING);
  //    break;
  //  }
  //  status_wifi_old = current_status;
}

void create_timers(){
	 lv_timer_create(sgp30_calib_timer_cb, 60000, &ui); // проверка раз в минуту
	lv_timer_create(timer_10000, 10000, NULL);
  lv_timer_create(timer_1000, 1000, NULL);
  lv_timer_create(timer_200, 200, NULL);
  lv_timer_create(co2_meter_anim_cb, 33, &ui);
}

float calc_dew_point(float temp, float humidity) {
  if (humidity < 1.0f)
    humidity = 1.0f; // защита от log(0)
  if (humidity > 100.0f)
    humidity = 100.0f;
  const float a = 17.27f;
  const float b = 237.7f;
  float alpha = ((a * temp) / (b + temp)) + logf(humidity / 100.0f);
  return (b * alpha) / (a - alpha);
}

// ─── вспомогательная функция — кружки TVOC ──────────────
// возвращает строку типа "●●●○○" и устанавливает цвет
void update_tvoc_dots(lv_obj_t *dots[5], uint16_t tvoc,
                             uint8_t theme_mode) {
  uint8_t filled;
  bool dark_theme;
  if (theme_mode == 2) { // Dunkel — всегда тёмная
    dark_theme = true;
  } else if (theme_mode == 1) { // Hell — всегда светлая
    dark_theme = false;
  } else { // Auto
    dark_theme = !get_is_day();
  }
  lv_color_t color;

  if (tvoc <= 150) {
    filled = 1;
    color = lv_color_hex(0x00E676);
  } // яркий зелёный
  else if (tvoc <= 300) {
    filled = 2;
    color = lv_color_hex(0xC6FF00);
  } // жёлто-зелёный
  else if (tvoc <= 500) {
    filled = 3;
    color = lv_color_hex(0xFFD600);
  } // жёлтый
  else if (tvoc <= 750) {
    filled = 4;
    color = lv_color_hex(0xFF6D00);
  } // оранжевый
  else {
    filled = 5;
    color = lv_color_hex(0xFF1744);
  } // красный

  // пустые кружки — контрастны для обеих тем
  lv_color_t empty_color =
      dark_theme ? lv_color_hex(0x555555) // тёмная тема — светло-серый
                 : lv_color_hex(0xCCCCCC); // светлая тема — тёмно-серый
  lv_opa_t empty_opa = LV_OPA_COVER; // всегда полностью непрозрачный

  for (int i = 0; i < 5; i++) {
    if (i < filled) {
      lv_obj_set_style_bg_color(dots[i], color, 0);
      lv_obj_set_style_bg_opa(dots[i], LV_OPA_COVER, 0);
    } else {
      lv_obj_set_style_bg_color(dots[i], empty_color, 0);
      lv_obj_set_style_bg_opa(dots[i], empty_opa, 0);
    }
  }
}

static float calc_heat_index(float t, float rh) {
  if (t < 27.0f)
    return t; // heat index не актуален при прохладе
  float hi = -8.78469f + 1.61139f * t + 2.33855f * rh - 0.14611f * t * rh -
             0.01230f * t * t - 0.01642f * rh * rh + 0.00221f * t * t * rh +
             0.00072f * t * rh * rh - 0.000003582f * t * t * rh * rh;
  return hi;
}

void update_block_top_left(ui_main_menu_t *ui) {
  bool sht41_ok = (sht41_data.life == SHT41_STATE_OK);
  bool sgp30_ok = (sgp30_data.state == SGP30_STATE_OK);
  bool wifi_ok = (get_wifi_status() == WIFI_CONNECTED);

  // ── температура ───────────────────────────────────────
  if (sht41_ok) {
    float temp = get_temperature_sht41();
    if (temp > -40.0f && temp < 85.0f) {
      print_value("%.1f°C", temp, ui->sensor.temperature_label);
    } else {
      lv_label_set_text(ui->sensor.temperature_label, "Err");
    }
  } else {
    lv_label_set_text(ui->sensor.temperature_label, "K.S");
  }

  // ── влажность ─────────────────────────────────────────
  if (sht41_ok) {
    float hum = get_humidity_sht41();
    if (hum >= 0.0f && hum <= 100.0f) {
      print_value("%.0f%%", hum, ui->sensor.humidity_label);
    } else {
      lv_label_set_text(ui->sensor.humidity_label, "Err");
    }
  } else {
    lv_label_set_text(ui->sensor.humidity_label, "K.S");
  }

  // ── feels like + точка росы ───────────────────────────
  if (sht41_ok) {
    float temp = get_temperature_sht41();
    float hum = get_humidity_sht41();

    if (temp > -40.0f && temp < 85.0f && hum >= 1.0f && hum <= 100.0f) {

      // feels like
      float feels = calc_heat_index(temp, hum);
      print_value("~%.1f°C", feels, ui->sensor.feels_like_label);

      // точка росы
      float dp = calc_dew_point(temp, hum);
      print_value("%.1f°C", dp, ui->sensor.dew_point_label);

      // цвет точки росы — единственное исключение, несёт смысловую нагрузку
      bool outdoor_valid = wifi_ok;
      float outdoor_temp = 0.0f;
      if (outdoor_valid) {
        outdoor_temp = get_weather_temperature();
        if (outdoor_temp < -60.0f || outdoor_temp > 60.0f)
          outdoor_valid = false;
      }
      if (outdoor_valid) {
        float margin = outdoor_temp - dp;
        if (margin <= 0.0f)
          lv_obj_set_style_text_color(ui->sensor.dew_point_label,
                                      lv_palette_main(LV_PALETTE_RED), 0);
        else if (margin < 1.0f)
          lv_obj_set_style_text_color(ui->sensor.dew_point_label,
                                      lv_palette_main(LV_PALETTE_ORANGE), 0);
        else if (margin < 3.0f)
          lv_obj_set_style_text_color(ui->sensor.dew_point_label,
                                      lv_palette_main(LV_PALETTE_LIME), 0);
        else
          lv_obj_remove_local_style_prop(ui->sensor.dew_point_label,
                                         LV_STYLE_TEXT_COLOR, 0);
      } else {
        lv_obj_remove_local_style_prop(ui->sensor.dew_point_label,
                                       LV_STYLE_TEXT_COLOR, 0);
      }

    } else {
      lv_label_set_text(ui->sensor.feels_like_label, "Err");
      lv_label_set_text(ui->sensor.dew_point_label, "Err");
    }
  } else {
    lv_label_set_text(ui->sensor.feels_like_label, "K.S");
    lv_label_set_text(ui->sensor.dew_point_label, "K.S");
  }

  // ── TVOC + кружки ─────────────────────────────────────
  if (sgp30_ok) {
    uint16_t tvoc = (uint16_t)get_tvoc_sgp30();
    if (tvoc == 0) {
      lv_label_set_text(ui->sensor.tvoc_label, "...");
      for (int i = 0; i < 5; i++) {
        lv_obj_set_style_bg_color(ui->sensor.tvoc_dots[i],
                                  lv_color_hex(0x555555), 0);
        lv_obj_set_style_bg_opa(ui->sensor.tvoc_dots[i], LV_OPA_50, 0);
      }
    } else {
      print_value("%.0f", (float)tvoc, ui->sensor.tvoc_label);
      update_tvoc_dots(ui->sensor.tvoc_dots, tvoc,
                       ui->settings.switch_.theme_mode);
    }
  } else {
    lv_label_set_text(ui->sensor.tvoc_label, "K.S");
    for (int i = 0; i < 5; i++) {
      lv_obj_set_style_bg_color(ui->sensor.tvoc_dots[i], lv_color_hex(0x555555),
                                0);
      lv_obj_set_style_bg_opa(ui->sensor.tvoc_dots[i], LV_OPA_50, 0);
    }
  }

  // ── запись истории ────────────────────────────────────
  if (sht41_ok) {
    sensor_temperature_record_values(ui);
  }
}

static int16_t co2_scale(int16_t real_ppm, uint8_t mode) {
  static const int16_t mode_max[] = {2400, 4000, 6000};
  int16_t max = mode_max[mode];
  if (real_ppm <= MIN_VALUE_CO2)
    return MIN_VALUE_CO2;
  if (real_ppm >= max)
    return MAX_VALUE_CO2;
  return (int16_t)((int32_t)(real_ppm - MIN_VALUE_CO2) *
                       (MAX_VALUE_CO2 - MIN_VALUE_CO2) / (max - MIN_VALUE_CO2) +
                   MIN_VALUE_CO2);
}

lv_color_t calc_co2_color(uint16_t co2) {
  if (co2 < 800)
    return lv_palette_main(LV_PALETTE_GREEN);
  if (co2 < 1000)
    return lv_color_hex(0xC6FF00); // жёлто-зелёный
  if (co2 < 1200)
    return lv_palette_main(LV_PALETTE_YELLOW);
  if (co2 < 1600)
    return lv_palette_main(LV_PALETTE_ORANGE);
  return lv_palette_main(LV_PALETTE_RED);
}
void update_block_top_middle(ui_main_menu_t *ui) {
  if (sgp30_data.state == SGP30_STATE_OK) {
    uint16_t raw = get_co2_sgp30();
    ui->co2.co2_target = co2_scale(raw, ui->settings.switch_.co2_mode);
    print_value("%.f", (float)raw, ui->co2.co2_label);
    update_co2_status_label(ui, raw); // передаём реальное ppm, не scaled
  } else {
    lv_label_set_text(ui->co2.co2_label, "KS");
    lv_label_set_text(ui->co2.co2_status_label, "---");
  }
}

void update_block_top_right(ui_main_menu_t *ui) {
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    print_mday(0, 0, ui);
    print_wday(WDAY_KEIN_WLAN, ui);
    print_time(0, 0, ui);
    lv_obj_add_flag(ui->time.holiday_label, LV_OBJ_FLAG_HIDDEN);
  } else {
    print_mday(get_time_mday(), get_time_month(), ui);
    print_wday(get_time_wday(), ui);
    print_time(get_time_hour(), get_time_minute(), ui);
    print_holiday(get_time_mday(), get_time_month(), get_time_year(), ui);
  }
}

static lv_color_t calc_uv_color(float uv) {
  if (uv < 3.0f)
    return lv_palette_main(LV_PALETTE_GREEN);
  if (uv < 6.0f)
    return lv_color_hex(0xB3B300);
  if (uv < 8.0f)
    return lv_palette_main(LV_PALETTE_ORANGE);
  if (uv < 11.0f)
    return lv_palette_main(LV_PALETTE_RED);
  return lv_color_hex(0x8B00FF); // фиолетовый — экстремальный
}

void update_block_bot_left(ui_main_menu_t *ui) {
  bool wifi_ok = (get_wifi_status() == WIFI_CONNECTED);

  if (!wifi_ok) {
    // ── нет WiFi — прочерки везде ─────────────────────
    lv_label_set_text(ui->weather.temperature_label, "K.W");
    lv_label_set_text(ui->weather.humidity_label, "K.W");
    lv_label_set_text(ui->weather.feels_like_label, "K.W");
    lv_label_set_text(ui->weather.uv_label, "UV -");
    lv_label_set_text(ui->weather.rain_label, "unwahrscheinli.");
    lv_label_set_text(ui->weather.wind_label, "- m/s");
    lv_label_set_text(ui->weather.pressure_label, "- hPa");
    return;
  }

  // ── строка 1 — температура + влажность ────────────────
  float temp = get_weather_temperature();
  float hum = (float)get_weather_humidity();

  if (temp > -60.0f && temp < 60.0f) {
    print_value("%.1f°C", temp, ui->weather.temperature_label);
  } else {
    lv_label_set_text(ui->weather.temperature_label, "Err");
  }

  if (hum >= 0.0f && hum <= 100.0f) {
    print_value("%.0f%%", hum, ui->weather.humidity_label);
  } else {
    lv_label_set_text(ui->weather.humidity_label, "Err");
  }

  // ── строка 2 — feels like + UV ────────────────────────
  float feels = (float)get_apparent_temperature();

  if (feels > -60.0f && feels < 60.0f) {
    print_value("~%.1f°C", feels, ui->weather.feels_like_label);
  } else {
    lv_label_set_text(ui->weather.feels_like_label, "Err");
  }

  float uv = (float)get_uv_index();
  if (uv >= 0.0f && uv <= 20.0f) {
    snprintf(ui->string_buffer, sizeof(ui->string_buffer), "%.0f", uv);
    lv_label_set_text(ui->weather.uv_label, ui->string_buffer);
    lv_obj_set_style_text_color(ui->weather.uv_label, calc_uv_color(uv), 0);
  } else {
    lv_label_set_text(ui->weather.uv_label, "UV -");
    lv_obj_remove_local_style_prop(ui->weather.uv_label, LV_STYLE_TEXT_COLOR,
                                   0);
  }

  // ── строка 3 — осадки + вероятность ──────────────────
  float rain_prob = (float)get_precipitation_probability();

  const char *precip_text;
  if (rain_prob < 0.0f) {
    precip_text = "unwahrscheinli.";
  } else if (rain_prob < 30.0f) {
    precip_text = "unwahrscheinli.";
  } else if (rain_prob < 60.0f) {
    precip_text = "moeglich";
  } else if (rain_prob < 80.0f) {
    precip_text = "wahrscheinlich";
  } else {
    precip_text = "erwartet";
  }
  lv_label_set_text(ui->weather.rain_label, precip_text);

  // ── строка 4 — ветер + давление ──────────────────────
  float wind = (float)get_weather_wind();
  float press = (float)get_surface_pressure();

  if (wind >= 0.0f) {
    print_value("%.0fm/s", wind, ui->weather.wind_label);
  } else {
    lv_label_set_text(ui->weather.wind_label, "- m/s");
  }

  if (press > 800.0f && press < 1100.0f) {
    print_value("%.0f", press, ui->weather.pressure_label);
  } else {
    lv_label_set_text(ui->weather.pressure_label, "- hPa");
  }
}

void update_co2_status_label(ui_main_menu_t *ui, uint16_t co2_ppm) {
  if (!ui->co2.co2_status_label)
    return;

  const char *text;
  lv_color_t color;

  if (co2_ppm < 800) {
    text = "GUT";
    color = lv_palette_main(LV_PALETTE_GREEN);
  } else if (co2_ppm < 1200) {
    text = "MITTEL";
    color = lv_palette_main(LV_PALETTE_YELLOW);
  } else if (co2_ppm < 1600) {
    text = "HOCH";
    color = lv_palette_main(LV_PALETTE_ORANGE);
  } else {
    text = "GEFAHR";
    color = lv_palette_main(LV_PALETTE_RED);
  }

  lv_label_set_text(ui->co2.co2_status_label, text);
  lv_obj_set_style_text_color(ui->co2.co2_status_label, color, 0);
  lv_obj_set_style_bg_color(ui->co2.co2_status_label, color, 0);
  lv_obj_set_style_border_color(ui->co2.co2_status_label, color, 0);
}

void update_block_bot_middle(ui_main_menu_t *ui) {
  static uint16_t cnt_chart_co2 = 0;
  cnt_chart_co2++;            // 1 tick == 1 sec
  if (cnt_chart_co2 > 3600) { // equal 1 hour
    int16_t raw_chart = (int16_t)get_co2_sgp30();
    int16_t scaled = co2_scale(raw_chart, ui->settings.switch_.co2_mode);
    lv_chart_set_next_value(ui->co2.chart, ui->co2.series_co2, scaled);
    cnt_chart_co2 = 0;
  }
  draw_symbol_wifi(ui);
}



void update_block_bot_right(ui_main_menu_t *ui) {
  static uint16_t cnt_weather = 0;
  cnt_weather++;
  if (cnt_weather < 2) {
    draw_weather(ui);
  }
  if (cnt_weather > UPDATE_WEATHER_SEC + 2) {
    draw_weather(ui);
    cnt_weather = 2;
  }
}

void sensor_temperature_record_values(ui_main_menu_t *ui) {
  static uint16_t cnt_sensor_history = 0;
  cnt_sensor_history++;
  if (cnt_sensor_history >= SENSOR_RECORD_INTERVAL) {
    sensor_temperature_history_push(&ui->sensor.popup.history);
    cnt_sensor_history = 0;
  }
}
