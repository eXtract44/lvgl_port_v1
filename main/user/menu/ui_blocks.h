/*
 * ui_blocks.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_MENU_UI_BLOCKS_H_
#define MAIN_USER_MENU_UI_BLOCKS_H_




#include "user/menu/ui_core.h"

void create_menu(ui_main_menu_t *ui);
void ui_create_sgp30_calib_popup(ui_main_menu_t *ui);
void btn_weather_close_forecast_popup_event_handler(lv_event_t *e);
void ui_create_sensor_temperature_history_popup(ui_main_menu_t *ui);
void update_co2_chart_labels(ui_main_menu_t *ui);
void sensor_temperature_history_push(sensor_history_t *h);
#endif /* MAIN_USER_MENU_UI_BLOCKS_H_ */
