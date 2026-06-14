/*
 * backlight.h
 *
 *  Created on: 07.05.2026
 *      Author: toose
 */

#ifndef MAIN_USER_PERIPHERY_BACKLIGHT_H_
#define MAIN_USER_PERIPHERY_BACKLIGHT_H_

#include "stdint.h"
// backlight.h
#pragma once
#include "stdbool.h"

void backlight_init(void);
void backlight_deinit(void);
void backlight_set(uint8_t percent); // 0–100
uint8_t backlight_get_zeitplan_pct(void);
uint8_t backlight_get_auto_pct(uint8_t min_pct, uint8_t max_pct);
void backlight_apply(uint8_t pct);  // задать цель, доедет tick
void backlight_tick(void);          // звать из таймера 33 мс

#endif /* MAIN_USER_PERIPHERY_BACKLIGHT_H_ */
