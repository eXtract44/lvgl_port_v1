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

void settings_save(bool standby, bool theme);
void settings_load(bool *standby, bool *theme);
void nvs_user_init();


#endif /* MAIN_USER_PERIPHERY_NVS_USER_H_ */
