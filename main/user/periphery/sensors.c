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
#include <string.h>
#include <time.h>
#include "nvs.h"
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
static esp_err_t sgp30_send_cmd(uint16_t cmd) {
    uint8_t cmd_buffer[2];
    cmd_buffer[0] = cmd >> 8;
    cmd_buffer[1] = (uint8_t)cmd;
    return i2c_master_write_to_device(
        I2C_MASTER_NUM, SGP30_ADDR_WRITE, cmd_buffer, 2,
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static uint8_t CheckCrc8(uint8_t *message, uint8_t initial_value);
static void sgp30_baseline_tick(void);
static void sgp30_try_restore_baseline(void);
typedef struct {
    uint16_t values[30];
    uint32_t sum;
    uint8_t head;
    uint8_t count;
} sgp30_filter_t;
static sgp30_filter_t sgp30_co2_filter;
static sgp30_filter_t sgp30_tvoc_filter;
static uint16_t sgp30_filter(sgp30_filter_t *filter, uint16_t raw);

#define SGP30_WARMUP_SEC 15U
#define SGP30_LEARNING_SEC (12U * 3600U)
#define SGP30_BASELINE_MAX_AGE_SEC (7U * 24U * 3600U)
#define SGP30_TIME_VALID_MIN 1704067200LL // 2024-01-01, before SNTP time is unknown

typedef struct {
    uint64_t serial;
    uint64_t timestamp;
    uint16_t eco2;
    uint16_t tvoc;
    uint32_t version;
} sgp30_baseline_record_t;

static uint64_t sgp30_serial;
static uint32_t sgp30_learning_samples;
static int64_t sgp30_baseline_next_save_us;

// Release the shared bus while SGP30 converts, so GT911 can still be read.
static esp_err_t sgp30_read_words(uint16_t cmd, uint8_t *rx, size_t len) {
    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    esp_err_t err = sgp30_send_cmd(cmd);
    xSemaphoreGive(i2c_bus_mutex);
    if (err != ESP_OK) return err;
    vTaskDelay(pdMS_TO_TICKS(20));
    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    err = i2c_master_read_from_device(I2C_MASTER_NUM, SGP30_ADDR, rx, len,
                                    pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    xSemaphoreGive(i2c_bus_mutex);
    if (err != ESP_OK) return err;
    for (size_t i = 0; i < len; i += 3) {
        if (CheckCrc8(&rx[i], 0xFF) != rx[i + 2]) return ESP_ERR_INVALID_CRC;
    }
    return ESP_OK;
}

int sgp30_init(void) {
    memset(&sgp30_data, 0, sizeof(sgp30_data));
    memset(&sgp30_co2_filter, 0, sizeof(sgp30_co2_filter));
    memset(&sgp30_tvoc_filter, 0, sizeof(sgp30_tvoc_filter));
    sgp30_learning_samples = 0;
    sgp30_serial = 0;
    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    // IAQ_INIT is the addressed SGP30 initialization command, not a bus reset.
    esp_err_t status = sgp30_send_cmd(INIT_AIR_QUALITY);
    xSemaphoreGive(i2c_bus_mutex);
    vTaskDelay(pdMS_TO_TICKS(20));
    if(status == 0){
		sgp30_data.init_ok = 1;
		sgp30_data.started_us = esp_timer_get_time();
        sgp30_baseline_next_save_us = sgp30_data.started_us + 3600LL * 1000000;
        uint8_t serial_rx[9];
        if (sgp30_read_words(0x3682, serial_rx, sizeof(serial_rx)) == ESP_OK) {
            for (int i = 0; i < 9; i += 3) {
                sgp30_serial = (sgp30_serial << 16) |
                    ((uint16_t)serial_rx[i] << 8) | serial_rx[i + 1];
            }
        }
        sgp30_try_restore_baseline();
		ESP_LOGI("SGP30", "Init OK");
	}
    else{
		ESP_LOGE("SGP30", "Init Fail");
		sgp30_data.init_ok = 0;
    } 
    
    return status == 0 ? 0 : -1;
}

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

static void sgp30_try_restore_baseline(void) {
#if !SIMULATE_SGP30_VALUES
    if (!sgp30_serial || sgp30_data.baseline_ready) return;
    time_t now = time(NULL);
    if (now < SGP30_TIME_VALID_MIN) return; // Retry after SNTP, without blocking measurements.

    nvs_handle_t nvs;
    if (nvs_open("sgp30", NVS_READONLY, &nvs) != ESP_OK) return;
    sgp30_baseline_record_t record = {0};
    size_t len = sizeof(record);
    esp_err_t err = nvs_get_blob(nvs, "baseline", &record, &len);
    nvs_close(nvs);
    if (err != ESP_OK || len != sizeof(record) || record.version != 1 ||
        record.serial != sgp30_serial || !(record.eco2 | record.tvoc) ||
        record.timestamp < SGP30_TIME_VALID_MIN || record.timestamp > (uint64_t)now ||
        (uint64_t)now - record.timestamp >= SGP30_BASELINE_MAX_AGE_SEC) return;

    // GET returns eCO2, TVOC; SET requires the opposite word order.
    uint8_t tx[8] = {0x20, 0x1E, record.tvoc >> 8, record.tvoc, 0,
                    record.eco2 >> 8, record.eco2, 0};
    tx[4] = CheckCrc8(&tx[2], 0xFF);
    tx[7] = CheckCrc8(&tx[5], 0xFF);
    xSemaphoreTake(i2c_bus_mutex, portMAX_DELAY);
    err = i2c_master_write_to_device(I2C_MASTER_NUM, SGP30_ADDR, tx, sizeof(tx),
                                   pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_MS));
    xSemaphoreGive(i2c_bus_mutex);
    vTaskDelay(pdMS_TO_TICKS(20));
    if (err == ESP_OK) {
        sgp30_data.baseline_restored = true;
        sgp30_data.baseline_ready = true;
        sgp30_data.baseline_saved = true;
        ESP_LOGI("SGP30", "Baseline restored for this sensor");
    }
#endif
}

static void sgp30_baseline_tick(void) {
#if !SIMULATE_SGP30_VALUES
    if (!sgp30_data.baseline_ready) {
        if (++sgp30_learning_samples >= SGP30_LEARNING_SEC)
            sgp30_data.baseline_ready = true;
    }
    int64_t now_us = esp_timer_get_time();
    if (!sgp30_data.baseline_ready || !sgp30_serial ||
        now_us < sgp30_baseline_next_save_us) return;
    sgp30_baseline_next_save_us = now_us + 60LL * 1000000; // Retry failures once per minute.
    time_t now = time(NULL);
    if (now < SGP30_TIME_VALID_MIN) return; // No trustworthy age without a clock.

    uint8_t rx[6];
    if (sgp30_read_words(0x2015, rx, sizeof(rx)) != ESP_OK) return;
    sgp30_baseline_record_t record = {
        .serial = sgp30_serial, .timestamp = (uint64_t)now, .version = 1,
        .eco2 = ((uint16_t)rx[0] << 8) | rx[1],
        .tvoc = ((uint16_t)rx[3] << 8) | rx[4],
    };
    if (!(record.eco2 | record.tvoc)) return;
    nvs_handle_t nvs;
    if (nvs_open("sgp30", NVS_READWRITE, &nvs) != ESP_OK) return;
    esp_err_t err = nvs_set_blob(nvs, "baseline", &record, sizeof(record));
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    if (err == ESP_OK) {
        sgp30_data.baseline_saved = true;
        sgp30_baseline_next_save_us = now_us + 3600LL * 1000000;
        ESP_LOGI("SGP30", "Baseline saved");
    }
#endif
}

uint32_t sgp30_warmup_remaining_sec(void) {
    if (!sgp30_data.init_ok) return SGP30_WARMUP_SEC;
    int64_t elapsed = (esp_timer_get_time() - sgp30_data.started_us) / 1000000;
    return elapsed >= SGP30_WARMUP_SEC ? 0 : SGP30_WARMUP_SEC - (uint32_t)elapsed;
}

bool sgp30_values_ready(void) {
    return sgp30_data.init_ok && sgp30_data.state == SGP30_STATE_OK &&
           sgp30_co2_filter.count > 0 &&
           sgp30_warmup_remaining_sec() == 0 &&
           esp_timer_get_time() - sgp30_data.last_sample_us < 3000000;
}

bool sgp30_is_learning(void) {
    return sgp30_data.init_ok && !sgp30_data.baseline_ready;
}

uint32_t sgp30_learning_remaining_sec(void) {
    return sgp30_data.baseline_ready ? 0 : SGP30_LEARNING_SEC - sgp30_learning_samples;
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
    vTaskDelay(pdMS_TO_TICKS(20)); // SET_HUMIDITY must finish before MEASURE_IAQ.
}

int sgp30_read(void) {
    if (!sgp30_data.init_ok) return -1;
    sgp30_try_restore_baseline();
#if SIMULATE_SGP30_VALUES
    static uint16_t temp = 400;
    temp += 10;
    if (temp > 9999) temp = 400;
    sgp30_data.co2  = temp;
    sgp30_data.tvoc = temp - 111;
    sgp30_data.baseline_ready = true;
#else
    uint8_t recv_buffer[6] = {0};
    if (sgp30_read_words(MEASURE_AIR_QUALITY, recv_buffer, sizeof(recv_buffer)) != ESP_OK) {
        sgp30_data.state = SGP30_STATE_FAIL;
        return -1;
    }

    sgp30_data.co2  = (uint16_t)(recv_buffer[0] << 8) | recv_buffer[1];
    sgp30_data.tvoc = (uint16_t)(recv_buffer[3] << 8) | recv_buffer[4];

#endif
    sgp30_data.state = SGP30_STATE_OK;
    sgp30_data.last_sample_us = esp_timer_get_time();
    if (sgp30_warmup_remaining_sec() == 0) {
        // Filter once per new sample. Getters must not change the averaging window.
        sgp30_data.co2_filtered = sgp30_filter(&sgp30_co2_filter, sgp30_data.co2);
        sgp30_data.tvoc_filtered = sgp30_filter(&sgp30_tvoc_filter, sgp30_data.tvoc);
    }
    if (sgp30_warmup_remaining_sec() == 0) sgp30_baseline_tick();
    return 0;
}

static uint16_t sgp30_filter(sgp30_filter_t *filter, uint16_t raw) {
    if (filter->count == 30) filter->sum -= filter->values[filter->head];
    else filter->count++;
    filter->values[filter->head] = raw;
    filter->sum += raw;
    filter->head = (filter->head + 1) % 30;
    return (uint16_t)(filter->sum / filter->count);
}

uint16_t get_co2_sgp30(void)  { return sgp30_data.co2_filtered; }
uint16_t get_tvoc_sgp30(void) { return sgp30_data.tvoc_filtered; }

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
    if (sht41_data.life == SHT41_STATE_OK && sgp30_data.init_ok)
        sgp30_set_humidity_compensation(sht41_data.temperature, (float)sht41_data.humidity);
    sgp30_read();
    #if CONFIG_HAS_SCD41
    scd41_read();
#endif
}




