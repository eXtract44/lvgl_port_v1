/*
 * ui_weather_anim.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_MENU_UI_WEATHER_ANIM_H_
#define MAIN_USER_MENU_UI_WEATHER_ANIM_H_



#include "ui_core.h"

lv_obj_t *create_snow_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                           int16_t slope, uint32_t speed);
lv_obj_t *create_rain_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                           int16_t slope, uint32_t speed);
                           void rain_snow_anim_cb(void *var, int32_t v);
lv_obj_t *create_wind_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                           lv_coord_t x_ofs, lv_coord_t y_ofs,
                           int16_t turbulence, uint32_t speed);
 void wind_anim_cb(void *var, int32_t v);
 lv_obj_t *create_cloud_anim(const lv_img_dsc_t *img_src, lv_obj_t *parent,
                            lv_coord_t x_ofs, lv_coord_t y_ofs,
                            int16_t amplitude, uint32_t speed);
 void cloud_anim_cb(void *var, int32_t v);
lv_obj_t *create_anim_image_orbit(const lv_img_dsc_t *img_src,
                                         lv_obj_t *parent, lv_align_t align,
                                         lv_coord_t x_ofs, lv_coord_t y_ofs);
void anim_sun_moon_orbit(void *var, int32_t angle);   
void draw_weather(ui_main_menu_t *ui);                                                                                      

#endif /* MAIN_USER_MENU_UI_WEATHER_ANIM_H_ */
