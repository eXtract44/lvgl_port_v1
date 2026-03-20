/*
 * lvgl_menu.h
 *
 *  Created on: Oct 20, 2023
 *      Author: Alex
 */

#ifndef INC_LVGL_MENU_H_
#define INC_LVGL_MENU_H_

#include "../components/lvgl__lvgl/lvgl.h"
#include "user/menu/lvgl_user_config.h"
#include "user/periphery/open_meteo.h"
#include <stdio.h>

#define constrain(input, min, max)                                             \
  ({                                                                           \
    __typeof__(input) _input = (input);                                        \
    __typeof__(min) _min = (min);                                              \
    __typeof__(max) _max = (max);                                              \
    (_input < _min) ? _min : ((_input > _max) ? _max : _input);                \
  })

#define map(in, in_min, in_max, out_min, out_max)                              \
  (((in) - (in_min)) * ((out_max) - (out_min)) / ((in_max) - (in_min)) +       \
   (out_min))

#define MY_HUMIDITY_SYMBOL "\xEF\x81\x83"
#define MY_WIND_SYMBOL "\xEF\x80\xA4"
#define MY_TVOC_SYMBOL "\xEF\x86\x8C"
#define MY_TEMPERATURE_SYMBOL "\xEF\x8B\x89"
#define MY_CLOUD_SYMBOL "\xEF\x83\x82"

#define SENSOR_HISTORY_POINTS 72 // 6 часов × 12 точек/час
#define SENSOR_RECORD_INTERVAL 300 // тиков (секунд) между записями

// Кольцевой буфер для истории датчиков
typedef struct {
  int16_t temperature[SENSOR_HISTORY_POINTS]; // °C × 10 (фиксированная точка)
  uint8_t humidity[SENSOR_HISTORY_POINTS]; // %
  uint8_t head;  // индекс следующей записи
  uint8_t count; // сколько точек уже заполнено (0..72)
} sensor_history_t;

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *icon_temperature;
  lv_obj_t *icon_humidity;
  lv_obj_t *icon_wind;
  lv_obj_t *temperature_label;
  lv_obj_t *humidity_label;
  lv_obj_t *wind_label;
} ui_weather_value_t;

typedef struct {
  lv_obj_t *popup;
  lv_obj_t *btn_open;
  lv_obj_t *btn_close;

  lv_obj_t *city_label;
  lv_obj_t *citys_list;
  lv_obj_t *btn_open_city_list;
  const city_t *cities_de;
  uint16_t city_count;
  uint16_t saved_city;

} ui_weather_t;

typedef struct {
  lv_obj_t *standby;
  lv_obj_t *theme;
  bool standby_status;
  bool theme_status;
} ui_switchs_t;

typedef struct {
  lv_obj_t *btn_open;

  lv_obj_t *popup;
  lv_obj_t *btn_close;

  lv_obj_t *ssid_label;
  lv_obj_t *pass_label;
  lv_obj_t *rssi_label;

  lv_obj_t *btn_keyboard_ssid;
  lv_obj_t *btn_keyboard_pass;

  lv_obj_t *keyboard;
  lv_obj_t *ta_ssid;
  lv_obj_t *ta_pass;

} ui_wifi_t;

typedef struct {
  ui_switchs_t switch_;
  lv_obj_t *btn_open;

  lv_obj_t *popup;
  lv_obj_t *btn_close;
  lv_obj_t *btn_save;

  lv_obj_t *standby_flag;
  lv_obj_t *standby_time;
  lv_obj_t *standby_day_night;
} ui_settings_t;

typedef struct {
  lv_obj_t *meter;
  lv_obj_t *co2_label;
  lv_meter_indicator_t *indicator;
  lv_obj_t *chart;
  lv_obj_t *title;
  lv_chart_series_t *series_co2;

} ui_co2_t;

typedef struct {
  lv_obj_t *bar;
} ui_standby_t;

typedef struct {
  lv_obj_t *big_110_50;
  lv_obj_t *mid_90_45;
  lv_obj_t *small_70_35;
  lv_obj_t *thin_80_30;
} ui_cloud_t;

typedef struct {
  lv_obj_t *sun_48_48;
  lv_obj_t *moon_42_42;
  ui_cloud_t cloud;
  lv_obj_t *wind_60_50;
  lv_obj_t *rain_9_22;
  lv_obj_t *snow_15_15;
} ui_images_t;

typedef struct {
  lv_obj_t *slow;
  lv_obj_t *med;
  lv_obj_t *fast;
} ui_wind_t;

typedef struct {
  lv_obj_t *screen;
  ui_images_t image;
  ui_wind_t wind;
  lv_obj_t *rain[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS];
  lv_obj_t *snow[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS];

} ui_weather_anim_t;

