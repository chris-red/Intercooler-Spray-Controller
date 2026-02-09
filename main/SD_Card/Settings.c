#include "Settings.h"

#include <stdio.h>
#include "ST7701S.h" // For LCD_Backlight
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "esp_log.h"

/* Access the global runtime settings */
#include "screen_trigger_temp.h"    /* g_trigger_temperature    */
#include "screen_spray_duration.h"  /* g_sprayer_duration       */
#include "screen_spray_interval.h"  /* g_sprayer_interval       */
#include "screen_brightness.h"      /* g_brightness             */

static const char *TAG = "Settings";

#define SETTINGS_DIR   "/sdcard/system"
#define SETTINGS_PATH  "/sdcard/system/SETTINGS.TXT"

/* --------------- helpers --------------------- */

/** Create directory if it doesn't exist */
static void ensure_dir(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        mkdir(path, 0775);
    }
}

/* --------------- public API ------------------ */

esp_err_t settings_load(void)
{
    FILE *f = fopen(SETTINGS_PATH, "r");
    if (!f) {
        ESP_LOGW(TAG, "No settings file found, using defaults");
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Loading settings from %s", SETTINGS_PATH);

    char line[128];
    while (fgets(line, sizeof(line), f)) {
        /* Strip newline */
        char *nl = strchr(line, '\n');
        if (nl) *nl = '\0';
        char *cr = strchr(line, '\r');
        if (cr) *cr = '\0';

        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\0') continue;

        /* Parse key=value */
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        const char *key = line;
        const char *val = eq + 1;

        if (strcmp(key, "trigger_temp") == 0) {
            int32_t v = atoi(val);
            if (v >= 20 && v <= 70) {
                g_trigger_temperature = v;
                ESP_LOGI(TAG, "  trigger_temp = %ld", (long)v);
            }
        } else if (strcmp(key, "spray_duration") == 0) {
            float v = strtof(val, NULL);
            if (v >= 0.5f && v <= 10.0f) {
                g_sprayer_duration = v;
                ESP_LOGI(TAG, "  spray_duration = %.1f", v);
            }
        } else if (strcmp(key, "spray_interval") == 0) {
            int32_t v = atoi(val);
            if (v >= 5 && v <= 30) {
                g_sprayer_interval = v;
                ESP_LOGI(TAG, "  spray_interval = %ld", (long)v);
            }
        } else if (strcmp(key, "brightness") == 0) {
            int v = atoi(val);
            if (v >= 0 && v <= 100) {
                g_brightness = (uint8_t)v;
                set_brightness(g_brightness);
                LCD_Backlight = g_brightness; // Ensure slider uses loaded value
                ESP_LOGI(TAG, "  brightness = %d", v);
            }
        }
    }

    fclose(f);
    ESP_LOGI(TAG, "Settings loaded");
    return ESP_OK;
}

esp_err_t settings_save(void)
{
    ensure_dir(SETTINGS_DIR);

    FILE *f = fopen(SETTINGS_PATH, "w");
    if (!f) {
        ESP_LOGE(TAG, "Failed to open %s for writing (errno %d)", SETTINGS_PATH, errno);
        return ESP_FAIL;
    }

    fprintf(f, "# Intercooler Spray Controller Settings\n");
    fprintf(f, "trigger_temp=%ld\n",  (long)g_trigger_temperature);
    fprintf(f, "spray_duration=%.1f\n", g_sprayer_duration);
    fprintf(f, "spray_interval=%ld\n", (long)g_sprayer_interval);
    fprintf(f, "brightness=%d\n",      (int)g_brightness);

    fflush(f);
    fsync(fileno(f));
    fclose(f);

    ESP_LOGI(TAG, "Settings saved to %s", SETTINGS_PATH);
    return ESP_OK;
}

app_settings_t settings_get_current(void)
{
    return (app_settings_t){
        .trigger_temp   = g_trigger_temperature,
        .spray_duration = g_sprayer_duration,
        .spray_interval = g_sprayer_interval,
        .brightness     = g_brightness,
    };
}
