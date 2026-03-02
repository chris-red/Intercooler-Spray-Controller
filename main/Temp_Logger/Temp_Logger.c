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

/* Track which day the current file belongs to so we can roll at midnight */
static int s_file_day  = -1;
static int s_file_mon  = -1;
static int s_file_year = -1;

/* --------------- helpers --------------------- */

static void ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        mkdir(path, 0775);
    }
}

/** Open (or reopen) a CSV file named by today's date. */
static esp_err_t open_log_file(void)
{
    if (s_file) {
        fflush(s_file);
        fsync(fileno(s_file));
        fclose(s_file);
        s_file = NULL;
    }

    ensure_dir(TEMP_LOG_DIR);

    /* Filename: YYYY-MM-DD.csv */
    char path[128];
    snprintf(path, sizeof(path), "%s/%04d-%02d-%02d%s",
             TEMP_LOG_DIR,
             datetime.year, datetime.month, datetime.day,
             TEMP_LOG_EXT);

    /* Check if file already exists (append) or is new (write header) */
    struct stat st;
    bool exists = (stat(path, &st) == 0);

    s_file = fopen(path, "a");
    if (!s_file) {
        ESP_LOGE(TAG, "Failed to open %s (errno %d)", path, errno);
        return ESP_FAIL;
    }

    if (!exists) {
        fprintf(s_file, "Date,Time,Temp_C\n");
    }

    s_file_day  = datetime.day;
    s_file_mon  = datetime.month;
    s_file_year = datetime.year;

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

    /* Roll to a new file at midnight */
    if (datetime.day != s_file_day ||
        datetime.month != s_file_mon ||
        datetime.year  != s_file_year) {
        open_log_file();
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
    if (!s_mutex || !s_file || !s_dirty) return;

    if (xSemaphoreTake(s_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        fflush(s_file);
        fsync(fileno(s_file));
        s_dirty = false;
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
