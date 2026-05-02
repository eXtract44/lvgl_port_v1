/*
 * lvgl_menu.h
 *
 *  Created on: Oct 20, 2023
 *      Author: Alex
 */

#ifndef INC_LVGL_MENU_H_
#define INC_LVGL_MENU_H_

#include "../components/lvgl__lvgl/lvgl.h"
#include "lvgl_port.h"
#include "esp_wifi.h"
#include "user/menu/lvgl_user_config.h"
#include "user/periphery/open_meteo.h"
#include <stdio.h>
#include "esp_heap_caps.h"
// #include "esp_log_timestamp.h"
#include "font/lv_symbol_def.h"

#include "lvgl_user_config.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"


#include "waveshare_rgb_lcd_port.h"
#include <esp_log.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>

#include "user/periphery/nvs_user.h"
#include "user/periphery/open_meteo.h"
#include "user/periphery/periphery.h"
#include "user/periphery/wifi.h"

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
#define MY_WIND_SYMBOL "\xEF\x9C\xAE"
#define MY_TVOC_SYMBOL "\xEF\x86\x8C"
#define MY_TEMPERATURE_SYMBOL "\xEF\x8B\x89"
#define MY_CLOUD_SYMBOL "\xEF\x83\x82"
#define MY_SNOWFLAKE_SYMBOL  "\xEF\x8B\x9C"  //  ❄ точка росы 
#define MY_FEELS_LIKE_SYMBOL  "\xEF\x8B\x89"  // термометр (тот же что температура)
#define MY_SUN_SYMBOL        "\xEF\x86\x85"  // 
#define MY_CLOUD_RAIN_SYMBOL "\xEF\x9C\xBD"  // 
#define MY_PRESSURE_SYMBOL   "\xEF\x8F\xBD"  //        
#define MY_BELL_SYMBOL       "\xEF\x83\xB3"  // 🔔 праздник                 


#define SENSOR_HISTORY_POINTS 144 // 12 часов × 12 точек/час
#define SENSOR_RECORD_INTERVAL 300 //300 тиков (секунд) между записями



// Кольцевой буфер для истории датчиков
typedef struct {
  int16_t temperature[SENSOR_HISTORY_POINTS]; // °C × 10 (фиксированная точка)
  uint16_t humidity[SENSOR_HISTORY_POINTS]; // %
  uint16_t head;  // индекс следующей записи
  uint16_t count; // сколько точек уже заполнено (0..72)
} sensor_history_t;

typedef struct {
    float temperature;
    float humidity;
    float feels_like;
    float pressure;
    bool  valid;       // false до первого заполнения
} weather_snapshot_t;

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
  sensor_history_t history;
} ui_weather_settings_popup_t;

typedef struct {
  lv_obj_t *popup;
  lv_obj_t *btn_close;
  // chart убран — прогноз текстом
  // day labels
  lv_obj_t *day_label[FORECAST_DAYS];      // "Heute" / "Morgen" / "Übermorgen"
  lv_obj_t *wcode_label[FORECAST_DAYS];    // "Schauer"
  lv_obj_t *temp_label[FORECAST_DAYS];     // "7°C / 15°C"
  lv_obj_t *humidity_label[FORECAST_DAYS]; // "85 %"
  lv_obj_t *sunrise_label[FORECAST_DAYS];  // "06:45"
  lv_obj_t *sunset_label[FORECAST_DAYS];   // "20:12"
} ui_weather_forecast_popup_t;

typedef struct {
  lv_obj_t *popup;
  lv_obj_t *btn_close;
  lv_obj_t *chart;
  sensor_history_t history;
} ui_sensor_history_popup_t;

typedef struct {
    ui_weather_settings_popup_t settings_popup;
  ui_weather_forecast_popup_t forecast_popup;
  lv_obj_t *screen;
  // иконки
  lv_obj_t *icon_temperature;
  lv_obj_t *icon_humidity;
  lv_obj_t *icon_feels_like;
  lv_obj_t *icon_uv;
  lv_obj_t *icon_rain;
  lv_obj_t *icon_rain_prob;
  lv_obj_t *icon_wind;
  lv_obj_t *icon_pressure;
  // строка 1 — температура + влажность
  lv_obj_t *temperature_label;
  lv_obj_t *humidity_label;
  // строка 2 — feels like + UV
  lv_obj_t *feels_like_label;
  lv_obj_t *uv_label;
  // строка 3 — осадки + вероятность
  lv_obj_t *rain_label;
  // строка 4 — ветер + давление
  lv_obj_t *wind_label;
  lv_obj_t *pressure_label;
} ui_weather_t;

typedef struct {
	uint8_t standby_mode;
	 uint8_t theme_mode;        // 0=Auto 1=Hell 2=Dunkel
	 bool    theme_last_is_day; // кэш — последнее состояние get_is_day()
	 uint8_t co2_mode;          // 0=Sensitiv 1=Normal 2=Robust
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
	lv_obj_t *standby_btnmatrix;
	lv_obj_t *standby_desc_label;
	lv_obj_t *theme_btnmatrix;   // Auto / Hell / Dunkel
	lv_obj_t *theme_desc_label;  // подсказка под кнопками
	 lv_obj_t *co2_btnmatrix;      // ← новое
  lv_obj_t *co2_desc_label;     // ← новое
} ui_settings_t;

