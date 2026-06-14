#include "ota.h"

#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_ota_ops.h"
#include "esp_app_desc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl_port.h"
#include "cJSON.h"
#include "esp_http_client.h"

#define VERSION_BUF_SIZE 64
static const char *TAG = "OTA";

typedef struct {
    ota_version_check_cb_t cb;
} ota_check_args_t;

typedef struct {
    char url[256];
    ota_progress_cb_t cb;
} ota_task_args_t;

// ---------------------------------------------------------------------------

#define OTA_MAX_RETRY   5
#define OTA_RETRY_DELAY 3000   // ms

static esp_err_t ota_attempt(ota_task_args_t *args)
{
    esp_http_client_config_t http_cfg = {
        .url               = args->url,
        .timeout_ms        = 20000,
        .keep_alive_enable = true,
        .crt_bundle_attach = esp_crt_bundle_attach,
        // skip_cert_common_name_check убран — проверка CN нужна
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config            = &http_cfg,
        .partial_http_download  = true,
        .max_http_request_size  = 64 * 1024,   // чанк Range-запроса
    };

    esp_https_ota_handle_t h = NULL;
    esp_err_t err = esp_https_ota_begin(&ota_cfg, &h);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ota_begin: %s", esp_err_to_name(err));
        return err;
    }

    if (args->cb) args->cb(OTA_STATE_DOWNLOADING, 0);
    int image_size = esp_https_ota_get_image_size(h);
    int last_pct = -1;

    while (1) {
        err = esp_https_ota_perform(h);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) break;
        if (args->cb && image_size > 0) {
            int pct = (esp_https_ota_get_image_len_read(h) * 100) / image_size;
            if (pct != last_pct) {            // не дёргать UI на каждой итерации
                last_pct = pct;
                args->cb(OTA_STATE_DOWNLOADING, pct);
            }
        }
    }

    if (err != ESP_OK) {                       // обрыв посреди загрузки
        ESP_LOGE(TAG, "ota_perform: %s", esp_err_to_name(err));
        esp_https_ota_abort(h);
        return err;
    }
    if (!esp_https_ota_is_complete_data_received(h)) {
        ESP_LOGE(TAG, "Unvollstaendige Daten");
        esp_https_ota_abort(h);
        return ESP_FAIL;
    }
    return esp_https_ota_finish(h);            // ESP_OK при успехе
}

static void ota_task(void *pvParameters)
{
    ota_task_args_t *args = (ota_task_args_t *)pvParameters;
    if (args->cb) args->cb(OTA_STATE_CHECKING, 0);
    ESP_LOGI(TAG, "Starte OTA von: %s", args->url);

    esp_err_t err = ESP_FAIL;
    for (int i = 1; i <= OTA_MAX_RETRY; i++) {
        ESP_LOGI(TAG, "OTA Versuch %d/%d", i, OTA_MAX_RETRY);
        err = ota_attempt(args);
        if (err == ESP_OK) break;
        ESP_LOGW(TAG, "Versuch %d fehlgeschlagen: %s", i, esp_err_to_name(err));
        if (i < OTA_MAX_RETRY) vTaskDelay(pdMS_TO_TICKS(OTA_RETRY_DELAY));
    }

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "OTA erfolgreich — Neustart...");
        if (args->cb) args->cb(OTA_STATE_SUCCESS, 100);
        vTaskDelay(pdMS_TO_TICKS(2000));
        esp_restart();
    } else {
        ESP_LOGE(TAG, "OTA nach %d Versuchen fehlgeschlagen", OTA_MAX_RETRY);
        if (args->cb) args->cb(OTA_STATE_FAILED, 0);
    }

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

static void ota_check_version_task(void *pvParameters)
{
    ota_check_args_t *args = (ota_check_args_t *)pvParameters;

    char buf[VERSION_BUF_SIZE] = {0};
    int buf_len = 0;

    esp_http_client_config_t cfg = {
        .url                        = OTA_VERSION_URL,
        .timeout_ms                 = 10000,
        .crt_bundle_attach          = esp_crt_bundle_attach,
        .skip_cert_common_name_check = true,
        .keep_alive_enable           = true,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        ESP_LOGE(TAG, "http_client_init failed");
        if (args->cb) args->cb(OTA_VERSION_CHECK_FAILED, NULL);
        goto cleanup;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "http_client_open failed: %s", esp_err_to_name(err));
        if (args->cb) args->cb(OTA_VERSION_CHECK_FAILED, NULL);
        goto cleanup_client;
    }

    esp_http_client_fetch_headers(client);
    //
	int status_code = esp_http_client_get_status_code(client);
	if (status_code != 200) {
	    ESP_LOGE(TAG, "version.json HTTP %d", status_code);
	    if (args->cb) args->cb(OTA_VERSION_CHECK_FAILED, NULL);
	    goto cleanup_client;
	}

	int r;
	while (buf_len < (int)sizeof(buf) - 1 &&
	       (r = esp_http_client_read(client, buf + buf_len, sizeof(buf) - 1 - buf_len)) > 0) {
	    buf_len += r;
	}
	if (buf_len <= 0) { /* как было */ }
	buf[buf_len] = '\0';
    ESP_LOGI(TAG, "version.json: %s", buf);

    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        ESP_LOGE(TAG, "cJSON_Parse failed");
        if (args->cb) args->cb(OTA_VERSION_CHECK_FAILED, NULL);
        goto cleanup_client;
    }

    cJSON *ver_item = cJSON_GetObjectItem(json, "version");
    if (!cJSON_IsString(ver_item)) {
        ESP_LOGE(TAG, "no 'version' field in JSON");
        if (args->cb) args->cb(OTA_VERSION_CHECK_FAILED, NULL);
        cJSON_Delete(json);
        goto cleanup_client;
    }

    const char *remote = ver_item->valuestring;
    const char *local  = ota_get_current_version();
    ESP_LOGI(TAG, "local: '%s', remote: '%s'", local, remote);

    ota_version_status_t status = (strcmp(local, remote) == 0)
        ? OTA_VERSION_UP_TO_DATE
        : OTA_VERSION_UPDATE_AVAILABLE;

    if (args->cb) args->cb(status, remote);

    cJSON_Delete(json);

cleanup_client:
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

cleanup:
    free(args);
    vTaskDelete(NULL);
}

void ota_check_version(ota_version_check_cb_t cb)
{
    ota_check_args_t *args = malloc(sizeof(ota_check_args_t));
    if (!args) {
        ESP_LOGE(TAG, "malloc failed");
        if (cb) cb(OTA_VERSION_CHECK_FAILED, NULL);
        return;
    }
    args->cb = cb;
    xTaskCreate(ota_check_version_task, "ota_check", 4096, args, 5, NULL);
}
