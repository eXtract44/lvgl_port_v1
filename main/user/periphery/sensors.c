/*
 * periphery.c
 *
 *  Created on: 07.02.2026
 *      Author: toose
 *
 *  SHT31 → SHT41 migration
 *
 *  Key differences vs SHT31:
 *    - Soft reset is a single byte (0x94), not a 2-byte command
 *    - Measure command is a single byte (0xFD / 0xF6 / 0xE0)
 *    - Humidity formula: RH = -6 + 125 * raw / 65535  (SHT41 datasheet §4.5)
 *    - Temperature formula: T  = -45 + 175 * raw / 65535  (same as SHT31)
 *    - CRC algorithm unchanged (CRC-8, poly 0x31, init 0xFF)
 */

#include "sensors.h"

#include "driver/i2c.h"
#include "esp_timer.h"
#include "i2c_bus.h"
#include "waveshare_rgb_lcd_port.h"
#include <math.h>
#include <stdint.h>
#include "../ui/ui_user_config.h"

sgp30_data_t sgp30_data = {0};

sht41_data_t sht41_data = {
    .temperature  = 0,
    .humidity     = 0,
    .state        = SHT41_STATE_IDLE,
    .trigger_time = 0,
};

// ---------------------------------------------------------------------------
// CRC-8 helper (poly 0x31, init 0xFF) — shared by SHT41 and SGP30
// ---------------------------------------------------------------------------
static bool sht41_check_crc(uint8_t *data, uint8_t len, uint8_t crc_byte) {
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
// SHT41 — Init
// ---------------------------------------------------------------------------
void sht41_init(void) {
    vTaskDelay(pdMS_TO_TICKS(50)); // power-on stabilisation

    // SHT41 soft reset is a SINGLE byte command (0x94)
    uint8_t soft_reset = SHT41_CMD_SOFTRESET;

    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = i2c_master_write_to_device(
        I2C_MASTER_NUM, SHT41_ADDRESS, &soft_reset, 1, pdMS_TO_TICKS(100));
    xSemaphoreGive(i2c_bus_mutex);

    if (ret != ESP_OK) {
        ESP_LOGE("SHT41", "Soft reset failed: %s", esp_err_to_name(ret));
        sht41_data.state = SHT41_STATE_IDLE;
        sht41_data.life  = SHT41_STATE_FAIL;
        return;
    }

    vTaskDelay(pdMS_TO_TICKS(1)); // reset takes ~1 ms per datasheet
    ESP_LOGI("SHT41", "__Init__ done");
    sht41_data.life = SHT41_STATE_OK;
}

// ---------------------------------------------------------------------------
// SHT41 — Non-blocking read  (call periodically from your task)
// ---------------------------------------------------------------------------
void sht41_read(void) {
#if SIMULATE_SHT41_VALUES
    // --- simulation block ---
    static uint8_t upper_humi = 0;
    static uint8_t sim_humi   = 30;
    if (upper_humi) {
        if (--sim_humi < 30) upper_humi = 0;
    } else {
        if (++sim_humi > 92) upper_humi = 1;
    }
    sht41_data.humidity = sim_humi;

    static uint8_t upper_temp = 0;
    static float   sim_temp   = 12.0f;
    if (upper_temp) {
        sim_temp -= 0.1f;
        if (sim_temp < 12.0f) upper_temp = 0;
    } else {
        sim_temp += 0.1f;
        if (sim_temp > 28.0f) upper_temp = 1;
    }
    sht41_data.temperature = sim_temp;

#else
    // --- real sensor ---
    static uint16_t try_to_read_cnt  = 0;
    const  uint16_t number_of_try    = 20;

    if (sht41_data.state == SHT41_STATE_IDLE) {
        // SHT41 measure command is a SINGLE byte
        uint8_t meas_cmd = SHT41_CMD_MEAS;

        xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
        esp_err_t ret = i2c_master_write_to_device(
            I2C_MASTER_NUM, SHT41_ADDRESS, &meas_cmd, 1, pdMS_TO_TICKS(100));
        xSemaphoreGive(i2c_bus_mutex);

        if (ret != ESP_OK) {
            ESP_LOGE("SHT41", "Trigger failed: %s", esp_err_to_name(ret));
            return;
        }

        sht41_data.trigger_time = esp_timer_get_time();
        sht41_data.state        = SHT41_STATE_TRIGGERED;
        return;
    }

    if (sht41_data.state == SHT41_STATE_TRIGGERED) {
        // Wait for measurement to complete (High precision: 10 ms)
        if ((esp_timer_get_time() - sht41_data.trigger_time) < SHT41_MEAS_DELAY_US) {
            return;
        }

        // Read 6 bytes: [T_MSB][T_LSB][T_CRC][RH_MSB][RH_LSB][RH_CRC]
        uint8_t rx[6];
        xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
        esp_err_t ret = i2c_master_read_from_device(
            I2C_MASTER_NUM, SHT41_ADDRESS, rx, 6, pdMS_TO_TICKS(100));
        xSemaphoreGive(i2c_bus_mutex);

        sht41_data.state = SHT41_STATE_IDLE; // reset regardless of outcome

        if (ret != ESP_OK) {
            ESP_LOGE("SHT41", "Read failed: %s", esp_err_to_name(ret));
            try_to_read_cnt++;
            goto check_fail;
        }

        // CRC check for temperature
        if (!sht41_check_crc(&rx[0], 2, rx[2])) {
            ESP_LOGE("SHT41", "Temperature CRC mismatch");
            try_to_read_cnt++;
            goto check_fail;
        }

        // CRC check for humidity
        if (!sht41_check_crc(&rx[3], 2, rx[5])) {
            ESP_LOGE("SHT41", "Humidity CRC mismatch");
            try_to_read_cnt++;
            goto check_fail;
        }

        {
            // SHT41 temperature formula (same as SHT31):
            //   T [°C] = -45 + 175 * raw / 65535
            uint16_t raw_temp = ((uint16_t)rx[0] << 8) | rx[1];
            sht41_data.temperature = -45.0f + 175.0f * (float)raw_temp / 65535.0f;

            // SHT41 humidity formula (DIFFERENT from SHT31):
            //   RH [%] = -6 + 125 * raw / 65535   (clamped to [0, 100])
            uint16_t raw_hum = ((uint16_t)rx[3] << 8) | rx[4];
            float rh = -6.0f + 125.0f * (float)raw_hum / 65535.0f;
            if (rh < 0.0f)   rh = 0.0f;
            if (rh > 100.0f) rh = 100.0f;
            sht41_data.humidity = (uint8_t)rh;
        }

        try_to_read_cnt  = 0;
        sht41_data.life  = SHT41_STATE_OK;
        return;

check_fail:
        if (try_to_read_cnt > number_of_try) {
            try_to_read_cnt = number_of_try + 1; // prevent wrap
            sht41_data.life = SHT41_STATE_FAIL;
        }
    }
#endif
}

// ---------------------------------------------------------------------------
// SHT41 — Getters
// ---------------------------------------------------------------------------
float get_temperature_sht41(void) {
    float ret = sht41_data.temperature;
    if (ret < -40.0f) ret = -40.0f;
    if (ret > 125.0f) ret = 125.0f;
    return ret;
}

uint8_t get_humidity_sht41(void) {
    uint8_t ret = sht41_data.humidity;
    if (ret > 100) ret = 100;
    return ret;
}

// ---------------------------------------------------------------------------
// SGP30  (unchanged from SHT31 version)
// ---------------------------------------------------------------------------
static uint8_t sgp30_send_cmd(sgp30_cmd_t cmd) {
    uint8_t cmd_buffer[2];
    cmd_buffer[0] = cmd >> 8;
    cmd_buffer[1] = (uint8_t)cmd;
    return i2c_master_write_to_device(
        I2C_MASTER_NUM, SGP30_ADDR_WRITE, cmd_buffer, 2,
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static int sgp30_soft_reset(void) {
    uint8_t cmd = 0x06;
    return i2c_master_write_to_device(
        I2C_MASTER_NUM, SGP30_ADDR_WRITE, &cmd, 1,
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

int sgp30_init(void) {
    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    int status = -1;
    status = sgp30_soft_reset(); 
    vTaskDelay(pdMS_TO_TICKS(100));
    status = sgp30_send_cmd(INIT_AIR_QUALITY);//0 ok -1 fail
    vTaskDelay(pdMS_TO_TICKS(100));
    xSemaphoreGive(i2c_bus_mutex);       
    if(status == 0){
		sgp30_data.init_ok = 1;
		ESP_LOGI("SGP30", "Init OK");
	}
    else{
		ESP_LOGE("SGP30", "Init Fail");
		sgp30_data.init_ok = 0;
    } 
    
    return status == 0 ? 0 : -1;
}

static int sgp30_start(void) { return sgp30_send_cmd(MEASURE_AIR_QUALITY); }

static uint8_t CheckCrc8(uint8_t *const message, uint8_t initial_value) {
    uint8_t remainder = initial_value;
    for (uint8_t j = 0; j < 2; j++) {
        remainder ^= message[j];
        for (uint8_t i = 0; i < 8; i++) {
            remainder = (remainder & 0x80) ? (remainder << 1) ^ CRC8_POLYNOMIAL
                                           : (remainder << 1);
        }
    }
    return remainder;
}

/**
 * Вычисляет абсолютную влажность (г/м³) и отправляет её в SGP30.
 * @param temperature_c           — температура от SHT41, °C
 * @param relative_humidity_pct   — относительная влажность от SHT41, %RH (0–100)
 */
void sgp30_set_humidity_compensation(float temperature_c, float relative_humidity_pct) {
    // Формула Magnus: абсолютная влажность в г/м³
    // AH = 216.7 * (RH/100 * 6.112 * exp(17.62*T/(243.12+T))) / (273.15+T)
    float exp_arg    = (17.62f * temperature_c) / (243.12f + temperature_c);
    float abs_humidity = 216.7f *
        ((relative_humidity_pct / 100.0f) * 6.112f * expf(exp_arg))
        / (273.15f + temperature_c);

    if (abs_humidity < 0.0f)   abs_humidity = 0.0f;
    if (abs_humidity > 255.9f) abs_humidity = 255.9f;

    uint8_t ah_int  = (uint8_t)abs_humidity;
    uint8_t ah_frac = (uint8_t)((abs_humidity - ah_int) * 256.0f);

    uint8_t payload[2] = {ah_int, ah_frac};
    uint8_t crc        = CheckCrc8(payload, 0xFF);

    uint8_t buf[5] = {0x20, 0x61, ah_int, ah_frac, crc}; // SET_HUMIDITY

    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    i2c_master_write_to_device(I2C_MASTER_NUM, SGP30_ADDR_WRITE, buf, 5,
                               pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    xSemaphoreGive(i2c_bus_mutex);
}

int sgp30_read(void) {
#if SIMULATE_SGP30_VALUES
    static uint16_t temp = 400;
    temp += 10;
    if (temp > 9999) temp = 400;
    sgp30_data.co2  = temp;
    sgp30_data.tvoc = temp - 111;
#else
    static uint16_t try_to_read_cnt = 0;
    const  uint16_t number_of_try   = 20;

    uint8_t recv_buffer[6] = {0};

    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);

    int status = sgp30_start();
    if (status != 0) {
        //ESP_LOGE("SGP30", "Start failed");
        try_to_read_cnt++;
        //sgp30_data.state = SGP30_STATE_FAIL;
        xSemaphoreGive(i2c_bus_mutex);
        return -1;
    }

    vTaskDelay(pdMS_TO_TICKS(100));

    status = i2c_master_read_from_device(
        I2C_MASTER_NUM, SGP30_ADDR_READ, recv_buffer, 6,
        pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));

    xSemaphoreGive(i2c_bus_mutex);

    if (status != 0) {
        //ESP_LOGE("SGP30", "Read failed");
        try_to_read_cnt++;
        return -1;
    }

    if (CheckCrc8(&recv_buffer[0], 0xFF) != recv_buffer[2]) return -1;
    if (CheckCrc8(&recv_buffer[3], 0xFF) != recv_buffer[5]) return -1;

    sgp30_data.co2  = (uint16_t)(recv_buffer[0] << 8) | recv_buffer[1];
    sgp30_data.tvoc = (uint16_t)(recv_buffer[3] << 8) | recv_buffer[4];

    if (try_to_read_cnt > number_of_try) {
        try_to_read_cnt  = number_of_try + 1;
        sgp30_data.state = SGP30_STATE_FAIL;
        ESP_LOGE("SGP30", "Read failed");
    } else {
        try_to_read_cnt  = 0;
        sgp30_data.state = SGP30_STATE_OK;
    }
#endif
    return 0;
}

uint16_t sgp30_smooth_co2(uint16_t raw) {
    static const uint8_t  WINDOW_SIZE = 30;
    static const uint16_t CLAMP_MIN   = 400;
    static const uint16_t CLAMP_MAX   = 9999;

    static uint16_t buf[30];
    static uint8_t  head  = 0;
    static uint32_t sum   = 0;
    static uint8_t  count = 0;

    if (raw < CLAMP_MIN) raw = CLAMP_MIN;
    if (raw > CLAMP_MAX) raw = CLAMP_MAX;

    if (count == WINDOW_SIZE) { sum -= buf[head]; } else { count++; }
    buf[head] = raw;
    sum      += raw;
    head      = (head + 1) % WINDOW_SIZE;
    return (uint16_t)(sum / count);
}

uint16_t sgp30_smooth_tvoc(uint16_t raw) {
    static const uint8_t  WINDOW_SIZE = 30;
    static const uint16_t CLAMP_MIN   = 0;
    static const uint16_t CLAMP_MAX   = 9999;

    static uint16_t buf[30];
    static uint8_t  head  = 0;
    static uint32_t sum   = 0;
    static uint8_t  count = 0;

    if (raw < CLAMP_MIN) raw = CLAMP_MIN;
    if (raw > CLAMP_MAX) raw = CLAMP_MAX;

    if (count == WINDOW_SIZE) { sum -= buf[head]; } else { count++; }
    buf[head] = raw;
    sum      += raw;
    head      = (head + 1) % WINDOW_SIZE;
    return (uint16_t)(sum / count);
}

uint16_t get_co2_sgp30(void)  { return sgp30_smooth_co2(sgp30_data.co2);   }
uint16_t get_tvoc_sgp30(void) { return sgp30_smooth_tvoc(sgp30_data.tvoc); }

// ---------------------------------------------------------------------------
// SCD41  (Periodic Mode, ~5 s / Messung)
// ---------------------------------------------------------------------------
#if CONFIG_HAS_SCD41
scd41_data_t scd41_data = {0};

static esp_err_t scd41_send_cmd(uint16_t cmd) {
    uint8_t buf[2] = { (uint8_t)(cmd >> 8), (uint8_t)cmd };
    return i2c_master_write_to_device(I2C_MASTER_NUM, SCD41_ADDRESS, buf, 2,
                                      pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
}

void scd41_init(void) {
    // Warm-Reset-Schutz: nach ESP-Reset ohne Power-Cycle kann der Sensor
    // noch im Periodic Mode sein -> START wuerde NACK geben.
    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    scd41_send_cmd(SCD41_CMD_STOP_PERIODIC);
    xSemaphoreGive(i2c_bus_mutex);
    vTaskDelay(pdMS_TO_TICKS(500)); // stop dauert bis 500 ms

    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = scd41_send_cmd(SCD41_CMD_START_PERIODIC);
    xSemaphoreGive(i2c_bus_mutex);

    if (ret != ESP_OK) {
        ESP_LOGE("SCD41", "Init Fail: %s", esp_err_to_name(ret));
        scd41_data.init_ok = 0;
        scd41_data.life    = SCD41_STATE_FAIL;
        return;
    }
    scd41_data.init_ok = 1;
    scd41_data.life    = SCD41_STATE_OK;
    ESP_LOGI("SCD41", "Init OK (periodic, erste Messung ~5 s)");
}

void scd41_read(void) {
#if SIMULATE_SCD41_VALUES
    static uint16_t sim = 450;
    sim += 25;
    if (sim > 2400) sim = 450;
    scd41_data.co2  = sim;
    scd41_data.life = SCD41_STATE_OK;
#else
    if (!scd41_data.init_ok) return;

    uint8_t rx[9] = {0};

    // 1) Data ready?
    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    esp_err_t ret = scd41_send_cmd(SCD41_CMD_GET_DATA_READY);
    if (ret == ESP_OK) {
        ret = i2c_master_read_from_device(I2C_MASTER_NUM, SCD41_ADDRESS, rx, 3,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    }
    xSemaphoreGive(i2c_bus_mutex);
    if (ret != ESP_OK) { scd41_data.life = SCD41_STATE_FAIL; return; }

    if (CheckCrc8(&rx[0], 0xFF) != rx[2]) return;
    uint16_t status = ((uint16_t)rx[0] << 8) | rx[1];
    if ((status & 0x07FF) == 0) return; // noch nicht bereit -> warten

    // 2) Messwert lesen (CO2 = erstes Wort, T/RH ignorieren)
    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    ret = scd41_send_cmd(SCD41_CMD_READ_MEAS);
    if (ret == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(1)); // Kommando braucht ~1 ms
        ret = i2c_master_read_from_device(I2C_MASTER_NUM, SCD41_ADDRESS, rx, 9,
                                          pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    }
    xSemaphoreGive(i2c_bus_mutex);
    if (ret != ESP_OK) { scd41_data.life = SCD41_STATE_FAIL; return; }

    if (CheckCrc8(&rx[0], 0xFF) != rx[2]) return;

    scd41_data.co2  = ((uint16_t)rx[0] << 8) | rx[1];
    scd41_data.life = SCD41_STATE_OK;
#endif
}

uint16_t get_co2_scd41(void) {
    uint16_t v = scd41_data.co2;
    if (v < 400)  v = 400;    // physikalisches Minimum der Atmosphaere
    if (v > 9999) v = 9999;
    return v;
}
#endif // CONFIG_HAS_SCD41


void read_sensors(void) {
    sht41_read();
    sgp30_set_humidity_compensation(sht41_data.temperature, (float)sht41_data.humidity);
    sgp30_read();
    #if CONFIG_HAS_SCD41
    scd41_read();
#endif
}




