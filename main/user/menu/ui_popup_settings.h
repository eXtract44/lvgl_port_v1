/*
 * ui_popup_settings.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_MENU_UI_POPUP_SETTINGS_H_
#define MAIN_USER_MENU_UI_POPUP_SETTINGS_H_

#include "user/menu/ui_core.h"

void ui_create_settings_popup_btns(ui_main_menu_t *ui);
void set_current_city_weather_event_handler(lv_event_t *e);
void btn_settings_open_popup_event_handler(lv_event_t *e);
void ui_create_settings_popup(ui_main_menu_t *ui);

#endif /* MAIN_USER_MENU_UI_POPUP_SETTINGS_H_ */
