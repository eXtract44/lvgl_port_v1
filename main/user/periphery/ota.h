#ifndef INC_OTA_H_
#define INC_OTA_H_

#include "esp_err.h"
#include "stdbool.h"

extern volatile bool ota_in_progress;
// URL до .bin файла на GitHub Releases
// Формат: https://github.com/<user>/<repo>/releases/download/<tag>/<file>.bin
#define OTA_FIRMWARE_URL "https://github.com/eXtract44/lvgl_port_v1/raw/refs/heads/main/releases/download/lvgl_porting.bin"

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_CHECKING,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_SUCCESS,
    OTA_STATE_FAILED,
} ota_state_t;

typedef void (*ota_progress_cb_t)(ota_state_t state, int progress_pct);

/**
 * @brief Запустить OTA обновление в отдельной задаче
 * @param url       URL до .bin файла прошивки
 * @param cb        Callback для обновления UI (вызывается из задачи OTA)
 */
void ota_start(const char *url, ota_progress_cb_t cb);

/**
 * @brief Получить версию текущей прошивки
 * @return Строка вида "v1.0.0" или описание из esp_app_desc
 */
const char *ota_get_current_version(void);


#endif /* INC_OTA_ */