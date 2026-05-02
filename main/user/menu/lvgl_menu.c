
#include "user/menu/lvgl_menu.h"
#include "esp_heap_caps.h"
// #include "esp_log_timestamp.h"
#include "font/lv_symbol_def.h"
#include "lvgl_port.h"
#include "lvgl_user_config.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"

#include "esp_wifi.h"
#include "waveshare_rgb_lcd_port.h"
#include <esp_log.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#include "user/periphery/nvs_user.h"
#include "user/periphery/open_meteo.h"
#include "user/periphery/periphery.h"
#include "user/periphery/wifi.h"

extern const city_t cities_de[];
extern lv_font_t my_symbols;
extern lv_font_t my_time_font;
extern wifi_ap_record_t ap_info;
extern sht41_data_t sht41_data;
extern sgp30_data_t sgp30_data;
extern forecast_data_t forecast_data;

ui_main_menu_t ui = {0};

int standby_touched = 0; // callback for extern touch driver

#define FEELS_LIKE_MIN_DIFF 1.5f

LV_IMG_DECLARE(sun_48_48);
LV_IMG_DECLARE(moon_42_42);
LV_IMG_DECLARE(cloud_small_70_35);
LV_IMG_DECLARE(cloud_mid_90_45);
LV_IMG_DECLARE(cloud_big_110_50);
LV_IMG_DECLARE(cloud_thin_80_30);
LV_IMG_DECLARE(wind_60_50);
LV_IMG_DECLARE(rain_drop_heavy_9_22);
LV_IMG_DECLARE(snow_flake_2_15_15);

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

static void create_text(const char *text, lv_obj_t *parent, uint16_t theme,
                        lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                        ui_main_menu_t *ui) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_text");
    return;
  }
  lv_obj_t *cont = lv_label_create(parent);
  lv_obj_remove_style_all(cont);
  switch (theme) {
  case STYLE_TEXT_SMALL:

    lv_obj_add_style(cont, &ui->font.medium_32, 0);
    break;
  case STYLE_TEXT_TITLE:
    lv_obj_add_style(cont, &ui->font.very_small_20, 0);
    break;
  }
  lv_label_set_text(cont, text);
  lv_obj_align(cont, align, x_ofs, y_ofs);
}

static lv_obj_t *create_label(lv_obj_t *parent, const char *text,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_label");
    return NULL;
  }

  lv_obj_t *con = lv_label_create(parent);
  lv_obj_align(con, align, x_ofs, y_ofs);
  lv_label_set_text(con, text);
  return con;
}

static lv_obj_t *create_meter(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs, ui_main_menu_t *ui) {
  const int16_t start_value = BLOCK_TOP_MID_START_CO2_LEFT_PART;
  const int16_t start_value_1 = BLOCK_TOP_MID_END_CO2_LEFT_PART;
  const int16_t end_value = BLOCK_TOP_MID_START_CO2_RIGHT_PART;
  const int16_t end_value_1 = BLOCK_TOP_MID_END_CO2_RIGHT_PART;

  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_meter");
    return NULL;
  }

  lv_obj_t *meter = lv_meter_create(parent);
  lv_obj_set_size(meter, w, h);
  lv_obj_align(meter, align, x_ofs, y_ofs);

  /*Add a scale first*/

  lv_meter_scale_t *scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_ticks(meter, scale, 41, 2, 10,
                           lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_range(meter, scale, MIN_VALUE_CO2, MAX_VALUE_CO2, 250,
                           145);

  /*Add a blue arc to the start*/
  ui->co2.indicator =
      lv_meter_add_arc(meter, scale, 4, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, start_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, start_value_1);

  /*Make the tick lines blue at the start of the scale*/
  ui->co2.indicator =
      lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_GREEN),
                               lv_palette_main(LV_PALETTE_GREEN), false, 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, start_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, start_value_1);

  /*Add a red arc to the end*/
  ui->co2.indicator =
      lv_meter_add_arc(meter, scale, 4, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, end_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, end_value_1);

  /*Make the tick lines red at the end of the scale*/
  ui->co2.indicator =
      lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED),
                               lv_palette_main(LV_PALETTE_RED), false, 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, end_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, end_value_1);

  ui->co2.needle_arc =
      lv_meter_add_arc(meter, scale, 12, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.needle_arc, MIN_VALUE_CO2);
  lv_meter_set_indicator_end_value(meter, ui->co2.needle_arc, MIN_VALUE_CO2);

  /*Add a needle line indicator*/
  ui->co2.indicator = lv_meter_add_needle_line(
      meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);

  return meter;
}

static lv_obj_t *create_chart(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_meter");
    return NULL;
  }

  lv_obj_t *chart = lv_chart_create(parent);
  lv_obj_set_size(chart, w, h);
  lv_obj_align(chart, align, x_ofs, y_ofs);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, 24); // 24 точки = 24 часа
  lv_chart_set_div_line_count(chart, 3, 6);
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, MIN_VALUE_CO2,
                     MAX_VALUE_CO2);

  ui.co2.series_co2 = lv_chart_add_series(chart, lv_color_hex(0x2E86C1),
                                          LV_CHART_AXIS_PRIMARY_Y);

  return chart;
}

void print_wday(uint8_t wday, ui_main_menu_t *ui) {
  // При отсутствии WiFi прячем highlight
  if (wday == WDAY_KEIN_WLAN) {
    if (ui->time.wday_highlight)
      lv_obj_add_flag(ui->time.wday_highlight, LV_OBJ_FLAG_HIDDEN);
    return;
  }
  if (wday >= 7) {
    ESP_LOGE(TAG, "ERROR print_wday: invalid wday=%d", wday);
    return;
  }
  if (ui->time.wday_labels[wday] == NULL || ui->time.wday_highlight == NULL) {
    ESP_LOGE(TAG, "ERROR print_wday: null ptr");
    return;
  }
  // показываем highlight и перемещаем к текущему дню
  static const uint8_t wday_to_idx[7] = {6, 0, 1, 2, 3, 4, 5}; // So→6, Mo→0
  lv_obj_clear_flag(ui->time.wday_highlight, LV_OBJ_FLAG_HIDDEN);
  lv_obj_align_to(ui->time.wday_highlight,
                  ui->time.wday_labels[wday_to_idx[wday]], LV_ALIGN_CENTER, 0,
                  0);
}

void print_time(uint8_t time_hour, uint8_t time_minute, ui_main_menu_t *ui) {
  lv_obj_t *parent = ui->time.hour_minute_label;
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR print_time");
    return;
  }
  if (get_wifi_status() == WIFI_CONNECTED) {
    if (time_hour < 10 && time_minute < 10) {
      sprintf(ui->string_buffer, "0%d:0%d", (int)time_hour, (int)time_minute);
    } else if (time_hour > 9 && time_minute < 10) {
      sprintf(ui->string_buffer, "%d:0%d", (int)time_hour, (int)time_minute);
    } else if (time_hour < 10 && time_minute > 9) {
      sprintf(ui->string_buffer, "0%d:%d", (int)time_hour, (int)time_minute);
    } else {
      sprintf(ui->string_buffer, "%d:%d", (int)time_hour, (int)time_minute);
    }
  } else {
    sprintf(ui->string_buffer, "0%d:0%d", (int)0, 0);
  }

  lv_label_set_text(parent, ui->string_buffer);
}

void print_mday(uint8_t date_day, uint8_t date_month, ui_main_menu_t *ui) {
  lv_obj_t *parent = ui->time.mday_month_label;
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR print_mday");
    return;
  }
  if (get_wifi_status() == WIFI_CONNECTED) {
    if (date_day < 10 && date_month < 10) {
      sprintf(ui->string_buffer, "0%d.0%d", (int)date_day, (int)date_month);
    } else if (date_day > 9 && date_month < 10) {
      sprintf(ui->string_buffer, "%d.0%d", (int)date_day, (int)date_month);
    } else if (date_day < 10 && date_month > 9) {
      sprintf(ui->string_buffer, "0%d.%d", (int)date_day, (int)date_month);
    } else {
      sprintf(ui->string_buffer, "%d.%d", (int)date_day, (int)date_month);
    }
  } else {
    sprintf(ui->string_buffer, "0%d.0%d", 0, 0);
  }
  lv_label_set_text(parent, ui->string_buffer);
}

// Алгоритм Гаусса — возвращает день и месяц Пасхи для заданного года
static void get_easter(uint16_t year, uint8_t *out_day, uint8_t *out_month) {
  int a = year % 19;
  int b = year % 4;
  int c = year % 7;
  int k = year / 100;
  int p = (13 + 8 * k) / 25;
  int q = k / 4;
  int M = (15 - p + k - q) % 30;
  int N = (4 + k - q) % 7;
  int d = (19 * a + M) % 30;
  int e = (2 * b + 4 * c + 6 * d + N) % 7;
  int day = 22 + d + e;
  int month = 3;
  if (day > 31) {
    day -= 31;
    month = 4;
    // исключения алгоритма
    if (day == 26)
      day = 19;
    if (day == 25 && d == 28 && e == 6 && a > 10)
      day = 18;
  }
  *out_day = (uint8_t)day;
  *out_month = (uint8_t)month;
}

// Возвращает название федерального праздника Германии или NULL
static const char *get_german_holiday(uint8_t day, uint8_t month,
                                      uint16_t year) {
  // Фиксированные праздники
  if (day == 1 && month == 1)
    return "Neujahr"; // 7  ✓
  if (day == 1 && month == 5)
    return "Tag d. Arbeit"; // 13 ✗ — "Maifeiertag"? (12 ✓)
  if (day == 3 && month == 10)
    return "Tag d. Einheit"; // 14 ✗ — "Dt. Einheit"? (11 ✓)
  if (day == 25 && month == 12)
    return "1. Weihnacht"; // 11 ✓
  if (day == 26 && month == 12)
    return "2. Weihnacht"; // 11 ✓

  // Праздники относительно Пасхи
  uint8_t e_day, e_month;
  get_easter(year, &e_day, &e_month);

  // Для расчёта смещений переводим Пасху в день года
  // Используем простой сдвиг через дату: вычитаем/прибавляем дни
  // Проще: сравниваем день+месяц с Пасхой ± смещение
  // Функция: easter_offset → проверяем совпадение
  // Считаем день года для Пасхи и для проверяемой даты
  static const uint8_t days_in_month[13] = {0,  31, 28, 31, 30, 31, 30,
                                            31, 31, 30, 31, 30, 31};
  uint16_t easter_doy = e_day;
  for (int m = 1; m < e_month; m++)
    easter_doy += days_in_month[m];

  uint16_t check_doy = day;
  for (int m = 1; m < month; m++)
    check_doy += days_in_month[m];

  int16_t diff = (int16_t)check_doy - (int16_t)easter_doy;

  if (diff == -2)
    return "Karfreitag"; // 10 ✓
  if (diff == 0)
    return "Ostersonntag"; // 12 ✓
  if (diff == 1)
    return "Ostermontag"; // 11 ✓
  if (diff == 39)
    return "Himmelfahrt"; // 11 ✓
  if (diff == 49)
    return "Pfingstsonntag"; // 14 ✗ — "Pfingstso."?  (10 ✓)
  if (diff == 50)
    return "Pfingstmontag"; // 13 ✗ — "Pfingstmo."?  (10 ✓)
  if (diff == 60)
    return "Fronleichnam"; // 12 ✓

  return NULL;
}

void print_holiday(uint8_t day, uint8_t month, uint16_t year,
                   ui_main_menu_t *ui) {
  if (ui->time.holiday_label == NULL) {
    ESP_LOGE(TAG, "ERROR print_holiday: null ptr");
    return;
  }
  const char *holiday = get_german_holiday(day, month, year);
  if (holiday == NULL) {
    lv_obj_add_flag(ui->time.holiday_label, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_label_set_text_fmt(ui->time.holiday_label, MY_BELL_SYMBOL " %s",
                          holiday);
    lv_obj_clear_flag(ui->time.holiday_label, LV_OBJ_FLAG_HIDDEN);
  }
}

static lv_obj_t *create_icon(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                             lv_align_t align, lv_coord_t x_ofs,
                             lv_coord_t y_ofs, const char *symbol,
                             ui_main_menu_t *ui) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_icon");
    return NULL;
  }
  lv_obj_t *cont =
      lv_obj_create(parent); // lv_obj вместо lv_btn — нет дефолтных стилей
  lv_obj_remove_style_all(cont);
  lv_obj_set_size(cont, w, h);
  lv_obj_align(cont, align, x_ofs, y_ofs);

  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, symbol);
  lv_obj_add_style(label, &ui->font.icon, 0);
  lv_obj_center(label);

  return cont;
}
static lv_obj_t *create_btn_icon(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                                 lv_align_t align, lv_coord_t x_ofs,
                                 lv_coord_t y_ofs, lv_event_cb_t event_cb,
                                 void *user_data, const char *symbol,
                                 lv_style_t *icon_style,
                                 const lv_font_t *font) { // добавили font
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_btn_icon");
    return NULL;
  }
  lv_obj_t *cont = lv_btn_create(parent);
  lv_obj_remove_style_all(cont);
  lv_obj_set_size(cont, w, h);
  lv_obj_align(cont, align, x_ofs, y_ofs);
  lv_obj_add_event_cb(cont, event_cb, LV_EVENT_CLICKED, user_data);

  lv_obj_t *label = lv_label_create(cont);
  lv_label_set_text(label, symbol);
  if (icon_style)
    lv_obj_add_style(label, icon_style, 0);
  if (font)
    lv_obj_set_style_text_font(label, font, 0);
  lv_obj_set_style_text_color(label, lv_color_hex(0x4A80B8), 0);
  lv_obj_center(label);

  return cont;
}

