/*
 * wifi.h
 *
 *  Created on: 22.02.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_WIFI_H_
#define MAIN_USER_PERIPHERY_WIFI_H_

void wifi_init_sta(void);
void wifi_print_info(void);
void wifi_connect(const char* ssid, const char* pass);

#define DEBUG_INET 0

#endif /* MAIN_USER_PERIPHERY_WIFI_H_ */
