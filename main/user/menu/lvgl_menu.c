
#include "user/menu/lvgl_menu.h"
#include "esp_heap_caps.h"
#include "font/lv_symbol_def.h"
#include "lvgl_port.h"
#include "lvgl_user_config.h"
#include "misc/lv_area.h"
#include "misc/lv_color.h"
#include "user/periphery/periphery.h"
#include "waveshare_rgb_lcd_port.h"
#include <esp_log.h>
#include <math.h>
#include <stdbool.h>

#include "esp_wifi.h"
#include "user/periphery/open_meteo.h"
#include "user/periphery/wifi.h"
#include "user/periphery/nvs_user.h"

#define SENSOR_HISTORY_POINTS     72    // 6 часов × 12 точек/час
#define SENSOR_RECORD_INTERVAL   300    // тиков (секунд) между записями

// Кольцевой буфер для истории датчиков
typedef struct {
    int16_t temperature[SENSOR_HISTORY_POINTS];  // °C × 10 (фиксированная точка)
    uint8_t humidity[SENSOR_HISTORY_POINTS];     // %
    uint8_t  head;       // индекс следующей записи
    uint8_t  count;      // сколько точек уже заполнено (0..72)
} sensor_history_t;

static sensor_history_t sensor_history = {0};

typedef struct {
  lv_obj_t *screen;
  lv_obj_t *btn_temperature;
  lv_obj_t *btn_humidity;
  lv_obj_t *btn_wind;
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
  uint16_t selected_city;

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
  lv_obj_t *btn_temperature;
  lv_obj_t *btn_humidity;
  lv_obj_t *btn_tvoc;
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

} font_style_t;

typedef struct {
  lv_style_t top_left;
  lv_style_t top_right;
  lv_style_t bot_left;
  lv_style_t bot_right;
  lv_style_t chart_co2;

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

extern const city_t cities_de[];

static ui_main_menu_t ui = {
    .weather = {.cities_de = cities_de, .city_count = CITY_COUNT}};

extern lv_font_t my_symbols;
extern lv_font_t my_time_font;
extern wifi_ap_record_t ap_info;

static char string_buffer[STRING_MAX_LENGHT];

int standby_touched = 0;
static uint32_t timer_standby_sec;

LV_IMG_DECLARE(sun_48_48);
LV_IMG_DECLARE(moon_42_42);
LV_IMG_DECLARE(cloud_small_70_35);
LV_IMG_DECLARE(cloud_mid_90_45);
LV_IMG_DECLARE(cloud_big_110_50);
LV_IMG_DECLARE(cloud_thin_80_30);
LV_IMG_DECLARE(wind_60_50);
LV_IMG_DECLARE(rain_drop_heavy_9_22);
LV_IMG_DECLARE(snow_flake_2_15_15);

void hide_all_blocks(ui_main_menu_t *ui);
void show_all_blocks(ui_main_menu_t *ui);
static void ui_create_wifi_popup(ui_main_menu_t *ui);
static void ui_create_weather_popup(ui_main_menu_t *ui);
static void ui_create_settings_popup(ui_main_menu_t *ui);
static void ui_create_sensor_popup(ui_main_menu_t *ui);

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

    lv_obj_add_style(cont, &ui->font.small, 0);
    break;
    //  case STYLE_TEXT_MEDIUM:
    //    lv_obj_add_style(cont, &style[STYLE_TEXT_MEDIUM], 0);
    //    break;
    //  case STYLE_TEXT_LARGE:
    //    lv_obj_add_style(cont, &style[STYLE_TEXT_LARGE], 0);
    //    break;
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
      lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_GREEN), 0);
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
      lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, end_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, end_value_1);

  /*Make the tick lines red at the end of the scale*/
  ui->co2.indicator =
      lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED),
                               lv_palette_main(LV_PALETTE_RED), false, 0);
  lv_meter_set_indicator_start_value(meter, ui->co2.indicator, end_value);
  lv_meter_set_indicator_end_value(meter, ui->co2.indicator, end_value_1);

  /*Add a needle line indicator*/
  ui->co2.indicator = lv_meter_add_needle_line(
      meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);

  return meter;
}
static lv_chart_series_t *ser_co2;

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
  lv_chart_set_div_line_count(chart, 6, 9);
  lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, MIN_VALUE_CO2,
                     MAX_VALUE_CO2);
  // lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y, 5, 3, 12,2,true,
  // 40);
  ser_co2 = lv_chart_add_series(chart, lv_palette_main(LV_PALETTE_BROWN),
                                LV_CHART_AXIS_PRIMARY_Y);

  return chart;
}

void print_wday(uint8_t wday, ui_main_menu_t *ui) {
  lv_obj_t *parent = ui->time.wday_label;
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR print_wday");
    return;
  }

  switch (wday) {
  case 1:
    lv_label_set_text(parent, "mo");
    break;
  case 2:
    lv_label_set_text(parent, "tu");
    break;
  case 3:
    lv_label_set_text(parent, "we");
    break;
  case 4:
    lv_label_set_text(parent, "th");
    break;
  case 5:
    lv_label_set_text(parent, "fr");
    break;
  case 6:
    lv_label_set_text(parent, "sa");
    break;
  case 7:
    lv_label_set_text(parent, "su");
    break;
  default:
    lv_label_set_text(parent, "no wifi");
    break;
  }
}

void print_time(uint8_t time_hour, uint8_t time_minute, ui_main_menu_t *ui) {
  lv_obj_t *parent = ui->time.hour_minute_label;
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR print_time");
    return;
  }
  if (get_wifi_status() == WIFI_CONNECTED) {
    if (time_hour < 10 && time_minute < 10) {
      sprintf(string_buffer, "0%d:0%d", (int)time_hour, (int)time_minute);
    } else if (time_hour > 9 && time_minute < 10) {
      sprintf(string_buffer, "%d:0%d", (int)time_hour, (int)time_minute);
    } else if (time_hour < 10 && time_minute > 9) {
      sprintf(string_buffer, "0%d:%d", (int)time_hour, (int)time_minute);
    } else {
      sprintf(string_buffer, "%d:%d", (int)time_hour, (int)time_minute);
    }
  } else {
    sprintf(string_buffer, "0%d:0%d", (int)0, 0);
  }

  lv_label_set_text(parent, string_buffer);
}

void print_mday(uint8_t date_day, uint8_t date_month, ui_main_menu_t *ui) {
  lv_obj_t *parent = ui->time.mday_month_label;
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR print_mday");
    return;
  }
  if (get_wifi_status() == WIFI_CONNECTED) {
    if (date_day < 10 && date_month < 10) {
      sprintf(string_buffer, "0%d.0%d", (int)date_day, (int)date_month);
    } else if (date_day > 9 && date_month < 10) {
      sprintf(string_buffer, "%d.0%d", (int)date_day, (int)date_month);
    } else if (date_day < 10 && date_month > 9) {
      sprintf(string_buffer, "0%d.%d", (int)date_day, (int)date_month);
    } else {
      sprintf(string_buffer, "%d.%d", (int)date_day, (int)date_month);
    }
  } else {
    sprintf(string_buffer, "0%d.0%d", 0, 0);
  }
  lv_label_set_text(parent, string_buffer);
}

