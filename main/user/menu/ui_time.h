/*
 * ui_time.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_MENU_UI_TIME_H_
#define MAIN_USER_MENU_UI_TIME_H_



#include <stdint.h>

#include "ui_core.h"
void print_wday(uint8_t wday, ui_main_menu_t *ui);
void print_time(uint8_t time_hour, uint8_t time_minute, ui_main_menu_t *ui);
void print_mday(uint8_t date_day, uint8_t date_month, ui_main_menu_t *ui);
void get_easter(uint16_t year, uint8_t *out_day, uint8_t *out_month);
const char *get_german_holiday(uint8_t day, uint8_t month,
                                      uint16_t year);
                                      void print_holiday(uint8_t day, uint8_t month, uint16_t year,
                   ui_main_menu_t *ui);
#endif /* MAIN_USER_MENU_UI_TIME_H_ */
