#pragma once

#include "esp_err.h"
#include <stdint.h>

/** Default sync interval in milliseconds */
#define SD_LOGGER_DEFAULT_SYNC_MS  1000

/**
 * @brief Initialize SD card logging.
 *
 * Creates /sdcard/system/logs/ if needed, rotates old logs (keeps last 5),
 * opens a new log file, and redirects ESP_LOGx output to both
 * the UART console and the SD card file.
 *
 * Must be called AFTER SD_Init().
 *
 * @param sync_interval_ms  How often (ms) to sync data to the SD card.
 *                          Pass 0 to use the default (1000 ms).
 * @return ESP_OK on success, ESP_FAIL on error (logging continues to UART only).
 */
esp_err_t SD_Logger_Init(uint32_t sync_interval_ms);

/**
 * @brief Flush any buffered log data to the SD card.
 *
 * Call periodically or before intentional reboot to avoid data loss.
 */
void SD_Logger_Flush(void);

/**
 * @brief Stop SD card logging and close the file.
 */
void SD_Logger_Deinit(void);
