/*
 * periphery.h
 *
 *  Created on: 07.02.2026
 *      Author: toose
 */

#ifndef MAIN_PERIPHERY_H_
#define MAIN_PERIPHERY_H_

#include <stdint.h>

#define AHT10_ADRESS (0x38 << 1) // 0b1110000; Adress[7-bit]

#define AHTX0_I2CADDR_DEFAULT 0x38   ///< AHT default i2c address
#define AHTX0_I2CADDR_ALTERNATE 0x39 ///< AHT alternate i2c address
#define AHTX0_CMD_CALIBRATE 0xE1     ///< Calibration command
#define AHTX0_CMD_TRIGGER 0xAC       ///< Trigger reading command
#define AHTX0_CMD_SOFTRESET 0xBA     ///< Soft reset command
#define AHTX0_STATUS_BUSY 0x80       ///< Status bit for busy
#define AHTX0_STATUS_CALIBRATED 0x08 ///< Status bit for calibrated

typedef struct aht10_data_st {
    float temperature;
    uint8_t humidity;
}aht10_data_t;

void aht10_init();
void aht10_read();
float get_temperature_aht10();
uint8_t get_humidity_aht10();

#define CRC8_POLYNOMIAL 0x31
#define SGP30_ADDR          0x58
#define	SGP30_ADDR_WRITE	SGP30_ADDR<<1       //0xb0
#define	SGP30_ADDR_READ		(SGP30_ADDR<<1)+1   //0xb1

typedef struct sgp30_data_st {
    uint16_t co2;
    uint16_t tvoc;
}sgp30_data_t;

typedef enum sgp30_cmd_en {
    INIT_AIR_QUALITY = 0x2003,
    MEASURE_AIR_QUALITY = 0x2008
} sgp30_cmd_t;


int sgp30_init(void);
int sgp30_read(void);
uint16_t get_co2_sgp30();
uint16_t get_tvoc_sgp30();

void read_sensors();

enum namesOfWiFiStatus{
	WIFI_RECONNECT,
	WIFI_CONNECTED,
	WIFI_DISCONNECTED
};

enum namesOfBatteryStatus{
	BATTERY_EMPTY,
	BATTERY_20,
	BATTERY_50,
	BATTERY_70,
	BATTERY_FULL,
};

typedef struct {
  double temperature_2m;
  int relative_humidity_2m;
  double snow;
  double rain;
  int cloud_cover;
  double wind_speed_10m;
  int wifi_connected;

} current_weather_t;

uint8_t get_wifi_status();
const char* get_wifi_ssid();
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

uint8_t get_battery_status();
uint8_t get_volume();


#endif /* MAIN_PERIPHERY_H_ */
