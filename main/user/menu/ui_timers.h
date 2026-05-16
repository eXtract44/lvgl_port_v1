/*
 * ui_timers.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_MENU_UI_TIMERS_H_
#define MAIN_USER_MENU_UI_TIMERS_H_

#include "user/menu/ui_core.h"
void create_timers();
void display_standby_handle(ui_main_menu_t *ui);
lv_color_t calc_co2_color(uint16_t co2);
bool is_screen_pressed(void);
uint8_t backlight_get_current_pct(ui_main_menu_t *ui);
void update_block_top_right(ui_main_menu_t *ui);
void update_block_top_left(ui_main_menu_t *ui);
void update_block_bot_left(ui_main_menu_t *ui);
void update_block_top_middle(ui_main_menu_t *ui);
void update_block_bot_middle(ui_main_menu_t *ui);
void update_block_bot_right(ui_main_menu_t *ui);
void sensor_temperature_record_values(ui_main_menu_t *ui);
void update_co2_status_label(ui_main_menu_t *ui, uint16_t co2_ppm);
   
#endif /* MAIN_USER_MENU_UI_TIMERS_H_ */
