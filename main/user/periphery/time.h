/*
 * time.h
 *
 *  Created on: 22.02.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_TIME_H_
#define MAIN_USER_PERIPHERY_TIME_H_
#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void start_ntp_time_task(void);
void initialize_sntp(void);


#endif /* MAIN_USER_PERIPHERY_TIME_H_ */
