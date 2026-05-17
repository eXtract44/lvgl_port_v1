/*
 * ui_blocks.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_UI_UI_BLOCKS_H_
#define MAIN_USER_UI_UI_BLOCKS_H_




#include "ui_core.h"

//Block All
void create_menu(ui_main_menu_t *ui);
void update_symbol_wifi(ui_main_menu_t *ui);
float calc_dew_point(float temp, float humidity);
float calc_heat_index(float t, float rh);

//Block Top Left Sensor Temperature Humidity 
void create_block_top_left(ui_main_menu_t *ui);
void update_block_top_left(ui_main_menu_t *ui);
void ui_create_sensor_temperature_history_popup(ui_main_menu_t *ui);
void block_top_left_open_popup_event_handler(lv_event_t *e);
void btn_sensor_close_history_popup_event_handler(lv_event_t *e);
void update_tvoc_dots(lv_obj_t *dots[5], uint16_t tvoc,
                             uint8_t theme_mode);
                             void sensor_temperature_history_push(sensor_history_t *h);
void sensor_temperature_record_values(ui_main_menu_t *ui);
void sensor_temperature_history_get(const sensor_history_t *h,
                                           uint16_t idx, int16_t *temp_x10,
                                           uint8_t *hum);

//Block Top Mid Sensor CO2 
void create_block_top_middle(ui_main_menu_t *ui);
void update_block_top_middle(ui_main_menu_t *ui);
void co2_calib_popup_close_cb(lv_event_t *e);
void co2_meter_click_event_cb(lv_event_t *e);
void ui_create_sgp30_calib_popup(ui_main_menu_t *ui);
int16_t co2_scale(int16_t real_ppm, uint8_t mode);
lv_color_t calc_co2_color(uint16_t co2);
void update_co2_status_label(ui_main_menu_t *ui, uint16_t co2_ppm);

//Block Top Right Time & Date
void create_block_top_right(ui_main_menu_t *ui);
void update_block_top_right(ui_main_menu_t *ui);

//Block Bot Left Weather Open Meteo
void create_block_bot_left(ui_main_menu_t *ui);
void update_block_bot_left(ui_main_menu_t *ui);
void block_bot_left_open_popup_event_handler(lv_event_t *e);
void btn_weather_close_forecast_popup_event_handler(lv_event_t *e);
void ui_create_weather_forecast_popup(ui_main_menu_t *ui);
lv_color_t calc_uv_color(float uv);

//Block Bot Mid CO2 Chart Settings Btn
void create_block_bot_middle(ui_main_menu_t *ui);
void update_block_bot_middle(ui_main_menu_t *ui);
void ui_create_co2_chart_bot_mid(ui_main_menu_t *ui);
void update_co2_chart_labels(ui_main_menu_t *ui);

//Block Bot Right Weather Animations
void create_block_bot_right(ui_main_menu_t *ui);
void update_block_bot_right(ui_main_menu_t *ui);
        
#endif /* MAIN_USER_UI_UI_BLOCKS_H_ */
