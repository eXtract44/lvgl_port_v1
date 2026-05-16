/*
 * ui_popup_settings.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_MENU_UI_POPUP_SETTINGS_H_
#define MAIN_USER_MENU_UI_POPUP_SETTINGS_H_

#include "user/menu/ui_core.h"
#include "../periphery/ota.h"

//UI Settings Popup
void ui_create_settings_popup(ui_main_menu_t *ui);
void ui_create_settings_popup_btns(ui_main_menu_t *ui);
void btn_settings_open_popup_event_handler(lv_event_t *e);
void btn_settings_close_popup_event_handler(lv_event_t *e);
void btn_settings_back_event_handler(lv_event_t *e);
void btn_settings_category_event_handler(lv_event_t *e);

//Display Settings
void ui_create_settigs_display(ui_main_menu_t *ui);
void backlight_btnmatrix_event_cb(lv_event_t *e);
void backlight_slider_event_cb(lv_event_t *e);
void standby_btnmatrix_event_cb(lv_event_t *e);
void theme_btnmatrix_event_cb(lv_event_t *e);

//Wifi Settings
void ui_create_settigs_wifi(ui_main_menu_t *ui);

//Weather Settings
void ui_create_settigs_weather(ui_main_menu_t *ui);
void ui_create_city_list_weather(ui_main_menu_t *ui);
void btn_weather_open_list_city_event_handler(lv_event_t *e);
void set_current_city_weather_event_handler(lv_event_t *e);

//Sensors Settings
void ui_create_settigs_sensors(ui_main_menu_t *ui);
void co2_btnmatrix_event_cb(lv_event_t *e);

//OTA Settings
void ota_progress_cb(ota_state_t state, int progress_pct);
void ota_start_timer_cb(lv_timer_t *t);
void ota_btn_event_cb(lv_event_t *e);


#endif /* MAIN_USER_MENU_UI_POPUP_SETTINGS_H_ */