static lv_obj_t *create_btn_cb(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                               lv_align_t align, lv_coord_t x_ofs,
                               lv_coord_t y_ofs, lv_event_cb_t event_cb,
                               void *user_data) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_btn_cb");
    // return;
  }
  lv_obj_t *cont = lv_btn_create(parent);
  lv_obj_set_size(cont, w, h);
  lv_obj_align(cont, align, x_ofs, y_ofs);
  lv_obj_add_event_cb(cont, event_cb, LV_EVENT_CLICKED, user_data);

  return cont;
}

static lv_obj_t *create_background(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                                   lv_align_t align, lv_coord_t x_ofs,
                                   lv_coord_t y_ofs) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_button");
    // return;
  }
  lv_obj_t *cont = lv_obj_create(parent);
  if (cont == NULL)
    return NULL;
  lv_obj_set_size(cont, w, h);
  lv_obj_align(cont, align, x_ofs, y_ofs);
  return cont;
}

static void anim_sun_moon_orbit(void *var, int32_t angle) {
  lv_obj_t *obj = var;

  const int cx = 60; // центр области 250x225
  const int cy = 60;
  const int r = 20;

  float rad = angle * 3.14159f / 1800.0f; // 0..3600

  lv_coord_t x = cx + r * cosf(rad);
  lv_coord_t y = cy - r * sinf(rad);

  lv_obj_align(obj, BLOCK_BOT_RIGHT_ALIGN_WEATHER_ANIM,
               x + BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_SUN_MOON,
               y + BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_SUN_MOON);
}

static lv_obj_t *create_anim_image_orbit(const lv_img_dsc_t *img_src,
                                         lv_obj_t *parent, lv_align_t align,
                                         lv_coord_t x_ofs, lv_coord_t y_ofs) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_anim_image_orbit");
    // return;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);
  lv_obj_align(img, align, x_ofs, y_ofs);
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, img);
  lv_anim_set_exec_cb(&a, anim_sun_moon_orbit);
  lv_anim_set_values(&a, 0, 3600);
  lv_anim_set_time(&a, BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_SUN_MOON);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

static void cloud_anim_cb(void *var, int32_t v) {
  cloud_anim_t *c = (cloud_anim_t *)var;
  // [FIX D] Проверка перед lv_obj_set_pos
  if (!c || !c->obj || !lv_obj_is_valid(c->obj))
    return;

  int32_t x = c->base_x + (lv_trigo_sin(v) * c->amplitude) / LV_TRIGO_SIN_MAX;
  lv_obj_set_pos(c->obj, x, c->base_y);
}

lv_obj_t *create_cloud_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                            lv_coord_t x_ofs, lv_coord_t y_ofs,
                            int16_t amplitude, uint32_t speed) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_cloud_anim: parent is NULL");
    return NULL;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);
  lv_obj_set_pos(img, x_ofs, y_ofs);

  static cloud_anim_t cloud_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_CLOUDS];
  static uint8_t pool_index = 0;

  // [FIX A] Проверка переполнения — раньше этой строки не было!
  if (pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_CLOUDS)
    pool_index = 0;

  cloud_anim_t *c = &cloud_pool[pool_index++];
  c->obj = img;
  c->base_x = x_ofs;
  c->base_y = y_ofs;
  c->amplitude = amplitude;

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, c);
  lv_anim_set_exec_cb(&a, cloud_anim_cb);
  lv_anim_set_values(&a, 0, 360);
  lv_anim_set_time(&a, speed);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

static void wind_anim_cb(void *var, int32_t v) {
  wind_anim_t *w = (wind_anim_t *)var;
  if (!w || !w->obj || !lv_obj_is_valid(w->obj))
    return;

  if (w->container_w < 1)
    w->container_w = 1;

  // [FIX #2] Правильный диапазон для lv_trigo_sin: 0..3600
  // v идёт от -img_w до container_w+img_w
  // нормализуем в 0..3600 для синуса
  int32_t sin_arg = ((v + w->img_w) * 3600) / (w->container_w + w->img_w * 2);
  sin_arg = sin_arg % 3600;

  lv_coord_t x =
      v; // [FIX #3] x = v напрямую, диапазон уже с отрицательным стартом
  lv_coord_t y = w->base_y +
                 (lv_trigo_sin(sin_arg) * w->turbulence) / LV_TRIGO_SIN_MAX / 3;

  lv_obj_set_pos(w->obj, x, y);
}

lv_obj_t *create_wind_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                           lv_coord_t x_ofs, lv_coord_t y_ofs,
                           int16_t turbulence, uint32_t speed) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_wind_anim: parent is NULL");
    return NULL;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);
  lv_obj_update_layout(parent); // ← форсируе

  // [FIX #1] Снимаем авто-позиционирование
  lv_obj_clear_flag(img, LV_OBJ_FLAG_SNAPPABLE);
  lv_obj_set_pos(img, x_ofs, y_ofs);

  lv_coord_t container_w = lv_obj_get_width(parent);
  lv_coord_t img_w = lv_obj_get_width(img);

  static wind_anim_t wind_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_WINDS];
  static uint8_t pool_index = 0;

  if (pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_WINDS)
    pool_index = 0;

  wind_anim_t *w = &wind_pool[pool_index++];
  w->obj = img;
  w->base_y = y_ofs;
  w->container_w = container_w;
  w->img_w = img_w;
  w->turbulence = turbulence;

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, w);
  lv_anim_set_exec_cb(&a, wind_anim_cb);
  // [FIX #3] Старт с -img_w (левее контейнера), конец за правым краем
  lv_anim_set_values(&a, -img_w, container_w);
  lv_anim_set_time(&a, speed);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

static void rain_snow_anim_cb(void *var, int32_t v) {
  rain_snow_anim_t *r = (rain_snow_anim_t *)var;
  // [FIX D] Проверка перед lv_obj_set_pos
  if (!r || !r->obj || !lv_obj_is_valid(r->obj))
    return;

  int32_t value = v + r->phase;
  lv_coord_t range_x = r->container_w + r->img_w;
  lv_coord_t range_y = r->container_h + r->img_h;

  if (range_x == 0 || range_y == 0)
    return;

  lv_coord_t x = (value % range_x) - r->img_w;
  lv_coord_t y = ((value * r->slope) % range_y) - r->img_h;
  lv_obj_set_pos(r->obj, x, y + 30);
}

lv_obj_t *create_rain_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                           int16_t slope, uint32_t speed) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_rain_anim: parent is NULL");
    return NULL;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);

  lv_coord_t container_w = lv_obj_get_width(parent);
  lv_coord_t container_h = lv_obj_get_height(parent);
  lv_coord_t img_w = lv_obj_get_width(img);
  lv_coord_t img_h = lv_obj_get_height(img);

  // [FIX B] Отдельный пул только для дождя
  static rain_snow_anim_t rain_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS];
  static uint8_t rain_pool_index = 0;

  if (rain_pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS)
    rain_pool_index = 0;

  rain_snow_anim_t *r = &rain_pool[rain_pool_index++];
  r->obj = img;
  r->container_w = container_w;
  r->container_h = container_h;
  r->img_w = img_w;
  r->img_h = img_h;
  r->slope = slope;
  r->phase = lv_rand(0, container_w);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, r);
  lv_anim_set_exec_cb(&a, rain_snow_anim_cb);
  lv_anim_set_values(&a, 0, container_w + img_w);
  lv_anim_set_time(&a, speed);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

lv_obj_t *create_snow_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                           int16_t slope, uint32_t speed) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_snow_anim: parent is NULL");
    return NULL;
  }

  lv_obj_t *img = lv_img_create(parent);
  if (img == NULL)
    return NULL;
  lv_img_set_src(img, img_src);

  lv_coord_t container_w = lv_obj_get_width(parent);
  lv_coord_t container_h = lv_obj_get_height(parent);
  lv_coord_t img_w = lv_obj_get_width(img);
  lv_coord_t img_h = lv_obj_get_height(img);

  // [FIX B] Отдельный пул только для снега
  static rain_snow_anim_t snow_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS];
  static uint8_t snow_pool_index = 0;

  if (snow_pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS)
    snow_pool_index = 0;

  rain_snow_anim_t *r = &snow_pool[snow_pool_index++];
  r->obj = img;
  r->container_w = container_w;
  r->container_h = container_h;
  r->img_w = img_w;
  r->img_h = img_h;
  r->slope = slope;
  r->phase = lv_rand(0, container_w);

  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, r);
  lv_anim_set_exec_cb(&a, rain_snow_anim_cb);
  lv_anim_set_values(&a, 0, container_w + img_w);
  lv_anim_set_time(&a, speed);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sensor_history_push(sensor_history_t *h) {
  // Записываем температуру с одним знаком после запятой (×10)
  // get_temperature_aht10() возвращает float — умножаем и округляем
  float t = get_temperature_sht41();
  h->temperature[h->head] =
      (int16_t)(t >= 0 ? (t * 10.0f + 0.5f) : (t * 10.0f - 0.5f));
  h->humidity[h->head] = get_humidity_sht41();

  h->head = (h->head + 1) % SENSOR_HISTORY_POINTS;

  if (h->count < SENSOR_HISTORY_POINTS)
    h->count++;
}
static void sensor_history_get(const sensor_history_t *h, uint16_t idx,
                               int16_t *temp_x10, uint8_t *hum) {
  // Вычисляем реальный индекс в кольцевом буфере
  // head указывает на следующую ячейку для записи
  // старейшая точка: (head - count + POINTS) % POINTS
  uint16_t real_idx = (h->head - h->count + idx + SENSOR_HISTORY_POINTS) %
                      SENSOR_HISTORY_POINTS;
  *temp_x10 = h->temperature[real_idx];
  *hum = h->humidity[real_idx];
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////temperature weather events
static void ui_create_weather_forecast_popup(ui_main_menu_t *ui) {

  int16_t popup_width = LVGL_PORT_H_RES - 10;
  int16_t popup_height = LVGL_PORT_V_RES - 10;

  // --- Фон ---
  ui->weather.forecast_popup.popup = create_background(
      ui->screen, popup_width, popup_height, POPUP_WINDOW_ALIGN, 0, 0);
  lv_obj_set_scrollbar_mode(ui->weather.forecast_popup.popup,
                            LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_style(ui->weather.forecast_popup.popup, &ui->style.popup, 0);

  // --- Заголовок ---
  create_text("Wettervorhersage (3 Tage)", ui->weather.forecast_popup.popup,
              STYLE_TEXT_SMALL, LV_ALIGN_TOP_MID, 0, 5, ui);

  // --- Кнопка закрытия ---
  ui->weather.forecast_popup.btn_close = create_btn_cb(
      ui->weather.forecast_popup.popup, 50, 50, LV_ALIGN_TOP_RIGHT, -5, -5,
      btn_weather_close_forecast_popup_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->weather.forecast_popup.btn_close,
                              LV_SYMBOL_HOME, 0);

  // --- 3 колонки для 3 дней ---
  const char *day_names[FORECAST_DAYS] = {"Heute", "Morgen", "+2 Tage"};
  int16_t col_width = (popup_width - 40) / FORECAST_DAYS;
  int16_t col_y_start = 70;
  int16_t row_h = 55; // высота строки

  for (int i = 0; i < FORECAST_DAYS; i++) {
    int16_t col_x = -popup_width / 2 + 20 + i * col_width + col_width / 2;

    // --- Разделитель между колонками ---
    if (i > 0) {
      lv_obj_t *sep = lv_obj_create(ui->weather.forecast_popup.popup);
      lv_obj_remove_style_all(sep);
      lv_obj_set_size(sep, 1, popup_height - 80);
      lv_obj_set_style_bg_color(sep, lv_color_hex(0x444444), 0);
      lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
      lv_obj_align(sep, LV_ALIGN_TOP_MID, -popup_width / 2 + 20 + i * col_width,
                   col_y_start - 10);
    }

    // --- День ---
    ui->weather.forecast_popup.day_label[i] =
        create_label(ui->weather.forecast_popup.popup, day_names[i],
                     LV_ALIGN_TOP_MID, col_x, col_y_start);
    lv_obj_add_style(ui->weather.forecast_popup.day_label[i],
                     &ui->font.medium_32, 0);

    // --- Погодный код ---
    ui->weather.forecast_popup.wcode_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h);
    lv_obj_add_style(ui->weather.forecast_popup.wcode_label[i],
                     &ui->font.very_small_20, 0);

    // --- Температура ---
    ui->weather.forecast_popup.temp_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h * 2);
    lv_obj_add_style(ui->weather.forecast_popup.temp_label[i],
                     &ui->font.medium_32, 0);

    // --- Влажность ---
    ui->weather.forecast_popup.humidity_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h * 3);
    lv_obj_add_style(ui->weather.forecast_popup.humidity_label[i],
                     &ui->font.very_small_20, 0);

    // --- Восход ---
    ui->weather.forecast_popup.sunrise_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h * 4);
    lv_obj_add_style(ui->weather.forecast_popup.sunrise_label[i],
                     &ui->font.very_small_20, 0);

    // --- Закат ---
    ui->weather.forecast_popup.sunset_label[i] =
        create_label(ui->weather.forecast_popup.popup, "-", LV_ALIGN_TOP_MID,
                     col_x, col_y_start + row_h * 5);
    lv_obj_add_style(ui->weather.forecast_popup.sunset_label[i],
                     &ui->font.very_small_20, 0);

    // --- Заполняем данными если они есть ---
    if (forecast_data.valid) {
      forecast_day_t *d = &forecast_data.day[i];
      char buf[32];

      // температура
      snprintf(buf, sizeof(buf), "%d / %d °C", d->temp_min, d->temp_max);
      lv_label_set_text(ui->weather.forecast_popup.temp_label[i], buf);

      // погода
      lv_label_set_text(ui->weather.forecast_popup.wcode_label[i],
                        weathercode_to_text(d->weathercode));

      // влажность
      snprintf(buf, sizeof(buf), "%d %%", d->humidity_max);
      lv_label_set_text(ui->weather.forecast_popup.humidity_label[i], buf);

      // восход — unix time → HH:MM
      time_t sr = (time_t)d->sunrise;
      struct tm *sr_tm = localtime(&sr);
      snprintf(buf, sizeof(buf), LV_SYMBOL_UP " %02d:%02d", sr_tm->tm_hour,
               sr_tm->tm_min);
      lv_label_set_text(ui->weather.forecast_popup.sunrise_label[i], buf);

      // закат
      time_t ss = (time_t)d->sunset;
      struct tm *ss_tm = localtime(&ss);
      snprintf(buf, sizeof(buf), LV_SYMBOL_DOWN " %02d:%02d", ss_tm->tm_hour,
               ss_tm->tm_min);
      lv_label_set_text(ui->weather.forecast_popup.sunset_label[i], buf);
    }
  }
}

