#include "user/menu/lvgl_menu.h"
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

extern lv_font_t my_symbols;
extern lv_font_t my_time_font;

static lv_meter_indicator_t *indicator[LAST_ELEMENT_OF_INDICATOR];
// static const lv_font_t *font[LAST_ELEMENT_OF_FONT];
static lv_style_t style[LAST_ELEMENT_OF_STYLE_TEXT];
static lv_obj_t *lv_object[LAST_ELEMENT_OF_OBJECTS];
static lv_obj_t *rain_objs[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS];
static lv_obj_t *snow_objs[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS];

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

void set_flag(lv_obj_t *obj, bool f) {
  if (f)
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN); // on
  else
    lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN); // off
}

void print_value(const char *flags, float value, lv_obj_t *value_lv) {
  lv_label_set_text_fmt(value_lv, flags, value);
}

static void create_text(const char *text, lv_obj_t *bg, uint16_t theme,
                        lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs) {
  lv_obj_t *cont = lv_label_create(bg);
  lv_obj_remove_style_all(cont);
  switch (theme) {
  case STYLE_TEXT_SMALL:
    lv_obj_add_style(cont, &style[STYLE_TEXT_SMALL], 0);
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

static lv_obj_t *create_label(lv_obj_t *screen, const char *text,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs) {
  lv_obj_t *con = lv_label_create(screen);
  lv_obj_align(con, align, x_ofs, y_ofs);
  lv_label_set_text(con, text);
  return con;
}

static lv_obj_t *create_meter(lv_obj_t *screen, lv_coord_t w, lv_coord_t h,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs) {
  const int16_t start_value = BLOCK_TOP_MID_START_CO2_LEFT_PART;
  const int16_t start_value_1 = BLOCK_TOP_MID_END_CO2_LEFT_PART;
  const int16_t end_value = BLOCK_TOP_MID_START_CO2_RIGHT_PART;
  const int16_t end_value_1 = BLOCK_TOP_MID_END_CO2_RIGHT_PART;

  lv_obj_t *meter = lv_meter_create(screen);
  lv_obj_set_size(meter, w, h);
  lv_obj_align(meter, align, x_ofs, y_ofs);
  /*Add a scale first*/

  lv_meter_scale_t *scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_ticks(meter, scale, 41, 2, 10,
                           lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_range(meter, scale, MIN_VALUE_CO2, MAX_VALUE_CO2, 250,
                           145);

  /*Add a blue arc to the start*/
  indicator[INDICATOR_CO2] =
      lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_GREEN), 0);
  lv_meter_set_indicator_start_value(meter, indicator[INDICATOR_CO2],
                                     start_value);
  lv_meter_set_indicator_end_value(meter, indicator[INDICATOR_CO2],
                                   start_value_1);

  /*Make the tick lines blue at the start of the scale*/
  indicator[INDICATOR_CO2] =
      lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_GREEN),
                               lv_palette_main(LV_PALETTE_GREEN), false, 0);
  lv_meter_set_indicator_start_value(meter, indicator[INDICATOR_CO2],
                                     start_value);
  lv_meter_set_indicator_end_value(meter, indicator[INDICATOR_CO2],
                                   start_value_1);

  /*Add a red arc to the end*/
  indicator[INDICATOR_CO2] =
      lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(meter, indicator[INDICATOR_CO2],
                                     end_value);
  lv_meter_set_indicator_end_value(meter, indicator[INDICATOR_CO2],
                                   end_value_1);

  /*Make the tick lines red at the end of the scale*/
  indicator[INDICATOR_CO2] =
      lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED),
                               lv_palette_main(LV_PALETTE_RED), false, 0);
  lv_meter_set_indicator_start_value(meter, indicator[INDICATOR_CO2],
                                     end_value);
  lv_meter_set_indicator_end_value(meter, indicator[INDICATOR_CO2],
                                   end_value_1);

  /*Add a needle line indicator*/
  indicator[INDICATOR_CO2] = lv_meter_add_needle_line(
      meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);

  return meter;
}
static lv_chart_series_t *ser_co2;

static lv_obj_t *create_chart(lv_obj_t *screen, lv_coord_t w, lv_coord_t h,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs) {
  lv_obj_t *chart = lv_chart_create(screen);
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

void print_wday(uint8_t wday) {
  if (get_wifi_status() == WIFI_CONNECTED) {
    switch (wday) {
    case 1:
      lv_label_set_text(lv_object[TIME_WDAY], "mo");
      break;
    case 2:
      lv_label_set_text(lv_object[TIME_WDAY], "tu");
      break;
    case 3:
      lv_label_set_text(lv_object[TIME_WDAY], "we");
      break;
    case 4:
      lv_label_set_text(lv_object[TIME_WDAY], "th");
      break;
    case 5:
      lv_label_set_text(lv_object[TIME_WDAY], "fr");
      break;
    case 6:
      lv_label_set_text(lv_object[TIME_WDAY], "sa");
      break;
    case 7:
      lv_label_set_text(lv_object[TIME_WDAY], "su");
      break;
    }
  } else {
    lv_label_set_text(lv_object[TIME_WDAY], "no wifi");
  }
}

void print_time(uint8_t time_hour, uint8_t time_minute) {
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
  lv_label_set_text(lv_object[TIME_HOUR_MINUTE], string_buffer);
}

void print_mday(uint8_t date_day, uint8_t date_month) {
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
  lv_label_set_text(lv_object[TIME_MDAY_MONTH], string_buffer);
}

static lv_obj_t *create_button_symbol(lv_obj_t *bg, lv_coord_t w, lv_coord_t h,
                                      lv_align_t align, lv_coord_t x_ofs,
                                      lv_coord_t y_ofs) {
  lv_obj_t *cont = lv_btn_create(bg);
  lv_obj_set_size(cont, w, h);
  // lv_obj_set_style_bg_img_src(cont, symbol, 0);
  lv_obj_align(cont, align, x_ofs, y_ofs);
  return cont;
}

static lv_obj_t *create_background(lv_obj_t *bg, lv_coord_t w, lv_coord_t h,
                                   lv_align_t align, lv_coord_t x_ofs,
                                   lv_coord_t y_ofs) {
  lv_obj_t *cont = lv_obj_create(bg);
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
                                         lv_obj_t *bg, lv_align_t align,
                                         lv_coord_t x_ofs, lv_coord_t y_ofs) {
  lv_obj_t *img = lv_img_create(bg);
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
  cloud_anim_t *c = var;
  int32_t x = c->base_x + (lv_trigo_sin(v) * c->amplitude) / LV_TRIGO_SIN_MAX;
  lv_obj_set_pos(c->obj, x, c->base_y);
}

lv_obj_t *create_cloud_anim(const lv_img_dsc_t *img_src, lv_obj_t *bg,
                            lv_coord_t x_ofs, lv_coord_t y_ofs,
                            int16_t amplitude, uint32_t speed) {
  lv_obj_t *img = lv_img_create(bg);
  lv_img_set_src(img, img_src);
  lv_obj_set_pos(img, x_ofs, y_ofs);

  lv_coord_t base_x = x_ofs;
  lv_coord_t base_y = y_ofs;

  static cloud_anim_t cloud_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_CLOUDS];
  static uint8_t pool_index = 0;

  cloud_anim_t *c = &cloud_pool[pool_index++];
  c->obj = img;
  c->base_x = base_x;
  c->base_y = base_y;
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
  wind_anim_t *w = var;

  /* движение слева направо */
  lv_coord_t x = v - w->img_w;

  /* лёгкая турбулентность по Y */
  lv_coord_t y =
      w->base_y + (lv_trigo_sin((v * 360) / w->container_w) * w->turbulence) /
                      LV_TRIGO_SIN_MAX / 3;
  lv_obj_set_pos(w->obj, x, y);
}

