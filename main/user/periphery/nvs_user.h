/*
 * nvs_user.h
 *
 *  Created on: 18.03.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_NVS_USER_H_
#define MAIN_USER_PERIPHERY_NVS_USER_H_
#include "nvs.h"
#include "nvs_flash.h"
#include "time.h"
#include "user/ui/ui_core.h"


void main_settings_save(ui_main_menu_t *input);
void main_settings_load(ui_main_menu_t *output);
void weather_settings_save(uint16_t city);
void weather_settings_load(uint16_t *city);

void ota_last_update_save(uint32_t timestamp);
uint32_t ota_last_update_load(void);

void nvs_user_init();

#endif /* MAIN_USER_PERIPHERY_NVS_USER_H_ */