static void block_bot_left_open_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  if (ui->weather.forecast_popup.popup != NULL &&
      lv_obj_is_valid(ui->weather.forecast_popup.popup))
    return;

  hide_all_blocks(ui);
  ui_create_weather_forecast_popup(ui);
}
/////////////////////////////////////////////////temperature inside events
static void block_top_left_open_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  // [FIX] Guard от двойного открытия
  if (ui->sensor.popup.popup != NULL && lv_obj_is_valid(ui->sensor.popup.popup))
    return;

  hide_all_blocks(ui);
  ui_create_sensor_history_popup(ui);
}
static void btn_sensor_close_history_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_del(ui->sensor.popup.popup);

  ui->sensor.popup.popup = NULL;
  ui->sensor.popup.btn_close = NULL;
  ui->sensor.popup.chart = NULL;

  show_all_blocks(ui);
}

static void btn_weather_close_forecast_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_del(ui->weather.forecast_popup.popup);
  ui->weather.forecast_popup.popup = NULL;
  ui->weather.forecast_popup.btn_close = NULL;

  show_all_blocks(ui);
}

/////////////////////////////////////////////////temperature inside events

static void ui_create_sensor_history_popup(ui_main_menu_t *ui) {
  // --- Фон popup ---
  int16_t popup_width = LVGL_PORT_H_RES - 10;
  int16_t popup_height = LVGL_PORT_V_RES - 10;

  ui->sensor.popup.popup = create_background(
      ui->screen, popup_width, popup_height, POPUP_WINDOW_ALIGN, 0, 0);
  lv_obj_add_style(ui->sensor.popup.popup, &ui->style.popup, 0);
  lv_obj_set_scrollbar_mode(ui->sensor.popup.popup, LV_SCROLLBAR_MODE_OFF);
  // --- Заголовок ---
  create_text("Temperatur / Feuchtigkeit (12 std)", ui->sensor.popup.popup,
              STYLE_TEXT_SMALL, LV_ALIGN_TOP_MID, 0, 5, ui);

  // --- Кнопка закрытия ---
  ui->sensor.popup.btn_close =
      create_btn_cb(ui->sensor.popup.popup, 50, 50, LV_ALIGN_TOP_RIGHT, -5, -5,
                    btn_sensor_close_history_popup_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->sensor.popup.btn_close, LV_SYMBOL_HOME, 0);

  // --- График ---
  int16_t chart_width = popup_width - 160;
  int16_t chart_height = popup_height - 125;

  lv_obj_t *chart = lv_chart_create(ui->sensor.popup.popup);
  lv_obj_set_size(chart, chart_width, chart_height);
  lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, 10);
  lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
  lv_chart_set_point_count(chart, SENSOR_HISTORY_POINTS);

  lv_obj_t *lbl_temp = lv_label_create(ui->sensor.popup.popup);
  lv_label_set_text(lbl_temp, "°C");
  lv_obj_set_style_text_color(lbl_temp, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_set_style_text_font(lbl_temp, &lv_font_montserrat_32, 0);
  lv_obj_align_to(lbl_temp, chart, LV_ALIGN_OUT_LEFT_TOP, -10, -35);

  // Лейбл "%" справа от графика — синий
  lv_obj_t *lbl_hum = lv_label_create(ui->sensor.popup.popup);
  lv_label_set_text(lbl_hum, "%");
  lv_obj_set_style_text_color(lbl_hum, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_set_style_text_font(lbl_hum, &lv_font_montserrat_32, 0);
  lv_obj_align_to(lbl_hum, chart, LV_ALIGN_OUT_RIGHT_TOP, 15, -35);

  // Сетка
  lv_chart_set_div_line_count(chart, 6, 12);

  // Диапазоны осей
  int16_t temperatur_range_min = 10;
  int16_t temperatur_range_max = 35;
  uint8_t humidity_range_min = 20;
  uint8_t humidity_range_max = 70;
  // Ось Y левая  — температура: 10..35 °C (×10 → 100..350)
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, temperatur_range_min,
                     temperatur_range_max);
  // Ось Y правая — влажность: 20..80 %
  lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, humidity_range_min,
                     humidity_range_max);

  // Засечки осей
  lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 5,
                         3,    // major/minor tick length
                         6, 1, // 6 делений, каждая подписана
                         true, 50);
  lv_chart_set_axis_tick(chart, LV_CHART_AXIS_SECONDARY_Y, 5, 3, 6, 1, true,
                         40);

  // --- Серия температуры (левая ось, красная) ---
  lv_chart_series_t *ser_temp = lv_chart_add_series(
      chart, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);

  // --- Серия влажности (правая ось, синяя) ---
  lv_chart_series_t *ser_hum = lv_chart_add_series(
      chart, lv_palette_main(LV_PALETTE_BLUE), LV_CHART_AXIS_SECONDARY_Y);

  // После создания серий — убрать точки совсем, оставить только линию
  lv_obj_set_style_size(chart, 0, LV_PART_INDICATOR); // точки = 0px

  // Толщина линии тонче
  lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);

  // --- Заполняем серии из кольцевого буфера ---
  uint16_t n = ui->sensor.popup.history.count;

  if (n == 0) {
    // Буфер пустой — заполняем LV_CHART_POINT_NONE (пунктир не рисуется)
    for (uint16_t i = 0; i < SENSOR_HISTORY_POINTS; i++) {
      lv_chart_set_value_by_id(chart, ser_temp, i, LV_CHART_POINT_NONE);
      lv_chart_set_value_by_id(chart, ser_hum, i, LV_CHART_POINT_NONE);
    }
  } else {
    // Сначала заполняем пустые слоты в начале (если буфер ещё не полный)
    uint16_t empty_slots = SENSOR_HISTORY_POINTS - n;
    for (uint16_t i = 0; i < empty_slots; i++) {
      lv_chart_set_value_by_id(chart, ser_temp, i, LV_CHART_POINT_NONE);
      lv_chart_set_value_by_id(chart, ser_hum, i, LV_CHART_POINT_NONE);
    }
    // Затем реальные данные из буфера (старые → новые)
    for (uint16_t i = 0; i < n; i++) {
      int16_t temp_x10;
      uint8_t hum;
      sensor_history_get(&ui->sensor.popup.history, i, &temp_x10, &hum);
      int16_t current_temperaure = temp_x10;
      if (current_temperaure < temperatur_range_min * 10) {
        current_temperaure = temperatur_range_min * 10;
      }
      if (current_temperaure > temperatur_range_max * 10) {
        current_temperaure = temperatur_range_max * 10;
      }

      uint8_t current_humidity = hum;
      if (current_humidity < humidity_range_min) {
        current_humidity = humidity_range_min;
      }
      if (current_humidity > humidity_range_max) {
        current_humidity = humidity_range_max;
      }

      lv_chart_set_value_by_id(chart, ser_temp, empty_slots + i,
                               current_temperaure / 10);
      lv_chart_set_value_by_id(chart, ser_hum, empty_slots + i,
                               current_humidity);
    }
  }

  lv_chart_refresh(chart);
  ui->sensor.popup.chart = chart;
}