typedef struct {
  lv_obj_t *popup;
  lv_obj_t *btn_close;
  lv_obj_t *chart;
  lv_obj_t *screen;
  lv_obj_t *icon_temperature;
  lv_obj_t *icon_humidity;
  lv_obj_t *icon_tvoc;
  lv_obj_t *icon_wind;
  lv_obj_t *temperature_label;
  lv_obj_t *humidity_label;
  lv_obj_t *tvoc_label;
} ui_sensor_t;

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *hour_minute_label;
  lv_obj_t *mday_month_label;
  lv_obj_t *wday_label;
} ui_time_t;

typedef struct {
  lv_style_t small;
  lv_style_t very_large;
  lv_style_t time;
  lv_style_t title;
  lv_style_t icon;
  lv_style_t nav_btn;
} font_style_t;

typedef struct {
  lv_style_t main;
  lv_style_t top_left;
  lv_style_t top_right;
  lv_style_t bot_left;
  lv_style_t bot_right;
  lv_style_t chart_co2;
  lv_style_t meter_co2;

} screen_style_t;

typedef struct {
  lv_obj_t *screen;

  ui_weather_anim_t animation;
  ui_co2_t co2;
  ui_weather_t weather;
  ui_wifi_t wifi;
  ui_standby_t standby;
  ui_settings_t settings;
  ui_sensor_t sensor;
  ui_time_t time;
  ui_weather_value_t meteo;
  font_style_t font;
  screen_style_t style;
} ui_main_menu_t;

enum namesOfStyles {
  STYLE_TEXT_TITLE,
  STYLE_TEXT_SMALL,
  STYLE_TEXT_VERY_LARGE,
  STYLE_SYMBOLS,
};

typedef struct {
  lv_obj_t *obj;
  int16_t base_x;
  int16_t base_y;
  int16_t amplitude;
} cloud_anim_t;

typedef struct {
  lv_obj_t *obj;
  lv_coord_t base_x;
  lv_coord_t base_y;
  lv_coord_t container_w;
  lv_coord_t img_w;
  int16_t turbulence; // вертикальная амплитуда
} wind_anim_t;

typedef struct {
  lv_obj_t *obj;
  lv_coord_t container_w;
  lv_coord_t container_h;
  lv_coord_t img_w;
  lv_coord_t img_h;
  int16_t slope; // наклон (например 1 = 45°)
  int32_t phase; // индивидуальный сдвиг
} rain_snow_anim_t;

void init_lv_objects();

static void apply_theme_dark(ui_main_menu_t *ui);
static void apply_theme_light(ui_main_menu_t *ui);

void hide_all_blocks(ui_main_menu_t *ui);
void show_all_blocks(ui_main_menu_t *ui);
static void ui_create_wifi_popup(ui_main_menu_t *ui);
static void ui_create_weather_popup(ui_main_menu_t *ui);
static void ui_create_settings_popup(ui_main_menu_t *ui);
static void ui_create_sensor_popup(ui_main_menu_t *ui);

void set_visible(lv_obj_t *parent, bool visible);

void print_value(const char *flags, float value, lv_obj_t *parent);
void print_wday(uint8_t wday, ui_main_menu_t *ui);
void print_time(uint8_t time_hour, uint8_t time_minute, ui_main_menu_t *ui);
void print_mday(uint8_t date_day, uint8_t date_month, ui_main_menu_t *ui);

static void create_text(const char *text, lv_obj_t *parent, uint16_t theme,
                        lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                        ui_main_menu_t *ui);
static lv_obj_t *create_label(lv_obj_t *parent, const char *text,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs);
static lv_obj_t *create_meter(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs, ui_main_menu_t *ui);
static lv_obj_t *create_chart(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs);
static lv_obj_t *create_icon(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                             lv_align_t align, lv_coord_t x_ofs,
                             lv_coord_t y_ofs, const char *symbol,
                             ui_main_menu_t *ui);
static lv_obj_t *create_btn_icon(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                                 lv_align_t align, lv_coord_t x_ofs,
                                 lv_coord_t y_ofs, lv_event_cb_t event_cb,
                                 void *user_data, const char *symbol,
                                 lv_style_t *icon_style, const lv_font_t *font);
static lv_obj_t *create_btn_cb(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                               lv_align_t align, lv_coord_t x_ofs,
                               lv_coord_t y_ofs, lv_event_cb_t event_cb,
                               void *user_data);
static lv_obj_t *create_background(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                                   lv_align_t align, lv_coord_t x_ofs,
                                   lv_coord_t y_ofs);

static void anim_sun_moon_orbit(void *var, int32_t angle);

#endif /* INC_LVGL_MENU_H_ */
