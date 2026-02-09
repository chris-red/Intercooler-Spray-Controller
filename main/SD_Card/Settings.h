#pragma once

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief Settings data structure.
 *
 * Holds all user-configurable values that are persisted to SD card.
 */
typedef struct {
    int32_t  trigger_temp;      /**< Trigger temperature (°C), 20–70  */
    float    spray_duration;    /**< Spray duration (s), 0.5–10.0     */
    int32_t  spray_interval;    /**< Spray interval (s), 5–30         */
    uint8_t  brightness;        /**< LCD brightness (%), 0–100        */
} app_settings_t;

/**
 * @brief Load settings from SD card.
 *
 * Reads /sdcard/system/SETTINGS.TXT and applies values to the
 * global runtime variables. Missing or corrupt values keep defaults.
 *
 * @return ESP_OK if file was read, ESP_ERR_NOT_FOUND if no file.
 */
esp_err_t settings_load(void);

/**
 * @brief Save current settings to SD card.
 *
 * Writes /sdcard/system/SETTINGS.TXT with current runtime values.
 *
 * @return ESP_OK on success, ESP_FAIL on write error.
 */
esp_err_t settings_save(void);

/**
 * @brief Get a snapshot of the current settings.
 */
app_settings_t settings_get_current(void);