static void create_block_top_left(ui_main_menu_t *ui) {
  lv_obj_t *bg =
      create_background(ui->screen, BLOCK_TOP_LEFT_WIDTH, BLOCK_TOP_LEFT_HEIGHT,
                        BLOCK_TOP_LEFT_ALIGN_BACKGROUND, BLOCK_TOP_LEFT_X_START,
                        BLOCK_TOP_LEFT_Y_START);
  if (!bg)
    return;
  ui->sensor.screen = bg;
  lv_obj_add_style(ui->sensor.screen, &ui->style.top_left, 0);
  lv_obj_set_scrollbar_mode(ui->sensor.screen, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_event_cb(ui->sensor.screen,
                      block_top_left_open_popup_event_handler, LV_EVENT_CLICKED,
                      ui);

  // --- заголовок ---
  create_text("innen", ui->sensor.screen, STYLE_TEXT_SMALL,
              BLOCK_TOP_LEFT_ALIGN_TITLE, 0, BLOCK_TOP_LEFT_Y_START_TITLE, ui);

  // ═══════════════════════════════════════════
  // СТРОКА 1 — температура (лево) + влажность (право)
  // ═══════════════════════════════════════════

  // иконка температуры
  lv_obj_t *icon =
      create_icon(ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
                  BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
                  BLOCK_TOP_LEFT_X_START_SYMBOLS,
                  BLOCK_TOP_LEFT_Y_START_SYMBOLS_1, MY_TEMPERATURE_SYMBOL, ui);
  if (!icon)
    return;
  ui->sensor.icon_temperature = icon;

  // значение температуры
  lv_obj_t *value = create_label(
      ui->sensor.screen, "0°C", BLOCK_TOP_LEFT_ALIGN_VALUES,
      BLOCK_TOP_LEFT_X_START_VALUES, BLOCK_TOP_LEFT_Y_START_VALUE_1 + 3);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->sensor.temperature_label = value;

  // иконка влажности (правая колонка строки 1)
  icon =
      create_icon(ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
                  BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
                  BLOCK_TOP_LEFT_X_START_SYMBOLS_2_RIGHT,
                  BLOCK_TOP_LEFT_Y_START_SYMBOLS_1, MY_HUMIDITY_SYMBOL, ui);
  if (!icon)
    return;
  ui->sensor.icon_humidity = icon;

  // значение влажности
  value = create_label(ui->sensor.screen, "0%", BLOCK_TOP_LEFT_ALIGN_VALUES,
                       BLOCK_TOP_LEFT_X_START_VALUES_R,
                       BLOCK_TOP_LEFT_Y_START_VALUE_1 + 3);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->sensor.humidity_label = value;

  // ═══════════════════════════════════════════
  // СТРОКА 2 — точка росы + индикатор конденсата
  // ═══════════════════════════════════════════

  // иконка feels like (термометр, левая колонка)
  icon =
      create_icon(ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
                  BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
                  BLOCK_TOP_LEFT_X_START_SYMBOLS,
                  BLOCK_TOP_LEFT_Y_START_SYMBOLS_3, MY_FEELS_LIKE_SYMBOL, ui);
  if (!icon)
    return;
  ui->sensor.icon_feels_like = icon;

  // значение feels like
  value = create_label(ui->sensor.screen, "0°C", BLOCK_TOP_LEFT_ALIGN_VALUES,
                       BLOCK_TOP_LEFT_X_START_VALUES,
                       BLOCK_TOP_LEFT_Y_START_VALUE_3 + 3);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->sensor.feels_like_label = value;

  // иконка точки росы (правая колонка строки 2)
  icon =
      create_icon(ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
                  BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
                  BLOCK_TOP_LEFT_X_START_SYMBOLS_2_RIGHT,
                  BLOCK_TOP_LEFT_Y_START_SYMBOLS_3, MY_SNOWFLAKE_SYMBOL, ui);
  if (!icon)
    return;
  ui->sensor.icon_dew_point = icon;

  // значение точки росы
  value = create_label(ui->sensor.screen, "0°C", BLOCK_TOP_LEFT_ALIGN_VALUES,
                       BLOCK_TOP_LEFT_X_START_VALUES_R,
                       BLOCK_TOP_LEFT_Y_START_VALUE_3 + 3);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->sensor.dew_point_label = value;

  // ═══════════════════════════════════════════
  // СТРОКА 3 — TVOC + кружки качества воздуха
  // ═══════════════════════════════════════════

  // иконка TVOC
  icon =
      create_icon(ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
                  BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
                  BLOCK_TOP_LEFT_X_START_SYMBOLS,
                  BLOCK_TOP_LEFT_Y_START_SYMBOLS_4, MY_TVOC_SYMBOL, ui);
  if (!icon)
    return;
  ui->sensor.icon_tvoc = icon;

  // значение TVOC
  value = create_label(ui->sensor.screen, "0", BLOCK_TOP_LEFT_ALIGN_VALUES,
                       BLOCK_TOP_LEFT_X_START_VALUES + 15,
                       BLOCK_TOP_LEFT_Y_START_VALUE_4 + 3);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->sensor.tvoc_label = value;

  // кружки качества воздуха ●●●○○
  for (int i = 0; i < 5; i++) {
    lv_obj_t *dot = lv_obj_create(ui->sensor.screen);
    lv_obj_set_size(dot, 12, 12);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_bg_color(dot, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_align(dot, LV_ALIGN_TOP_LEFT,
                 BLOCK_TOP_LEFT_X_START_INDICATOR_R + i * 16,
                 BLOCK_TOP_LEFT_Y_START_VALUE_4 + 11);
    ui->sensor.tvoc_dots[i] = dot;
  }
}

static void create_block_top_middle(ui_main_menu_t *ui) {
  /*BLOCK TOP MID*/

  ui->co2.meter = create_meter(
      ui->screen, BLOCK_TOP_MID_WIDTH_CO2_METER, BLOCK_TOP_MID_HEIGHT_CO2_METER,
      BLOCK_TOP_MID_ALIGN_CO2_CHART, BLOCK_TOP_MID_X_START,
      BLOCK_TOP_MID_Y_START, ui);
  if (!ui->co2.meter)
    return;
  lv_obj_add_style(ui->co2.meter, &ui->style.meter_co2, 0);
  lv_meter_set_indicator_value(ui->co2.meter, ui->co2.indicator, 0);
  //	create_text("ppm", ui->screen, STYLE_TEXT_SMALL,
  //			BLOCK_TOP_MID_ALIGN_CO2_CHART, BLOCK_TOP_MID_X_START,
  // meter_co2.y_ofs + 45,ui);

  lv_obj_t *value =
      create_label(ui->screen, "0", BLOCK_TOP_MID_ALIGN_CO2_CHART,
                   BLOCK_TOP_MID_X_START, BLOCK_TOP_MID_Y_START_CO2_VALUE);
  if (!value)
    return;
  ui->co2.co2_label = value;
  lv_obj_add_style(ui->co2.co2_label, &ui->font.large_48, 0);
  /*BLOCK TOP MID*/
}

static void create_block_top_right(ui_main_menu_t *ui) {
  /*BLOCK TOP RIGHT*/

  lv_obj_t *bg = create_background(
      ui->screen, BLOCK_TOP_RIGHT_WIDTH, BLOCK_TOP_RIGHT_HEIGHT,
      BLOCK_TOP_RIGHT_ALIGN_BACKGROUND, BLOCK_TOP_RIGHT_X_START,
      BLOCK_TOP_RIGHT_Y_START);
  if (!bg)
    return;
  ui->time.screen = bg;
  lv_obj_add_style(ui->time.screen, &ui->style.top_right, 0);
  lv_obj_set_scrollbar_mode(ui->time.screen, LV_SCROLLBAR_MODE_OFF);

  // --- время HH:MM ---
  lv_obj_t *lbl = create_label(
      ui->time.screen, "00:00", BLOCK_TOP_RIGHT_ALIGN_VALUES,
      BLOCK_TOP_RIGHT_X_START_VALUES, BLOCK_TOP_RIGHT_Y_START_VALUE_1);
  if (!lbl)
    return;
  ui->time.hour_minute_label = lbl;
  lv_obj_add_style(ui->time.hour_minute_label, &ui->font.time, 0);

  // --- дата DD.MM ---
  lbl = create_label(ui->time.screen, "00.00", BLOCK_TOP_RIGHT_ALIGN_VALUES,
                     BLOCK_TOP_RIGHT_X_START_VALUES_2,
                     BLOCK_TOP_RIGHT_Y_START_VALUE_2);
  if (!lbl)
    return;
  ui->time.mday_month_label = lbl;
  lv_obj_add_style(ui->time.mday_month_label, &ui->font.time, 0);

  // --- колонка дней недели (Mo Di Mi Do Fr Sa So) ---
  static const char *wday_names[7] = {"Mo", "Di", "Mi", "Do", "Fr", "Sa", "So"};

  for (int i = 0; i < 7; i++) {
    lv_coord_t y = BLOCK_TOP_RIGHT_WDAY_COL_Y_START +
                   i * (BLOCK_TOP_RIGHT_WDAY_COL_ITEM_HEIGHT +
                        BLOCK_TOP_RIGHT_WDAY_COL_ITEM_GAP);
    lbl = create_label(ui->time.screen, wday_names[i],
                       BLOCK_TOP_RIGHT_WDAY_COL_ALIGN,
                       BLOCK_TOP_RIGHT_WDAY_COL_X_START, y);
    if (!lbl)
      return;
    lv_obj_add_style(lbl, &ui->font.small_24, 0);
    lv_obj_set_width(lbl, BLOCK_TOP_RIGHT_WDAY_COL_WIDTH);
    lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
    ui->time.wday_labels[i] = lbl;
  }

  // --- highlight прямоугольник за текущим днём ---
  lv_obj_t *hl = lv_obj_create(ui->time.screen);
  lv_obj_remove_style_all(hl);
  lv_obj_set_size(hl, BLOCK_TOP_RIGHT_WDAY_HIGHLIGHT_WIDTH,
                  BLOCK_TOP_RIGHT_WDAY_HIGHLIGHT_HEIGHT);
  lv_obj_set_style_radius(hl, BLOCK_TOP_RIGHT_WDAY_HIGHLIGHT_RADIUS, 0);
  lv_obj_set_style_border_width(hl, 2, 0);
  lv_obj_set_style_border_color(hl, lv_color_hex(0x5E4E90), 0);
  lv_obj_set_style_bg_opa(hl, LV_OPA_TRANSP, 0);
  // начальная позиция — So (индекс 0), уточнится в первом вызове print_wday
  lv_obj_align_to(hl, ui->time.wday_labels[0], LV_ALIGN_CENTER, 0, 0);
  ui->time.wday_highlight = hl;

  // --- строка праздника внизу ---
  lbl = create_label(ui->time.screen, "", BLOCK_TOP_RIGHT_HOLIDAY_ALIGN, -28,
                     BLOCK_TOP_RIGHT_HOLIDAY_Y_START);
  if (!lbl)
    return;
  lv_obj_add_style(lbl, &ui->font.very_small_20, 0);
  lv_obj_set_style_text_align(lbl, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_add_flag(lbl, LV_OBJ_FLAG_HIDDEN); // скрыт пока нет праздника
  ui->time.holiday_label = lbl;

  /*BLOCK TOP RIGHT*/
}

static void create_block_bot_left(ui_main_menu_t *ui) {
  /*BLOCK BOT LEFT*/
  lv_obj_t *bg =
      create_background(ui->screen, BLOCK_BOT_LEFT_WIDTH, BLOCK_BOT_LEFT_HEIGHT,
                        BLOCK_BOT_LEFT_ALIGN_BACKGROUND, BLOCK_BOT_LEFT_X_START,
                        BLOCK_BOT_LEFT_Y_START);
  if (!bg)
    return;

  ui->weather.screen = bg;
  lv_obj_add_style(ui->weather.screen, &ui->style.bot_left, 0);
  lv_obj_set_scrollbar_mode(ui->weather.screen, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_event_cb(ui->weather.screen,
                      block_bot_left_open_popup_event_handler, LV_EVENT_CLICKED,
                      ui);

  create_text("aussen", ui->weather.screen, STYLE_TEXT_SMALL,
              BLOCK_BOT_LEFT_ALIGN_TITLE, 0, BLOCK_BOT_LEFT_Y_START_TITLE, ui);

  // ═══════════════════════════════════════════
  // СТРОКА 1 — температура (лево) + влажность (право)
  // ═══════════════════════════════════════════
  lv_obj_t *icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_1, MY_TEMPERATURE_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_temperature = icon;

  lv_obj_t *value = create_label(
      ui->weather.screen, "0°C", BLOCK_BOT_LEFT_ALIGN_VALUES,
      BLOCK_BOT_LEFT_X_START_VALUES, BLOCK_BOT_LEFT_Y_START_VALUE_1);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.temperature_label = value;

  // иконка влажности (право)
  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS_R,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_1, MY_HUMIDITY_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_humidity = icon;

  value = create_label(ui->weather.screen, "0%", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES_R,
                       BLOCK_BOT_LEFT_Y_START_VALUE_1);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.humidity_label = value;

  // ═══════════════════════════════════════════
  // СТРОКА 2 — feels like (лево) + UV (право)
  // ═══════════════════════════════════════════
  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_2, MY_FEELS_LIKE_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_feels_like = icon;

  value = create_label(ui->weather.screen, "0°C", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES,
                       BLOCK_BOT_LEFT_Y_START_VALUE_2);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.feels_like_label = value;

  // иконка UV (право)
  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS_R,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_2, MY_SUN_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_uv = icon;

  value = create_label(ui->weather.screen, "UV 0", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES_R,
                       BLOCK_BOT_LEFT_Y_START_VALUE_2);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.uv_label = value;

  // ═══════════════════════════════════════════
  // СТРОКА 3 — осадки (лево) + вероятность (право)
  // ═══════════════════════════════════════════
  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_3, MY_CLOUD_RAIN_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_rain = icon;

  value = create_label(ui->weather.screen, "0mm", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES,
                       BLOCK_BOT_LEFT_Y_START_VALUE_3);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.rain_label = value;

  // иконка вероятности (право)

  // ═══════════════════════════════════════════
  // СТРОКА 4 — ветер (лево) + давление (право)
  // ═══════════════════════════════════════════
  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_4, MY_WIND_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_wind = icon;

  value = create_label(ui->weather.screen, "0m/s", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES,
                       BLOCK_BOT_LEFT_Y_START_VALUE_4);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.wind_label = value;

  // иконка давления (право)
  icon =
      create_icon(ui->weather.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
                  BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
                  BLOCK_BOT_LEFT_X_START_SYMBOLS_R,
                  BLOCK_BOT_LEFT_Y_START_SYMBOLS_4, MY_PRESSURE_SYMBOL, ui);
  if (!icon)
    return;
  ui->weather.icon_pressure = icon;

  value = create_label(ui->weather.screen, "0hPa", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES_R,
                       BLOCK_BOT_LEFT_Y_START_VALUE_4);
  if (!value)
    return;
  lv_obj_add_style(value, &ui->font.small_24, 0);
  ui->weather.pressure_label = value;

  /*BLOCK BOT LEFT*/
}
/////////////////////////////////////////////////wifi events
static void btn_wifi_open_popup_event_handler(lv_event_t *e) {

  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  // [FIX] Guard от двойного открытия
  if (ui->wifi.popup != NULL && lv_obj_is_valid(ui->wifi.popup))
    return;

  hide_all_blocks(ui);
  ui_create_wifi_popup(ui);

  // Заполняем данные СРАЗУ после создания popup (объекты уже валидны)
  if (lv_obj_is_valid(ui->wifi.ssid_label))
    lv_label_set_text(ui->wifi.ssid_label, (char *)ap_info.ssid);

  if (lv_obj_is_valid(ui->wifi.rssi_label))
    lv_label_set_text_fmt(ui->wifi.rssi_label, "%d dBm", ap_info.rssi);
}
static void btn_wifi_close_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  // show_all_blocks ПЕРЕД del — иначе обращаемся к уже удалённым объектам
  show_all_blocks(ui);

  lv_obj_del(ui->wifi.popup);

  // [FIX] Обнуляем ВСЕ дочерние указатели — они удалены вместе с popup
  ui->wifi.popup = NULL;
  ui->wifi.btn_close = NULL;
  ui->wifi.btn_keyboard_ssid = NULL;
  ui->wifi.btn_keyboard_pass = NULL;
  ui->wifi.ssid_label = NULL;
  ui->wifi.pass_label = NULL;
  ui->wifi.rssi_label = NULL;
  ui->wifi.keyboard = NULL;
  ui->wifi.ta_ssid = NULL;
  ui->wifi.ta_pass = NULL;
}
static void btn_keyboard_open_ssid_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  // [FIX] Проверяем валидность перед использованием
  if (!lv_obj_is_valid(ui->wifi.keyboard) || !lv_obj_is_valid(ui->wifi.ta_ssid))
    return;

  lv_keyboard_set_textarea(ui->wifi.keyboard, ui->wifi.ta_ssid);
  set_visible(ui->wifi.keyboard, true);
  set_visible(ui->wifi.ta_ssid, true);
  set_visible(ui->wifi.ta_pass, false); // скрываем pass если был открыт
}
static void btn_keyboard_open_pass_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  // [FIX] Проверяем валидность перед использованием
  if (!lv_obj_is_valid(ui->wifi.keyboard) || !lv_obj_is_valid(ui->wifi.ta_pass))
    return;

  lv_keyboard_set_textarea(ui->wifi.keyboard, ui->wifi.ta_pass);
  set_visible(ui->wifi.keyboard, true);
  set_visible(ui->wifi.ta_pass, true);
  set_visible(ui->wifi.ta_ssid, false); // скрываем ssid если был открыт
}
static void ta_wifi_event_cb(lv_event_t *e) {
  // [FIX] Было &ui (адрес локального параметра!) — теперь ui
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *kb = lv_event_get_target(e);

  if (code == LV_EVENT_READY) {
    if (!lv_obj_is_valid(ui->wifi.ta_ssid) ||
        !lv_obj_is_valid(ui->wifi.ta_pass))
      return;

    const char *ssid = lv_textarea_get_text(ui->wifi.ta_ssid);
    const char *pass = lv_textarea_get_text(ui->wifi.ta_pass);

    if (lv_obj_is_valid(ui->wifi.ssid_label)) {
      lv_label_set_text(ui->wifi.ssid_label, (char *)ssid);
    }

    if (strlen(ssid) < 4) {
      return;
    }

    wifi_connect(ssid, pass);
    // Показываем что идёт подключение
    if (lv_obj_is_valid(ui->wifi.ssid_label))
      lv_label_set_text(ui->wifi.ssid_label, "verbinde...");

    // Запускаем таймер проверки
    lv_timer_t *t = lv_timer_create(wifi_check_timer_cb, 500, ui);
    lv_timer_set_repeat_count(t, 10); // максимум 10 попыток

    set_visible(kb, false);
    set_visible(ui->wifi.ta_ssid, false);
    set_visible(ui->wifi.ta_pass, false);
  }

  if (code == LV_EVENT_CANCEL) {
    set_visible(kb, false);
    if (lv_obj_is_valid(ui->wifi.ta_ssid))
      set_visible(ui->wifi.ta_ssid, false);
    if (lv_obj_is_valid(ui->wifi.ta_pass))
      set_visible(ui->wifi.ta_pass, false);
  }
}
/////////////////////////////////////////////////wifi events

/////////////////////////////////////////////////settings events
static void btn_settings_open_popup_event_handler(lv_event_t *e) {
  // [FIX] Было &ui в user_data при регистрации — исправь и там на ui
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  // [FIX] Guard от двойного открытия
  if (ui->settings.popup != NULL && lv_obj_is_valid(ui->settings.popup))
    return;

  hide_all_blocks(ui);
  ui_create_settings_popup(ui);
}
static void btn_settings_close_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_del(ui->settings.popup);

  ui->settings.popup = NULL;
  ui->settings.btn_close = NULL;
  ui->settings.standby_btnmatrix = NULL;
  ui->settings.standby_desc_label = NULL;
  ui->settings.theme_btnmatrix = NULL;
  ui->settings.theme_desc_label = NULL;
  ui->settings.co2_btnmatrix  = NULL;
ui->settings.co2_desc_label = NULL;

  show_all_blocks(ui);
}
/////////////////////////////////////////////////settings events

