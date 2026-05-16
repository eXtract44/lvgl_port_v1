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
void timer_60000(lv_timer_t *timer);
void timer_10000(lv_timer_t *timer);
void timer_1000(lv_timer_t *timer);
void timer_200(lv_timer_t *timer);
void timer_33(lv_timer_t *timer);

void display_standby_handle(ui_main_menu_t *ui);
uint8_t backlight_get_current_pct(ui_main_menu_t *ui);
bool is_screen_pressed(void);

#endif /* MAIN_USER_MENU_UI_TIMERS_H_ */