lv_obj_t *create_wind_anim(const lv_img_dsc_t *img_src, lv_obj_t *bg,
                           lv_coord_t x_ofs, lv_coord_t y_ofs,
                           int16_t turbulence, uint32_t speed) {
  lv_obj_update_layout(bg);

  lv_obj_t *img = lv_img_create(bg);
  lv_img_set_src(img, img_src);

  lv_coord_t container_w = lv_obj_get_width(bg);
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

  /* диапазон движения */
  lv_anim_set_values(&a, -150, container_w + img_w);

  lv_anim_set_time(&a, speed);
  lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
  lv_anim_start(&a);

  return img;
}

static void rain_snow_anim_cb(void *var, int32_t v) {
  rain_snow_anim_t *r = var;

  int32_t value = v + r->phase;

  lv_coord_t range_x = r->container_w + r->img_w;
  lv_coord_t range_y = r->container_h + r->img_h;

  lv_coord_t x = (value % range_x) - r->img_w;
  lv_coord_t y = ((value * r->slope) % range_y) - r->img_h;

  lv_obj_set_pos(r->obj, x, y + 30);
}

lv_obj_t *create_rain_snow_anim(const lv_img_dsc_t *img_src, lv_obj_t *bg,
                                int16_t slope, uint32_t speed) {
  lv_obj_update_layout(bg);

  lv_obj_t *img = lv_img_create(bg);
  lv_img_set_src(img, img_src);

  lv_coord_t container_w = lv_obj_get_width(bg);
  lv_coord_t container_h = lv_obj_get_height(bg);

  lv_coord_t img_w = lv_obj_get_width(img);
  lv_coord_t img_h = lv_obj_get_height(img);

  static rain_snow_anim_t
      rain_snow_pool[BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS +
                     BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS];
  static uint8_t pool_index = 0;

  if (pool_index >= BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS +
                        BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS)
    pool_index = 0;

  rain_snow_anim_t *r = &rain_snow_pool[pool_index++];

  r->obj = img;
  r->container_w = container_w;
  r->container_h = container_h;
  r->img_w = img_w;
  r->img_h = img_h;
  r->slope = slope; // 1 = 45°, 2 = круче, 0 = вертикально
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

static void create_block_top_left() {
  /*STYLES*/
  static lv_style_t style_bg_top_left;
  lv_style_init(&style_bg_top_left);
  lv_style_set_bg_color(&style_bg_top_left,
                        lv_palette_lighten(LV_PALETTE_GREEN, 1));
  /*BLOCK TOP LEFT*/

  lv_object[BACKGROUND_BLOCK_TOP_LEFT] =
      create_background(lv_object[SCREEN_MAIN_MENU], BLOCK_TOP_LEFT_WIDTH,
                        BLOCK_TOP_LEFT_HEIGHT, BLOCK_TOP_LEFT_ALIGN_BACKGROUND,
                        BLOCK_TOP_LEFT_X_START, BLOCK_TOP_LEFT_Y_START);
  lv_obj_add_style(lv_object[BACKGROUND_BLOCK_TOP_LEFT], &style_bg_top_left, 0);

  create_text("inside", lv_object[BACKGROUND_BLOCK_TOP_LEFT], STYLE_TEXT_SMALL,
              BLOCK_TOP_LEFT_ALIGN_TITLE, 0, BLOCK_TOP_LEFT_Y_START_TITLE);

  lv_object[SYMBOL_TEMPERATURE_INSIDE] = create_button_symbol(
      lv_object[BACKGROUND_BLOCK_TOP_LEFT], BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
      BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
      BLOCK_TOP_LEFT_X_START_SYMBOLS, BLOCK_TOP_LEFT_Y_START_SYMBOLS_1);

  lv_obj_set_style_text_font(lv_object[SYMBOL_TEMPERATURE_INSIDE], &my_symbols,
                             0);
  lv_obj_set_style_bg_img_src(lv_object[SYMBOL_TEMPERATURE_INSIDE],
                              MY_TEMPERATURE_SYMBOL, 0);

  lv_object[SYMBOL_HUMIDITY_INSIDE] = create_button_symbol(
      lv_object[BACKGROUND_BLOCK_TOP_LEFT], BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
      BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
      BLOCK_TOP_LEFT_X_START_SYMBOLS, BLOCK_TOP_LEFT_Y_START_SYMBOLS_2);
  lv_obj_set_style_text_font(lv_object[SYMBOL_HUMIDITY_INSIDE], &my_symbols, 0);
  lv_obj_set_style_bg_img_src(lv_object[SYMBOL_HUMIDITY_INSIDE],
                              MY_HUMIDITY_SYMBOL, 0);

  lv_object[SYMBOL_TVOC] = create_button_symbol(
      lv_object[BACKGROUND_BLOCK_TOP_LEFT], BLOCK_TOP_LEFT_WIDTH_SYMBOLS,
      BLOCK_TOP_LEFT_HEIGHT_SYMBOLS, BLOCK_TOP_LEFT_ALIGN_SYMBOLS,
      BLOCK_TOP_LEFT_X_START_SYMBOLS, BLOCK_TOP_LEFT_Y_START_SYMBOLS_3);
  lv_obj_set_style_text_font(lv_object[SYMBOL_TVOC], &my_symbols, 0);
  lv_obj_set_style_bg_img_src(lv_object[SYMBOL_TVOC], MY_TVOC_SYMBOL, 0);

  lv_object[VALUE_TEMPERATURE_INSIDE] = create_label(
      lv_object[BACKGROUND_BLOCK_TOP_LEFT], "0 c*", BLOCK_TOP_LEFT_ALIGN_VALUES,
      BLOCK_TOP_LEFT_X_START_VALUES, BLOCK_TOP_LEFT_Y_START_VALUE_1);
  lv_object[VALUE_HUMIDITY_INSIDE] = create_label(
      lv_object[BACKGROUND_BLOCK_TOP_LEFT], "0 %*", BLOCK_TOP_LEFT_ALIGN_VALUES,
      BLOCK_TOP_LEFT_X_START_VALUES, BLOCK_TOP_LEFT_Y_START_VALUE_2);
  lv_object[VALUE_TVOC_INSIDE] =
      create_label(lv_object[BACKGROUND_BLOCK_TOP_LEFT], "0 ppm",
                   BLOCK_TOP_LEFT_ALIGN_VALUES, BLOCK_TOP_LEFT_X_START_VALUES,
                   BLOCK_TOP_LEFT_Y_START_VALUE_3);

  /*BLOCK TOP LEFT*/
}

static void create_block_top_middle() {
  /*BLOCK TOP MID*/
  lv_object[METER_CO2_MAIN_MENU] = create_meter(
      lv_object[SCREEN_MAIN_MENU], BLOCK_TOP_MID_WIDTH_CO2_METER,
      BLOCK_TOP_MID_HEIGHT_CO2_METER, BLOCK_TOP_MID_ALIGN_CO2_CHART,
      BLOCK_TOP_MID_X_START, BLOCK_TOP_MID_Y_START);
  lv_meter_set_indicator_value(lv_object[METER_CO2_MAIN_MENU],
                               indicator[INDICATOR_CO2], 0);

  //	create_text("ppm", lv_object[SCREEN_MAIN_MENU], STYLE_TEXT_SMALL,
  //			BLOCK_TOP_MID_ALIGN_CO2_CHART, BLOCK_TOP_MID_X_START,
  // meter_co2.y_ofs + 45);

  lv_object[VALUE_CO2_INSIDE] = create_label(
      lv_object[SCREEN_MAIN_MENU], "0", BLOCK_TOP_MID_ALIGN_CO2_CHART,
      BLOCK_TOP_MID_X_START, BLOCK_TOP_MID_Y_START_CO2_VALUE);
  lv_obj_add_style(lv_object[VALUE_CO2_INSIDE], &style[STYLE_TEXT_VERY_LARGE],
                   0);
  /*BLOCK TOP MID*/
}

static void create_block_top_right() {
  /*STYLES*/
  static lv_style_t style_bg_top_right;
  lv_style_init(&style_bg_top_right);
  lv_style_set_bg_color(&style_bg_top_right,
                        lv_palette_lighten(LV_PALETTE_GREEN, 1));
  /*BLOCK TOP RIGHT*/

  lv_object[BACKGROUND_CLOCK_VALUES] = create_background(
      lv_object[SCREEN_MAIN_MENU], BLOCK_TOP_RIGHT_WIDTH,
      BLOCK_TOP_RIGHT_HEIGHT, BLOCK_TOP_RIGHT_ALIGN_BACKGROUND,
      BLOCK_TOP_RIGHT_X_START, BLOCK_TOP_RIGHT_Y_START);
  lv_obj_add_style(lv_object[BACKGROUND_CLOCK_VALUES], &style_bg_top_right, 0);
  lv_obj_set_scrollbar_mode(lv_object[BACKGROUND_CLOCK_VALUES],
                            LV_SCROLLBAR_MODE_OFF);

  create_text("time", lv_object[BACKGROUND_CLOCK_VALUES], STYLE_TEXT_SMALL,
              BLOCK_TOP_RIGHT_ALIGN_TITLE, 0, BLOCK_TOP_RIGHT_Y_START_TITLE);

  lv_object[TIME_HOUR_MINUTE] = create_label(
      lv_object[BACKGROUND_CLOCK_VALUES], "00:00", BLOCK_TOP_RIGHT_ALIGN_VALUES,
      BLOCK_TOP_RIGHT_X_START_VALUES, BLOCK_TOP_RIGHT_Y_START_VALUE_1);
  lv_obj_set_style_text_font(lv_object[TIME_HOUR_MINUTE], &my_time_font, 0);

  lv_object[TIME_MDAY_MONTH] = create_label(
      lv_object[BACKGROUND_CLOCK_VALUES], "00.00", BLOCK_TOP_RIGHT_ALIGN_VALUES,
      BLOCK_TOP_RIGHT_X_START_VALUES, BLOCK_TOP_RIGHT_Y_START_VALUE_2);
  lv_obj_set_style_text_font(lv_object[TIME_MDAY_MONTH], &my_time_font, 0);

  lv_object[TIME_WDAY] = create_label(lv_object[BACKGROUND_CLOCK_VALUES],
                                      "load...", BLOCK_TOP_RIGHT_ALIGN_VALUES,
                                      BLOCK_TOP_RIGHT_X_START_VALUE_3,
                                      BLOCK_TOP_RIGHT_Y_START_VALUE_3);
  lv_obj_set_style_text_font(lv_object[TIME_WDAY], &lv_font_montserrat_32, 0);
  /*BLOCK TOP RIGHT*/
}

static void create_block_bot_left() {
  /*STYLES*/
  static lv_style_t style_bg_bot_left;
  lv_style_init(&style_bg_bot_left);
  lv_style_set_bg_color(&style_bg_bot_left,
                        lv_palette_lighten(LV_PALETTE_ORANGE, 1));
  /*BLOCK BOT LEFT*/

  lv_object[BACKGROUND_BLOCK_BOT_LEFT] =
      create_background(lv_object[SCREEN_MAIN_MENU], BLOCK_BOT_LEFT_WIDTH,
                        BLOCK_BOT_LEFT_HEIGHT, BLOCK_BOT_LEFT_ALIGN_BACKGROUND,
                        BLOCK_BOT_LEFT_X_START, BLOCK_BOT_LEFT_Y_START);
  lv_obj_add_style(lv_object[BACKGROUND_BLOCK_BOT_LEFT], &style_bg_bot_left, 0);
  create_text("outside", lv_object[BACKGROUND_BLOCK_BOT_LEFT], STYLE_TEXT_SMALL,
              BLOCK_BOT_LEFT_ALIGN_TITLE, 0, BLOCK_BOT_LEFT_Y_START_TITLE);

  lv_object[SYMBOL_TEMPERATURE_OUTSIDE] = create_button_symbol(
      lv_object[BACKGROUND_BLOCK_BOT_LEFT], BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
      BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
      BLOCK_BOT_LEFT_X_START_SYMBOLS, BLOCK_BOT_LEFT_Y_START_SYMBOLS_1);
  lv_obj_set_style_text_font(lv_object[SYMBOL_TEMPERATURE_OUTSIDE], &my_symbols,
                             0);
  lv_obj_set_style_bg_img_src(lv_object[SYMBOL_TEMPERATURE_OUTSIDE],
                              MY_TEMPERATURE_SYMBOL, 0);

  lv_object[SYMBOL_HUMIDITY_OUTSIDE] = create_button_symbol(
      lv_object[BACKGROUND_BLOCK_BOT_LEFT], BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
      BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
      BLOCK_BOT_LEFT_X_START_SYMBOLS, BLOCK_BOT_LEFT_Y_START_SYMBOLS_2);
  lv_obj_set_style_text_font(lv_object[SYMBOL_HUMIDITY_OUTSIDE], &my_symbols,
                             0);
  lv_obj_set_style_bg_img_src(lv_object[SYMBOL_HUMIDITY_OUTSIDE],
                              MY_HUMIDITY_SYMBOL, 0);

  lv_object[SYMBOL_WIND] = create_button_symbol(
      lv_object[BACKGROUND_BLOCK_BOT_LEFT], BLOCK_BOT_LEFT_WIDTH_SYMBOLS,
      BLOCK_BOT_LEFT_HEIGHT_SYMBOLS, BLOCK_BOT_LEFT_ALIGN_SYMBOLS,
      BLOCK_BOT_LEFT_X_START_SYMBOLS, BLOCK_BOT_LEFT_Y_START_SYMBOLS_3);
  lv_obj_set_style_text_font(lv_object[SYMBOL_WIND], &my_symbols, 0);
  lv_obj_set_style_bg_img_src(lv_object[SYMBOL_WIND], MY_WIND_SYMBOL, 0);

  lv_object[VALUE_TEMPERATURE_OUTSIDE] = create_label(
      lv_object[BACKGROUND_BLOCK_BOT_LEFT], "0 c*", BLOCK_BOT_LEFT_ALIGN_VALUES,
      BLOCK_BOT_LEFT_X_START_VALUES, BLOCK_BOT_LEFT_Y_START_VALUE_1);
  lv_object[VALUE_HUMIDITY_OUTSIDE] = create_label(
      lv_object[BACKGROUND_BLOCK_BOT_LEFT], "0 %", BLOCK_BOT_LEFT_ALIGN_VALUES,
      BLOCK_BOT_LEFT_X_START_VALUES, BLOCK_BOT_LEFT_Y_START_VALUE_2);
  lv_object[VALUE_WIND_OUTSIDE] =
      create_label(lv_object[BACKGROUND_BLOCK_BOT_LEFT], "0 m/s",
                   BLOCK_BOT_LEFT_ALIGN_VALUES, BLOCK_BOT_LEFT_X_START_VALUES,
                   BLOCK_BOT_LEFT_Y_START_VALUE_3);
  /*BLOCK BOT LEFT*/
}

static void btn_wifi_open_popup_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {

    lv_label_set_text(lv_object[POPUP_WIFI_SSID], get_wifi_ssid());
    lv_label_set_text_fmt(lv_object[POPUP_WIFI_RSSI], "%2d dBm",
                          get_wifi_rssi());
    set_flag(lv_object[BACKGROUND_POPUP_WIFI], true);
  }
}

