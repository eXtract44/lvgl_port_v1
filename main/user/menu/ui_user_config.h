/*
 * lvgl_user_config.h
 *
 *  Created on: 10.02.2026
 *      Author: toose
 Главная ошибка сборки (из-за неё нет build.ninja)
 The "path" field in idf_component.yml does not point to a directory
 Решение (правильное)
 Открой:
 C:\Espressif\Workspace\lvgl_port_v1\components\espressif__esp_lcd_touch\idf_component.yml
 И удали строку
 path: ...
 */

#ifndef MAIN_LVGL_USER_CONFIG_H_
#define MAIN_LVGL_USER_CONFIG_H_

#define CURRENT_SOFT_VERSION "16.05.26"

#define ACTIVATE_BLOCK_TOP_LEFT 1  
#define ACTIVATE_BLOCK_TOP_MID 	1 
#define ACTIVATE_BLOCK_TOP_RIGHT 1 
#define ACTIVATE_BLOCK_BOT_LEFT 1  
#define ACTIVATE_BLOCK_BOT_MID 1
#define ACTIVATE_BLOCK_BOT_RIGHT 1

#define ACTIVATE_ANIM_SUN_MOON 1
#define ACTIVATE_ANIM_CLOUD 1
#define ACTIVATE_ANIM_WIND 1
#define ACTIVATE_ANIM_RAIN 1
#define ACTIVATE_ANIM_SNOW 1

#define SIMULATE_ANIM_SUN 0
#define SIMULATE_ANIM_CLOUD 0
#define SIMULATE_ANIM_WIND 0
#define SIMULATE_ANIM_RAIN 0
#define SIMULATE_ANIM_SNOW 0

#define SIMULATE_SHT31_VALUES 0
#define SIMULATE_SGP30_VALUES 0
#define SIMULATE_INET_VALUES 0
#define SIMULATE_TIME_VALUES 0

#define UPDATE_WEATHER_SEC 60

#define DAY_START_HOUR 7
#define DAY_END_HOUR 20

#define MAX_STANDBY_TIME 180 // sec
#define FEELS_LIKE_MIN_DIFF 1.5f
#define CO2_CALIB_TIME (12UL * 3600UL * configTICK_RATE_HZ)

#define SENSOR_HISTORY_POINTS 144 // 12 часов × 12 точек/час
#define SENSOR_RECORD_INTERVAL 300 //300 тиков (секунд) между записями


#endif /* MAIN_LVGL_USER_CONFIG_H_ */