static lv_obj_t *create_button(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,

                               lv_align_t align, lv_coord_t x_ofs,
                               lv_coord_t y_ofs) {
  if (parent == NULL) {
    ESP_LOGE(TAG, "ERROR create_button");
    // return;
  }

  lv_obj_t *cont = lv_btn_create(parent);
  lv_obj_set_size(cont, w, h);
  lv_obj_align(cont, align, x_ofs, y_ofs);
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
    c->obj     = img;
    c->base_x  = x_ofs;
    c->base_y  = y_ofs;
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
    // [FIX D] Проверка перед lv_obj_set_pos
    if (!w || !w->obj || !lv_obj_is_valid(w->obj))
        return;
 
    if (w->container_w < 1)
        w->container_w = 1;
 
    lv_coord_t x = v - w->img_w;
    lv_coord_t y = w->base_y +
        (lv_trigo_sin((v * 360) / w->container_w) * w->turbulence) /
        LV_TRIGO_SIN_MAX / 3;
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
 
    lv_coord_t container_w = lv_obj_get_width(parent);
    lv_coord_t img_w       = lv_obj_get_width(img);
 
    static wind_anim_t wind_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_WINDS];
    static uint8_t pool_index = 0;
 
    if (pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_WINDS)
        pool_index = 0;
 
    wind_anim_t *w   = &wind_pool[pool_index++];
    w->obj           = img;
    w->base_y        = y_ofs;
    w->container_w   = container_w;
    w->img_w         = img_w;
    w->turbulence    = turbulence;
 
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, w);
    lv_anim_set_exec_cb(&a, wind_anim_cb);
    lv_anim_set_values(&a, -150, container_w + img_w);
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
    lv_coord_t img_w       = lv_obj_get_width(img);
    lv_coord_t img_h       = lv_obj_get_height(img);
 
    // [FIX B] Отдельный пул только для дождя
    static rain_snow_anim_t rain_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS];
    static uint8_t rain_pool_index = 0;
 
    if (rain_pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS)
        rain_pool_index = 0;
 
    rain_snow_anim_t *r = &rain_pool[rain_pool_index++];
    r->obj         = img;
    r->container_w = container_w;
    r->container_h = container_h;
    r->img_w       = img_w;
    r->img_h       = img_h;
    r->slope       = slope;
    r->phase       = lv_rand(0, container_w);
 
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
    lv_coord_t img_w       = lv_obj_get_width(img);
    lv_coord_t img_h       = lv_obj_get_height(img);
 
    // [FIX B] Отдельный пул только для снега
    static rain_snow_anim_t snow_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS];
    static uint8_t snow_pool_index = 0;
 
    if (snow_pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS)
        snow_pool_index = 0;
 
    rain_snow_anim_t *r = &snow_pool[snow_pool_index++];
    r->obj         = img;
    r->container_w = container_w;
    r->container_h = container_h;
    r->img_w       = img_w;
    r->img_h       = img_h;
    r->slope       = slope;
    r->phase       = lv_rand(0, container_w);
 
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
    h->temperature[h->head] = (int16_t)(get_temperature_aht10() * 10.0f + 0.5f);
    h->humidity[h->head]    = get_humidity_aht10();
 
    h->head = (h->head + 1) % SENSOR_HISTORY_POINTS;
 
    if (h->count < SENSOR_HISTORY_POINTS)
        h->count++;
}
static void sensor_history_get(const sensor_history_t *h, uint8_t idx,
                                int16_t *temp_x10, uint8_t *hum) {
    // Вычисляем реальный индекс в кольцевом буфере
    // head указывает на следующую ячейку для записи
    // старейшая точка: (head - count + POINTS) % POINTS
    uint8_t real_idx = (h->head - h->count + idx + SENSOR_HISTORY_POINTS)
                       % SENSOR_HISTORY_POINTS;
    *temp_x10 = h->temperature[real_idx];
    *hum      = h->humidity[real_idx];
}  


  
/////////////////////////////////////////////////temperature inside events
static void btn_tepmerature_inside_open_popup_event_handler(lv_event_t *e){
	 ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    // [FIX] Guard от двойного открытия
    if (ui->sensor.popup != NULL && lv_obj_is_valid(ui->sensor.popup))
        return;
 
    hide_all_blocks(ui);
    ui_create_sensor_popup(ui); 
}
static void btn_sensor_close_popup_event_handler(lv_event_t *e) {
    ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    lv_obj_del(ui->sensor.popup);
 
    ui->sensor.popup     = NULL;
    ui->sensor.btn_close = NULL;
    ui->sensor.chart     = NULL;
 
    show_all_blocks(ui);
}
/////////////////////////////////////////////////temperature inside events

static void ui_create_sensor_popup(ui_main_menu_t *ui){

   // --- Фон popup ---
    ui->sensor.popup =
        create_background(ui->screen, POPUP_WINDOW_WIDTH, POPUP_WINDOW_HEIGHT,
                          POPUP_WINDOW_ALIGN, 0, 0);
    lv_obj_set_scrollbar_mode(ui->sensor.popup, LV_SCROLLBAR_MODE_OFF);
 
    // --- Заголовок ---
    create_text("Temperature / Humidity  (6h)", ui->sensor.popup,
                STYLE_TEXT_SMALL, LV_ALIGN_TOP_MID, 0, 5, ui);
 
    // --- Кнопка закрытия ---
    ui->sensor.btn_close =
        create_btn_cb(ui->sensor.popup, 50, 50, LV_ALIGN_TOP_RIGHT, -5, -5,
                      btn_sensor_close_popup_event_handler, ui);
    lv_obj_set_style_bg_img_src(ui->sensor.btn_close, LV_SYMBOL_HOME, 0);
 
    // --- График ---
    // Размер: почти весь popup, отступы под оси
    lv_obj_t *chart = lv_chart_create(ui->sensor.popup);
    lv_obj_set_size(chart, POPUP_WINDOW_WIDTH - 80, POPUP_WINDOW_HEIGHT - 150);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, SENSOR_HISTORY_POINTS);
 
    // Сетка
    lv_chart_set_div_line_count(chart, 6, 6);
 
    // Диапазоны осей
    // Ось Y левая  — температура: 10..35 °C (×10 → 100..350)
    lv_chart_set_range(chart, LV_CHART_AXIS_PRIMARY_Y, 100, 350);
    // Ось Y правая — влажность: 20..80 %
    lv_chart_set_range(chart, LV_CHART_AXIS_SECONDARY_Y, 20, 80);
 
    // Засечки осей
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_PRIMARY_Y,
                           5, 3,   // major/minor tick length
                           6, 1,   // 6 делений, каждая подписана
                           true, 50);
    lv_chart_set_axis_tick(chart, LV_CHART_AXIS_SECONDARY_Y,
                           5, 3,
                           6, 1,
                           true, 40);
 
    // --- Серия температуры (левая ось, красная) ---
    lv_chart_series_t *ser_temp =
        lv_chart_add_series(chart,
                            lv_palette_main(LV_PALETTE_RED),
                            LV_CHART_AXIS_PRIMARY_Y);
 
    // --- Серия влажности (правая ось, синяя) ---
    lv_chart_series_t *ser_hum =
        lv_chart_add_series(chart,
                            lv_palette_main(LV_PALETTE_BLUE),
                            LV_CHART_AXIS_SECONDARY_Y);
 
    // --- Заполняем серии из кольцевого буфера ---
    uint8_t n = sensor_history.count;
 
    if (n == 0) {
        // Буфер пустой — заполняем LV_CHART_POINT_NONE (пунктир не рисуется)
        for (uint8_t i = 0; i < SENSOR_HISTORY_POINTS; i++) {
            lv_chart_set_value_by_id(chart, ser_temp, i, LV_CHART_POINT_NONE);
            lv_chart_set_value_by_id(chart, ser_hum,  i, LV_CHART_POINT_NONE);
        }
    } else {
        // Сначала заполняем пустые слоты в начале (если буфер ещё не полный)
        uint8_t empty_slots = SENSOR_HISTORY_POINTS - n;
        for (uint8_t i = 0; i < empty_slots; i++) {
            lv_chart_set_value_by_id(chart, ser_temp, i, LV_CHART_POINT_NONE);
            lv_chart_set_value_by_id(chart, ser_hum,  i, LV_CHART_POINT_NONE);
        }
        // Затем реальные данные из буфера (старые → новые)
        for (uint8_t i = 0; i < n; i++) {
            int16_t temp_x10;
            uint8_t hum;
            sensor_history_get(&sensor_history, i, &temp_x10, &hum);
            lv_chart_set_value_by_id(chart, ser_temp, empty_slots + i, temp_x10);
            lv_chart_set_value_by_id(chart, ser_hum,  empty_slots + i, hum);
        }
    }
 
    lv_chart_refresh(chart);
    ui->sensor.chart = chart;
                
}

