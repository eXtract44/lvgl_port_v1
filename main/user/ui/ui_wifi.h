/*
 * ui_wifi.h
 *
 *  Created on: 16.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_UI_UI_WIFI_H_
#define MAIN_USER_UI_UI_WIFI_H_

#include "ui_core.h"

uint8_t rssi_to_bars(int16_t rssi);
void wifi_update_signal_bars(ui_main_menu_t *ui, uint8_t bars);
void wifi_pass_popup_connect_cb(lv_event_t *e);
void wifi_open_pass_popup(ui_main_menu_t *ui, const char *ssid);
void wifi_ap_item_click_cb(lv_event_t *e);
void wifi_manual_keyboard_cb(lv_event_t *e);
void wifi_manual_ta_focus_cb(lv_event_t *e);
void wifi_manual_entry_cb(lv_event_t *e);
void wifi_refresh_cb(lv_event_t *e);
void wifi_scan_timer_cb(lv_timer_t *timer);
void wifi_check_timer_cb(lv_timer_t *timer);
#endif /* MAIN_USER_UI_UI_WIFI_H_ */
