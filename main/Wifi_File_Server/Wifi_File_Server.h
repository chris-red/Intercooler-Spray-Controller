#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Start the WiFi AP + HTTP file server.
 *
 * Creates a SoftAP ("ICSpray-Logs", no password) and starts an
 * HTTP server that lets a phone browser browse and download files
 * from /sdcard.  IP address: 192.168.4.1
 *
 * Safe to call multiple times — subsequent calls are no-ops.
 *
 * @return ESP_OK on success.
 */
esp_err_t wifi_file_server_start(void);

/**
 * @brief Stop the WiFi AP + HTTP file server.
 *
 * Tears down the HTTP server and stops WiFi.
 *
 * @return ESP_OK on success.
 */
esp_err_t wifi_file_server_stop(void);

/**
 * @brief Check whether the file server is currently running.
 */
bool wifi_file_server_is_running(void);
