/*
 * periphery.c
 *
 *  Created on: 07.02.2026
 *      Author: toose
 */

#include "periphery.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "i2c_bus.h"
#include "time_user.h"
#include "user/menu/lvgl_user_config.h"
#include "waveshare_rgb_lcd_port.h"
#include <stdint.h>

current_weather_t current_weather_data = {0};
extern struct tm timeinfo_user;
extern wifi_ap_record_t ap_info;
extern uint8_t wifi_ssid[];
extern uint8_t wifi_password[];

#define SIMULATE_SHT31_VALUES 1
#define SIMULATE_SGP30_VALUES 0
#define SIMULATE_INET_VALUES 0

static bool sht31_check_crc(uint8_t *data, uint8_t len, uint8_t crc_byte) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
        }
    }
    return crc == crc_byte;
}
 
// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------
void sht31_init(void) {
    vTaskDelay(pdMS_TO_TICKS(50)); // power-on stabilisation
 
    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
 
    uint8_t soft_reset[2] = {SHT31_CMD_SOFTRESET_MSB, SHT31_CMD_SOFTRESET_LSB};
    esp_err_t ret = i2c_master_write_to_device(
        I2C_MASTER_NUM, SHT31_ADDRESS,
        soft_reset, 2,
        pdMS_TO_TICKS(100));
 
    xSemaphoreGive(i2c_bus_mutex);
 
    if (ret != ESP_OK) {
        ESP_LOGE("SHT31", "Soft reset failed: %s", esp_err_to_name(ret));
        return;
    }
 
    vTaskDelay(pdMS_TO_TICKS(2)); // reset takes ~1.5 ms per datasheet
    ESP_LOGI("SHT31", "__Init__ done");
}
 
// ---------------------------------------------------------------------------
// Non-blocking read  (call periodically from your task)
// ---------------------------------------------------------------------------
void sht31_read(void) {
#if SIMULATE_SHT31_VALUES
    // --- simulation block (mirrors AHT10 simulation) ---
    static uint8_t upper_humi = 0;
    static uint8_t sim_humi   = 30;
    if (upper_humi) { if (--sim_humi < 30) upper_humi = 0; }
    else            { if (++sim_humi > 92) upper_humi = 1; }
    sht31_data.humidity = sim_humi;
 
    static uint8_t upper_temp   = 0;
    static float   sim_temp     = 12.0f;
    if (upper_temp) { sim_temp -= 0.1f; if (sim_temp < 12.0f) upper_temp = 0; }
    else            { sim_temp += 0.1f; if (sim_temp > 28.0f) upper_temp = 1; }
    sht31_data.temperature = sim_temp;
 
#else
    // --- real sensor ---
 
    if (sht31_data.state == SHT31_STATE_IDLE) {
        // Send measurement trigger command (2 bytes)
        uint8_t meas_cmd[2] = {SHT31_CMD_MEAS_MSB, SHT31_CMD_MEAS_LSB};
 
        xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
        esp_err_t ret = i2c_master_write_to_device(
            I2C_MASTER_NUM, SHT31_ADDRESS,
            meas_cmd, 2,
            pdMS_TO_TICKS(100));
        xSemaphoreGive(i2c_bus_mutex);
 
        if (ret != ESP_OK) {
            ESP_LOGE("SHT31", "Trigger failed: %s", esp_err_to_name(ret));
            return;
        }
 
        sht31_data.trigger_time = esp_timer_get_time();
        sht31_data.state        = SHT31_STATE_TRIGGERED;
        return;
    }
 
    if (sht31_data.state == SHT31_STATE_TRIGGERED) {
        // Wait at least 15 ms for High-Repeatability measurement
        if ((esp_timer_get_time() - sht31_data.trigger_time) < SHT31_MEAS_DELAY_US) {
            return;
        }
 
        // Read 6 bytes: [Temp MSB][Temp LSB][Temp CRC][Hum MSB][Hum LSB][Hum CRC]
        uint8_t rx[6];
        xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
        esp_err_t ret = i2c_master_read_from_device(
            I2C_MASTER_NUM, SHT31_ADDRESS,
            rx, 6,
            pdMS_TO_TICKS(100));
        xSemaphoreGive(i2c_bus_mutex);
 
        sht31_data.state = SHT31_STATE_IDLE; // reset regardless of outcome
 
        if (ret != ESP_OK) {
            ESP_LOGE("SHT31", "Read failed: %s", esp_err_to_name(ret));
            return;
        }
 
        // CRC check for temperature
        if (!sht31_check_crc(&rx[0], 2, rx[2])) {
            ESP_LOGE("SHT31", "Temperature CRC mismatch");
            return;
        }
 
        // CRC check for humidity
        if (!sht31_check_crc(&rx[3], 2, rx[5])) {
            ESP_LOGE("SHT31", "Humidity CRC mismatch");
            return;
        }
 
        // Convert temperature: T = -45 + 175 * raw / 65535
        uint16_t raw_temp = ((uint16_t)rx[0] << 8) | rx[1];
        sht31_data.temperature = -45.0f + 175.0f * (float)raw_temp / 65535.0f;
 
        // Convert humidity: RH = 100 * raw / 65535
        uint16_t raw_hum  = ((uint16_t)rx[3] << 8) | rx[4];
        sht31_data.humidity = (uint8_t)(100.0f * (float)raw_hum / 65535.0f);
 
        ESP_LOGI("SHT31", "__Temp__: %.1f C, Humidity: %d%%",
                 sht31_data.temperature, sht31_data.humidity);
    }
#endif
}
 
// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------
float   get_temperature_sht31(void) { return sht31_data.temperature; }
uint8_t get_humidity_sht31(void)    { return sht31_data.humidity;    }



sgp30_data_t sgp30_data;

static uint8_t sgp30_send_cmd(sgp30_cmd_t cmd) {
	uint8_t cmd_buffer[2];
	cmd_buffer[0] = cmd >> 8;
	cmd_buffer[1] = cmd;
	return i2c_master_write_to_device(
		I2C_MASTER_NUM, SGP30_ADDR_WRITE, (uint8_t *)cmd_buffer, 2,
		I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static int sgp30_soft_reset(void) {
	uint8_t cmd = 0x06;
	return i2c_master_write_to_device(I2C_MASTER_NUM, SGP30_ADDR_WRITE, &cmd, 1,
									  I2C_MASTER_TIMEOUT_MS /
										  portTICK_PERIOD_MS);
}

int sgp30_init(void) {
	xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);

	int status;
	status = sgp30_soft_reset();
	vTaskDelay(pdMS_TO_TICKS(100));

	status = sgp30_send_cmd(INIT_AIR_QUALITY);
	vTaskDelay(pdMS_TO_TICKS(100));

	xSemaphoreGive(i2c_bus_mutex);
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
static uint8_t upper_co2 = 0;
static uint16_t temp_co2 = 30;
if (upper_co2) {
	temp_co2-=13;
	if (temp_co2 < 400) {
		upper_co2 = 0;
	}
} else {
	temp_co2+=13;
	if (temp_co2 > 2400) {
		upper_co2 = 1;
	}
}

	static uint16_t temp = 400;
	temp += 10;
	if (temp > 9999)
		temp = 400;
	sgp30_data.co2 = temp;
	sgp30_data.tvoc = temp - 111;
#else
	int status;
	uint8_t recv_buffer[6] = {0};

	xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);

	status = sgp30_start();
	if (status != 0) {
		ESP_LOGE("SGP30", "Start failed");
		xSemaphoreGive(i2c_bus_mutex);
		return -1;
	}

	vTaskDelay(pdMS_TO_TICKS(100));

	status = i2c_master_read_from_device(I2C_MASTER_NUM, SGP30_ADDR_READ,
										 recv_buffer, 6,
										 pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

	xSemaphoreGive(i2c_bus_mutex);

	if (status != 0) {
		ESP_LOGE("SGP30", "Read failed");
		return -1;
	}

	if (CheckCrc8(&recv_buffer[0], 0xFF) != recv_buffer[2]) {
		return -1;
	}
	if (CheckCrc8(&recv_buffer[3], 0xFF) != recv_buffer[5]) {
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
	
	sht31_read();
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
	if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
		current_wifi_status = WIFI_CONNECTED;
	} else {
		current_wifi_status = WIFI_DISCONNECTED;
	}

#endif
	return current_wifi_status;
}

const char *get_wifi_ssid() {
	if (get_wifi_status() == WIFI_CONNECTED) {
		return (const char *)ap_info.ssid;
	}
	return 0;
}

const char *get_wifi_pass() {
	if (get_wifi_status() == WIFI_CONNECTED) {
		return (const char *)wifi_password;
	}
	return 0;
}

int16_t get_wifi_rssi() {
	if (get_wifi_status() == WIFI_CONNECTED) {
		return ap_info.rssi;
	}
	return 0;
}

uint8_t get_time_mday() {
	static uint8_t current_mday = 1;
#if SIMULATE_INET_VALUES
	current_mday++;
	if (current_mday > 31) {
		current_mday = 1;
	}
#else
	current_mday = timeinfo_user.tm_mday;
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
	current_monts = timeinfo_user.tm_mon + 1;
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
	current_hour = timeinfo_user.tm_hour;
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
	current_minute = timeinfo_user.tm_min;
#endif
	return current_minute;
}

uint8_t get_time_wday() {
	static uint8_t current_wday = 8;
#if SIMULATE_INET_VALUES
	current_wday++;
	if (current_wday > 6) {
		current_wday = 0;
	}
#else
	current_wday = timeinfo_user.tm_wday;
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

uint8_t get_is_day() { return current_weather_data.is_day; }
