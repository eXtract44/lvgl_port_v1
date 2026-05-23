#include "time_user.h"
#include <time.h>
#include "esp_sntp.h"

static const char *TAG = "NTP_TIME";
struct tm timeinfo_user;
// Инициализация SNTP (NTP клиент)
void user_initialize_sntp(void)
{
	if(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
     esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
     esp_sntp_setservername(0, "pool.ntp.org");
     esp_sntp_setservername(1, "time.google.com");
     esp_sntp_init();
} else {
    ESP_LOGI(TAG, "SNTP already initialized");
}
ESP_LOGI(TAG, "SNTP already initialized");
}

void user_sntp_stop(){
	esp_sntp_stop();
}

// Получение локального времени с учётом часового пояса
void get_local_time(struct tm *timeinfo)
{
    time_t now = 0;
    int retry = 0;
    const int retry_count = 10;

    // Ждём синхронизации SNTP
    while(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && retry < retry_count) {
       // ESP_LOGI(TAG, "Waiting for NTP sync...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        retry++;
    }

    time(&now);
    localtime_r(&now, timeinfo);
}

// Функция для LVGL таймера: обновляет время
void update_time_task(void *pvParameter)
{
    while(1) {
        
        get_local_time(&timeinfo_user);

//        ESP_LOGI(TAG, "Current time: year:%04d mon:%02d mday:%02d wday:%02d time:%02d:%02d:%02d",
//                 timeinfo_user.tm_year + 1900,
//                 timeinfo_user.tm_mon + 1,
//                 timeinfo_user.tm_mday,
//				 timeinfo_user.tm_wday,
//                 timeinfo_user.tm_hour,
//                 timeinfo_user.tm_min,
//                 timeinfo_user.tm_sec);

        vTaskDelay(5000 / portTICK_PERIOD_MS); // обновлять каждую минуту
    }
}

void start_ntp_time_task(void)
{
    //initialize_sntp();
    // Запускаем задачу обновления времени
    xTaskCreate(update_time_task, "update_time_task", 4096, NULL, 5, NULL);
}
// ---------------------------------------------------------------------------
// Time helpers (unchanged)
// ---------------------------------------------------------------------------
uint8_t get_time_mday(void) {
    static uint8_t v = 1;
#if SIMULATE_TIME_VALUES
    if (++v > 31) v = 1;
#else
    v = timeinfo_user.tm_mday;
#endif
    return v;
}

uint16_t get_time_year(void) {
    static uint16_t v = 0;
#if SIMULATE_TIME_VALUES
    if (++v > 2050) v = 2025;
#else
    v = (uint16_t)(timeinfo_user.tm_year + 1900);
#endif
    return v;
}

uint8_t get_time_month(void) {
    static uint8_t v = 1;
#if SIMULATE_TIME_VALUES
    if (++v > 12) v = 1;
#else
    v = (uint8_t)(timeinfo_user.tm_mon + 1);
#endif
    return v;
}

uint8_t get_time_hour(void) {
    static uint8_t v = 0;
#if SIMULATE_TIME_VALUES
    if (++v > 23) v = 0;
#else
    v = timeinfo_user.tm_hour;
#endif
    return v;
}

uint8_t get_time_minute(void) {
    static uint8_t v = 0;
#if SIMULATE_TIME_VALUES
    if (++v > 59) v = 0;
#else
    v = timeinfo_user.tm_min;
#endif
    return v;
}

uint8_t get_time_wday(void) {
    static uint8_t v = 0;
#if SIMULATE_TIME_VALUES
    if (++v > 6) v = 0;
#else
    v = timeinfo_user.tm_wday;
#endif
    return v;
}