/////////////////////////////////////////////////weather settings events
static void btn_weather_open_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  // Сразу логируем что видим
  // ESP_LOGI("WEATHER", "popup open: saved_city = %d",
  // ui->weather.saved_city);

  // Проверяем напрямую из NVS
  // uint16_t nvs_city = 0;
  // weather_settings_load(&nvs_city);
  // ESP_LOGI("WEATHER", "NVS city = %d", nvs_city);

  // [FIX] Guard от двойного открытия
  if (ui->weather.settings_popup.popup != NULL &&
      lv_obj_is_valid(ui->weather.settings_popup.popup))
    return;
  hide_all_blocks(ui);
  ui_create_weather_settings_popup(ui);

  // Показываем последний выбранный город сразу при открытии
  // (ui_create_city_list_weather тоже это делает, но на случай изменений)
  if (lv_obj_is_valid(ui->weather.settings_popup.city_label)) {
    lv_label_set_text(ui->weather.settings_popup.city_label,
                      ui->weather.settings_popup
                          .cities_de[ui->weather.settings_popup.saved_city]
                          .name);
    lv_obj_add_style(ui->weather.settings_popup.city_label, &ui->font.medium_32,
                     0);
  }
}
static void btn_weather_open_list_city_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  if (lv_obj_is_valid(ui->weather.settings_popup.citys_list))
    set_visible(ui->weather.settings_popup.citys_list, true);
}
static void btn_weather_close_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  lv_obj_del(ui->weather.settings_popup.popup);

  // [FIX] Обнуляем ВСЕ дочерние указатели
  // citys_list и city_label — дочерние popup, удалены автоматически
  ui->weather.settings_popup.popup = NULL;
  ui->weather.settings_popup.btn_close = NULL;
  ui->weather.settings_popup.btn_open_city_list = NULL;
  ui->weather.settings_popup.city_label = NULL;
  ui->weather.settings_popup.citys_list = NULL;

  show_all_blocks(ui);
}
static void set_current_city_weather_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  if (code != LV_EVENT_CLICKED)
    return;

  // [FIX] ui передаётся через event user_data (было: передавался индекс i!)
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  if (!ui)
    return;

  // [FIX] Индекс города хранится в самом объекте кнопки через
  // lv_obj_set_user_data Не конфликтует с event user_data — это отдельный
  // слот LVGL объекта
  lv_obj_t *btn = lv_event_get_target(e);
  uint16_t city = (uint16_t)(uintptr_t)lv_obj_get_user_data(btn);

  // Сохраняем выбор — пригодится при следующем открытии popup
  weather_settings_save(city);

  ui->weather.settings_popup.saved_city = city;

  if (lv_obj_is_valid(ui->weather.settings_popup.citys_list))
    set_visible(ui->weather.settings_popup.citys_list, false);

  if (lv_obj_is_valid(ui->weather.settings_popup.city_label))
    lv_label_set_text(ui->weather.settings_popup.city_label,
                      ui->weather.settings_popup.cities_de[city].name);

  build_weather_url(city);
}
/////////////////////////////////////////////////weather settings events

static void ui_create_co2_chart_bot_mid(ui_main_menu_t *ui) {
  //  create_text("CO2 24H", ui->screen, STYLE_TEXT_TITLE,
  //              BLOCK_BOT_MID_ALIGN_CO2_CHART, 0,
  //              BLOCK_BOT_MID_Y_START_TITLE_CO2_CHART, ui);

  ui->co2.chart = create_chart(
      ui->screen, BLOCK_BOT_MID_WIDTH_CO2_CHART, BLOCK_BOT_MID_HEIGHT_CO2_CHART,
      BLOCK_BOT_MID_ALIGN_CO2_CHART, 0, BLOCK_BOT_MID_Y_START_CO2_CHART);

  lv_obj_add_style(ui->co2.chart, &ui->style.chart_co2, 0);
}

static void ui_create_city_list_weather(ui_main_menu_t *ui) {

  // [FIX] Родитель — ui->weather.popup, НЕ ui->screen!
  // Если родитель screen — список не удаляется вместе с popup
  // и зависает поверх интерфейса навсегда
  ui->weather.settings_popup.citys_list =
      lv_list_create(ui->weather.settings_popup.popup);
  lv_obj_set_size(ui->weather.settings_popup.citys_list, 350, 250);
  lv_obj_center(ui->weather.settings_popup.citys_list);

  for (int i = 0; i < ui->weather.settings_popup.city_count; i++) {
    lv_obj_t *btn =
        lv_list_add_btn(ui->weather.settings_popup.citys_list, NULL,
                        ui->weather.settings_popup.cities_de[i].name);

    // [FIX] Индекс города — в объект кнопки (lv_obj_set_user_data)
    lv_obj_set_user_data(btn, (void *)(uintptr_t)i);

    // [FIX] ui — как event user_data (указатель на глобальную структуру)
    // Раньше здесь был (void*)(uintptr_t)i — разыменовывался как указатель!
    lv_obj_add_event_cb(btn, set_current_city_weather_event_handler,
                        LV_EVENT_CLICKED, ui);
  }

  // Показываем текущий выбранный город в лейбле сразу при открытии списка
  if (lv_obj_is_valid(ui->weather.settings_popup.city_label))
    lv_label_set_text(ui->weather.settings_popup.city_label,
                      ui->weather.settings_popup
                          .cities_de[ui->weather.settings_popup.saved_city]
                          .name);

  set_visible(ui->weather.settings_popup.citys_list, false);
}

static void ui_create_weather_settings_popup(ui_main_menu_t *ui) {
  ui->weather.settings_popup.popup =
      create_background(ui->screen, POPUP_WINDOW_WIDTH, POPUP_WINDOW_HEIGHT,
                        POPUP_WINDOW_ALIGN, 0, 0);
  lv_obj_add_style(ui->weather.settings_popup.popup, &ui->style.popup, 0);

  ui->weather.settings_popup.btn_close =
      create_btn_cb(ui->weather.settings_popup.popup, 50, 50, LV_ALIGN_TOP_LEFT,
                    500, -10, btn_weather_close_popup_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->weather.settings_popup.btn_close,
                              LV_SYMBOL_HOME, 0);

  ui->weather.settings_popup.btn_open_city_list =
      create_btn_cb(ui->weather.settings_popup.popup, 50, 50, LV_ALIGN_TOP_LEFT,
                    500, 90, btn_weather_open_list_city_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->weather.settings_popup.btn_open_city_list,
                              LV_SYMBOL_GPS, 0);

  ui->weather.settings_popup.city_label = create_label(
      ui->weather.settings_popup.popup, "Stadt", LV_ALIGN_TOP_LEFT, 220, 90);

  create_text("Wetter Einstellungen", ui->weather.settings_popup.popup,
              STYLE_TEXT_SMALL, LV_ALIGN_TOP_MID, 0, 0, ui);
  create_text("Stadt:", ui->weather.settings_popup.popup, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_LEFT, 15, 90, ui);
  ui_create_city_list_weather(ui);
}

static void ui_create_settings_popup_btns(ui_main_menu_t *ui) {
  ui->weather.settings_popup.btn_open = create_btn_icon(
      ui->screen, BLOCK_BOT_MID_WIDTH_SYMBOL, BLOCK_BOT_MID_HEIGHT_SYMBOL,
      BLOCK_BOT_MID_ALIGN_SYMBOL, BLOCK_BOT_MID_X_START_SYMBOL_1,
      BLOCK_BOT_MID_Y_START_SYMBOLS, btn_weather_open_popup_event_handler, ui,
      MY_CLOUD_SYMBOL, &ui->font.nav_btn, NULL); // my_symbols через стиль

  ui->wifi.btn_open = create_btn_icon(
      ui->screen, BLOCK_BOT_MID_WIDTH_SYMBOL, BLOCK_BOT_MID_HEIGHT_SYMBOL,
      BLOCK_BOT_MID_ALIGN_SYMBOL, BLOCK_BOT_MID_X_START_SYMBOL_2,
      BLOCK_BOT_MID_Y_START_SYMBOLS, btn_wifi_open_popup_event_handler, ui,
      LV_SYMBOL_WIFI, NULL, &lv_font_montserrat_32); // стандартный шрифт

  ui->settings.btn_open = create_btn_icon(
      ui->screen, BLOCK_BOT_MID_WIDTH_SYMBOL, BLOCK_BOT_MID_HEIGHT_SYMBOL,
      BLOCK_BOT_MID_ALIGN_SYMBOL, BLOCK_BOT_MID_X_START_SYMBOL_3,
      BLOCK_BOT_MID_Y_START_SYMBOLS, btn_settings_open_popup_event_handler, ui,
      LV_SYMBOL_SETTINGS, NULL, &lv_font_montserrat_32);
}

static void ui_create_wifi_popup(ui_main_menu_t *ui) {

  ui->wifi.popup =
      create_background(ui->screen, POPUP_WINDOW_WIDTH, POPUP_WINDOW_HEIGHT,
                        POPUP_WINDOW_ALIGN, 0, 0);
  lv_obj_set_scrollbar_mode(ui->wifi.popup, LV_SCROLLBAR_MODE_OFF);
  lv_obj_add_style(ui->wifi.popup, &ui->style.popup, 0);

  create_text("WIFI Einstellungen", ui->wifi.popup, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_MID, 0, 0, ui);
  create_text("Name:", ui->wifi.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT, 15,
              90, ui);
  create_text("Passwort:", ui->wifi.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT,
              15, 180, ui);
  create_text("Signal:", ui->wifi.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT,
              15, 270, ui);

  ui->wifi.btn_close =
      create_btn_cb(ui->wifi.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, -10,
                    btn_wifi_close_popup_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->wifi.btn_close, LV_SYMBOL_HOME, 0);

  ui->wifi.btn_keyboard_ssid =
      create_btn_cb(ui->wifi.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, 90,
                    btn_keyboard_open_ssid_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->wifi.btn_keyboard_ssid, LV_SYMBOL_KEYBOARD,
                              0);

  ui->wifi.btn_keyboard_pass =
      create_btn_cb(ui->wifi.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, 180,
                    btn_keyboard_open_pass_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->wifi.btn_keyboard_pass, LV_SYMBOL_KEYBOARD,
                              0);

  ui->wifi.ssid_label =
      create_label(ui->wifi.popup, "WiFiName", LV_ALIGN_TOP_LEFT, 220, 90);
  lv_obj_add_style(ui->wifi.ssid_label, &ui->font.medium_32, 0);
  ui->wifi.pass_label =
      create_label(ui->wifi.popup, "*********", LV_ALIGN_TOP_LEFT, 220, 180);
  lv_obj_add_style(ui->wifi.pass_label, &ui->font.medium_32, 0);
  ui->wifi.rssi_label =
      create_label(ui->wifi.popup, "WiFiRSSI", LV_ALIGN_TOP_LEFT, 220, 270);
  lv_obj_add_style(ui->wifi.rssi_label, &ui->font.medium_32, 0);
  ui->wifi.keyboard = lv_keyboard_create(ui->wifi.popup);
  // [FIX] Было &ui (адрес локального параметра функции — UB после возврата!)
  // Теперь ui — указатель на глобальную структуру, всегда валиден
  lv_obj_add_event_cb(ui->wifi.keyboard, ta_wifi_event_cb, LV_EVENT_ALL, ui);

  ui->wifi.ta_ssid = lv_textarea_create(ui->wifi.popup);
  lv_obj_align(ui->wifi.ta_ssid, LV_ALIGN_TOP_MID, 0, 75);
  lv_textarea_set_placeholder_text(ui->wifi.ta_ssid, "WiFi Name");
  lv_obj_set_size(ui->wifi.ta_ssid, 550, 70);

  ui->wifi.ta_pass = lv_textarea_create(ui->wifi.popup);
  lv_obj_align(ui->wifi.ta_pass, LV_ALIGN_TOP_MID, 0, 75);
  // [FIX] Было "WiFi SSID" — опечатка в placeholder для pass
  lv_textarea_set_placeholder_text(ui->wifi.ta_pass, "WiFi Passwort");
  lv_obj_set_size(ui->wifi.ta_pass, 550, 70);

  set_visible(ui->wifi.ta_ssid, false);
  set_visible(ui->wifi.ta_pass, false);
  set_visible(ui->wifi.keyboard, false);
}