static void btn_wifi_close_popup_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    set_flag(lv_object[BACKGROUND_POPUP_WIFI], false);
  }
}

static void btn_settings_open_popup_event_handler(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    //    lv_label_set_text(lv_object[POPUP_WIFI_SSID], get_wifi_ssid());
    //    lv_label_set_text_fmt(lv_object[POPUP_WIFI_RSSI], "%d dBm",
    //                          get_wifi_rssi());
    //    set_flag(lv_object[BACKGROUND_POPUP_WIFI], true);
  }
}

static void create_window_popup(char *window_name, lv_obj_t *bg) {
  bg = create_background(lv_object[SCREEN_MAIN_MENU], POPUP_WINDOW_WIDTH,
                         POPUP_WINDOW_HEIGHT, POPUP_WINDOW_ALIGN, 0, 0);
  // lv_obj_add_style(lv_object[BACKGROUND_POPUP_WIFI], &style_bg_bot_right, 0);
  lv_obj_set_scrollbar_mode(bg, LV_SCROLLBAR_MODE_OFF);

  lv_object[POPUP_WIFI_TITLE_TEXT] =
      lv_label_create(bg);
  lv_label_set_text(lv_object[POPUP_WIFI_TITLE_TEXT], "WIFI Settings");
  lv_obj_align(lv_object[POPUP_WIFI_RSSI], LV_ALIGN_TOP_LEFT, 10, 10);

  
}

