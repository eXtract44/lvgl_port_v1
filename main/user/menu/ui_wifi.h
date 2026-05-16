/*
 * ui_wifi.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_MENU_UI_WIFI_H_
#define MAIN_USER_MENU_UI_WIFI_H_



#include "misc/lv_timer.h"

void wifi_check_timer_cb(lv_timer_t *timer);
void wifi_scan_timer_cb(lv_timer_t *timer);
uint8_t rssi_to_bars(int16_t rssi);
#endif /* MAIN_USER_MENU_UI_WIFI_H_ */
