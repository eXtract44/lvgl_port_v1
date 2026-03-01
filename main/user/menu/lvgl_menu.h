/*
 * lvgl_menu.h
 *
 *  Created on: Oct 20, 2023
 *      Author: Alex
 */

#ifndef INC_LVGL_MENU_H_
#define INC_LVGL_MENU_H_

#include "../components/lvgl__lvgl/lvgl.h"
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



enum namesOfIndicators {
  INDICATOR_CO2,
  LAST_ELEMENT_OF_INDICATOR
};

enum namesOfFonts {
  FONT_SMALL,
  //FONT_MEDIUM,
 // FONT_LARGE,
  FONT_VERY_LARGE,
  LAST_ELEMENT_OF_FONT
};

enum namesOfStyles {
  STYLE_TEXT_SMALL,
//  STYLE_TEXT_MEDIUM,
//  STYLE_TEXT_LARGE,
  STYLE_TEXT_VERY_LARGE,
  STYLE_SYMBOLS,
  LAST_ELEMENT_OF_STYLE_TEXT
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
    int16_t turbulence;   // вертикальная амплитуда
} wind_anim_t;

typedef struct {
    lv_obj_t *obj;
    lv_coord_t container_w;
    lv_coord_t container_h;
    lv_coord_t img_w;
    lv_coord_t img_h;
    int16_t slope;      // наклон (например 1 = 45°)
    int32_t phase;   // индивидуальный сдвиг
} rain_snow_anim_t;

enum nameOfObjects {
  SCREEN_MAIN_MENU,
  BACKGROUND_MAIN_MENU,
  BACKGROUND_BLOCK_TOP_LEFT,
  BACKGROUND_BLOCK_BOT_LEFT,
  BACKGROUND_CLOCK_VALUES,
  BACKGROUND_WEATHER,
  METER_CO2_MAIN_MENU,
  CHART_CO2_MAIN_MENU,
  SLIDER_DISTANCE_MAIN_MENU,
  SYMBOL_TEMPERATURE_INSIDE,
  SYMBOL_HUMIDITY_INSIDE,
  SYMBOL_TVOC,
  SYMBOL_BRIGHTNESS,
  SYMBOL_TEMPERATURE_OUTSIDE,
  SYMBOL_HUMIDITY_OUTSIDE,
  SYMBOL_WIND,
  SYMBOL_PRESS,
  SYMBOL_EYE,
  SYMBOL_BUZZER,
  BTN_WIFI_SETTINGS,
  BACKGROUND_POPUP_WIFI,
  POPUP_WIFI_TITLE_TEXT,
  POPUP_WIFI_SSID_TEXT,
  POPUP_WIFI_SSID,
  POPUP_WIFI_RSSI_TEXT,
  POPUP_WIFI_RSSI,
  BTN_CLOUSE_POPUP_WIFI,
  BTN_UI_SETTINGS,
  BAR_STANDBY,
  KEYBOARD_WIFI,
  KEYBOARD_WIFI_TEXT,
  BTN_KEYBOARD,
  VALUE_TEMPERATURE_INSIDE,
  VALUE_HUMIDITY_INSIDE,
  VALUE_CO2_INSIDE,
  VALUE_TVOC_INSIDE,
  VALUE_BRIGHTNESS_INSIDE,
  VALUE_TEMPERATURE_OUTSIDE,
  VALUE_HUMIDITY_OUTSIDE,
  VALUE_WIND_OUTSIDE,
  VALUE_PRESS_OUTSIDE,
  IMAGE_SUN_48_48,
  IMAGE_MOON_42_42,
  IMAGE_CLOUD_THIN_80_30,
  IMAGE_CLOUD_SMALL_70_35,
  IMAGE_CLOUD_MID_90_45,
  IMAGE_CLOUD_BIG_110_50,
  IMAGE_WIND_SLOW,
  IMAGE_WIND_MED,
  IMAGE_WIND_FAST,
  TIME_HOUR_MINUTE,
  TIME_MINUTE,
  TIME_MDAY_MONTH,
  TIME_WDAY,
  LAST_ELEMENT_OF_OBJECTS
};


void init_lv_objects();

#endif /* INC_LVGL_MENU_H_ */
