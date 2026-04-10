/*
 * periphery.h
 *
 *  Created on: 07.02.2026
 *      Author: toose
 */

#ifndef MAIN_PERIPHERY_H_
#define MAIN_PERIPHERY_H_

#include <stdint.h>

// SHT31 I2C address (ADDR pin low = 0x44, high = 0x45)
#define SHT31_ADDRESS           0x44
 
// SHT31 commands (2 bytes each)
#define SHT31_CMD_SOFTRESET_MSB 0x30
#define SHT31_CMD_SOFTRESET_LSB 0xA2
 
// Single-shot measurement: High Repeatability, Clock Stretching Disabled
#define SHT31_CMD_MEAS_MSB      0x24
#define SHT31_CMD_MEAS_LSB      0x00
 
// Status register read
#define SHT31_CMD_STATUS_MSB    0xF3
#define SHT31_CMD_STATUS_LSB    0x2D
 
// Measurement ready time: High repeatability requires ~15ms
#define SHT31_MEAS_DELAY_US     15000
 
typedef enum {
    SHT31_STATE_IDLE = 0,
    SHT31_STATE_TRIGGERED,
} sht31_state_t;
 

typedef enum {
	SHT31_STATE_FAIL,
	SHT31_STATE_OK,
} sht31_life_t;

typedef struct {
    float    temperature;
    uint8_t  humidity;
    sht31_state_t state;
	sht31_life_t life;
    int64_t  trigger_time;
} sht31_data_t;
 


void    sht31_init(void);
void    sht31_read(void);
float   get_temperature_sht31(void);
uint8_t get_humidity_sht31(void);

#define CRC8_POLYNOMIAL 0x31
#define SGP30_ADDR          0x58
#define	SGP30_ADDR_WRITE	SGP30_ADDR      
#define	SGP30_ADDR_READ		SGP30_ADDR   

typedef enum {
	SGP30_STATE_FAIL,
	SGP30_STATE_OK,
} sgp30_state_t;

typedef struct sgp30_data_st {
    uint16_t co2;
    uint16_t tvoc;
	sgp30_state_t state;
}sgp30_data_t;



typedef enum sgp30_cmd_en {
    INIT_AIR_QUALITY = 0x2003,
    MEASURE_AIR_QUALITY = 0x2008
} sgp30_cmd_t;


int sgp30_init();
int sgp30_read();
uint16_t get_co2_sgp30();
uint16_t get_tvoc_sgp30();

void read_sensors();

enum namesOfWiFiStatus{
	WIFI_RECONNECT,
	WIFI_CONNECTED,
	WIFI_DISCONNECTED
};

typedef struct {
  double temperature_2m;
  int relative_humidity_2m;
  double snow;
  double rain;
  int cloud_cover;
  double wind_speed_10m;
  int wifi_connected;
  int is_day;

} current_weather_t;

void sht31_init();

uint8_t get_wifi_status();
const char* get_wifi_ssid();
const char* get_wifi_pass();
int16_t get_wifi_rssi();

uint8_t get_time_mday();
uint8_t get_time_month();
uint8_t get_time_hour();
uint8_t get_time_minute();
uint8_t get_time_wday();
	
float get_weather_temperature();
uint8_t get_weather_humidity();
uint8_t get_weather_wind();
uint8_t get_weather_clouds();
uint8_t get_weather_rain();
uint8_t get_weather_snow();
uint8_t get_is_day();


#endif /* MAIN_PERIPHERY_H_ */