static void create_block_top_left(ui_main_menu_t *ui) {
  /*STYLES*/

  /*BLOCK TOP LEFT*/
  lv_obj_t *bg =

      create_background(ui->screen, BLOCK_TOP_LEFT_WIDTH, BLOCK_TOP_LEFT_HEIGHT,
                        BLOCK_TOP_LEFT_ALIGN_BACKGROUND, BLOCK_TOP_LEFT_X_START,
                        BLOCK_TOP_LEFT_Y_START);
  if (!bg)
    return;
  ui->sensor.screen = bg;

  lv_obj_add_style(ui->sensor.screen, &ui->style.top_left, 0);
    lv_obj_set_scrollbar_mode(ui->sensor.screen, LV_SCROLLBAR_MODE_OFF);
  

  create_text("inside", ui->sensor.screen, STYLE_TEXT_SMALL,
              BLOCK_TOP_LEFT_ALIGN_TITLE, 0, BLOCK_TOP_LEFT_Y_START_TITLE, ui);

  lv_obj_t *btn_symbol =   create_btn_cb(
      ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
      BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
      BLOCK_TOP_LEFT_X_START_SYMBOLS, BLOCK_TOP_LEFT_Y_START_SYMBOLS_1,btn_tepmerature_inside_open_popup_event_handler,ui);
  if (!btn_symbol)
    return;
  ui->sensor.btn_temperature = btn_symbol;

  lv_obj_set_style_text_font(ui->sensor.btn_temperature, &my_symbols, 0);
  lv_obj_set_style_bg_img_src(ui->sensor.btn_temperature, MY_TEMPERATURE_SYMBOL,
                              0);

  btn_symbol = create_button(
      ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
      BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
      BLOCK_TOP_LEFT_X_START_SYMBOLS, BLOCK_TOP_LEFT_Y_START_SYMBOLS_2);
  if (!btn_symbol)
    return;

  ui->sensor.btn_humidity = btn_symbol;
  lv_obj_set_style_text_font(ui->sensor.btn_humidity, &my_symbols, 0);
  lv_obj_set_style_bg_img_src(ui->sensor.btn_humidity, MY_HUMIDITY_SYMBOL, 0);

  btn_symbol = create_button(
      ui->sensor.screen, BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
      BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
      BLOCK_TOP_LEFT_X_START_SYMBOLS, BLOCK_TOP_LEFT_Y_START_SYMBOLS_3);
  if (!btn_symbol)
    return;

  ui->sensor.btn_tvoc = btn_symbol;
  lv_obj_set_style_text_font(ui->sensor.btn_tvoc, &my_symbols, 0);
  lv_obj_set_style_bg_img_src(ui->sensor.btn_tvoc, MY_TVOC_SYMBOL, 0);

  lv_obj_t *value = create_label(
      ui->sensor.screen, "0 c*", BLOCK_TOP_LEFT_ALIGN_VALUES,
      BLOCK_TOP_LEFT_X_START_VALUES, BLOCK_TOP_LEFT_Y_START_VALUE_1);
  if (!value)
    return;

  ui->sensor.temperature_label = value;

  value = create_label(ui->sensor.screen, "0 %*", BLOCK_TOP_LEFT_ALIGN_VALUES,
                       BLOCK_TOP_LEFT_X_START_VALUES,
                       BLOCK_TOP_LEFT_Y_START_VALUE_2);
  if (!value)
    return;

  ui->sensor.humidity_label = value;

  value = create_label(ui->sensor.screen, "0 ppm", BLOCK_TOP_LEFT_ALIGN_VALUES,
                       BLOCK_TOP_LEFT_X_START_VALUES,
                       BLOCK_TOP_LEFT_Y_START_VALUE_3);
  if (!value)
    return;
  ui->sensor.tvoc_label = value;

  /*BLOCK TOP LEFT*/
}

