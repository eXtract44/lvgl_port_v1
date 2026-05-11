/*
 * wifi.h
 *
 *  Created on: 22.02.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_WIFI_USER_H_
#define MAIN_USER_PERIPHERY_WIFI_USER_H_

#include "stdint.h"
#include "esp_wifi.h"
void wifi_init_sta(void);
void wifi_print_info(void);
void wifi_connect(const char* ssid, const char* pass);

// ---------------------------------------------------------------------------
// WiFi helpers
// ---------------------------------------------------------------------------
enum namesOfWiFiStatus {
    WIFI_RECONNECT,
    WIFI_CONNECTED,
    WIFI_DISCONNECTED
};

uint8_t     get_wifi_status(void);
const char *get_wifi_ssid(void);
const char *get_wifi_pass(void);
int16_t     get_wifi_rssi(void);
void wifi_disconnect(void);
esp_err_t wifi_scan_start(void);          // запускает async scan
bool      wifi_scan_is_done(void);        // готовы ли результаты
uint16_t  wifi_scan_get_count(void);      // кол-во найденных сетей
esp_err_t wifi_scan_get_results(wifi_ap_record_t *list, uint16_t *count); // забрать результат

#define DEBUG_INET 0

#endif /* MAIN_USER_PERIPHERY_WIFI_USER_H_ */
