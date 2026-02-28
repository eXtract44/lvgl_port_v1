/*
 * periphery.c
 *
 *  Created on: 07.02.2026
 *      Author: toose
 */

#include "periphery.h"
#include "driver/i2c.h"
#include "waveshare_rgb_lcd_port.h"
#include "user/menu/lvgl_user_config.h"
#include <time.h>
#include "esp_wifi.h"

current_weather_t current_weather_data = {0};
extern struct tm timeinfo;
extern wifi_ap_record_t ap_info;

#define SIMULATE_AHT10_VALUES 1
#define SIMULATE_SGP30_VALUES 1
#define SIMULATE_INET_VALUES 0
#define SIMULATE_STANDBY 0
#define SIMULATE_BATTERY 0
#define SIMULATE_VOLUME 0
#define SIMULATE_DISTANCE 0

aht10_data_t aht_data;

uint8_t AHT10_TmpHum_Cmd[3] = {AHTX0_CMD_TRIGGER, 0x33, 0x00};
uint8_t AHT10_RX_Data[6];

void aht10_init() {
  uint8_t soft_res = AHTX0_CMD_SOFTRESET;
  i2c_master_write_to_device(I2C_MASTER_NUM, AHT10_ADRESS, &soft_res, 1,
                             I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  vTaskDelay(pdMS_TO_TICKS(50));
  uint8_t calib = AHTX0_CMD_CALIBRATE;
  i2c_master_write_to_device(I2C_MASTER_NUM, AHT10_ADRESS, &calib, 1,
                             I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

void aht10_read() {
#if SIMULATE_AHT10_VALUES
  static uint8_t temp = 0;
  temp++;
  if (temp > 100)
    temp = 0;
  aht_data.humidity = temp;

  static float temp_f = -10;
  temp_f = temp_f + 0.3f;
  if (temp_f > 60)
    temp_f = -10;
  aht_data.temperature = temp_f;

#else
  i2c_master_write_to_device(I2C_MASTER_NUM, AHT10_ADRESS,
                             (uint8_t *)AHT10_TmpHum_Cmd, 3,
                             I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
  vTaskDelay(pdMS_TO_TICKS(50));
  /* Receive data: STATUS[1]:HIMIDITY[2.5]:TEMPERATURE[2.5] */
  i2c_master_read_from_device(I2C_MASTER_NUM, AHT10_ADRESS,
                              (uint8_t *)AHT10_RX_Data, 6,
                              I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

  if (~AHT10_RX_Data[0] & AHTX0_STATUS_BUSY) // aht10 not busy
  {
    /* Convert to Temperature in °C */
    AHT10_ADC_Raw = (((uint32_t)AHT10_RX_Data[3] & 15U) << 16U) |
                    ((uint32_t)AHT10_RX_Data[4] << 8U) | AHT10_RX_Data[5];
    aht_data.temperature =
        (float)(AHT10_ADC_Raw * 200.00f / 1048576.00f) - 50.00f;
    //			/* Convert to Relative Humidity in % */
    AHT10_ADC_Raw = ((uint32_t)AHT10_RX_Data[1] << 12U) |
                    ((uint32_t)AHT10_RX_Data[2] << 4U) |
                    (AHT10_RX_Data[3] >> 4U);
    aht_data.humidity = (uint8_t)(AHT10_ADC_Raw * 100.00f / 1048576.00f);
  }
#endif
}
float get_temperature_aht10() { return aht_data.temperature; }
uint8_t get_humidity_aht10() { return aht_data.humidity; }

sgp30_data_t sgp30_data;

static uint8_t sgp30_send_cmd(sgp30_cmd_t cmd) {
  uint8_t cmd_buffer[2];
  cmd_buffer[0] = cmd >> 8;
  cmd_buffer[1] = cmd;
  return i2c_master_write_to_device(I2C_MASTER_NUM, SGP30_ADDR_WRITE,
                                    (uint8_t *)cmd_buffer, 2,
                                    I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static int sgp30_soft_reset(void) {
  uint8_t cmd = 0x06;
  return i2c_master_write_to_device(I2C_MASTER_NUM, SGP30_ADDR_WRITE, &cmd, 1,
                                    I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

int sgp30_init(void) {
  int status;

  status = sgp30_soft_reset();
  vTaskDelay(pdMS_TO_TICKS(100));

  status = sgp30_send_cmd(INIT_AIR_QUALITY);

  vTaskDelay(pdMS_TO_TICKS(100));

  return status == 0 ? 0 : -1;
}

static int sgp30_start(void) { return sgp30_send_cmd(MEASURE_AIR_QUALITY); }

static uint8_t CheckCrc8(uint8_t *const message, uint8_t initial_value) {
  uint8_t remainder;
  uint8_t i = 0, j = 0;
  remainder = initial_value;
  for (j = 0; j < 2; j++) {
    remainder ^= message[j];
    for (i = 0; i < 8; i++) {
      if (remainder & 0x80) {
        remainder = (remainder << 1) ^ CRC8_POLYNOMIAL;
      } else {
        remainder = (remainder << 1);
      }
    }
  }

  /*CRC */
  return remainder;
}

int sgp30_read(void) {
#if SIMULATE_SGP30_VALUES
  static uint16_t temp = 400;
  temp += 10;
  if (temp > 9999)
    temp = 400;
  sgp30_data.co2 = temp;
  sgp30_data.tvoc = temp - 111;
#else
  int status;
  uint8_t recv_buffer[6] = {0};

  /* 启动测量 */
  status = sgp30_start();
  if (status != 0) {
  }

  vTaskDelay(pdMS_TO_TICKS(100));

  status = i2c_master_read_from_device(
      I2C_MASTER_NUM, SGP30_ADDR_READ, (uint8_t *)recv_buffer, 6,
      I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

  if (CheckCrc8(&recv_buffer[0], 0xFF) != recv_buffer[2]) {
    // printf("co2 recv data crc check fail\r\n");
    return -1;
  }
  if (CheckCrc8(&recv_buffer[3], 0xFF) != recv_buffer[5]) {
    // printf("tvoc recv data crc check fail\r\n");
    return -1;
  }
  sgp30_data.co2 = recv_buffer[0] << 8 | recv_buffer[1];
  sgp30_data.tvoc = recv_buffer[3] << 8 | recv_buffer[4];

#endif
  return 0;
}

uint16_t get_co2_sgp30() { return sgp30_data.co2; }

uint16_t get_tvoc_sgp30() { return sgp30_data.tvoc; }

void read_sensors() {
  sgp30_read();
  aht10_read();
}

uint8_t get_wifi_status() {
  static uint8_t current_wifi_status = WIFI_DISCONNECTED;
#if SIMULATE_INET_VALUES
  // current_wifi_status++;
  // if(current_wifi_status ==WIFI_DISCONNECTED){
  //	current_wifi_status=WIFI_RECONNECT;
  // }
  current_wifi_status = WIFI_CONNECTED;
#else
if(esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK){
	current_wifi_status = WIFI_CONNECTED;
}else{
	current_wifi_status = WIFI_DISCONNECTED;
}

#endif
  return current_wifi_status;
}

const char* get_wifi_ssid(){
	return (const char*)ap_info.ssid;
}

int16_t get_wifi_rssi(){
	return ap_info.rssi;
}
 
uint8_t get_time_mday() {
  static uint8_t current_mday = 1;
#if SIMULATE_INET_VALUES
  current_mday++;
  if (current_mday > 31) {
    current_mday = 1;
  }
#else
current_mday = timeinfo.tm_mday;
#endif
  return current_mday;
}

uint8_t get_time_month() {
  static uint8_t current_monts = 1;
#if SIMULATE_INET_VALUES
  current_monts++;
  if (current_monts > 12) {
    current_monts = 1;
  }
#else
 current_monts = timeinfo.tm_mon+1;
#endif
  return current_monts;
}

uint8_t get_time_hour() {
  static uint8_t current_hour = 0;
#if SIMULATE_INET_VALUES
  current_hour++;
  if (current_hour > 23) {
    current_hour = 0;
  }
#else
current_hour = timeinfo.tm_hour;
#endif
  return current_hour;
}

uint8_t get_time_minute() {
  static uint8_t current_minute = 0;
#if SIMULATE_INET_VALUES
  current_minute++;
  if (current_minute > 59) {
    current_minute = 0;
  }
#else
current_minute =timeinfo.tm_min;
#endif
  return current_minute;
}

uint8_t get_time_wday() {
  static uint8_t current_wday = 8;
#if SIMULATE_INET_VALUES
  current_wday++;
  if (current_wday > 7) {
    current_wday = 1;
  }
#else
current_wday = timeinfo.tm_wday;
#endif
  return current_wday;
}

float get_weather_temperature() {
  static float current_temperature_outside = 0;
#if SIMULATE_INET_VALUES
  current_temperature_outside += 0.05;
  if (current_temperature_outside > 35) {
    current_temperature_outside = -20.0;
  }
#else
current_temperature_outside = current_weather_data.temperature_2m;
#endif
  return current_temperature_outside;
}

uint8_t get_weather_humidity() {
  static uint8_t current_humidity_outside = 0;
#if SIMULATE_INET_VALUES
  current_humidity_outside++;
  if (current_humidity_outside > 99) {
    current_humidity_outside = 0;
  }
#else
current_humidity_outside = current_weather_data.relative_humidity_2m;
#endif
  return current_humidity_outside;
}

uint8_t get_weather_wind() {
  static uint8_t current_wind_outside = 0;
#if SIMULATE_INET_VALUES
  current_wind_outside += 1;
  if (current_wind_outside > 50) {
    current_wind_outside = 0;
  }
#else
current_wind_outside = current_weather_data.wind_speed_10m;
#endif
  return current_wind_outside;
}

uint8_t get_weather_clouds() {
  static uint8_t current_clouds = 0;
#if SIMULATE_INET_VALUES
  current_clouds += 1;
  if (current_clouds > 100) {
    current_clouds = 0;
  }
#else
current_clouds = current_weather_data.cloud_cover;
#endif
  return current_clouds;
}

uint8_t get_weather_rain() {
  static uint8_t current_rain = 0;
#if SIMULATE_INET_VALUES
  current_rain += 1;
  if (current_rain > 5) {
    current_rain = 0;
  }
#else
current_rain = current_weather_data.rain;
#endif
  return current_rain;
}

uint8_t get_weather_snow() {
  static uint8_t current_snow = 0;
#if SIMULATE_INET_VALUES
  current_snow += 1;
  if (current_snow > 5) {
    current_snow = 0;
  }
#else
current_snow = current_weather_data.snow;
#endif
  return current_snow;
}

uint8_t get_battery_status() {
  static uint8_t current_battery_status = BATTERY_EMPTY;
#if SIMULATE_BATTERY
  current_battery_status++;
  if (current_battery_status > BATTERY_FULL) {
    current_battery_status = BATTERY_EMPTY;
  }
#else

#endif
  return current_battery_status;
}

uint8_t get_volume() {
  static uint8_t current_volume = 1;
#if SIMULATE_VOLUME
  current_volume++;
  if (current_volume > 3) {
    current_volume = 1;
  }
#else

#endif
  return current_volume;
}

