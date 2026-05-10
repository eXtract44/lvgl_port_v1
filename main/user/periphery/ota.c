#include "ota.h"

#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl_port.h"

static const char *TAG = "OTA";

typedef struct {
    char url[256];
    ota_progress_cb_t cb;
} ota_task_args_t;

// ---------------------------------------------------------------------------

static void ota_task(void *pvParameters)
{
    ota_task_args_t *args = (ota_task_args_t *)pvParameters;
lvgl_port_suspend();
    if (args->cb) args->cb(OTA_STATE_CHECKING, 0);
    ESP_LOGI(TAG, "Starte OTA von: %s", args->url);

esp_http_client_config_t http_cfg = {
    .url                        = args->url,
    .timeout_ms                 = 15000,
    .keep_alive_enable          = true,
    .crt_bundle_attach          = esp_crt_bundle_attach,
    .skip_cert_common_name_check = true,
};

    esp_https_ota_config_t ota_cfg = {
        .http_config            = &http_cfg,
        .http_client_init_cb    = NULL,
        .bulk_flash_erase       = false,
        .partial_http_download  = false,
    };

    esp_https_ota_handle_t ota_handle = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_begin fehlgeschlagen: %s", esp_err_to_name(err));
        if (args->cb) args->cb(OTA_STATE_FAILED, 0);
        goto cleanup;
    }

    if (args->cb) args->cb(OTA_STATE_DOWNLOADING, 0);

    // Получаем размер для расчёта прогресса
    int image_size = esp_https_ota_get_image_size(ota_handle);

    while (1) {
        err = esp_https_ota_perform(ota_handle);
        if (err == ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            if (args->cb && image_size > 0) {
                int written = esp_https_ota_get_image_len_read(ota_handle);
                int pct = (written * 100) / image_size;
                args->cb(OTA_STATE_DOWNLOADING, pct);
            }
            continue;
        }
        break;
    }

    if (!esp_https_ota_is_complete_data_received(ota_handle)) {
        ESP_LOGE(TAG, "Unvollständige Daten empfangen");
        if (args->cb) args->cb(OTA_STATE_FAILED, 0);
        goto cleanup;
    }

    err = esp_https_ota_finish(ota_handle);
    ota_handle = NULL;

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA erfolgreich — starte neu...");
        if (args->cb) args->cb(OTA_STATE_SUCCESS, 100);
        vTaskDelay(pdMS_TO_TICKS(2000)); // дать UI обновиться
        esp_restart();
    } else {
        ESP_LOGE(TAG, "esp_https_ota_finish fehlgeschlagen: %s", esp_err_to_name(err));
        if (args->cb) args->cb(OTA_STATE_FAILED, 0);
    }

cleanup:
    if (ota_handle) {
        esp_https_ota_abort(ota_handle);
    }
    lvgl_port_resume();
    free(args);
    vTaskDelete(NULL);
}

// ---------------------------------------------------------------------------

void ota_start(const char *url, ota_progress_cb_t cb)
{
    ota_task_args_t *args = malloc(sizeof(ota_task_args_t));
    if (!args) {
        ESP_LOGE(TAG, "malloc fehlgeschlagen");
        if (cb) cb(OTA_STATE_FAILED, 0);
        return;
    }

    strncpy(args->url, url, sizeof(args->url) - 1);
    args->url[sizeof(args->url) - 1] = '\0';
    args->cb = cb;

    xTaskCreate(ota_task, "ota_task", 8192, args, 5, NULL);
}

const char *ota_get_current_version(void)
{
    const esp_app_desc_t *desc = esp_app_get_description();
    return desc->version;
}
