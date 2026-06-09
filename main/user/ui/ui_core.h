

#ifndef INC_UI_CORE_H_
#define INC_UI_CORE_H_

#include "waveshare_rgb_lcd_port.h"

#include "../components/lvgl__lvgl/lvgl.h"
#include "font/lv_symbol_def.h"
#include "math.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "open_meteo.h"
#include "stdbool.h"
#include "stdio.h"
#include "time.h"

#include "lvgl_port.h"
#include "ui_geometry.h"
#include "ui_user_config.h"
#include "wifi_user.h"

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
#define MY_SNOWFLAKE_SYMBOL "\xEF\x8B\x9C" //  ❄ точка росы
#define MY_FEELS_LIKE_SYMBOL                                                   \
  "\xEF\x8B\x89" // термометр (тот же что температура)
#define MY_SUN_SYMBOL "\xEF\x86\x85"        //
#define MY_CLOUD_RAIN_SYMBOL "\xEF\x9C\xBD" //
#define MY_PRESSURE_SYMBOL "\xEF\x8F\xBD"   //
#define MY_BELL_SYMBOL "\xEF\x83\xB3"       // 🔔 праздник

enum namesOfStyles {
  STYLE_TEXT_TITLE,
  STYLE_TEXT_SMALL, //32
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

typedef enum {
  SETTINGS_PAGE_HOME = 0,
  SETTINGS_PAGE_DISPLAY,
  SETTINGS_PAGE_WIFI,
  SETTINGS_PAGE_BLUETOOTH,
  SETTINGS_PAGE_WEATHER,
  SETTINGS_PAGE_SENSORS,
  SETTINGS_PAGE_UPDATE,
  SETTINGS_PAGE_INFO,
  SETTINGS_PAGE_LAST_ELEMENT,
} settings_page_t;

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
  bool valid; // false до первого заполнения
} weather_snapshot_t;

typedef struct {
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
  uint8_t theme_mode; // 0=Auto 1=Hell 2=Dunkel
  bool theme_last_is_day; // кэш — последнее состояние get_is_day()
  uint8_t co2_mode;       // 0=Sensitiv 1=Normal 2=Robust
  uint8_t backlight_pct;  // ← новое
  uint8_t backlight_mode; // ← добавить: 0=Manuell 1=Auto 2=Zeitplan
  bool standby_screen_off;
} ui_switchs_t;

#define WIFI_SCAN_MAX_AP 15

typedef struct {
  // текущее подключение (верхний блок)
  lv_obj_t *connected_ssid_label;
  lv_obj_t *signal_bars[4];

  // список сетей
  lv_obj_t *scan_list;         // lv_list контейнер
  lv_obj_t *scan_status_label; // "Suche..." / "X Netze gefunden"

  // popup пароля (появляется по тапу на сеть)
  lv_obj_t *pass_popup;
  lv_obj_t *pass_popup_ssid_label;
  lv_obj_t *ta_pass;
  lv_obj_t *keyboard;

  // внутреннее состояние
  wifi_ap_record_t scan_results[WIFI_SCAN_MAX_AP];
  uint16_t scan_count;
  char pending_ssid[33]; // сеть выбранная пользователем
} ui_wifi_t;

typedef struct {
  ui_switchs_t switch_;
  lv_obj_t *btn_open;
  lv_obj_t *popup;
  lv_obj_t *btn_close;
  lv_obj_t *standby_btnmatrix;
  lv_obj_t *standby_desc_label;
  lv_obj_t *theme_btnmatrix; // Auto / Hell / Dunkel
  lv_obj_t *theme_desc_label;
  lv_obj_t *co2_btnmatrix;
  lv_obj_t *co2_desc_label;
  lv_obj_t *backlight_btnmatrix;
  lv_obj_t *backlight_desc_label;
  lv_obj_t *backlight_slider;
  lv_obj_t *backlight_pct_label;

  lv_obj_t *ota_btn;
  lv_obj_t *ota_status_label;
  lv_obj_t *ota_local_ver_label;  // "Aktuelle Version: 1.0.0"
  lv_obj_t *ota_remote_ver_label; // "Verfuegbar: 1.1.0"
  lv_obj_t *ota_check_btn;        // кнопка "Pruefen"
  lv_obj_t *ota_last_update_label;

  lv_obj_t *page_home;
  lv_obj_t *page_content;
  lv_obj_t *btn_back;
  settings_page_t current_page;
} ui_settings_t;

typedef struct {
  lv_obj_t *meter;
  lv_obj_t *co2_label;
  lv_obj_t *co2_unit_label;   // "ppm" под значением
  lv_obj_t *co2_status_label; // "GUT" / "MITTEL" / "SCHLECHT"
  lv_obj_t *co2_footer_label; // "CO₂ | Luftqualität" внизу
  lv_obj_t *co2_divider;      // горизонтальная линия
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
    lv_obj_t *ventilation_label;
#if CONFIG_HAS_SCD41
 lv_obj_t *icon_tvoc;
  lv_obj_t *tvoc_label;
  lv_obj_t *tvoc_calib_icon;
#endif

  lv_obj_t *icon_wind;
  lv_obj_t *temperature_label;
  lv_obj_t *humidity_label;


} ui_sensor_t;

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *hour_minute_label;
  lv_obj_t *mday_month_label;
  lv_obj_t *wday_labels[7]; // Mo Di Mi Do Fr Sa So
  lv_obj_t *wday_highlight; // скруглённый прямоугольник за текущим днём
  lv_obj_t *holiday_label; // MY_BELL_SYMBOL + название праздника
  lv_obj_t *wifi_status_icon;
} ui_time_t;

typedef struct {
  lv_style_t very_small_20;
  lv_style_t small_24; 
  lv_style_t medium_32;
  lv_style_t large_48;
  lv_style_t time;

  lv_style_t icon;
  lv_style_t nav_btn;

} font_style_t;

typedef struct {
  lv_style_t main;
  lv_style_t popup; 
  lv_style_t top_left;
  lv_style_t top_right;
  lv_style_t bot_left;
  lv_style_t bot_right;
  lv_style_t chart_co2;
  lv_style_t meter_co2;
  lv_style_t category_btn;

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

extern ui_main_menu_t ui;

void init_lv_objects();
void set_visible(lv_obj_t *parent, bool visible);
void show_all_blocks(ui_main_menu_t *ui);
void hide_all_blocks(ui_main_menu_t *ui);
void print_value(const char *flags, float value, lv_obj_t *parent);
#endif /* INC_UI_CORE_H_ */