typedef struct {
lv_obj_t *meter;
  lv_obj_t *co2_label;
  lv_obj_t *co2_unit_label;    // "ppm" под значением
  lv_obj_t *co2_status_label;  // "GUT" / "MITTEL" / "SCHLECHT"
  lv_obj_t *co2_footer_label;  // "CO₂ | Luftqualität" внизу
  lv_obj_t *co2_divider;       // горизонтальная линия
  lv_meter_indicator_t *indicator;
  lv_meter_indicator_t *needle_arc;
  lv_obj_t *chart;
  lv_obj_t *title;
  lv_chart_series_t *series_co2;
  int32_t co2_target;
  int32_t co2_display;
  lv_obj_t *chart_lbl_max;
  lv_obj_t *chart_lbl_mid;
  lv_obj_t *chart_lbl_min;
  lv_obj_t *chart_lbl_t0;
  lv_obj_t *chart_lbl_t12;
  lv_obj_t *chart_lbl_t24;
  lv_obj_t *calib_icon_label;
lv_obj_t *calib_popup;
lv_obj_t *calib_popup_text;
TickType_t calib_start_tick;
  
} ui_co2_t;

typedef struct {
  lv_obj_t *background;
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
  ui_sensor_history_popup_t popup;
  lv_obj_t *screen;
  lv_obj_t *icon_temperature;
  lv_obj_t *icon_humidity;
  lv_obj_t *icon_feels_like;
  lv_obj_t *icon_dew_point;
  lv_obj_t *icon_tvoc;
  lv_obj_t *icon_wind;
  lv_obj_t *temperature_label;
  lv_obj_t *humidity_label;
  lv_obj_t *tvoc_label;
  lv_obj_t *tvoc_dots[5];
  lv_obj_t *feels_like_label;
  lv_obj_t *dew_point_label;     // точка росы °C
  
} ui_sensor_t;

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *hour_minute_label;
  lv_obj_t *mday_month_label;
   // wday_label убран — заменён колонкой Mo–So
  lv_obj_t *wday_labels[7];      // Mo Di Mi Do Fr Sa So
  lv_obj_t *wday_highlight;      // скруглённый прямоугольник за текущим днём
  lv_obj_t *holiday_label;       // MY_BELL_SYMBOL + название праздника
} ui_time_t;

typedef struct {
lv_style_t very_small_20;
lv_style_t small_24;   // ← новый
lv_style_t medium_32;  
lv_style_t large_48;
lv_style_t time;
 
lv_style_t icon;
lv_style_t nav_btn;

} font_style_t;

typedef struct {
  lv_style_t main;
  lv_style_t popup;     // плоский фон для попапов, без градиента
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
  // ui_weather_value_t meteo;
  font_style_t font;
  screen_style_t style;
  char string_buffer[20];
} ui_main_menu_t;

enum namesOfStyles {
  STYLE_TEXT_TITLE,
  STYLE_TEXT_SMALL,
  STYLE_TEXT_VERY_LARGE,
  STYLE_SYMBOLS,
};

enum namesOfwDay {
WDAY_SONNTAG = 0,
WDAY_MONTAG,
WDAY_DIENSTAG,
WDAY_MITTWOCH,
WDAY_DONNERSTAG,
WDAY_FREITAG,
WDAY_SAMSTAG,
WDAY_KEIN_WLAN,
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
static void ui_create_weather_settings_popup(ui_main_menu_t *ui);
static void ui_create_settings_popup(ui_main_menu_t *ui);
static void ui_create_sensor_history_popup(ui_main_menu_t *ui);

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
                                 lv_style_t *icon_style, const lv_font_t *font,
                                 const char *label_text, ui_main_menu_t *ui_ptr);
static lv_obj_t *create_btn_cb(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                               lv_align_t align, lv_coord_t x_ofs,
                               lv_coord_t y_ofs, lv_event_cb_t event_cb,
                               void *user_data);
static lv_obj_t *create_background(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                                   lv_align_t align, lv_coord_t x_ofs,
                                   lv_coord_t y_ofs);

static void anim_sun_moon_orbit(void *var, int32_t angle);

static void sensor_record_values(ui_main_menu_t *ui);
static void wifi_check_timer_cb(lv_timer_t *timer);
static void btn_weather_close_forecast_popup_event_handler(lv_event_t *e);
void print_holiday(uint8_t day, uint8_t month, uint16_t year, ui_main_menu_t *ui);
void update_co2_chart_labels(ui_main_menu_t *ui);
void update_co2_status_label(ui_main_menu_t *ui, uint16_t co2_ppm);
static void co2_calib_popup_close_cb(lv_event_t *e);
static void co2_meter_click_event_cb(lv_event_t *e);
static void ui_create_sgp30_calib_popup(ui_main_menu_t *ui);
static void sgp30_calib_timer_cb(lv_timer_t *t);

#endif /* INC_LVGL_MENU_H_ */
