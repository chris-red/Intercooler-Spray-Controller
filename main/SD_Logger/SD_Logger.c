#include "SD_Logger.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <errno.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

/* --------------- configuration --------------- */
#define SD_LOG_DIR      "/sdcard/system/logs"
#define SD_LOG_PREFIX   "L"
#define SD_LOG_EXT      ".txt"
#define SD_LOG_MAX_KEEP 5          /* number of log files to retain */

static const char *TAG = "SD_Logger";

/* --------------- state ----------------------- */
static FILE              *s_log_file    = NULL;
static vprintf_like_t     s_orig_vprintf = NULL;   /* original log handler */
static SemaphoreHandle_t  s_log_mutex   = NULL;
static TimerHandle_t      s_sync_timer  = NULL;
static bool               s_dirty       = false;   /* true if writes since last sync */

/* --------------- helpers --------------------- */

/** Create a directory and all parents (like mkdir -p). */
static void mkdirs(const char *path)
{
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0775);
            *p = '/';
        }
    }
    mkdir(tmp, 0775);
}

/** Comparison for qsort – sort filenames ascending (oldest first). */
static int cmp_str(const void *a, const void *b)
{
    return strcmp(*(const char **)a, *(const char **)b);
}

/**
 * Scan the log directory, build a sorted list of existing log files,
 * delete the oldest ones so that at most (SD_LOG_MAX_KEEP - 1) remain
 * (leaving room for the new file we are about to create).
 */
static void rotate_logs(void)
{
    DIR *dir = opendir(SD_LOG_DIR);
    if (!dir) return;

    /* Collect matching filenames */
    char *names[64];
    int   count = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && count < 64) {
        if (strncmp(ent->d_name, SD_LOG_PREFIX, strlen(SD_LOG_PREFIX)) == 0) {
            names[count] = strdup(ent->d_name);
            if (names[count]) count++;
        }
    }
    closedir(dir);

    if (count == 0) return;

    /* Sort ascending so oldest (smallest number) is first */
    qsort(names, count, sizeof(char *), cmp_str);

    /* Delete oldest files, keep at most (MAX_KEEP - 1) */
    int to_delete = count - (SD_LOG_MAX_KEEP - 1);
    for (int i = 0; i < count; i++) {
        if (i < to_delete) {
            char full[300];
            snprintf(full, sizeof(full), "%s/%s", SD_LOG_DIR, names[i]);
            ESP_LOGI(TAG, "Deleting old log: %s", names[i]);
            unlink(full);
        }
        free(names[i]);
    }
}

/**
 * Find the next log sequence number by scanning existing files.
 */
static int next_sequence(void)
{
    DIR *dir = opendir(SD_LOG_DIR);
    if (!dir) return 1;

    int max_seq = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL) {
        if (strncmp(ent->d_name, SD_LOG_PREFIX, strlen(SD_LOG_PREFIX)) == 0) {
            int seq = atoi(ent->d_name + strlen(SD_LOG_PREFIX));
            if (seq > max_seq) max_seq = seq;
        }
    }
    closedir(dir);
    return max_seq + 1;
}

/* Helper: flush C buffer AND sync FAT to SD card */
static void sd_flush_sync(void)
{
    if (s_log_file) {
        fflush(s_log_file);
        fsync(fileno(s_log_file));
        s_dirty = false;
    }
}

/* Timer callback — runs every sync_interval_ms from FreeRTOS timer task */
static void sync_timer_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    if (s_log_file && s_log_mutex && s_dirty) {
        if (xSemaphoreTake(s_log_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            sd_flush_sync();
            xSemaphoreGive(s_log_mutex);
        }
    }
}

/* --------------- log sink -------------------- */

/**
 * Custom vprintf-like function that writes to both the original UART output
 * and the SD card log file.  This captures all ESP_LOGx() output.
 */
