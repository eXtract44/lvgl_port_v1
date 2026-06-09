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


// ==== Luftqualitaet-Variante ====
#define CONFIG_HAS_SCD41 0   // 1 = SCD41+SGP30, 0 = nur SGP30

#if CONFIG_HAS_SCD41
  #define AQ_UNIT_STR        "ppm"
  #define AQ_FOOTER_STR      "CO2 | Luftqualitaet"
  #define AQ_SCALE_MIN       400
  #define AQ_SCALE_MAX       2400
  #define AQ_ZONE_GREEN_END  1000   // зелёная дуга: MIN..1000  (GUT)
  #define AQ_ZONE_RED_START  2000   // красная дуга: 2000..MAX  (HOCH)
  #define AQ_THRESH_GUT      1000
  #define AQ_THRESH_MITTEL   2000
  #define AQ_GET_VALUE()     get_co2_scd41()
  #define AQ_SENSOR_OK()   (scd41_data.life == SCD41_STATE_OK)
   // Farbzonen Meter: gruen 400..1000, rot 2000..2400
     #define MIN_VALUE_CO2    400
  #define MAX_VALUE_CO2    2400
  #define BLOCK_TOP_MID_START_CO2_LEFT_PART   MIN_VALUE_CO2
  #define BLOCK_TOP_MID_END_CO2_LEFT_PART     AQ_THRESH_GUT
  #define BLOCK_TOP_MID_START_CO2_RIGHT_PART  AQ_THRESH_MITTEL
  #define BLOCK_TOP_MID_END_CO2_RIGHT_PART    MAX_VALUE_CO2
#else
  #define AQ_UNIT_STR        "ppb"
  #define AQ_FOOTER_STR      "TVOC | Luftqualitaet"
  #define AQ_SCALE_MIN       0
  #define AQ_SCALE_MAX       1000
  #define AQ_ZONE_GREEN_END  150
  #define AQ_ZONE_RED_START  850
  #define AQ_THRESH_GUT      150
  #define AQ_THRESH_MITTEL   350
  #define AQ_GET_VALUE()     get_tvoc_sgp30()
  #define AQ_SENSOR_OK()   (sgp30_data.state == SGP30_STATE_OK)
   // Farbzonen Meter: gruen 0..150, rot 850..1000  (wie bisher)
   #define MIN_VALUE_CO2    0
  #define MAX_VALUE_CO2    1000
  #define BLOCK_TOP_MID_START_CO2_LEFT_PART   MIN_VALUE_CO2
  #define BLOCK_TOP_MID_END_CO2_LEFT_PART     150
  #define BLOCK_TOP_MID_START_CO2_RIGHT_PART  850
  #define BLOCK_TOP_MID_END_CO2_RIGHT_PART    MAX_VALUE_CO2
#endif


#define CURRENT_SOFT_VERSION "18.05.26"

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
#define SIMULATE_SCD41_VALUES 1
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
