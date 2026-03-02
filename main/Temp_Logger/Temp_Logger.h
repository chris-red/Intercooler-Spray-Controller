#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initialize the temperature data logger.
 *
 * Creates /sdcard/system/logs/ if needed and starts a FreeRTOS
 * timer that writes a timestamped temperature reading to a CSV file
 * once every second when logging is enabled.
 *
 * Must be called AFTER SD_Init().
 *
 * @return ESP_OK on success, ESP_FAIL on error.
 */
esp_err_t Temp_Logger_Init(void);

/**
 * @brief Start or stop temperature logging based on the global setting.
 *
 * When enabled, opens a new CSV log file (or appends to today's file)
 * and begins writing once per second.  When disabled, flushes and
 * closes the current file.
 *
 * @param enable  true to start logging, false to stop.
 */
void Temp_Logger_SetEnabled(bool enable);

/**
 * @brief Check if temperature logging is currently active.
 */
bool Temp_Logger_IsActive(void);

/**
 * @brief Record one temperature sample.
 *
 * Called from the driver loop with the latest reading.
 * The logger decides internally whether it is time to write.
 */
void Temp_Logger_Feed(float temp_celsius);

/**
 * @brief Flush buffered data to SD card.
 */
void Temp_Logger_Flush(void);

/**
 * @brief Stop logging and release resources.
 */
void Temp_Logger_Deinit(void);