static int sd_log_vprintf(const char *fmt, va_list args)
{
    /* Always write to UART first (so we never lose output) */
    va_list args_copy;
    va_copy(args_copy, args);
    int ret = s_orig_vprintf(fmt, args_copy);
    va_end(args_copy);

    /* Format into a buffer and write to SD file */
    if (s_log_file && s_log_mutex) {
        if (xSemaphoreTake(s_log_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
            char buf[512];
            va_copy(args_copy, args);
            int n = vsnprintf(buf, sizeof(buf), fmt, args_copy);
            va_end(args_copy);
            if (n > 0) {
                size_t len = (size_t)(n < (int)sizeof(buf) ? n : (int)sizeof(buf) - 1);
                fwrite(buf, 1, len, s_log_file);
                s_dirty = true;
            }
            xSemaphoreGive(s_log_mutex);
        }
    }

    return ret;
}

/* --------------- public API ------------------ */

esp_err_t SD_Logger_Init(uint32_t sync_interval_ms)
{
    if (sync_interval_ms == 0) {
        sync_interval_ms = SD_LOGGER_DEFAULT_SYNC_MS;
    }
    /* Create directory tree */
    ESP_LOGI(TAG, "Creating log directory: %s", SD_LOG_DIR);
    mkdirs(SD_LOG_DIR);

    /* Verify directory exists */
    struct stat st;
    if (stat(SD_LOG_DIR, &st) != 0) {
        ESP_LOGE(TAG, "Log directory does not exist after mkdirs (errno %d)", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Log directory OK");

    /* Rotate old logs */
    rotate_logs();

    /* Determine new filename */
    int seq = next_sequence();
    char path[300];
    snprintf(path, sizeof(path), "%s/%s%05d%s", SD_LOG_DIR, SD_LOG_PREFIX, seq, SD_LOG_EXT);

    /* Open file for writing */
    ESP_LOGI(TAG, "Opening log file: %s", path);
    s_log_file = fopen(path, "w");
    if (!s_log_file) {
        ESP_LOGE(TAG, "Failed to open log file: %s (errno %d)", path, errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Log file opened successfully");

    /* Write a header */
    int hdr = fprintf(s_log_file, "=== Log #%d started ===\n", seq);
    sd_flush_sync();
    ESP_LOGI(TAG, "Header written (%d bytes), flushed+synced", hdr);

    /* Create mutex */
    s_log_mutex = xSemaphoreCreateMutex();
    if (!s_log_mutex) {
        fclose(s_log_file);
        s_log_file = NULL;
        ESP_LOGE(TAG, "Failed to create log mutex");
        return ESP_FAIL;
    }

    /* Redirect ESP log output through our vprintf wrapper */
    s_orig_vprintf = esp_log_set_vprintf(sd_log_vprintf);

    /* Start periodic sync timer */
    s_sync_timer = xTimerCreate("sd_sync", pdMS_TO_TICKS(sync_interval_ms),
                                pdTRUE, NULL, sync_timer_cb);
    if (s_sync_timer) {
        xTimerStart(s_sync_timer, 0);
    }

    ESP_LOGI(TAG, "SD card logging started: %s (sync every %lu ms)", path, (unsigned long)sync_interval_ms);
    return ESP_OK;
}

void SD_Logger_Flush(void)
{
    if (s_log_file && s_log_mutex) {
        if (xSemaphoreTake(s_log_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            sd_flush_sync();
            xSemaphoreGive(s_log_mutex);
        }
    }
}

void SD_Logger_Deinit(void)
{
    /* Stop sync timer */
    if (s_sync_timer) {
        xTimerStop(s_sync_timer, portMAX_DELAY);
        xTimerDelete(s_sync_timer, portMAX_DELAY);
        s_sync_timer = NULL;
    }
    if (s_orig_vprintf) {
        esp_log_set_vprintf(s_orig_vprintf);
        s_orig_vprintf = NULL;
    }
    if (s_log_file) {
        if (s_log_mutex) {
            xSemaphoreTake(s_log_mutex, portMAX_DELAY);
        }
        fflush(s_log_file);
        fsync(fileno(s_log_file));
        fclose(s_log_file);
        s_log_file = NULL;
        if (s_log_mutex) {
            xSemaphoreGive(s_log_mutex);
            vSemaphoreDelete(s_log_mutex);
            s_log_mutex = NULL;
        }
    }
}
