#include "Temp_Logger.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <errno.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "PCF85063.h"           /* datetime global */

/* --------------- configuration --------------- */
#define TEMP_LOG_DIR    "/sdcard/system/temp_logs"
#define TEMP_LOG_EXT    ".csv"
#define LOG_INTERVAL_MS 1000    /* write one sample per second */

static const char *TAG = "TempLog";

/* --------------- state ----------------------- */
static FILE             *s_file     = NULL;
static SemaphoreHandle_t s_mutex    = NULL;
static bool              s_active   = false;
static bool              s_dirty    = false;
static TickType_t        s_last_write_tick = 0;

/* --------------- helpers --------------------- */

static void ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        ESP_LOGI(TAG, "Creating directory: %s", path);
        mkdir(path, 0775);
    }
}

/** Open a new CSV file named by current date and time. */
static esp_err_t open_log_file(void)
{
    if (s_file) {
        fflush(s_file);
        fsync(fileno(s_file));
        fclose(s_file);
        s_file = NULL;
    }

    /* Make sure parent dirs exist */
    ensure_dir("/sdcard/system");
    ensure_dir(TEMP_LOG_DIR);

    /* Use date+time from RTC; fall back if date is clearly invalid */
    uint16_t y = datetime.year;
    uint8_t  m = datetime.month;
    uint8_t  d = datetime.day;

    char path[128];
    if (y >= 2020 && y <= 2099 && m >= 1 && m <= 12 && d >= 1 && d <= 31) {
        snprintf(path, sizeof(path), "%s/%04d-%02d-%02d_%02d-%02d-%02d%s",
                 TEMP_LOG_DIR, y, m, d,
                 datetime.hour, datetime.minute, datetime.second,
                 TEMP_LOG_EXT);
    } else {
        ESP_LOGW(TAG, "RTC date invalid (%04d-%02d-%02d), using fallback name", y, m, d);
        snprintf(path, sizeof(path), "%s/datalog%s", TEMP_LOG_DIR, TEMP_LOG_EXT);
    }

    ESP_LOGI(TAG, "Opening log file: %s", path);

    s_file = fopen(path, "w");
    if (!s_file) {
        ESP_LOGE(TAG, "Failed to open %s (errno %d: %s)", path, errno, strerror(errno));
        return ESP_FAIL;
    }

    fprintf(s_file, "Date,Time,Temp_C\n");

    ESP_LOGI(TAG, "Logging to %s", path);
    return ESP_OK;
}

/* --------------- public API ------------------ */

esp_err_t Temp_Logger_Init(void)
{
    if (!s_mutex) {
        s_mutex = xSemaphoreCreateMutex();
        if (!s_mutex) {
            ESP_LOGE(TAG, "Failed to create mutex");
            return ESP_FAIL;
        }
    }
    ESP_LOGI(TAG, "Temp Logger initialised");
    return ESP_OK;
}

void Temp_Logger_SetEnabled(bool enable)
{
    if (!s_mutex) return;

    xSemaphoreTake(s_mutex, portMAX_DELAY);

    if (enable && !s_active) {
        if (open_log_file() == ESP_OK) {
            s_active = true;
            s_last_write_tick = xTaskGetTickCount();
            ESP_LOGI(TAG, "Logging ENABLED");
        }
    } else if (!enable && s_active) {
        if (s_file) {
            fflush(s_file);
            fsync(fileno(s_file));
            fclose(s_file);
            s_file = NULL;
        }
        s_active = false;
        ESP_LOGI(TAG, "Logging DISABLED");
    }

    xSemaphoreGive(s_mutex);
}

bool Temp_Logger_IsActive(void)
{
    return s_active;
}

void Temp_Logger_Feed(float temp_celsius)
{
    if (!s_active || !s_mutex) return;

    /* Rate-limit to one write per second */
    TickType_t now = xTaskGetTickCount();
    if ((now - s_last_write_tick) < pdMS_TO_TICKS(LOG_INTERVAL_MS)) {
        return;
    }

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(50)) != pdTRUE) {
        return;
    }

    if (s_file) {
        fprintf(s_file, "%04d-%02d-%02d,%02d:%02d:%02d,%.1f\n",
                datetime.year, datetime.month, datetime.day,
                datetime.hour, datetime.minute, datetime.second,
                temp_celsius);
        s_dirty = true;
        s_last_write_tick = now;
    }

    xSemaphoreGive(s_mutex);
}

void Temp_Logger_Flush(void)
{
    if (!s_mutex) return;

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (s_file && s_dirty) {
            fflush(s_file);
            fsync(fileno(s_file));
            s_dirty = false;
        }
        xSemaphoreGive(s_mutex);
    }
}

void Temp_Logger_Deinit(void)
{
    Temp_Logger_SetEnabled(false);

    if (s_mutex) {
        vSemaphoreDelete(s_mutex);
        s_mutex = NULL;
    }
}
