/*
 * time.h
 *
 *  Created on: 22.02.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_TIME_USER_H_
#define MAIN_USER_PERIPHERY_TIME_USER_H_
#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void start_ntp_time_task(void);
void user_initialize_sntp(void);
void user_sntp_stop(void);

// ---------------------------------------------------------------------------
// Time helpers
// ---------------------------------------------------------------------------
uint8_t  get_time_mday(void);
uint16_t get_time_year(void);
uint8_t  get_time_month(void);
uint8_t  get_time_hour(void);
uint8_t  get_time_minute(void);
uint8_t  get_time_wday(void);


#endif /* MAIN_USER_PERIPHERY_TIME_USER_H_ */