static void create_block_top_middle(ui_main_menu_t *ui) {
  /*BLOCK TOP MID*/

  ui->co2.meter = create_meter(
      ui->screen, BLOCK_TOP_MID_WIDTH_CO2_METER, BLOCK_TOP_MID_HEIGHT_CO2_METER,
      BLOCK_TOP_MID_ALIGN_CO2_CHART, BLOCK_TOP_MID_X_START,
      BLOCK_TOP_MID_Y_START, ui);
  if (!ui->co2.meter)
    return;

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
  lv_obj_add_style(ui->co2.co2_label, &ui->font.very_large, 0);
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

  ui->time.screen = bg;
  lv_obj_add_style(ui->time.screen, &ui->style.top_right, 0);
  lv_obj_set_scrollbar_mode(ui->time.screen, LV_SCROLLBAR_MODE_OFF);

  create_text("time", ui->time.screen, STYLE_TEXT_SMALL,
              BLOCK_TOP_RIGHT_ALIGN_TITLE, 0, BLOCK_TOP_RIGHT_Y_START_TITLE,
              ui);

  lv_obj_t *time = create_label(
      ui->time.screen, "00:00", BLOCK_TOP_RIGHT_ALIGN_VALUES,
      BLOCK_TOP_RIGHT_X_START_VALUES, BLOCK_TOP_RIGHT_Y_START_VALUE_1);
  if (!time)
    return;

  ui->time.hour_minute_label = time;
  lv_obj_set_style_text_font(ui->time.hour_minute_label, &my_time_font, 0);

  time = create_label(ui->time.screen, "00.00", BLOCK_TOP_RIGHT_ALIGN_VALUES,
                      BLOCK_TOP_RIGHT_X_START_VALUES,
                      BLOCK_TOP_RIGHT_Y_START_VALUE_2);
  if (!time)
    return;

  ui->time.mday_month_label = time;
  lv_obj_set_style_text_font(ui->time.mday_month_label, &my_time_font, 0);

  time = create_label(ui->time.screen, "load...", BLOCK_TOP_RIGHT_ALIGN_VALUES,
                      BLOCK_TOP_RIGHT_X_START_VALUE_3,
                      BLOCK_TOP_RIGHT_Y_START_VALUE_3);
  if (!time)
    return;
  ui->time.wday_label = time;

  lv_obj_set_style_text_font(ui->time.wday_label, &lv_font_montserrat_32, 0);
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

  ui->meteo.screen = bg;
  lv_obj_add_style(ui->meteo.screen, &ui->style.bot_left, 0);
  lv_obj_set_scrollbar_mode(ui->meteo.screen, LV_SCROLLBAR_MODE_OFF);
  create_text("outside", ui->meteo.screen, STYLE_TEXT_SMALL,
              BLOCK_BOT_LEFT_ALIGN_TITLE, 0, BLOCK_BOT_LEFT_Y_START_TITLE, ui);

  ui->meteo.screen = bg;
  lv_obj_add_style(ui->meteo.screen, &ui->style.bot_left, 0);
  create_text("outside", ui->meteo.screen, STYLE_TEXT_SMALL,
              BLOCK_BOT_LEFT_ALIGN_TITLE, 0, BLOCK_BOT_LEFT_Y_START_TITLE, ui);

  lv_obj_t *btn_symbol = create_button(
      ui->meteo.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
      BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
      BLOCK_BOT_LEFT_X_START_SYMBOLS, BLOCK_BOT_LEFT_Y_START_SYMBOLS_1);
  if (!btn_symbol)
    return;

  ui->meteo.btn_temperature = btn_symbol;
  lv_obj_set_style_text_font(ui->meteo.btn_temperature, &my_symbols, 0);
  lv_obj_set_style_bg_img_src(ui->meteo.btn_temperature, MY_TEMPERATURE_SYMBOL,
                              0);

  btn_symbol = create_button(
      ui->meteo.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
      BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
      BLOCK_BOT_LEFT_X_START_SYMBOLS, BLOCK_BOT_LEFT_Y_START_SYMBOLS_2);
  if (!btn_symbol)
    return;
  ui->meteo.btn_humidity = btn_symbol;
  lv_obj_set_style_text_font(ui->meteo.btn_humidity, &my_symbols, 0);
  lv_obj_set_style_bg_img_src(ui->meteo.btn_humidity, MY_HUMIDITY_SYMBOL, 0);

  btn_symbol = create_button(
      ui->meteo.screen, BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
      BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
      BLOCK_BOT_LEFT_X_START_SYMBOLS, BLOCK_BOT_LEFT_Y_START_SYMBOLS_3);
  if (!btn_symbol)
    return;

  ui->meteo.btn_wind = btn_symbol;
  lv_obj_set_style_text_font(ui->meteo.btn_wind, &my_symbols, 0);
  lv_obj_set_style_bg_img_src(ui->meteo.btn_wind, MY_WIND_SYMBOL, 0);

  lv_obj_t *value = create_label(
      ui->meteo.screen, "0 c*", BLOCK_BOT_LEFT_ALIGN_VALUES,
      BLOCK_BOT_LEFT_X_START_VALUES, BLOCK_BOT_LEFT_Y_START_VALUE_1);

  if (!value)
    return;

  ui->meteo.temperature_label = value;

  value = create_label(ui->meteo.screen, "0 %", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES,
                       BLOCK_BOT_LEFT_Y_START_VALUE_2);

  if (!value)
    return;

  ui->meteo.humidity_label = value;

  value = create_label(ui->meteo.screen, "0 m/s", BLOCK_BOT_LEFT_ALIGN_VALUES,
                       BLOCK_BOT_LEFT_X_START_VALUES,
                       BLOCK_BOT_LEFT_Y_START_VALUE_3);
  if (!value)
    return;
  ui->meteo.wind_label = value;
  /*BLOCK BOT LEFT*/
}
/////////////////////////////////////////////////wifi events
static void btn_wifi_open_popup_event_handler(lv_event_t *e) {

  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
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
    if (!ui) return;
 
    // show_all_blocks ПЕРЕД del — иначе обращаемся к уже удалённым объектам
    show_all_blocks(ui);
 
    lv_obj_del(ui->wifi.popup);
 
    // [FIX] Обнуляем ВСЕ дочерние указатели — они удалены вместе с popup
    ui->wifi.popup             = NULL;
    ui->wifi.btn_close         = NULL;
    ui->wifi.btn_keyboard_ssid = NULL;
    ui->wifi.btn_keyboard_pass = NULL;
    ui->wifi.ssid_label        = NULL;
    ui->wifi.pass_label        = NULL;
    ui->wifi.rssi_label        = NULL;
    ui->wifi.keyboard          = NULL;
    ui->wifi.ta_ssid           = NULL;
    ui->wifi.ta_pass           = NULL;
}
static void btn_keyboard_open_ssid_event_handler(lv_event_t *e) {
   ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    // [FIX] Проверяем валидность перед использованием
    if (!lv_obj_is_valid(ui->wifi.keyboard) ||
        !lv_obj_is_valid(ui->wifi.ta_ssid))
        return;
 
    lv_keyboard_set_textarea(ui->wifi.keyboard, ui->wifi.ta_ssid);
    set_visible(ui->wifi.keyboard, true);
    set_visible(ui->wifi.ta_ssid, true);
    set_visible(ui->wifi.ta_pass, false); // скрываем pass если был открыт
}
static void btn_keyboard_open_pass_event_handler(lv_event_t *e) {
   ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    // [FIX] Проверяем валидность перед использованием
    if (!lv_obj_is_valid(ui->wifi.keyboard) ||
        !lv_obj_is_valid(ui->wifi.ta_pass))
        return;
 
    lv_keyboard_set_textarea(ui->wifi.keyboard, ui->wifi.ta_pass);
    set_visible(ui->wifi.keyboard, true);
    set_visible(ui->wifi.ta_pass, true);
    set_visible(ui->wifi.ta_ssid, false); // скрываем ssid если был открыт
}
static void ta_wifi_event_cb(lv_event_t *e) {
   // [FIX] Было &ui (адрес локального параметра!) — теперь ui
    ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *kb = lv_event_get_target(e);
 
    if (code == LV_EVENT_READY) {
        if (!lv_obj_is_valid(ui->wifi.ta_ssid) ||
            !lv_obj_is_valid(ui->wifi.ta_pass))
            return;
 
        const char *ssid = lv_textarea_get_text(ui->wifi.ta_ssid);
        const char *pass = lv_textarea_get_text(ui->wifi.ta_pass);
 
        wifi_connect(ssid, pass);
 
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
    if (!ui) return;
 
    // [FIX] Guard от двойного открытия
    if (ui->settings.popup != NULL && lv_obj_is_valid(ui->settings.popup))
        return;
 
    hide_all_blocks(ui);
    ui_create_settings_popup(ui);
}
static void btn_settings_close_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    lv_obj_del(ui->settings.popup);
 
    // [FIX] Обнуляем ВСЕ дочерние указатели
    ui->settings.popup            = NULL;
    ui->settings.btn_close        = NULL;
    ui->settings.btn_save         = NULL;
    ui->settings.standby_flag     = NULL;
    ui->settings.standby_time     = NULL;
    ui->settings.standby_day_night= NULL;
    ui->settings.switch_.standby  = NULL;
 
    show_all_blocks(ui);
}
/////////////////////////////////////////////////settings events