void ui_apply_theme(ui_main_menu_t *ui) {
  switch (ui->settings.switch_.theme_mode) {
  case 0: // Auto
    ui->settings.switch_.theme_last_is_day = get_is_day();
    ui->settings.switch_.theme_last_is_day ? apply_theme_light(ui)
                                           : apply_theme_dark(ui);
    break;
  case 1:
    apply_theme_light(ui);
    break; // Hell
  case 2:
    apply_theme_dark(ui);
    break; // Dunkel
  }
  lv_obj_report_style_change(&ui->style.main);
  lv_obj_report_style_change(&ui->style.popup);
  lv_obj_report_style_change(&ui->style.top_left);
  lv_obj_report_style_change(&ui->style.bot_left);
  lv_obj_report_style_change(&ui->style.top_right);
  lv_obj_report_style_change(&ui->style.bot_right);
  lv_obj_report_style_change(&ui->font.very_small_20);
  lv_obj_report_style_change(&ui->font.small_24);
  lv_obj_report_style_change(&ui->font.medium_32);
  lv_obj_report_style_change(&ui->font.large_48);
  lv_obj_report_style_change(&ui->font.nav_btn);
  lv_obj_report_style_change(&ui->font.time);
  lv_obj_report_style_change(&ui->font.icon);
}

// callback для btnmatrix
static void standby_btnmatrix_event_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  lv_obj_t *obj = lv_event_get_target(e);
  uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
  ui->settings.switch_.standby_mode = (uint8_t)btn_id;

  // обновляем подсказку
  static const char *descs[] = {"Immer eingeschaltet.", "Aus nach 180 Sek.",
                                "Aus Nachts nach 180 Sek."};
  lv_label_set_text(ui->settings.standby_desc_label, descs[btn_id]);

  main_settings_save(ui->settings.switch_.standby_mode,
                     ui->settings.switch_.theme_mode,
                     ui->settings.switch_.co2_mode);
}
static void co2_btnmatrix_event_cb(lv_event_t *e) {
    ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    lv_obj_t *obj = lv_event_get_target(e);
    uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
    ui->settings.switch_.co2_mode = (uint8_t)btn_id;

    static const char *descs[] = {
    "400-2400 ppm: fuer Schlafraeume und Bueros.",
    "400-4000 ppm: fuer Wohnraeume.",
    "400-6000 ppm: fuer Industrie und Lager."};
    lv_label_set_text(ui->settings.co2_desc_label, descs[btn_id]);

    main_settings_save(ui->settings.switch_.standby_mode,
                       ui->settings.switch_.theme_mode,
                       ui->settings.switch_.co2_mode);
}
static void theme_btnmatrix_event_cb(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
  lv_obj_t *obj = lv_event_get_target(e);
  uint16_t btn_id = lv_btnmatrix_get_selected_btn(obj);
  ui->settings.switch_.theme_mode = (uint8_t)btn_id;

  static const char *descs[] = {"Automatisch: Tagsueber hell, nachts dunkel.",
                                "Immer helles Design.",
                                "Immer dunkles Design."};
  lv_label_set_text(ui->settings.theme_desc_label, descs[btn_id]);

  // применяем тему сразу
  switch (btn_id) {
  case 0: // Auto
    ui->settings.switch_.theme_last_is_day = get_is_day();
    ui->settings.switch_.theme_last_is_day ? apply_theme_light(ui)
                                           : apply_theme_dark(ui);
    break;
  case 1:
    apply_theme_light(ui);
    break; // Hell
  case 2:
    apply_theme_dark(ui);
    break; // Dunkel
  }
  lv_obj_report_style_change(&ui->style.main);
  lv_obj_report_style_change(&ui->style.popup);
  lv_obj_report_style_change(&ui->style.top_left);
  lv_obj_report_style_change(&ui->style.bot_left);
  lv_obj_report_style_change(&ui->style.top_right);
  lv_obj_report_style_change(&ui->style.bot_right);
  lv_obj_report_style_change(&ui->font.very_small_20);
  lv_obj_report_style_change(&ui->font.small_24);
  lv_obj_report_style_change(&ui->font.medium_32);
  lv_obj_report_style_change(&ui->font.large_48);
  lv_obj_report_style_change(&ui->font.nav_btn);
  lv_obj_report_style_change(&ui->font.time);
  lv_obj_report_style_change(&ui->font.icon);

  main_settings_save(ui->settings.switch_.standby_mode,
                     ui->settings.switch_.theme_mode,
                     ui->settings.switch_.co2_mode);
}

