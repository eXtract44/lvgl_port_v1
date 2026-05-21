/*
 * nvs_user.h
 *
 *  Created on: 18.03.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_NVS_USER_H_
#define MAIN_USER_PERIPHERY_NVS_USER_H_
#include "nvs_flash.h"
#include "nvs.h"
#include "time.h"

void main_settings_save(uint8_t standby_mode, uint8_t backlight_pct,
                        uint8_t backlight_mode, uint8_t theme_mode, uint8_t co2_mode);
void main_settings_load(uint8_t *standby_mode, uint8_t *backlight_pct,
                        uint8_t *backlight_mode, uint8_t *theme_mode, uint8_t *co2_mode);                    
void weather_settings_save(uint16_t city);
void weather_settings_load(uint16_t *city) ;

void ota_last_update_save(uint32_t timestamp);
uint32_t ota_last_update_load(void);

void nvs_user_init();


#endif /* MAIN_USER_PERIPHERY_NVS_USER_H_ */