/////////////////////////////////////////////////weather settings events
static void btn_weather_open_popup_event_handler(lv_event_t *e) {
 ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    // [FIX] Guard от двойного открытия
    if (ui->weather.popup != NULL && lv_obj_is_valid(ui->weather.popup))
        return;
 
    hide_all_blocks(ui);
    ui_create_weather_popup(ui);
 
    // Показываем последний выбранный город сразу при открытии
    // (ui_create_city_list_weather тоже это делает, но на случай изменений)
    if (lv_obj_is_valid(ui->weather.city_label))
        lv_label_set_text(ui->weather.city_label,
                          ui->weather.cities_de[ui->weather.selected_city].name);
}
static void btn_weather_open_list_city_event_handler(lv_event_t *e) {
   ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    if (lv_obj_is_valid(ui->weather.citys_list))
        set_visible(ui->weather.citys_list, true);
}
static void btn_weather_close_popup_event_handler(lv_event_t *e) {
  ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    lv_obj_del(ui->weather.popup);
 
    // [FIX] Обнуляем ВСЕ дочерние указатели
    // citys_list и city_label — дочерние popup, удалены автоматически
    ui->weather.popup              = NULL;
    ui->weather.btn_close          = NULL;
    ui->weather.btn_open_city_list = NULL;
    ui->weather.city_label         = NULL;
    ui->weather.citys_list         = NULL;
 
    show_all_blocks(ui);
}
static void set_current_city_weather_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;
 
    // [FIX] ui передаётся через event user_data (было: передавался индекс i!)
    ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);
    if (!ui) return;
 
    // [FIX] Индекс города хранится в самом объекте кнопки через lv_obj_set_user_data
    // Не конфликтует с event user_data — это отдельный слот LVGL объекта
    lv_obj_t *btn = lv_event_get_target(e);
    uint16_t city = (uint16_t)(uintptr_t)lv_obj_get_user_data(btn);
 
    // Сохраняем выбор — пригодится при следующем открытии popup
    ui->weather.selected_city = city;
 
    if (lv_obj_is_valid(ui->weather.citys_list))
        set_visible(ui->weather.citys_list, false);
 
    if (lv_obj_is_valid(ui->weather.city_label))
        lv_label_set_text(ui->weather.city_label,
                          ui->weather.cities_de[city].name);
 
    build_weather_url(city);
}
/////////////////////////////////////////////////weather settings events



static void ui_create_co2(ui_main_menu_t *ui) {
  create_text("co2 24h", ui->screen, STYLE_TEXT_SMALL,
              BLOCK_BOT_MID_ALIGN_CO2_CHART, 0,
              BLOCK_BOT_MID_Y_START_TITLE_CO2_CHART, ui);

  ui->co2.chart = create_chart(
      ui->screen, BLOCK_BOT_MID_WIDTH_CO2_CHART, BLOCK_BOT_MID_HEIGHT_CO2_CHART,
      BLOCK_BOT_MID_ALIGN_CO2_CHART, 0, BLOCK_BOT_MID_Y_START_CO2_CHART);

  lv_obj_add_style(ui->co2.chart, &ui->style.chart_co2, 0);
}

static void ui_create_city_list_weather(ui_main_menu_t *ui) {

   // [FIX] Родитель — ui->weather.popup, НЕ ui->screen!
    // Если родитель screen — список не удаляется вместе с popup
    // и зависает поверх интерфейса навсегда
    ui->weather.citys_list = lv_list_create(ui->weather.popup);
    lv_obj_set_size(ui->weather.citys_list, 350, 250);
    lv_obj_center(ui->weather.citys_list);
 
    for (int i = 0; i < ui->weather.city_count; i++) {
        lv_obj_t *btn = lv_list_add_btn(
            ui->weather.citys_list, NULL, ui->weather.cities_de[i].name);
 
        // [FIX] Индекс города — в объект кнопки (lv_obj_set_user_data)
        lv_obj_set_user_data(btn, (void *)(uintptr_t)i);
 
        // [FIX] ui — как event user_data (указатель на глобальную структуру)
        // Раньше здесь был (void*)(uintptr_t)i — разыменовывался как указатель!
        lv_obj_add_event_cb(btn, set_current_city_weather_event_handler,
                            LV_EVENT_CLICKED, ui);
    }
 
    // Показываем текущий выбранный город в лейбле сразу при открытии списка
    if (lv_obj_is_valid(ui->weather.city_label))
        lv_label_set_text(ui->weather.city_label,
                          ui->weather.cities_de[ui->weather.selected_city].name);
 
    set_visible(ui->weather.citys_list, false);
}

static void ui_create_weather_popup(ui_main_menu_t *ui) {
  ui->weather.popup =
      create_background(ui->screen, POPUP_WINDOW_WIDTH, POPUP_WINDOW_HEIGHT,
                        POPUP_WINDOW_ALIGN, 0, 0);

  ui->weather.btn_close =
      create_btn_cb(ui->weather.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, -10,
                    btn_weather_close_popup_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->weather.btn_close, LV_SYMBOL_HOME, 0);

  ui->weather.btn_open_city_list =
      create_btn_cb(ui->weather.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, 90,
                    btn_weather_open_list_city_event_handler, ui);
  lv_obj_set_style_bg_img_src(ui->weather.btn_open_city_list, LV_SYMBOL_GPS, 0);

  ui->weather.city_label =
      create_label(ui->weather.popup, "CityName", LV_ALIGN_TOP_LEFT, 220, 90);

  create_text("Weather Settings", ui->weather.popup, STYLE_TEXT_SMALL,
              LV_ALIGN_TOP_MID, 0, 0, ui);
  create_text("CITY:", ui->weather.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT,
              15, 90, ui);
  create_text("OPTION:", ui->weather.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT,
              15, 180, ui);
  create_text("OPTION:", ui->weather.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT,
              15, 270, ui);
  ui_create_city_list_weather(ui);
}

static void ui_create_settings_popup_btns(ui_main_menu_t *ui) {
  ///////////////////////////////////////////////////////////////
 ui->weather.btn_open = create_btn_cb(
        ui->screen, BLOCK_BOT_MID_WIDTH_SYMBOL, BLOCK_BOT_MID_HEIGHT_SYMBOL,
        BLOCK_BOT_MID_ALIGN_SYMBOL, BLOCK_BOT_MID_X_START_SYMBOL_1,
        BLOCK_BOT_MID_Y_START_SYMBOLS, btn_weather_open_popup_event_handler, ui);
    lv_obj_set_style_text_font(ui->weather.btn_open, &my_symbols, 0);
    lv_obj_set_style_bg_img_src(ui->weather.btn_open, MY_CLOUD_SYMBOL, 0);
 
    ui->wifi.btn_open = create_btn_cb(
        ui->screen, BLOCK_BOT_MID_WIDTH_SYMBOL, BLOCK_BOT_MID_HEIGHT_SYMBOL,
        BLOCK_BOT_MID_ALIGN_SYMBOL, BLOCK_BOT_MID_X_START_SYMBOL_2,
        BLOCK_BOT_MID_Y_START_SYMBOLS, btn_wifi_open_popup_event_handler, ui);
    lv_obj_set_style_bg_img_src(ui->wifi.btn_open, LV_SYMBOL_SETTINGS, 0);
 
    ui->settings.btn_open = create_btn_cb(
        ui->screen, BLOCK_BOT_MID_WIDTH_SYMBOL, BLOCK_BOT_MID_HEIGHT_SYMBOL,
        BLOCK_BOT_MID_ALIGN_SYMBOL, BLOCK_BOT_MID_X_START_SYMBOL_3,
        BLOCK_BOT_MID_Y_START_SYMBOLS, btn_settings_open_popup_event_handler,
        ui);  // [FIX] Было &ui — адрес локального параметра!
    lv_obj_set_style_bg_img_src(ui->settings.btn_open, LV_SYMBOL_SETTINGS, 0);
}