static void ui_create_settigs_switches(ui_main_menu_t *ui) {
  // --- standby btnmatrix ---
  static const char *standby_map[] = {"Ein", "Auto", "Auto Nachts", ""};
  ui->settings.standby_btnmatrix = lv_btnmatrix_create(ui->settings.popup);
  lv_obj_set_size(ui->settings.standby_btnmatrix, 540, 90);
  lv_obj_align(ui->settings.standby_btnmatrix, LV_ALIGN_TOP_LEFT, 10, 130);
  lv_btnmatrix_set_map(ui->settings.standby_btnmatrix, standby_map);
  lv_obj_set_style_text_font(ui->settings.standby_btnmatrix,
                             &lv_font_montserrat_20, LV_PART_ITEMS);
  lv_btnmatrix_set_btn_ctrl_all(ui->settings.standby_btnmatrix,
                                LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(ui->settings.standby_btnmatrix, true);
  lv_obj_add_event_cb(ui->settings.standby_btnmatrix,
                      standby_btnmatrix_event_cb, LV_EVENT_VALUE_CHANGED, ui);

  ui->settings.standby_desc_label =
      create_label(ui->settings.popup, "", LV_ALIGN_TOP_LEFT, 15, 225);
  lv_obj_add_style(ui->settings.standby_desc_label, &ui->font.very_small_20, 0);

  // --- theme btnmatrix ---
  static const char *theme_map[] = {"Auto", "Hell", "Dunkel", ""};
  ui->settings.theme_btnmatrix = lv_btnmatrix_create(ui->settings.popup);
  lv_obj_set_size(ui->settings.theme_btnmatrix, 540, 90);
  lv_obj_align(ui->settings.theme_btnmatrix, LV_ALIGN_TOP_LEFT, 10, 310);
  lv_btnmatrix_set_map(ui->settings.theme_btnmatrix, theme_map);
  lv_obj_set_style_text_font(ui->settings.theme_btnmatrix,
                             &lv_font_montserrat_20, LV_PART_ITEMS);
  lv_btnmatrix_set_btn_ctrl_all(ui->settings.theme_btnmatrix,
                                LV_BTNMATRIX_CTRL_CHECKABLE);
  lv_btnmatrix_set_one_checked(ui->settings.theme_btnmatrix, true);
  lv_obj_add_event_cb(ui->settings.theme_btnmatrix, theme_btnmatrix_event_cb,
                      LV_EVENT_VALUE_CHANGED, ui);

  ui->settings.theme_desc_label =
      create_label(ui->settings.popup, "", LV_ALIGN_TOP_LEFT, 15, 405);
  lv_obj_add_style(ui->settings.theme_desc_label, &ui->font.very_small_20, 0);

create_text("Luftqualitaetssensor:", ui->settings.popup, STYLE_TEXT_SMALL,
            LV_ALIGN_TOP_LEFT, 15, 450, ui);

static const char *co2_map[] = {"Sensitiv", "Normal",
                                 "Robust", ""};
ui->settings.co2_btnmatrix = lv_btnmatrix_create(ui->settings.popup);
lv_obj_set_size(ui->settings.co2_btnmatrix, 540, 90);
lv_obj_align(ui->settings.co2_btnmatrix, LV_ALIGN_TOP_LEFT, 10, 490);
lv_btnmatrix_set_map(ui->settings.co2_btnmatrix, co2_map);
lv_obj_set_style_text_font(ui->settings.co2_btnmatrix,
                           &lv_font_montserrat_20, LV_PART_ITEMS);
lv_btnmatrix_set_btn_ctrl_all(ui->settings.co2_btnmatrix,
                               LV_BTNMATRIX_CTRL_CHECKABLE);
lv_btnmatrix_set_one_checked(ui->settings.co2_btnmatrix, true);
lv_obj_add_event_cb(ui->settings.co2_btnmatrix, co2_btnmatrix_event_cb,
                    LV_EVENT_VALUE_CHANGED, ui);

ui->settings.co2_desc_label =
    create_label(ui->settings.popup, "", LV_ALIGN_TOP_LEFT, 15, 585);
lv_obj_add_style(ui->settings.co2_desc_label, &ui->font.very_small_20, 0);

// применяем сохранённое значение
lv_btnmatrix_set_btn_ctrl(ui->settings.co2_btnmatrix,
                          ui->settings.switch_.co2_mode,
                          LV_BTNMATRIX_CTRL_CHECKED);

static const char *co2_descs[] = {
    "400-2400 ppm: fuer Schlafraeume und Bueros.",
    "400-4000 ppm: fuer Wohnraeume.",
    "400-6000 ppm: fuer Industrie und Lager."};
lv_label_set_text(ui->settings.co2_desc_label,
                  co2_descs[ui->settings.switch_.co2_mode]);

  // --- загружаем сохранённые значения ---
  main_settings_load(&ui->settings.switch_.standby_mode,
                     &ui->settings.switch_.theme_mode,
                     &ui->settings.switch_.co2_mode);

  // применяем standby к btnmatrix
  lv_btnmatrix_set_btn_ctrl(ui->settings.standby_btnmatrix,
                            ui->settings.switch_.standby_mode,
                            LV_BTNMATRIX_CTRL_CHECKED);

  // применяем theme к btnmatrix
  lv_btnmatrix_set_btn_ctrl(ui->settings.theme_btnmatrix,
                            ui->settings.switch_.theme_mode,
                            LV_BTNMATRIX_CTRL_CHECKED);

  // подсказки при загрузке
  static const char *standby_descs[] = {
      "Immer eingeschaltet.", "Aus nach 180 Sek.", "Aus Nachts nach 180 Sek."};
  lv_label_set_text(ui->settings.standby_desc_label,
                    standby_descs[ui->settings.switch_.standby_mode]);

  static const char *theme_descs[] = {
      "Automatisch: Tagsueber hell, nachts dunkel.", "Immer helles Design.",
      "Immer dunkles Design."};
  lv_label_set_text(ui->settings.theme_desc_label,
                    theme_descs[ui->settings.switch_.theme_mode]);
}

static void ui_create_settings_popup(ui_main_menu_t *ui) {
  ui->settings.popup =
      create_background(ui->screen, POPUP_WINDOW_WIDTH, POPUP_WINDOW_HEIGHT,
                        POPUP_WINDOW_ALIGN, 0, 0);
  lv_obj_set_scrollbar_mode(ui->settings.popup, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_width(ui->settings.popup, 8, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(ui->settings.popup, lv_color_hex(0x2196F3),
                            LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_opa(ui->settings.popup, LV_OPA_COVER, LV_PART_SCROLLBAR);
  lv_obj_set_style_radius(ui->settings.popup, 4, LV_PART_SCROLLBAR);
  lv_obj_add_style(ui->settings.popup, &ui->style.popup, 0);

  create_text("Einstellungen", ui->settings.popup, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_MID, 0, 0, ui);

  create_text("Display:", ui->settings.popup, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_LEFT, 15, 90, ui);
  create_text("Thema:", ui->settings.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT,
              15, 270, ui);

  ui->settings.btn_close =
      create_btn_cb(ui->settings.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, -10,
                    btn_settings_close_popup_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->settings.btn_close, LV_SYMBOL_HOME, 0);

  // [FIX 4A] Switches создаются здесь — внутри popup, не при init
  ui_create_settigs_switches(ui);
}

static void create_block_bot_middle(ui_main_menu_t *ui) {
  ui_create_co2_chart_bot_mid(ui);
  ui_create_settings_popup_btns(ui);
  // ui_create_standby(ui);
}

static void create_block_bot_right(ui_main_menu_t *ui) {
  ui->animation.screen = create_background(
      ui->screen, BLOCK_BOT_RIGHT_WIDTH, BLOCK_BOT_RIGHT_HEIGHT,
      BLOCK_BOT_RIGHT_ALIGN_BACKGROUND, BLOCK_BOT_RIGHT_X_START,
      BLOCK_BOT_RIGHT_Y_START);
  lv_obj_add_style(ui->animation.screen, &ui->style.bot_right, 0);
  lv_obj_set_scrollbar_mode(ui->animation.screen, LV_SCROLLBAR_MODE_OFF);

  create_text("wetter", ui->animation.screen, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_MID, 0, BLOCK_BOT_RIGHT_Y_START_TITLE, ui);

#if ACTIVATE_ANIM_SUN_MOON
  ui->animation.image.sun_48_48 = create_anim_image_orbit(
      &sun_48_48, ui->animation.screen, BLOCK_BOT_RIGHT_ALIGN_WEATHER_ANIM,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_SUN_MOON,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_SUN_MOON);

  ui->animation.image.moon_42_42 = create_anim_image_orbit(
      &moon_42_42, ui->animation.screen, BLOCK_BOT_RIGHT_ALIGN_WEATHER_ANIM,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_SUN_MOON,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_SUN_MOON);
#endif
  set_visible(ui->animation.image.sun_48_48, false);
  set_visible(ui->animation.image.moon_42_42, false);

#if ACTIVATE_ANIM_CLOUD
  ui->animation.image.cloud.big_110_50 = create_cloud_anim(
      &cloud_big_110_50, ui->animation.screen,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_BIG_110_50,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_BIG_110_50,
      BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_BIG_110_50,
      BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_BIG_110_50);
  set_visible(ui->animation.image.cloud.big_110_50, false);

  ui->animation.image.cloud.mid_90_45 =
      create_cloud_anim(&cloud_mid_90_45, ui->animation.screen,
                        BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_MID_90_45,
                        BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_MID_90_45,
                        BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_MID_90_45,
                        BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_MID_90_45);
  set_visible(ui->animation.image.cloud.mid_90_45, false);

  ui->animation.image.cloud.small_70_35 = create_cloud_anim(
      &cloud_small_70_35, ui->animation.screen,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_SMALL_70_35,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_SMALL_70_35,
      BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_SMALL_70_35,
      BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_SMALL_70_35);
  set_visible(ui->animation.image.cloud.small_70_35, false);

  ui->animation.image.cloud.thin_80_30 = create_cloud_anim(
      &cloud_thin_80_30, ui->animation.screen,
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_THIN_80_30,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_THIN_80_30,
      BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_THIN_80_30,
      BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_THIN_80_30);
  set_visible(ui->animation.image.cloud.thin_80_30, false);
#endif

#if ACTIVATE_ANIM_WIND
  ui->animation.wind.slow =
      create_wind_anim(&wind_60_50, ui->animation.screen,
                       BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_WIND_SLOW,
                       BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_WIND_SLOW,
                       BLOCK_BOT_RIGHT_TURBULENCE_WEATHER_ANIM_WIND_SLOW,
                       BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_WIND_SLOW);
  set_visible(ui->animation.wind.slow, false);

  ui->animation.wind.med =
      create_wind_anim(&wind_60_50, ui->animation.screen,
                       BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_WIND_MED,
                       BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_WIND_MED,
                       BLOCK_BOT_RIGHT_TURBULENCE_WEATHER_ANIM_WIND_MED,
                       BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_WIND_MED);
  set_visible(ui->animation.wind.med, false);

  ui->animation.wind.fast =
      create_wind_anim(&wind_60_50, ui->animation.screen,
                       BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_WIND_FAST,
                       BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_WIND_FAST,
                       BLOCK_BOT_RIGHT_TURBULENCE_WEATHER_ANIM_WIND_FAST,
                       BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_WIND_FAST);
  set_visible(ui->animation.wind.fast, false);
#endif

#if ACTIVATE_ANIM_RAIN
  // [FIX 4B] Используем create_rain_anim — отдельный пул только для дождя
  // [FIX 4C] slope=2 — дождь под углом 45°
  for (uint8_t i = 0; i < BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS; i++) {
    ui->animation.rain[i] =
        create_rain_anim(&rain_drop_heavy_9_22, ui->animation.screen,
                         2, // slope: 2 = крутой угол
                         BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_RAIN_SNOW);
    set_visible(ui->animation.rain[i], false);
  }
#endif

#if ACTIVATE_ANIM_SNOW
  // [FIX 4B] Используем create_snow_anim — отдельный пул только для снега
  // [FIX 4C] slope=0 — снег падает вертикально
  for (uint8_t i = 0; i < BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS; i++) {
    ui->animation.snow[i] =
        create_snow_anim(&snow_flake_2_15_15, ui->animation.screen,
                         0, // slope: 0 = вертикально
                         BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_RAIN_SNOW);
    set_visible(ui->animation.snow[i], false);
  }
#endif
}

static void create_menu(ui_main_menu_t *ui) {

  ui->screen = lv_obj_create(NULL);
  lv_obj_add_style(ui->screen, &ui->style.main, 0);
  lv_obj_set_size(ui->screen, LVGL_PORT_H_RES, LVGL_PORT_V_RES);

  ui->standby.background = lv_obj_create(ui->screen);
  lv_obj_add_style(ui->standby.background, &ui->style.main, 0);
  lv_obj_set_size(ui->standby.background, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
  lv_obj_move_foreground(ui->standby.background); // ← всегда поверх всего
  lv_obj_clear_flag(ui->standby.background, LV_OBJ_FLAG_SCROLLABLE);
  set_visible(ui->standby.background, false);

#if ACTIVATE_BLOCK_TOP_LEFT
  create_block_top_left(ui);
#endif
#if ACTIVATE_BLOCK_BOT_LEFT
  create_block_bot_left(ui);
#endif
#if ACTIVATE_BLOCK_TOP_MID
  create_block_top_middle(ui);
#endif
#if ACTIVATE_BLOCK_TOP_RIGHT
  create_block_top_right(ui);
#endif
#if ACTIVATE_BLOCK_BOT_MID
  create_block_bot_middle(ui);
#endif
#if ACTIVATE_BLOCK_BOT_RIGHT
  create_block_bot_right(ui);
#endif
  // printf("After UI: %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}

void draw_menu_main(ui_main_menu_t *ui) { lv_scr_load(ui->screen); }

void rain_set_intensity(uint8_t visible_count, ui_main_menu_t *ui) {
  static uint8_t prev =
      255; // 255 = невалидное значение → форсирует первый update

  if (visible_count == prev)
    return;

  // Клипуем на максимум — защита от выхода за пределы массива
  if (visible_count > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS)
    visible_count = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS;

  if (visible_count > prev) {
    for (uint8_t i = prev == 255 ? 0 : prev; i < visible_count; i++) {
      if (ui->animation.rain[i] && lv_obj_is_valid(ui->animation.rain[i]))
        lv_obj_clear_flag(ui->animation.rain[i], LV_OBJ_FLAG_HIDDEN);
    }
  } else {
    for (uint8_t i = visible_count;
         i < (prev == 255 ? BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS : prev);
         i++) {
      if (ui->animation.rain[i] && lv_obj_is_valid(ui->animation.rain[i]))
        lv_obj_add_flag(ui->animation.rain[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  prev = visible_count;
}

void snow_set_intensity(uint8_t visible_count, ui_main_menu_t *ui) {

  static uint8_t prev =
      255; // 255 = невалидное значение → форсирует первый update

  if (visible_count == prev)
    return;

  // Клипуем на максимум — защита от выхода за пределы массива
  if (visible_count > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS)
    visible_count = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS;

  if (visible_count > prev) {
    for (uint8_t i = prev == 255 ? 0 : prev; i < visible_count; i++) {
      if (ui->animation.snow[i] && lv_obj_is_valid(ui->animation.snow[i]))
        lv_obj_clear_flag(ui->animation.snow[i], LV_OBJ_FLAG_HIDDEN);
    }
  } else {
    for (uint8_t i = visible_count;
         i < (prev == 255 ? BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS : prev);
         i++) {
      if (ui->animation.snow[i] && lv_obj_is_valid(ui->animation.snow[i]))
        lv_obj_add_flag(ui->animation.snow[i], LV_OBJ_FLAG_HIDDEN);
    }
  }

  prev = visible_count;
}

void draw_weather_sun_moon(ui_main_menu_t *ui) {
  static uint8_t prev = 255;
#if SIMULATE_ANIM_SUN
  set_visible(ui->animation.image.sun_48_48, true);
  set_visible(ui->animation.image.moon_42_42, false);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    set_visible(ui->animation.image.sun_48_48, false);
    set_visible(ui->animation.image.moon_42_42, false);
    prev = 255;
  } else {

    uint8_t is_day = get_is_day();

    if (is_day == prev)
      return;

    if (get_is_day()) {
      set_visible(ui->animation.image.sun_48_48, true);
      set_visible(ui->animation.image.moon_42_42, false);
    } else {
      set_visible(ui->animation.image.sun_48_48, false);
      set_visible(ui->animation.image.moon_42_42, true);
    }
    prev = is_day;
  }

#endif
}

void draw_weather_clouds(ui_main_menu_t *ui) {
  static uint8_t prev = 255;
#if SIMULATE_ANIM_CLOUD
  set_visible(ui->animation.image.cloud.big_110_50, true);
  set_visible(ui->animation.image.cloud.mid_90_45, true);
  set_visible(ui->animation.image.cloud.small_70_35, true);
  set_visible(ui->animation.image.cloud.thin_80_30, true);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    set_visible(ui->animation.image.cloud.thin_80_30, false);
    set_visible(ui->animation.image.cloud.small_70_35, false);
    set_visible(ui->animation.image.cloud.mid_90_45, false);
    set_visible(ui->animation.image.cloud.big_110_50, false);
    prev = 255;
  } else {
    uint8_t current_clouds = get_weather_clouds();

    if (current_clouds == prev)
      return;
    if (current_clouds < 15) {
      set_visible(ui->animation.image.cloud.thin_80_30, false);
      set_visible(ui->animation.image.cloud.small_70_35, false);
      set_visible(ui->animation.image.cloud.mid_90_45, false);
      set_visible(ui->animation.image.cloud.big_110_50, false);
    } else if (current_clouds > 15 && current_clouds < 25) {
      set_visible(ui->animation.image.cloud.thin_80_30, true);
      set_visible(ui->animation.image.cloud.small_70_35, false);
      set_visible(ui->animation.image.cloud.mid_90_45, false);
      set_visible(ui->animation.image.cloud.big_110_50, false);
    } else if (current_clouds > 25 && current_clouds < 50) {
      set_visible(ui->animation.image.cloud.thin_80_30, true);
      set_visible(ui->animation.image.cloud.small_70_35, true);
      set_visible(ui->animation.image.cloud.mid_90_45, false);
      set_visible(ui->animation.image.cloud.big_110_50, false);
    } else if (current_clouds > 50 && current_clouds < 75) {
      set_visible(ui->animation.image.cloud.thin_80_30, true);
      set_visible(ui->animation.image.cloud.small_70_35, true);
      set_visible(ui->animation.image.cloud.mid_90_45, true);
      set_visible(ui->animation.image.cloud.big_110_50, false);
    } else if (current_clouds > 75) {
      set_visible(ui->animation.image.cloud.thin_80_30, true);
      set_visible(ui->animation.image.cloud.small_70_35, true);
      set_visible(ui->animation.image.cloud.mid_90_45, true);
      set_visible(ui->animation.image.cloud.big_110_50, true);
    }
    prev = current_clouds;
  }

#endif
}

void draw_weather_wind(ui_main_menu_t *ui) {
  static uint8_t prev = 255;
#if SIMULATE_ANIM_WIND
  set_visible(ui->animation.wind.slow, true);
  set_visible(ui->animation.wind.med, true);
  set_visible(ui->animation.wind.fast, true);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    set_visible(ui->animation.wind.slow, false);
    set_visible(ui->animation.wind.med, false);
    set_visible(ui->animation.wind.fast, false);
    prev = 255;
  } else {
    uint8_t current_wind = get_weather_wind();

    if (current_wind == prev)
      return;

    // < 2  м/с — всё выключено
    // 2..9  — только slow
    // 10..19 — slow + med
    // 20+   — все три
    set_visible(ui->animation.wind.slow, current_wind >= 2);
    set_visible(ui->animation.wind.med, current_wind >= 10);
    set_visible(ui->animation.wind.fast, current_wind >= 20);
    prev = current_wind;
  }

#endif
}

void draw_weather_rain(ui_main_menu_t *ui) {
#if SIMULATE_ANIM_RAIN
  rain_set_intensity(10);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    rain_set_intensity(0, ui);
  } else {
    uint8_t current_rain = get_weather_rain();
    if (current_rain > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS) {
      current_rain = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS;
    }
    rain_set_intensity(current_rain, ui);
  }

#endif
}

void draw_weather_snow(ui_main_menu_t *ui) {
#if SIMULATE_ANIM_SNOW
  snow_set_intensity(10);
#else
  if (get_wifi_status() == WIFI_DISCONNECTED) {
    snow_set_intensity(0, ui);
  } else {
    uint8_t current_snow = get_weather_snow();
    current_snow =
        current_snow * BLOCK_BOT_RIGHT_MULT_FACTOR_WEATHER_ANIM_SNOWS;
    if (current_snow > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS) {
      current_snow = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS;
    }
    snow_set_intensity(current_snow, ui);
  }
#endif
}

void draw_weather(ui_main_menu_t *ui) {
#if ACTIVATE_ANIM_SUN_MOON
  draw_weather_sun_moon(ui);
#endif
#if ACTIVATE_ANIM_CLOUD
  draw_weather_clouds(ui);
#endif
#if ACTIVATE_ANIM_WIND
  draw_weather_wind(ui);
#endif
#if ACTIVATE_ANIM_RAIN
  draw_weather_rain(ui);
#endif
#if ACTIVATE_ANIM_SNOW
  draw_weather_snow(ui);
#endif
}

void draw_symbol_wifi(ui_main_menu_t *ui) {
  static uint8_t status_wifi_old = 254;
  uint8_t current_status = get_wifi_status();
  if (current_status == status_wifi_old)
    return;

  // label — первый дочерний объект кнопки
  lv_obj_t *label = lv_obj_get_child(ui->wifi.btn_open, 0);
  if (!label)
    return;

  switch (current_status) {
  case WIFI_RECONNECT:
    lv_label_set_text(label, LV_SYMBOL_REFRESH);
    break;
  case WIFI_CONNECTED:
    lv_label_set_text(label, LV_SYMBOL_WIFI);
    break;
  case WIFI_DISCONNECTED:
  default:
    lv_label_set_text(label, LV_SYMBOL_WARNING);
    break;
  }
  status_wifi_old = current_status;
}

void hide_all_blocks(ui_main_menu_t *ui) {
  set_visible(ui->animation.screen, false);
  set_visible(ui->sensor.screen, false);
  set_visible(ui->weather.screen, false);
  set_visible(ui->time.screen, false);
  set_visible(ui->co2.meter, false);
  set_visible(ui->co2.chart, false);
  // set_visible(ui->standby.bar, false);
}
void show_all_blocks(ui_main_menu_t *ui) {
  set_visible(ui->animation.screen, true);
  set_visible(ui->sensor.screen, true);
  set_visible(ui->weather.screen, true);
  set_visible(ui->time.screen, true);
  set_visible(ui->co2.meter, true);
  set_visible(ui->co2.chart, true);
  // set_visible(ui->standby.bar, true);
}

static float calc_dew_point(float temp, float humidity) {
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
static void update_tvoc_dots(lv_obj_t *dots[5], uint16_t tvoc,
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
    sensor_record_values(ui);
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

static lv_color_t calc_co2_color(uint16_t co2) {
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
    print_value("%.f", (float)raw,
                ui->co2.co2_label); // реальное, без изменений
  } else {
    lv_label_set_text(ui->co2.co2_label, "KS");
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
    precip_text = "möglich";
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

// Таймер который проверяет статус каждые 500мс, максимум 10 раз (5 секунд)
static void wifi_check_timer_cb(lv_timer_t *timer) {
  ui_main_menu_t *ui = (ui_main_menu_t *)timer->user_data;
  static uint8_t attempts = 0;

  attempts++;

  if (get_wifi_status() == WIFI_CONNECTED) {
    if (lv_obj_is_valid(ui->wifi.ssid_label))
      lv_label_set_text(ui->wifi.ssid_label, LV_SYMBOL_OK " verbunden");
    attempts = 0;
    lv_timer_del(timer);
    return;
  }

  if (attempts >= 10) {
    if (lv_obj_is_valid(ui->wifi.ssid_label))
      lv_label_set_text(ui->wifi.ssid_label, LV_SYMBOL_CLOSE " Fehler");
    attempts = 0;
    lv_timer_del(timer);
  }
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

bool is_screen_pressed(void) { return standby_touched; }

static void standby_handle(ui_main_menu_t *ui) {
  static uint32_t timer_standby_sec = 0;

  uint8_t mode = ui->settings.switch_.standby_mode;

  // определяем активен ли standby сейчас
  bool standby_active = false;
  if (mode == 1) {
    standby_active = true; // Immer — всегда
  } else if (mode == 2) {
    standby_active = !get_is_day(); // Nachts — только ночью
  }
  // mode == 0 (Aus) — standby_active остаётся false

  if (!standby_active) {
    // standby выключен — сбрасываем таймер, экран всегда включён
    timer_standby_sec = 0;
    set_visible(ui->standby.background, false);
    wavesahre_rgb_lcd_bl_on();
    return;
  }

  // standby активен — обычная логика
  if (is_screen_pressed()) {
    timer_standby_sec = 0;
    set_visible(ui->standby.background, false);
    wavesahre_rgb_lcd_bl_on();
  } else {
    timer_standby_sec++;
    if (timer_standby_sec > MAX_STANDBY_TIME * 5) {
      wavesahre_rgb_lcd_bl_off();
      lv_obj_move_foreground(ui->standby.background);
      set_visible(ui->standby.background, true);
      timer_standby_sec = MAX_STANDBY_TIME * 5;
    }
  }
}

static void timer_10000(lv_timer_t *timer) {
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
    }
  }
}

static void sensor_record_values(ui_main_menu_t *ui) {
  static uint16_t cnt_sensor_history = 0;
  cnt_sensor_history++;
  if (cnt_sensor_history >= SENSOR_RECORD_INTERVAL) {
    sensor_history_push(&ui->sensor.popup.history);
    cnt_sensor_history = 0;
  }
}

static void timer_1000(lv_timer_t *timer) {
  LV_UNUSED(timer);
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
  standby_handle(&ui);
}

static void init_fonts(ui_main_menu_t *ui) {
  lv_style_init(&ui->font.very_small_20);
  lv_style_set_text_font(&ui->font.very_small_20, &lv_font_montserrat_20);

  lv_style_init(&ui->font.small_24);
  lv_style_set_text_font(&ui->font.small_24, &lv_font_montserrat_24);

  lv_style_init(&ui->font.medium_32);
  lv_style_set_text_font(&ui->font.medium_32, &lv_font_montserrat_32);

  lv_style_init(&ui->font.large_48);
  lv_style_set_text_font(&ui->font.large_48, &lv_font_montserrat_48);

  lv_style_init(&ui->font.time);
  lv_style_set_text_font(&ui->font.time, &my_time_font);

  lv_style_init(&ui->font.icon);
  lv_style_set_bg_opa(&ui->font.icon, LV_OPA_TRANSP); // убираем фон
  lv_style_set_border_width(&ui->font.icon, 0); // убираем рамку
  lv_style_set_shadow_width(&ui->font.icon, 0); // убираем тень
  lv_style_set_text_font(&ui->font.icon, &my_symbols); // шрифт символов

  lv_style_init(&ui->font.nav_btn);
  lv_style_set_bg_opa(&ui->font.nav_btn, LV_OPA_TRANSP);
  lv_style_set_border_width(&ui->font.nav_btn, 0);
  lv_style_set_shadow_width(&ui->font.nav_btn, 0);
  lv_style_set_text_font(&ui->font.nav_btn, &my_symbols);
}

static void apply_theme_dark(ui_main_menu_t *ui) {
  lv_style_reset(&ui->style.main);
  lv_style_set_bg_color(&ui->style.main,
                        lv_color_hex(0x0D1117)); // почти чёрный сверху
  lv_style_set_bg_grad_color(&ui->style.main,
                             lv_color_hex(0x1E3A5F)); // тёмно-синий снизу
  lv_style_set_bg_grad_dir(&ui->style.main, LV_GRAD_DIR_VER);
  lv_style_set_bg_opa(&ui->style.main, LV_OPA_COVER);

  lv_style_reset(&ui->style.popup);
  lv_style_set_bg_color(&ui->style.popup,
                        lv_color_hex(0x161C24)); // тёмный, без градиента
  lv_style_set_bg_grad_dir(&ui->style.popup, LV_GRAD_DIR_NONE);
  lv_style_set_bg_opa(&ui->style.popup, LV_OPA_COVER);

  lv_style_reset(&ui->style.top_left);
  lv_style_set_bg_color(&ui->style.top_left, lv_color_hex(0x1A2E22));
  lv_style_set_border_color(&ui->style.top_left, lv_color_hex(0x2EA843));
  lv_style_set_border_width(&ui->style.top_left, 1);
  lv_style_set_radius(&ui->style.top_left, 12);

  lv_style_reset(&ui->style.bot_left);
  lv_style_set_bg_color(&ui->style.bot_left, lv_color_hex(0x2A2212));
  lv_style_set_border_color(&ui->style.bot_left, lv_color_hex(0xD48200));
  lv_style_set_border_width(&ui->style.bot_left, 1);
  lv_style_set_radius(&ui->style.bot_left, 12);

  lv_style_reset(&ui->style.top_right);
  lv_style_set_bg_color(&ui->style.top_right, lv_color_hex(0x1A1A2A));
  lv_style_set_border_color(&ui->style.top_right, lv_color_hex(0x5E4E90));
  lv_style_set_border_width(&ui->style.top_right, 1);
  lv_style_set_radius(&ui->style.top_right, 12);

  lv_style_reset(&ui->style.bot_right);
  lv_style_set_bg_color(&ui->style.bot_right, lv_color_hex(0x161B22));
  lv_style_set_border_color(&ui->style.bot_right, lv_color_hex(0x30363D));
  lv_style_set_border_width(&ui->style.bot_right, 1);
  lv_style_set_radius(&ui->style.bot_right, 12);

  lv_style_reset(&ui->style.chart_co2);
  lv_style_set_bg_opa(&ui->style.chart_co2, LV_OPA_COVER);
  lv_style_set_bg_color(&ui->style.chart_co2,
                        lv_color_hex(0x7A2E2E)); // тёмный красный
  lv_style_set_bg_grad_color(&ui->style.chart_co2,
                             lv_color_hex(0x1E5C30)); // тёмный зелёный
  lv_style_set_bg_grad_dir(&ui->style.chart_co2, LV_GRAD_DIR_VER);
  lv_style_set_bg_main_stop(&ui->style.chart_co2, 128);
  lv_style_set_bg_grad_stop(&ui->style.chart_co2, 192);

  lv_style_reset(&ui->style.meter_co2);
  lv_style_set_bg_color(&ui->style.meter_co2, lv_color_hex(0x1A2E22));
  lv_style_set_border_color(&ui->style.meter_co2, lv_color_hex(0x2EA843));
  lv_style_set_border_width(&ui->style.meter_co2, 1);

  // Шрифты
  lv_style_set_text_color(&ui->font.medium_32, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.small_24, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.large_48, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.time, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.very_small_20, lv_color_hex(0xE6EDF3));
  lv_style_set_text_color(&ui->font.icon, lv_color_hex(0x60A5FA));
  lv_style_set_text_color(&ui->font.nav_btn, lv_color_hex(0x60A5FA));
}

static void apply_theme_light(ui_main_menu_t *ui) {
  lv_style_reset(&ui->style.main);
  lv_style_set_bg_color(&ui->style.main,
                        lv_color_hex(0xF0F4F8)); // светло-серый сверху
  lv_style_set_bg_grad_color(&ui->style.main,
                             lv_color_hex(0xB8D0E8)); // голубой снизу
  lv_style_set_bg_grad_dir(&ui->style.main, LV_GRAD_DIR_VER);
  lv_style_set_bg_opa(&ui->style.main, LV_OPA_COVER);

  lv_style_reset(&ui->style.popup);
  lv_style_set_bg_color(&ui->style.popup,
                        lv_color_hex(0xE8EEF4)); // светлый, без градиента
  lv_style_set_bg_grad_dir(&ui->style.popup, LV_GRAD_DIR_NONE);
  lv_style_set_bg_opa(&ui->style.popup, LV_OPA_COVER);

  lv_style_reset(&ui->style.top_left);
  lv_style_set_bg_color(&ui->style.top_left, lv_color_hex(0xDCF5E7));
  lv_style_set_border_color(&ui->style.top_left, lv_color_hex(0x2EA843));
  lv_style_set_border_width(&ui->style.top_left, 1);
  lv_style_set_radius(&ui->style.top_left, 12);

  lv_style_reset(&ui->style.bot_left);
  lv_style_set_bg_color(&ui->style.bot_left, lv_color_hex(0xFFF3D6));
  lv_style_set_border_color(&ui->style.bot_left, lv_color_hex(0xD48200));
  lv_style_set_border_width(&ui->style.bot_left, 1);
  lv_style_set_radius(&ui->style.bot_left, 12);

  lv_style_reset(&ui->style.top_right);
  lv_style_set_bg_color(&ui->style.top_right, lv_color_hex(0xEDE9FF));
  lv_style_set_border_color(&ui->style.top_right, lv_color_hex(0x5E4E90));
  lv_style_set_border_width(&ui->style.top_right, 1);
  lv_style_set_radius(&ui->style.top_right, 12);

  lv_style_reset(&ui->style.bot_right);
  lv_style_set_bg_color(&ui->style.bot_right, lv_color_hex(0x6B9FD4)); // небо
  lv_style_set_border_color(&ui->style.bot_right, lv_color_hex(0x4A80B8));
  lv_style_set_border_width(&ui->style.bot_right, 1);
  lv_style_set_radius(&ui->style.bot_right, 12);

  lv_style_reset(&ui->style.chart_co2);
  lv_style_set_bg_opa(&ui->style.chart_co2, LV_OPA_COVER);
  lv_style_set_bg_color(&ui->style.chart_co2,
                        lv_color_hex(0xD4888A)); // пастельный красный
  lv_style_set_bg_grad_color(&ui->style.chart_co2,
                             lv_color_hex(0x88B898)); // пастельный зелёный
  lv_style_set_bg_grad_dir(&ui->style.chart_co2, LV_GRAD_DIR_VER);
  lv_style_set_bg_main_stop(&ui->style.chart_co2, 128);
  lv_style_set_bg_grad_stop(&ui->style.chart_co2, 192);

  lv_style_reset(&ui->style.meter_co2);
  lv_style_set_bg_color(&ui->style.meter_co2, lv_color_hex(0xEDE9FF));
  lv_style_set_border_color(&ui->style.meter_co2, lv_color_hex(0x5E4E90));
  lv_style_set_border_width(&ui->style.meter_co2, 1);

  // Шрифты — только text_color, font не трогаем
  lv_style_set_text_color(&ui->font.very_small_20, lv_color_hex(0x1A1A2A));
  lv_style_set_text_color(&ui->font.medium_32, lv_color_hex(0x1A1A2A));
  lv_style_set_text_color(&ui->font.small_24, lv_color_hex(0x1A1A2A));
  lv_style_set_text_color(&ui->font.large_48, lv_color_hex(0x1A1A2A));
  lv_style_set_text_color(&ui->font.time, lv_color_hex(0x1A1A2A));

  lv_style_set_text_color(&ui->font.icon, lv_color_hex(0x4A80B8));
  lv_style_set_text_color(&ui->font.nav_btn, lv_color_hex(0x4A80B8));
}

static void init_styles(ui_main_menu_t *ui) {
  lv_style_init(&ui->style.main);
  lv_style_init(&ui->style.popup);
  lv_style_init(&ui->style.top_left);
  lv_style_init(&ui->style.bot_left);
  lv_style_init(&ui->style.top_right);
  lv_style_init(&ui->style.bot_right);
  lv_style_init(&ui->style.meter_co2);
  lv_style_init(&ui->style.chart_co2);

  ui_apply_theme(ui); // применяем нужную тему
}

void init_lv_objects() {
  ui.co2.co2_display = -1;
  ui.co2.co2_target = -1;
  ui.weather.settings_popup.cities_de = cities_de;
  ui.weather.settings_popup.city_count = CITY_COUNT;

  main_settings_load(&ui.settings.switch_.standby_mode,
                     &ui.settings.switch_.theme_mode,
                     &ui.settings.switch_.co2_mode);

  ui.settings.switch_.theme_last_is_day = get_is_day(); // ← инициализируем кэш
  weather_settings_load(&ui.weather.settings_popup.saved_city);
  build_weather_url(ui.weather.settings_popup.saved_city);
  init_fonts(&ui);
  init_styles(&ui);
  create_menu(&ui);
  draw_menu_main(&ui);
  lv_timer_create(timer_10000, 10000, NULL);
  lv_timer_create(timer_1000, 1000, NULL);
  lv_timer_create(timer_200, 200, NULL);
  lv_timer_create(co2_meter_anim_cb, 33, &ui);
}
