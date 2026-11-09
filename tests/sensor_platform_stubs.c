// Minimal Windows host platform for compiling the actual sensor code without IDF.
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef unsigned long long size_t;
typedef long long time_t;
typedef _Bool bool;
typedef int esp_err_t;
typedef int nvs_handle_t;
#define true 1
#define false 0
#define NULL ((void *)0)
#define ESP_OK 0
#define ESP_ERR_INVALID_CRC 1
#define NVS_READONLY 0
#define NVS_READWRITE 1
#define I2C_MASTER_NUM 0
#define I2C_MASTER_TIMEOUT_MS 1000
#define portTICK_PERIOD_MS 1
#define portMAX_DELAY 0xffffffff
#define pdMS_TO_TICKS(x) (x)
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGE(...) ((void)0)
#define xSemaphoreTake(...) ((void)0)
#define xSemaphoreGive(...) ((void)0)
int i2c_bus_mutex;
int _fltused;
static int64_t fake_us = 1000000;
static time_t fake_epoch = 1800000000;
static uint16_t fake_command;
static uint16_t fake_eco2 = 800, fake_tvoc = 20;
static uint8_t fake_nvs[64], fake_set_baseline[8];
static size_t fake_nvs_len;
static int fake_nvs_commits, fake_io_error, fake_crc_error;
static uint16_t fake_serial = 0x1234;
static void *memset(void *p, int v, size_t n) {
    uint8_t *b = p;
    while (n--) *b++ = (uint8_t)v;
    return p;
}
static float expf(float x) { (void)x; return 1.0f; } // Humidity math is outside these tests.
static int64_t esp_timer_get_time(void) { return fake_us; }
static time_t time(time_t *p) { if (p) *p = fake_epoch; return fake_epoch; }
static void vTaskDelay(uint32_t ms) { fake_us += (int64_t)ms * 1000; }
static uint8_t fake_crc(uint16_t word) {
    uint8_t crc = 0xff;
    for (int b = 1; b >= 0; b--) {
        crc ^= (uint8_t)(word >> (8 * b));
        for (int i = 0; i < 8; i++) crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : crc << 1;
    }
    return crc;
}
static int i2c_master_write_to_device(int bus, int addr, const uint8_t *tx, size_t len, uint32_t wait) {
    (void)bus; (void)addr; (void)wait;
    if (fake_io_error) return 1;
    fake_command = len >= 2 ? ((uint16_t)tx[0] << 8) | tx[1] : tx[0];
    if (fake_command == 0x201e && len == 8)
        for (int i = 0; i < 8; i++) fake_set_baseline[i] = tx[i];
    return 0;
}
static int i2c_master_read_from_device(int bus, int addr, uint8_t *rx, size_t len, uint32_t wait) {
    (void)bus; (void)addr; (void)wait;
    if (fake_io_error) return 1;
    for (size_t i = 0; i < len; i += 3) {
        uint16_t word = fake_command == 0x3682 ? fake_serial :
                        fake_command == 0x2015 ? (i == 0 ? 0xabcd : 0x4567) :
                        (i == 0 ? fake_eco2 : fake_tvoc);
        rx[i] = word >> 8; rx[i + 1] = word; rx[i + 2] = fake_crc(word);
    }
    if (fake_crc_error) rx[2] ^= 1;
    return 0;
}
static int nvs_open(const char *name, int mode, nvs_handle_t *h) {
    (void)name; (void)mode; *h = 1; return 0;
}
static void nvs_close(nvs_handle_t h) { (void)h; }
static int nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *len) {
    (void)h; (void)key;
    if (!fake_nvs_len || *len < fake_nvs_len) return 1;
    for (size_t i = 0; i < fake_nvs_len; i++) ((uint8_t *)out)[i] = fake_nvs[i];
    *len = fake_nvs_len;
    return 0;
}
static int nvs_set_blob(nvs_handle_t h, const char *key, const void *data, size_t len) {
    (void)h; (void)key;
    for (size_t i = 0; i < len; i++) fake_nvs[i] = ((const uint8_t *)data)[i];
    fake_nvs_len = len;
    return 0;
}
static int nvs_commit(nvs_handle_t h) { (void)h; fake_nvs_commits++; return 0; }