static void ui_create_wifi_popup(ui_main_menu_t *ui) {

   ui->wifi.popup =
        create_background(ui->screen, POPUP_WINDOW_WIDTH, POPUP_WINDOW_HEIGHT,
                          POPUP_WINDOW_ALIGN, 0, 0);
    lv_obj_set_scrollbar_mode(ui->wifi.popup, LV_SCROLLBAR_MODE_OFF);
 
    create_text("WIFI Settings", ui->wifi.popup, STYLE_TEXT_SMALL,
                LV_ALIGN_TOP_MID, 0, 0, ui);
    create_text("SSID:", ui->wifi.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT,
                15, 90, ui);
    create_text("PASS:", ui->wifi.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT,
                15, 180, ui);
    create_text("RSSI:", ui->wifi.popup, STYLE_TEXT_SMALL, LV_ALIGN_TOP_LEFT,
                15, 270, ui);
 
    ui->wifi.btn_close =
        create_btn_cb(ui->wifi.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, -10,
                      btn_wifi_close_popup_event_handler, ui);
    lv_obj_set_style_bg_img_src(ui->wifi.btn_close, LV_SYMBOL_HOME, 0);
 
    ui->wifi.btn_keyboard_ssid =
        create_btn_cb(ui->wifi.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, 90,
                      btn_keyboard_open_ssid_event_handler, ui);
    lv_obj_set_style_bg_img_src(ui->wifi.btn_keyboard_ssid, LV_SYMBOL_KEYBOARD, 0);
 
    ui->wifi.btn_keyboard_pass =
        create_btn_cb(ui->wifi.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, 180,
                      btn_keyboard_open_pass_event_handler, ui);
    lv_obj_set_style_bg_img_src(ui->wifi.btn_keyboard_pass, LV_SYMBOL_KEYBOARD, 0);
 
    ui->wifi.ssid_label =
        create_label(ui->wifi.popup, "WiFiName", LV_ALIGN_TOP_LEFT, 220, 90);
    ui->wifi.pass_label =
        create_label(ui->wifi.popup, "*********", LV_ALIGN_TOP_LEFT, 220, 180);
    ui->wifi.rssi_label =
        create_label(ui->wifi.popup, "WiFiRSSI", LV_ALIGN_TOP_LEFT, 220, 270);
 
    ui->wifi.keyboard = lv_keyboard_create(ui->wifi.popup);
    // [FIX] Было &ui (адрес локального параметра функции — UB после возврата!)
    // Теперь ui — указатель на глобальную структуру, всегда валиден
    lv_obj_add_event_cb(ui->wifi.keyboard, ta_wifi_event_cb, LV_EVENT_ALL, ui);
 
    ui->wifi.ta_ssid = lv_textarea_create(ui->wifi.popup);
    lv_obj_align(ui->wifi.ta_ssid, LV_ALIGN_TOP_MID, 0, 35);
    lv_textarea_set_placeholder_text(ui->wifi.ta_ssid, "WiFi SSID");
    lv_obj_set_size(ui->wifi.ta_ssid, 600, 120);
 
    ui->wifi.ta_pass = lv_textarea_create(ui->wifi.popup);
    lv_obj_align(ui->wifi.ta_pass, LV_ALIGN_TOP_MID, 0, 35);
    // [FIX] Было "WiFi SSID" — опечатка в placeholder для pass
    lv_textarea_set_placeholder_text(ui->wifi.ta_pass, "WiFi Password");
    lv_obj_set_size(ui->wifi.ta_pass, 600, 120);
 
    set_visible(ui->wifi.ta_ssid, false);
    set_visible(ui->wifi.ta_pass, false);
    set_visible(ui->wifi.keyboard, false);
}

static void ui_create_standby(ui_main_menu_t *ui) {

  ui->standby.bar = lv_bar_create(ui->screen);
  lv_obj_align(ui->standby.bar, BLOCK_BOT_MID_ALIGN_STANDBY_BAR, 0,
               BLOCK_BOT_MID_Y_START_STANDBY_BAR);
  lv_obj_set_size(ui->standby.bar, BLOCK_BOT_MID_WIDTH_STANDBY_BAR,
                  BLOCK_BOT_MID_HEIGHT_STANDBY_BAR);
  lv_bar_set_range(ui->standby.bar, 0, MAX_STANDBY_TIME);
}

static void switch_event_cb(lv_event_t *e) {
    // Вызывается при любом изменении любого свитча
    ui_main_menu_t *ui = (ui_main_menu_t *)lv_event_get_user_data(e);

    ui->settings.switch_.standby_status = lv_obj_has_state(ui->settings.switch_.standby, LV_STATE_CHECKED);
    ui->settings.switch_.theme_status   = lv_obj_has_state(ui->settings.switch_.theme,   LV_STATE_CHECKED);

    settings_save(ui->settings.switch_.standby_status, ui->settings.switch_.theme_status);
}

static void ui_create_settigs_switches(ui_main_menu_t *ui) {
  ui->settings.switch_.standby = lv_switch_create(ui->settings.popup);
    lv_obj_align(ui->settings.switch_.standby, LV_ALIGN_TOP_LEFT, 500, 90);

    ui->settings.switch_.theme = lv_switch_create(ui->settings.popup);
    lv_obj_align(ui->settings.switch_.theme, LV_ALIGN_TOP_LEFT, 500, 180);
lv_obj_add_event_cb(ui->settings.switch_.standby, switch_event_cb, LV_EVENT_VALUE_CHANGED, ui);
lv_obj_add_event_cb(ui->settings.switch_.theme,   switch_event_cb, LV_EVENT_VALUE_CHANGED, ui);
    // Загружаем сохранённые значения

    settings_load(&ui->settings.switch_.standby_status, &ui->settings.switch_.theme_status);


    if (ui->settings.switch_.standby_status) lv_obj_add_state(ui->settings.switch_.standby, LV_STATE_CHECKED);
    else         lv_obj_clear_state(ui->settings.switch_.standby, LV_STATE_CHECKED);

    if (ui->settings.switch_.theme_status)   lv_obj_add_state(ui->settings.switch_.theme, LV_STATE_CHECKED);
    else         lv_obj_clear_state(ui->settings.switch_.theme, LV_STATE_CHECKED);
}


