/*
 * ui_widgets.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_MENU_UI_WIDGETS_H_
#define MAIN_USER_MENU_UI_WIDGETS_H_

#include <stdint.h>
#include "ui_core.h"

void create_text(const char *text, lv_obj_t *parent, uint16_t theme,
                        lv_align_t align, lv_coord_t x_ofs, lv_coord_t y_ofs,
                        ui_main_menu_t *ui);
lv_obj_t *create_meter(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs, ui_main_menu_t *ui);
  lv_obj_t *create_chart(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs,ui_main_menu_t *ui);
 lv_obj_t *create_icon(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                             lv_align_t align, lv_coord_t x_ofs,
                             lv_coord_t y_ofs, const char *symbol,
                             ui_main_menu_t *ui);
 lv_obj_t *create_btn_icon(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                                 lv_align_t align, lv_coord_t x_ofs,
                                 lv_coord_t y_ofs, lv_event_cb_t event_cb,
                                 void *user_data, const char *symbol,
                                 lv_style_t *icon_style, const lv_font_t *font,
                                 const char *label_text,
                                 ui_main_menu_t *ui_ptr);                                                                                     
lv_obj_t *create_btn_cb(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                               lv_align_t align, lv_coord_t x_ofs,
                               lv_coord_t y_ofs, lv_event_cb_t event_cb,
                               void *user_data);
  lv_obj_t *create_background(lv_obj_t *parent, lv_coord_t w, lv_coord_t h,
                                   lv_align_t align, lv_coord_t x_ofs,
                                   lv_coord_t y_ofs);
                           lv_obj_t *create_label(lv_obj_t *parent, const char *text,
                              lv_align_t align, lv_coord_t x_ofs,
                              lv_coord_t y_ofs);                                     

#endif /* MAIN_USER_MENU_UI_WIDGETS_H_ */