static void create_btn_close(lv_obj_t *btn, lv_obj_t *bg, lv_coord_t x_ofs,
                             lv_coord_t y_ofs, lv_event_cb_t event_cb) {
  btn = lv_btn_create(bg);
  lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_HOME, 0);
  lv_obj_set_size(btn, 50, 50);
  lv_obj_align(btn, LV_ALIGN_TOP_LEFT, x_ofs, y_ofs);

  lv_obj_add_event_cb(btn, event_cb, LV_EVENT_CLICKED, NULL);
}

static void create_wifi_popup() {
  create_window_popup("WIFI Settings", lv_object[BACKGROUND_POPUP_WIFI]);
  create_btn_close(lv_object[BTN_CLOUSE_POPUP_WIFI],
                   lv_object[BACKGROUND_POPUP_WIFI], 530, 30,
                   btn_wifi_close_popup_event_handler);

  lv_object[POPUP_WIFI_SSID_TEXT] =
      lv_label_create(lv_object[BACKGROUND_POPUP_WIFI]);
  lv_label_set_text(lv_object[POPUP_WIFI_SSID_TEXT], "WiFi Name: ");
  lv_obj_set_pos(lv_object[POPUP_WIFI_SSID_TEXT], 20, 10);

  lv_object[POPUP_WIFI_SSID] =
      lv_label_create(lv_object[BACKGROUND_POPUP_WIFI]);
  lv_label_set_text(lv_object[POPUP_WIFI_SSID], "WIFI_SSID");
  lv_obj_align(lv_object[POPUP_WIFI_SSID], LV_ALIGN_TOP_LEFT, 280, 20);

  lv_object[POPUP_WIFI_RSSI_TEXT] =
      lv_label_create(lv_object[BACKGROUND_POPUP_WIFI]);
  lv_label_set_text(lv_object[POPUP_WIFI_RSSI_TEXT], "WiFi Signal: ");
  lv_obj_set_pos(lv_object[POPUP_WIFI_RSSI_TEXT], 20, 80);

  lv_object[POPUP_WIFI_RSSI] =
      lv_label_create(lv_object[BACKGROUND_POPUP_WIFI]);
  lv_label_set_text(lv_object[POPUP_WIFI_RSSI], "WIFI_RSSI");
  lv_obj_align(lv_object[POPUP_WIFI_RSSI], LV_ALIGN_TOP_LEFT, 280, 80);

  set_flag(lv_object[BACKGROUND_POPUP_WIFI], false);
}