static void ui_create_settings_popup(ui_main_menu_t *ui) {
   ui->settings.popup =
        create_background(ui->screen, POPUP_WINDOW_WIDTH, POPUP_WINDOW_HEIGHT,
                          POPUP_WINDOW_ALIGN, 0, 0);
    lv_obj_set_scrollbar_mode(ui->settings.popup, LV_SCROLLBAR_MODE_OFF);
 
    create_text("Settings", ui->settings.popup, STYLE_TEXT_SMALL,
                LV_ALIGN_TOP_MID, 0, 0, ui);
    create_text("Standby:", ui->settings.popup, STYLE_TEXT_SMALL,
                LV_ALIGN_TOP_LEFT, 15, 90, ui);
    create_text("Theme Light/Dark:", ui->settings.popup, STYLE_TEXT_SMALL,
                LV_ALIGN_TOP_LEFT, 15, 180, ui);
 
    ui->settings.btn_close =
        create_btn_cb(ui->settings.popup, 50, 50, LV_ALIGN_TOP_LEFT, 500, -10,
                      btn_settings_close_popup_event_handler, ui);
    lv_obj_set_style_bg_img_src(ui->settings.btn_close, LV_SYMBOL_HOME, 0);
 
 
    // [FIX 4A] Switches создаются здесь — внутри popup, не при init
    ui_create_settigs_switches(ui);
}

static void create_block_bot_middle(ui_main_menu_t *ui) {
  ui_create_co2(ui);
  ui_create_settings_popup_btns(ui);
  ui_create_standby(ui);
}

static void create_block_bot_right(ui_main_menu_t *ui) {
  ui->animation.screen = create_background(
        ui->screen, BLOCK_BOT_RIGHT_WIDTH, BLOCK_BOT_RIGHT_HEIGHT,
        BLOCK_BOT_RIGHT_ALIGN_BACKGROUND, BLOCK_BOT_RIGHT_X_START,
        BLOCK_BOT_RIGHT_Y_START);
    lv_obj_add_style(ui->animation.screen, &ui->style.bot_right, 0);
    lv_obj_set_scrollbar_mode(ui->animation.screen, LV_SCROLLBAR_MODE_OFF);
 
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
 
    ui->animation.image.cloud.mid_90_45 = create_cloud_anim(
        &cloud_mid_90_45, ui->animation.screen,
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
        ui->animation.rain[i] = create_rain_anim(
            &rain_drop_heavy_9_22,
            ui->animation.screen,
            2,  // slope: 2 = крутой угол
            BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_RAIN_SNOW);
        set_visible(ui->animation.rain[i], false);
    }
#endif
 
#if ACTIVATE_ANIM_SNOW
    // [FIX 4B] Используем create_snow_anim — отдельный пул только для снега
    // [FIX 4C] slope=0 — снег падает вертикально
    for (uint8_t i = 0; i < BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS; i++) {
        ui->animation.snow[i] = create_snow_anim(
            &snow_flake_2_15_15,
            ui->animation.screen,
            0,  // slope: 0 = вертикально
            BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_RAIN_SNOW);
        set_visible(ui->animation.snow[i], false);
    }
#endif
}

