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
#define MY_CLOUD_SYMBOL "\xEF\x83\x82"



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


void init_lv_objects();

#endif /* INC_LVGL_MENU_H_ */
