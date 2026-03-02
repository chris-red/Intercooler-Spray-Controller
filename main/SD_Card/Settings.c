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
#include "screen_data_logging.h"    /* g_logging_enabled        */
#include "screen_gmeter_cal.h"      /* g_trail_enabled          */
#include "G_Meter.h"                /* G_Meter_SetMax/GetMax    */

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

    /* Temporary storage for directional max values until fully parsed */
    float s_loaded_left = 0.0f, s_loaded_right = 0.0f;
    float s_loaded_fwd  = 0.0f, s_loaded_brk   = 0.0f;
    float s_loaded_cal_y = 0.0f, s_loaded_cal_z = 0.0f;
    bool  s_has_cal = false;

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
        } else if (strcmp(key, "logging_enabled") == 0) {
            int v = atoi(val);
            g_logging_enabled = (v != 0);
            ESP_LOGI(TAG, "  logging_enabled = %d", g_logging_enabled);
        } else if (strcmp(key, "max_g_left") == 0) {
            float v = strtof(val, NULL);
            if (v >= 0.0f) { s_loaded_left = v; }
        } else if (strcmp(key, "max_g_right") == 0) {
            float v = strtof(val, NULL);
            if (v >= 0.0f) { s_loaded_right = v; }
        } else if (strcmp(key, "max_g_forward") == 0) {
            float v = strtof(val, NULL);
            if (v >= 0.0f) { s_loaded_fwd = v; }
        } else if (strcmp(key, "max_g_brake") == 0) {
            float v = strtof(val, NULL);
            if (v >= 0.0f) { s_loaded_brk = v; }
        } else if (strcmp(key, "cal_offset_y") == 0) {
            s_loaded_cal_y = strtof(val, NULL);
            s_has_cal = true;
        } else if (strcmp(key, "cal_offset_z") == 0) {
            s_loaded_cal_z = strtof(val, NULL);
            s_has_cal = true;
        } else if (strcmp(key, "trail_enabled") == 0) {
            int v = atoi(val);
            g_trail_enabled = (v != 0);
            ESP_LOGI(TAG, "  trail_enabled = %d", g_trail_enabled);
        }
    }

    /* Apply loaded directional max values */
    G_Meter_SetMaxDirectional(s_loaded_left, s_loaded_right, s_loaded_fwd, s_loaded_brk);
    ESP_LOGI(TAG, "  max_g L=%.2f R=%.2f F=%.2f B=%.2f",
             s_loaded_left, s_loaded_right, s_loaded_fwd, s_loaded_brk);

    /* Restore calibration if available */
    if (s_has_cal) {
        G_Meter_SetCalibration(s_loaded_cal_y, s_loaded_cal_z);
        ESP_LOGI(TAG, "  cal offsets Y=%.4f Z=%.4f", s_loaded_cal_y, s_loaded_cal_z);
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
    fprintf(f, "logging_enabled=%d\n",  (int)g_logging_enabled);
    fprintf(f, "max_g_left=%.2f\n",     G_Meter_GetMaxLeft());
    fprintf(f, "max_g_right=%.2f\n",    G_Meter_GetMaxRight());
    fprintf(f, "max_g_forward=%.2f\n",  G_Meter_GetMaxForward());
    fprintf(f, "max_g_brake=%.2f\n",    G_Meter_GetMaxBrake());

    float cal_y, cal_z;
    G_Meter_GetCalibration(&cal_y, &cal_z);
    fprintf(f, "cal_offset_y=%.6f\n",   cal_y);
    fprintf(f, "cal_offset_z=%.6f\n",   cal_z);
    fprintf(f, "trail_enabled=%d\n",    (int)g_trail_enabled);

    fflush(f);
    fsync(fileno(f));
    fclose(f);

    ESP_LOGI(TAG, "Settings saved to %s", SETTINGS_PATH);
    return ESP_OK;
}

app_settings_t settings_get_current(void)
{
    float cy, cz;
    G_Meter_GetCalibration(&cy, &cz);
    return (app_settings_t){
        .trigger_temp   = g_trigger_temperature,
        .spray_duration = g_sprayer_duration,
        .spray_interval = g_sprayer_interval,
        .brightness     = g_brightness,
        .logging_enabled = g_logging_enabled,
        .max_g_left      = G_Meter_GetMaxLeft(),
        .max_g_right     = G_Meter_GetMaxRight(),
        .max_g_forward   = G_Meter_GetMaxForward(),
        .max_g_brake     = G_Meter_GetMaxBrake(),
        .cal_offset_y    = cy,
        .cal_offset_z    = cz,
        .trail_enabled   = g_trail_enabled,
    };
}