static void create_menu(ui_main_menu_t *ui) {
settings_load(&ui->settings.switch_.standby_status, &ui->settings.switch_.theme_status);
  ui->screen = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(ui->screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(ui->screen, LV_OPA_COVER, 0);
  lv_obj_set_size(ui->screen, LVGL_PORT_H_RES, LVGL_PORT_V_RES);
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
  printf("After UI: %d\n", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
}

void draw_menu_main(ui_main_menu_t *ui) { lv_scr_load(ui->screen); }

void rain_set_intensity(uint8_t visible_count, ui_main_menu_t *ui) {
  static uint8_t prev = 255; // 255 = невалидное значение → форсирует первый update
 
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
        for (uint8_t i = visible_count; i < (prev == 255 ? BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS : prev); i++) {
            if (ui->animation.rain[i] && lv_obj_is_valid(ui->animation.rain[i]))
                lv_obj_add_flag(ui->animation.rain[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
 
    prev = visible_count;
}

void snow_set_intensity(uint8_t visible_count, ui_main_menu_t *ui) {

  static uint8_t prev = 255; // 255 = невалидное значение → форсирует первый update
 
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
        for (uint8_t i = visible_count; i < (prev == 255 ? BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS : prev); i++) {
            if (ui->animation.snow[i] && lv_obj_is_valid(ui->animation.snow[i]))
                lv_obj_add_flag(ui->animation.snow[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
 
    prev = visible_count;
}

void draw_weather_sun_moon(ui_main_menu_t *ui) {
#if SIMULATE_ANIM_SUN
  set_visible(ui->animation.image.sun_48_48, true);
  set_visible(ui->animation.image.moon_42_42, false);
#else

  uint8_t is_day = get_is_day();
  static uint8_t prev = 255;

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
#endif
}

void draw_weather_clouds(ui_main_menu_t *ui) {
#if SIMULATE_ANIM_CLOUD
  set_visible(ui->animation.image.cloud.big_110_50, true);
  set_visible(ui->animation.image.cloud.mid_90_45, true);
  set_visible(ui->animation.image.cloud.small_70_35, true);
  set_visible(ui->animation.image.cloud.thin_80_30, true);
#else
  uint8_t current_clouds = get_weather_clouds();
  static uint8_t prev = 0;

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

#endif
}

void draw_weather_wind(ui_main_menu_t *ui) {
#if SIMULATE_ANIM_WIND
  set_visible(ui->animation.wind.slow, true);
  set_visible(ui->animation.wind.med, true);
  set_visible(ui->animation.wind.fast, true);
#else
  uint8_t current_wind = get_weather_wind();
  static uint8_t prev = 0;

  if (current_wind == prev)
    return;

  if (current_wind < 2) {
    set_visible(ui->animation.wind.slow, false);
    set_visible(ui->animation.wind.med, false);
    set_visible(ui->animation.wind.fast, false);
  } else if (current_wind > 2 && current_wind < 10) {
    set_visible(ui->animation.wind.slow, true);
    set_visible(ui->animation.wind.med, false);
    set_visible(ui->animation.wind.fast, false);
  } else if (current_wind > 10 && current_wind < 20) {
    set_visible(ui->animation.wind.slow, true);
    set_visible(ui->animation.wind.med, true);
    set_visible(ui->animation.wind.fast, false);
  } else if (current_wind > 30) {
    set_visible(ui->animation.wind.slow, true);
    set_visible(ui->animation.wind.med, true);
    set_visible(ui->animation.wind.fast, true);
  }
  prev = current_wind;
#endif
}

void draw_weather_rain(ui_main_menu_t *ui) {
#if SIMULATE_ANIM_RAIN
  rain_set_intensity(10);
#else
  uint8_t current_rain = get_weather_rain();
  if (current_rain > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS) {
    current_rain = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS;
  }
  rain_set_intensity(current_rain, ui);
#endif
}

void draw_weather_snow(ui_main_menu_t *ui) {
#if SIMULATE_ANIM_SNOW
  snow_set_intensity(10);
#else
  uint8_t current_snow = get_weather_snow();
  current_snow = current_snow * BLOCK_BOT_RIGHT_MULT_FACTOR_WEATHER_ANIM_SNOWS;
  if (current_snow > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS) {
    current_snow = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS;
  }
  snow_set_intensity(current_snow, ui);
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
  uint8_t curent_status = get_wifi_status();
  if (curent_status != status_wifi_old) {
    switch (curent_status) {
    case WIFI_RECONNECT:

      lv_obj_set_style_bg_img_src(ui->wifi.btn_open, LV_SYMBOL_REFRESH, 0);
      break;
    case WIFI_CONNECTED:
      lv_obj_set_style_bg_img_src(ui->wifi.btn_open, LV_SYMBOL_WIFI, 0);
      break;
    case WIFI_DISCONNECTED:
      lv_obj_set_style_bg_img_src(ui->wifi.btn_open, LV_SYMBOL_WARNING, 0);
      break;
    default:
      lv_obj_set_style_bg_img_src(ui->wifi.btn_open, LV_SYMBOL_WARNING, 0);

      break;
    }
    status_wifi_old = curent_status;
  }
}

void hide_all_blocks(ui_main_menu_t *ui) {
  set_visible(ui->animation.screen, false);
  set_visible(ui->sensor.screen, false);
  set_visible(ui->meteo.screen, false);
  set_visible(ui->time.screen, false);
  set_visible(ui->co2.meter, false);
  set_visible(ui->co2.chart, false);
  set_visible(ui->standby.bar, false);
}
void show_all_blocks(ui_main_menu_t *ui) {
  set_visible(ui->animation.screen, true);
  set_visible(ui->sensor.screen, true);
  set_visible(ui->meteo.screen, true);
  set_visible(ui->time.screen, true);
  set_visible(ui->co2.meter, true);
  set_visible(ui->co2.chart, true);
  set_visible(ui->standby.bar, true);
}

void update_block_top_left(ui_main_menu_t *ui) {
  read_sensors();
  print_value("%.1f c*", get_temperature_aht10(), ui->sensor.temperature_label);
  print_value("%.f %%", get_humidity_aht10(), ui->sensor.humidity_label);
  print_value("%.f ppm", get_tvoc_sgp30(), ui->sensor.tvoc_label);
}

void update_block_top_middle(ui_main_menu_t *ui) {
  uint16_t current_co2_value = get_co2_sgp30();

  print_value("%.f", current_co2_value, ui->co2.co2_label);
  lv_meter_set_indicator_value(ui->co2.meter, ui->co2.indicator,
                               current_co2_value);
}

void update_block_top_right(ui_main_menu_t *ui) {
  print_mday(get_time_mday(), get_time_month(), ui);
  print_wday(get_time_wday(), ui);
  print_time(get_time_hour(), get_time_minute(), ui);
}

void update_block_bot_left(ui_main_menu_t *ui) {
  print_value("%.1f c*", get_weather_temperature(),
              ui->meteo.temperature_label);
  print_value("%.f %%", get_weather_humidity(), ui->meteo.humidity_label);

  print_value("%.1f m/s", get_weather_wind(), ui->meteo.wind_label);
}

void update_block_bot_middle(ui_main_menu_t *ui) {
  static uint16_t cnt_chart_co2 = 0;
  cnt_chart_co2++;            // 1 tick == 1 sec
  if (cnt_chart_co2 > 3600) { // equal 1 hour
    uint16_t temp_co2_chart = get_co2_sgp30();
    if (temp_co2_chart > MAX_VALUE_CO2)
      temp_co2_chart = MAX_VALUE_CO2;

    lv_chart_set_next_value(ui->co2.chart, ser_co2, temp_co2_chart);
    cnt_chart_co2 = 0;
  }
  draw_symbol_wifi(ui);

  lv_bar_set_value(ui->standby.bar, timer_standby_sec, LV_ANIM_OFF);
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

bool is_screen_pressed(void) { return standby_touched; }

static void standby_handle(ui_main_menu_t *ui) {

  if (is_screen_pressed()) {
    timer_standby_sec = 0;
    wavesahre_rgb_lcd_bl_on();
  } else {
    timer_standby_sec++;
    if (timer_standby_sec > MAX_STANDBY_TIME * 5) {
      wavesahre_rgb_lcd_bl_off();
      timer_standby_sec = MAX_STANDBY_TIME * 5;
    }
  }
  if (ui->settings.switch_.standby_status) {
    timer_standby_sec = 0;
  }
}

static void timer_10000(lv_timer_t *timer) {
  LV_UNUSED(timer);
#if ACTIVATE_BLOCK_TOP_RIGHT
  update_block_top_right(&ui);
#endif
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
     static uint16_t cnt_sensor_history = 0;
   cnt_sensor_history++;
   if (cnt_sensor_history >= SENSOR_RECORD_INTERVAL) {
       sensor_history_push(&sensor_history);
       cnt_sensor_history = 0;
}
}
static void timer_200(lv_timer_t *timer) {
  LV_UNUSED(timer);
  standby_handle(&ui);
}

static void init_fonts(ui_main_menu_t *ui) {
  lv_style_init(&ui->font.small);
  lv_style_set_text_font(&ui->font.small, &lv_font_montserrat_32);
  //  lv_style_init(&style[STYLE_TEXT_MEDIUM]);
  //  lv_style_set_text_font(&style[STYLE_TEXT_MEDIUM],
  //  &lv_font_montserrat_40); lv_style_init(&style[STYLE_TEXT_LARGE]);
  //  lv_style_set_text_font(&style[STYLE_TEXT_LARGE],
  //  &lv_font_montserrat_48);

  lv_style_init(&ui->font.very_large);
  lv_style_set_text_font(&ui->font.very_large, &lv_font_montserrat_48);
}

static void init_styles(ui_main_menu_t *ui) {

  lv_style_init(&ui->style.chart_co2);
  lv_style_set_bg_opa(&ui->style.chart_co2, LV_OPA_COVER);
  lv_style_set_bg_color(&ui->style.chart_co2, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_bg_grad_color(&ui->style.chart_co2,
                             lv_palette_lighten(LV_PALETTE_GREEN, 1));
  lv_style_set_bg_grad_dir(&ui->style.chart_co2, LV_GRAD_DIR_VER);
  lv_style_set_bg_main_stop(&ui->style.chart_co2, 128);
  lv_style_set_bg_grad_stop(&ui->style.chart_co2, 192);

  lv_style_init(&ui->style.bot_right);
  lv_style_set_bg_color(&ui->style.bot_right, lv_color_black());

  lv_style_init(&ui->style.bot_left);
  lv_style_set_bg_color(&ui->style.bot_left,
                        lv_palette_lighten(LV_PALETTE_ORANGE, 1));

  lv_style_init(&ui->style.top_left);
  lv_style_set_bg_color(&ui->style.top_left,
                        lv_palette_lighten(LV_PALETTE_GREEN, 1));

  lv_style_init(&ui->style.top_right);
  lv_style_set_bg_color(&ui->style.top_right,
                        lv_palette_lighten(LV_PALETTE_GREEN, 1));
}

void init_lv_objects() {
  init_fonts(&ui);
  init_styles(&ui);
  create_menu(&ui);
  draw_menu_main(&ui);
  lv_timer_create(timer_10000, 10000, NULL);
  lv_timer_create(timer_1000, 1000, NULL);
  lv_timer_create(timer_200, 200, NULL);
}