static void create_block_bot_middle() {
  /*STYLES*/
  static lv_style_t style_chart_co2;
  lv_style_init(&style_chart_co2);
  lv_style_set_bg_opa(&style_chart_co2, LV_OPA_COVER);
  lv_style_set_bg_color(&style_chart_co2, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_bg_grad_color(&style_chart_co2,
                             lv_palette_lighten(LV_PALETTE_GREEN, 1));
  lv_style_set_bg_grad_dir(&style_chart_co2, LV_GRAD_DIR_VER);
  lv_style_set_bg_main_stop(&style_chart_co2, 128);
  lv_style_set_bg_grad_stop(&style_chart_co2, 192);
  /*BLOCK BOT MID*/

  create_text("co2           24h", lv_object[SCREEN_MAIN_MENU],
              STYLE_TEXT_SMALL, BLOCK_BOT_MID_ALIGN_CO2_CHART, 0,
              BLOCK_BOT_MID_Y_START_TITLE_CO2_CHART);

  lv_object[CHART_CO2_MAIN_MENU] = create_chart(
      lv_object[SCREEN_MAIN_MENU], BLOCK_BOT_MID_WIDTH_CO2_CHART,
      BLOCK_BOT_MID_HEIGHT_CO2_CHART, BLOCK_BOT_MID_ALIGN_CO2_CHART, 0,
      BLOCK_BOT_MID_Y_START_CO2_CHART);
  lv_obj_add_style(lv_object[CHART_CO2_MAIN_MENU], &style_chart_co2, 0);

  lv_object[BTN_WIFI_SETTINGS] = create_button_symbol(
      lv_object[SCREEN_MAIN_MENU], BLOCK_BOT_MID_WIDTH_SYMBOL,
      BLOCK_BOT_MID_HEIGHT_SYMBOL, BLOCK_BOT_MID_ALIGN_SYMBOL,
      BLOCK_BOT_MID_X_START_SYMBOL_3, BLOCK_BOT_MID_Y_START_SYMBOLS);
  lv_obj_add_event_cb(lv_object[BTN_WIFI_SETTINGS],
                      btn_wifi_open_popup_event_handler, LV_EVENT_ALL, NULL);

  lv_object[BTN_UI_SETTINGS] = create_button_symbol(
      lv_object[SCREEN_MAIN_MENU], BLOCK_BOT_MID_WIDTH_SYMBOL,
      BLOCK_BOT_MID_HEIGHT_SYMBOL, BLOCK_BOT_MID_ALIGN_SYMBOL,
      BLOCK_BOT_MID_X_START_SYMBOL_4, BLOCK_BOT_MID_Y_START_SYMBOLS);
  lv_obj_set_style_bg_img_src(lv_object[BTN_UI_SETTINGS], LV_SYMBOL_SETTINGS,
                              0);
  lv_obj_add_event_cb(lv_object[BTN_UI_SETTINGS],
                      btn_settings_open_popup_event_handler, LV_EVENT_ALL,
                      NULL);

  lv_object[BAR_STANDBY] = lv_bar_create(lv_object[SCREEN_MAIN_MENU]);
  lv_obj_align(lv_object[BAR_STANDBY], BLOCK_BOT_MID_ALIGN_STANDBY_BAR, 0,
               BLOCK_BOT_MID_Y_START_STANDBY_BAR);
  lv_obj_set_size(lv_object[BAR_STANDBY], BLOCK_BOT_MID_WIDTH_STANDBY_BAR,
                  BLOCK_BOT_MID_HEIGHT_STANDBY_BAR);
  lv_bar_set_range(lv_object[BAR_STANDBY], 0, MAX_STANDBY_TIME);
  /*BLOCK BOT MID*/
}

static void create_block_bot_right() {
  /*BLOCK BOT RIGHT*/

  /*STYLES*/
  static lv_style_t style_bg_bot_right;
  lv_style_init(&style_bg_bot_right);
  lv_style_set_bg_color(&style_bg_bot_right, lv_color_black());
  lv_object[BACKGROUND_WEATHER] = create_background(
      lv_object[SCREEN_MAIN_MENU], BLOCK_BOT_RIGHT_WIDTH,
      BLOCK_BOT_RIGHT_HEIGHT, BLOCK_BOT_RIGHT_ALIGN_BACKGROUND,
      BLOCK_BOT_RIGHT_X_START, BLOCK_BOT_RIGHT_Y_START);
  lv_obj_add_style(lv_object[BACKGROUND_WEATHER], &style_bg_bot_right, 0);
  lv_obj_set_scrollbar_mode(lv_object[BACKGROUND_WEATHER],
                            LV_SCROLLBAR_MODE_OFF);
#if ACTIVATE_ANIM_SUN_MOON
  lv_object[IMAGE_SUN_48_48] =
      create_anim_image_orbit(&sun_48_48, lv_object[BACKGROUND_WEATHER],
                              BLOCK_BOT_RIGHT_ALIGN_WEATHER_ANIM,
                              BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_SUN_MOON,
                              BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_SUN_MOON);

  lv_object[IMAGE_MOON_42_42] =
      create_anim_image_orbit(&moon_42_42, lv_object[BACKGROUND_WEATHER],
                              BLOCK_BOT_RIGHT_ALIGN_WEATHER_ANIM,
                              BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_SUN_MOON,
                              BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_SUN_MOON);
#endif
#if ACTIVATE_ANIM_CLOUD
  lv_object[IMAGE_CLOUD_BIG_110_50] = create_cloud_anim(
      &cloud_big_110_50, lv_object[BACKGROUND_WEATHER],
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_BIG_110_50,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_BIG_110_50,
      BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_BIG_110_50,
      BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_BIG_110_50);

  lv_object[IMAGE_CLOUD_MID_90_45] =
      create_cloud_anim(&cloud_mid_90_45, lv_object[BACKGROUND_WEATHER],
                        BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_MID_90_45,
                        BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_MID_90_45,
                        BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_MID_90_45,
                        BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_MID_90_45);
  lv_object[IMAGE_CLOUD_SMALL_70_35] = create_cloud_anim(
      &cloud_small_70_35, lv_object[BACKGROUND_WEATHER],
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_SMALL_70_35,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_SMALL_70_35,
      BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_SMALL_70_35,
      BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_SMALL_70_35);

  lv_object[IMAGE_CLOUD_THIN_80_30] = create_cloud_anim(
      &cloud_thin_80_30, lv_object[BACKGROUND_WEATHER],
      BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_CLOUDS_THIN_80_30,
      BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_CLOUDS_THIN_80_30,
      BLOCK_BOT_RIGHT_AMPLITUDE_WEATHER_ANIM_CLOUDS_THIN_80_30,
      BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_CLOUDS_THIN_80_30);
#endif

#if ACTIVATE_ANIM_WIND
  lv_object[IMAGE_WIND_SLOW] =
      create_wind_anim(&wind_60_50, lv_object[BACKGROUND_WEATHER],
                       BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_WIND_SLOW,
                       BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_WIND_SLOW,
                       BLOCK_BOT_RIGHT_TURBULENCE_WEATHER_ANIM_WIND_SLOW,
                       BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_WIND_SLOW);

  lv_object[IMAGE_WIND_MED] =
      create_wind_anim(&wind_60_50, lv_object[BACKGROUND_WEATHER],
                       BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_WIND_MED,
                       BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_WIND_MED,
                       BLOCK_BOT_RIGHT_TURBULENCE_WEATHER_ANIM_WIND_MED,
                       BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_WIND_MED);

  lv_object[IMAGE_WIND_FAST] =
      create_wind_anim(&wind_60_50, lv_object[BACKGROUND_WEATHER],
                       BLOCK_BOT_RIGHT_X_START_WEATHER_ANIM_WIND_FAST,
                       BLOCK_BOT_RIGHT_Y_START_WEATHER_ANIM_WIND_FAST,
                       BLOCK_BOT_RIGHT_TURBULENCE_WEATHER_ANIM_WIND_FAST,
                       BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_WIND_FAST);
#endif
  for (uint8_t i = 0; i < BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS; i++) {
    rain_objs[i] = create_rain_snow_anim(
        &rain_drop_heavy_9_22, lv_object[BACKGROUND_WEATHER], 2,
        BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_RAIN_SNOW);
  }

  for (uint8_t i = 0; i < BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS; i++) {
    snow_objs[i] = create_rain_snow_anim(
        &snow_flake_2_15_15, lv_object[BACKGROUND_WEATHER], 2,
        BLOCK_BOT_RIGHT_SPEED_WEATHER_ANIM_RAIN_SNOW);
  }

  /*BLOCK BOT RIGHT*/
}

static void create_all_popups() { create_wifi_popup(); }

static void create_menu() {
  lv_object[SCREEN_MAIN_MENU] = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(lv_object[SCREEN_MAIN_MENU], lv_color_black(), 0);
  lv_obj_set_style_bg_opa(lv_object[SCREEN_MAIN_MENU], LV_OPA_COVER, 0);
  lv_object[BACKGROUND_MAIN_MENU] = lv_obj_create(lv_object[SCREEN_MAIN_MENU]);
  lv_obj_set_size(lv_object[BACKGROUND_MAIN_MENU], LVGL_PORT_H_RES,
                  LVGL_PORT_V_RES);
  lv_obj_set_style_bg_color(lv_object[BACKGROUND_MAIN_MENU], lv_color_black(),
                            0);
  lv_obj_set_style_bg_opa(lv_object[BACKGROUND_MAIN_MENU], LV_OPA_COVER, 0);

/*STYLES*/
#if ACTIVATE_BLOCK_TOP_LEFT
  create_block_top_left();
#endif
#if ACTIVATE_BLOCK_BOT_LEFT
  create_block_bot_left();
#endif
#if ACTIVATE_BLOCK_TOP_MID
  create_block_top_middle();
#endif
#if ACTIVATE_BLOCK_TOP_RIGHT
  create_block_top_right();
#endif
#if ACTIVATE_BLOCK_BOT_MID
  create_block_bot_middle();
#endif
#if ACTIVATE_BLOCK_BOT_RIGHT
  create_block_bot_right();
#endif
  create_all_popups();
}

void draw_menu_main() { lv_scr_load(lv_object[SCREEN_MAIN_MENU]); }

void rain_set_intensity(uint8_t visible_count) {
  for (uint8_t i = 0; i < BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS; i++) {
    if (i < visible_count)
      lv_obj_clear_flag(rain_objs[i], LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_add_flag(rain_objs[i], LV_OBJ_FLAG_HIDDEN);
  }
}

void snow_set_intensity(uint8_t visible_count) {
  for (uint8_t i = 0; i < BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS; i++) {
    if (i < visible_count)
      lv_obj_clear_flag(snow_objs[i], LV_OBJ_FLAG_HIDDEN);
    else
      lv_obj_add_flag(snow_objs[i], LV_OBJ_FLAG_HIDDEN);
  }
}

bool get_day_activated_now() {
  if (get_time_hour() > DAY_START_HOUR && get_time_hour() < DAY_END_HOUR)
    return true;
  else
    return false;
}

void draw_weather_sun_moon() {
#if SIMULATE_ANIM_SUN
  set_flag(lv_object[IMAGE_SUN_48_48], true);
  set_flag(lv_object[IMAGE_MOON_42_42], false);
#else
  if (get_day_activated_now()) {
    set_flag(lv_object[IMAGE_SUN_48_48], true);
    set_flag(lv_object[IMAGE_MOON_42_42], false);
  } else {
    set_flag(lv_object[IMAGE_SUN_48_48], false);
    set_flag(lv_object[IMAGE_MOON_42_42], true);
  }

#endif
}

void draw_weather_clouds() {
#if SIMULATE_ANIM_CLOUD
  set_flag(lv_object[IMAGE_CLOUD_BIG_110_50], true);
  set_flag(lv_object[IMAGE_CLOUD_MID_90_45], true);
  set_flag(lv_object[IMAGE_CLOUD_SMALL_70_35], true);
  set_flag(lv_object[IMAGE_CLOUD_THIN_80_30], true);
#else
  uint8_t current_clouds = get_weather_clouds();
  if (current_clouds < 15) {
    set_flag(lv_object[IMAGE_CLOUD_THIN_80_30], false);
    set_flag(lv_object[IMAGE_CLOUD_SMALL_70_35], false);
    set_flag(lv_object[IMAGE_CLOUD_MID_90_45], false);
    set_flag(lv_object[IMAGE_CLOUD_BIG_110_50], false);
  } else if (current_clouds > 15 && current_clouds < 25) {
    set_flag(lv_object[IMAGE_CLOUD_THIN_80_30], true);
    set_flag(lv_object[IMAGE_CLOUD_SMALL_70_35], false);
    set_flag(lv_object[IMAGE_CLOUD_MID_90_45], false);
    set_flag(lv_object[IMAGE_CLOUD_BIG_110_50], false);
  } else if (current_clouds > 25 && current_clouds < 50) {
    set_flag(lv_object[IMAGE_CLOUD_THIN_80_30], true);
    set_flag(lv_object[IMAGE_CLOUD_SMALL_70_35], true);
    set_flag(lv_object[IMAGE_CLOUD_MID_90_45], false);
    set_flag(lv_object[IMAGE_CLOUD_BIG_110_50], false);
  } else if (current_clouds > 50 && current_clouds < 75) {
    set_flag(lv_object[IMAGE_CLOUD_THIN_80_30], true);
    set_flag(lv_object[IMAGE_CLOUD_SMALL_70_35], true);
    set_flag(lv_object[IMAGE_CLOUD_MID_90_45], true);
    set_flag(lv_object[IMAGE_CLOUD_BIG_110_50], false);
  } else if (current_clouds > 75) {
    set_flag(lv_object[IMAGE_CLOUD_THIN_80_30], true);
    set_flag(lv_object[IMAGE_CLOUD_SMALL_70_35], true);
    set_flag(lv_object[IMAGE_CLOUD_MID_90_45], true);
    set_flag(lv_object[IMAGE_CLOUD_BIG_110_50], true);
  }

#endif
}

void draw_weather_wind() {
#if SIMULATE_ANIM_WIND
  set_flag(lv_object[IMAGE_WIND_SLOW], true);
  set_flag(lv_object[IMAGE_WIND_MED], true);
  set_flag(lv_object[IMAGE_WIND_FAST], true);
#else
  uint8_t current_wind = get_weather_wind();
  if (current_wind < 2) {
    set_flag(lv_object[IMAGE_WIND_SLOW], false);
    set_flag(lv_object[IMAGE_WIND_MED], false);
    set_flag(lv_object[IMAGE_WIND_FAST], false);
  } else if (current_wind > 2 && current_wind < 10) {
    set_flag(lv_object[IMAGE_WIND_SLOW], true);
    set_flag(lv_object[IMAGE_WIND_MED], false);
    set_flag(lv_object[IMAGE_WIND_FAST], false);
  } else if (current_wind > 10 && current_wind < 20) {
    set_flag(lv_object[IMAGE_WIND_SLOW], true);
    set_flag(lv_object[IMAGE_WIND_MED], true);
    set_flag(lv_object[IMAGE_WIND_FAST], false);
  } else if (current_wind > 30) {
    set_flag(lv_object[IMAGE_WIND_SLOW], true);
    set_flag(lv_object[IMAGE_WIND_MED], true);
    set_flag(lv_object[IMAGE_WIND_FAST], true);
  }
#endif
}

void draw_weather_rain() {
#if SIMULATE_ANIM_RAIN
  rain_set_intensity(10);
#else
  uint8_t current_rain = get_weather_rain();
  if (current_rain > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS) {
    current_rain = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_RAINS;
  }
  rain_set_intensity(current_rain);
#endif
}

void draw_weather_snow() {
#if SIMULATE_ANIM_SNOW
  snow_set_intensity(10);
#else
  uint8_t current_snow = get_weather_snow();
  current_snow = current_snow * BLOCK_BOT_RIGHT_MULT_FACTOR_WEATHER_ANIM_SNOWS;
  if (current_snow > BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS) {
    current_snow = BLOCK_BOT_RIGHT_MAX_WEATHER_ANIM_SNOWS;
  }
  snow_set_intensity(current_snow);
#endif
}

void draw_weather() {
#if ACTIVATE_ANIM_SUN_MOON
  draw_weather_sun_moon();
#endif
#if ACTIVATE_ANIM_CLOUD
  draw_weather_clouds();
#endif
#if ACTIVATE_ANIM_WIND
  draw_weather_wind();
#endif
#if ACTIVATE_ANIM_RAIN
  draw_weather_rain();
#endif
#if ACTIVATE_ANIM_SNOW
  draw_weather_snow();
#endif
}

void draw_symbol_wifi() {
  static uint8_t status_wifi_old = 254;
  uint8_t curent_status = get_wifi_status();
  if (curent_status != status_wifi_old) {
    switch (curent_status) {
    case WIFI_RECONNECT:
      lv_obj_set_style_bg_img_src(lv_object[BTN_WIFI_SETTINGS],
                                  LV_SYMBOL_REFRESH, 0);
      break;
    case WIFI_CONNECTED:
      lv_obj_set_style_bg_img_src(lv_object[BTN_WIFI_SETTINGS], LV_SYMBOL_WIFI,
                                  0);
      break;
    case WIFI_DISCONNECTED:
      lv_obj_set_style_bg_img_src(lv_object[BTN_WIFI_SETTINGS],
                                  LV_SYMBOL_WARNING, 0);
      break;
    default:
      lv_obj_set_style_bg_img_src(lv_object[BTN_WIFI_SETTINGS],
                                  LV_SYMBOL_WARNING, 0);

      break;
    }
    status_wifi_old = curent_status;
  }
}

void update_block_top_left() {
  read_sensors();
  print_value("%.1f c*", get_temperature_aht10(),
              lv_object[VALUE_TEMPERATURE_INSIDE]);
  print_value("%.f %%", get_humidity_aht10(), lv_object[VALUE_HUMIDITY_INSIDE]);
  print_value("%.f ppm", get_tvoc_sgp30(), lv_object[VALUE_TVOC_INSIDE]);
}

void update_block_top_middle() {
  uint16_t current_co2_value = get_co2_sgp30();
  print_value("%.f", current_co2_value, lv_object[VALUE_CO2_INSIDE]);
  lv_meter_set_indicator_value(lv_object[METER_CO2_MAIN_MENU],
                               indicator[INDICATOR_CO2], current_co2_value);
}

void update_block_top_right() {
  print_mday(get_time_mday(), get_time_month());
  print_wday(get_time_wday());
  print_time(get_time_hour(), get_time_minute());
}

void update_block_bot_left() {
  print_value("%.1f c*", get_weather_temperature(),
              lv_object[VALUE_TEMPERATURE_OUTSIDE]);
  print_value("%.f %%", get_weather_humidity(),
              lv_object[VALUE_HUMIDITY_OUTSIDE]);
  print_value("%.1f m/s", get_weather_wind(), lv_object[VALUE_WIND_OUTSIDE]);
}

void update_block_bot_middle() {
  static uint16_t cnt_chart_co2 = 0;
  cnt_chart_co2++;            // 1 tick == 1 sec
  if (cnt_chart_co2 > 3600) { // equal 1 hour
    uint16_t temp_co2_chart = get_co2_sgp30();
    if (temp_co2_chart > MAX_VALUE_CO2)
      temp_co2_chart = MAX_VALUE_CO2;
    lv_chart_set_next_value(lv_object[CHART_CO2_MAIN_MENU], ser_co2,
                            temp_co2_chart);
    cnt_chart_co2 = 0;
  }
  draw_symbol_wifi();
  lv_bar_set_value(lv_object[BAR_STANDBY], timer_standby_sec, LV_ANIM_OFF);
}

void update_block_bot_right() {
  static uint16_t cnt_weather = 0;
  cnt_weather++;
  if (cnt_weather < 2) {
    draw_weather();
  }
  if (cnt_weather > UPDATE_WEATHER_ANIM_TICKS) {
    ///////////////////////////////////////////////////////////////////////
    // equal 4 min
    draw_weather();
    cnt_weather = 2;
  }
}

bool is_screen_pressed(void) { return standby_touched; }

static void standby_handle() {

  if (is_screen_pressed()) {
    timer_standby_sec = 0;
    wavesahre_rgb_lcd_bl_on();
  } else if (!get_day_activated_now()) {
    timer_standby_sec++;
    if (timer_standby_sec > MAX_STANDBY_TIME) {
      wavesahre_rgb_lcd_bl_off();
      timer_standby_sec = MAX_STANDBY_TIME;
    }
  }
}

static void timer_10000(lv_timer_t *timer) {
  LV_UNUSED(timer);
#if ACTIVATE_BLOCK_TOP_RIGHT
  update_block_top_right();
#endif
}

static void timer_1000(lv_timer_t *timer) {
  LV_UNUSED(timer);
#if ACTIVATE_BLOCK_TOP_LEFT
  update_block_top_left();
#endif
#if ACTIVATE_BLOCK_BOT_LEFT
  update_block_bot_left();
#endif
#if ACTIVATE_BLOCK_TOP_MID
  update_block_top_middle();
#endif
#if ACTIVATE_BLOCK_BOT_MID
  update_block_bot_middle();
#endif
#if ACTIVATE_BLOCK_BOT_RIGHT
  update_block_bot_right();
#endif
}

static void timer_200(lv_timer_t *timer) {
  LV_UNUSED(timer);
  standby_handle();
}

static void init_fonts() {
  lv_style_init(&style[STYLE_TEXT_SMALL]);
  lv_style_set_text_font(&style[STYLE_TEXT_SMALL], &lv_font_montserrat_32);
  //  lv_style_init(&style[STYLE_TEXT_MEDIUM]);
  //  lv_style_set_text_font(&style[STYLE_TEXT_MEDIUM],
  //  &lv_font_montserrat_40); lv_style_init(&style[STYLE_TEXT_LARGE]);
  //  lv_style_set_text_font(&style[STYLE_TEXT_LARGE],
  //  &lv_font_montserrat_48);
  lv_style_init(&style[STYLE_TEXT_VERY_LARGE]);
  lv_style_set_text_font(&style[STYLE_TEXT_VERY_LARGE], &lv_font_montserrat_48);
}

void init_lv_objects() {
  init_fonts();
  create_menu();
  draw_menu_main();
  lv_timer_create(timer_10000, 10000, NULL);
  lv_timer_create(timer_1000, 1000, NULL);
  lv_timer_create(timer_200, 200, NULL);
}
