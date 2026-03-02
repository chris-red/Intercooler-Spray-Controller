/**
 * @file Wifi_File_Server.c
 * @brief Minimal WiFi SoftAP + HTTP file server.
 *
 * When started the ESP32 creates an open access point called
 * "ICSpray-Logs".  A phone can connect and open http://192.168.4.1
 * in a browser to see a directory listing of /sdcard and download
 * any file (CSV logs etc.).
 */

#include "Wifi_File_Server.h"

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_fs";

/* ---- state ---- */
static bool          s_running  = false;
static httpd_handle_t s_server  = NULL;
static esp_netif_t   *s_netif   = NULL;

/* ---- configuration ---- */
#define AP_SSID         "ICSpray-Logs"
#define AP_PASS         ""            /* open network */
#define AP_CHANNEL      1
#define AP_MAX_CONN     2
#define SD_BASE_PATH    "/sdcard/system"
#define FILE_BUF_SIZE   4096

/* ================================================================
 *  HTML helpers
 * ================================================================ */

/** Send a simple HTML page header */
static void send_html_head(httpd_req_t *req, const char *title)
{
    httpd_resp_set_type(req, "text/html; charset=UTF-8");
    httpd_resp_sendstr_chunk(req,
        "<!DOCTYPE html><html><head>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<style>"
        "body{font-family:sans-serif;background:#1a1a2e;color:#eee;padding:16px}"
        "a{color:#00bfff;text-decoration:none;font-size:1.1em}"
        "a:hover{text-decoration:underline}"
        "li{padding:6px 0}"
        "h1{color:#fff;font-size:1.4em}"
        ".sz{color:#888;font-size:0.85em;margin-left:8px}"
        "</style>");
    httpd_resp_sendstr_chunk(req, "<title>");
    httpd_resp_sendstr_chunk(req, title);
    httpd_resp_sendstr_chunk(req, "</title></head><body>");
}

/** Format a file size for display */
static void format_size(char *buf, size_t buflen, size_t bytes)
{
    if (bytes >= 1024 * 1024)
        snprintf(buf, buflen, "%.1f MB", (double)bytes / (1024 * 1024));
    else if (bytes >= 1024)
        snprintf(buf, buflen, "%.1f KB", (double)bytes / 1024);
    else
        snprintf(buf, buflen, "%u B", (unsigned)bytes);
}

/* ================================================================
 *  HTTP handlers
 * ================================================================ */

/**
 * GET handler.
 * If the URI maps to a directory  → show directory listing.
 * If the URI maps to a file       → stream the file back.
 */
static esp_err_t file_get_handler(httpd_req_t *req)
{
    /* Build the filesystem path from the URI.
     * URI "/" → "/sdcard", "/system/logs" */
    char path[256];
    const char *uri = req->uri;

    /* URL-decode is not needed for simple paths; just strip query string */
    const char *q = strchr(uri, '?');
    size_t uri_len = q ? (size_t)(q - uri) : strlen(uri);

    /* Remove trailing slash (except root) */
    while (uri_len > 1 && uri[uri_len - 1] == '/') uri_len--;

    /* Path traversal protection — reject any URI containing ".." */
    {
        const char *p = uri;
        const char *end = uri + uri_len;
        while (p + 1 < end) {
            if (p[0] == '.' && p[1] == '.') {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid path");
                return ESP_OK;
            }
            p++;
        }
    }

    snprintf(path, sizeof(path), "%s%.*s", SD_BASE_PATH, (int)uri_len, uri);

    struct stat st;
    if (stat(path, &st) != 0) {
        ESP_LOGW(TAG, "404: %s", path);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_OK;
    }

    /* ---- Directory listing ---- */
    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot open directory");
            return ESP_OK;
        }

        char title[128];
        snprintf(title, sizeof(title), "Index of %.*s", (int)uri_len, uri);
        send_html_head(req, title);

        char buf[512];
        snprintf(buf, sizeof(buf), "<h1>%s</h1><ul>", title);
        httpd_resp_sendstr_chunk(req, buf);

        /* Parent link (if not root) */
        if (uri_len > 1) {
            /* Find parent path */
            char parent[128];
            strncpy(parent, uri, uri_len);
            parent[uri_len] = '\0';
            char *slash = strrchr(parent, '/');
            if (slash && slash != parent) *slash = '\0';
            else strcpy(parent, "/");

            snprintf(buf, sizeof(buf), "<li>&#x1F4C1; <a href='%s'>..</a></li>", parent);
            httpd_resp_sendstr_chunk(req, buf);
        }

        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            /* Build full path for stat */
            char entry_path[512];
            snprintf(entry_path, sizeof(entry_path), "%s/%s", path, ent->d_name);

            struct stat est;
            bool is_dir = false;
            size_t fsize = 0;
            if (stat(entry_path, &est) == 0) {
                is_dir = S_ISDIR(est.st_mode);
                fsize  = est.st_size;
            }

            /* Build the link URI */
            char link[512];
            if (uri_len == 1) {
                snprintf(link, sizeof(link), "/%s", ent->d_name);
            } else {
                snprintf(link, sizeof(link), "%.*s/%s", (int)uri_len, uri, ent->d_name);
            }

            if (is_dir) {
                snprintf(buf, sizeof(buf),
                    "<li>&#x1F4C1; <a href='%.256s'>%.128s/</a></li>",
                    link, ent->d_name);
            } else {
                char sz[32];
                format_size(sz, sizeof(sz), fsize);
                snprintf(buf, sizeof(buf),
                    "<li>&#x1F4C4; <a href='%.256s'>%.128s</a><span class='sz'>%s</span></li>",
                    link, ent->d_name, sz);
            }
            httpd_resp_sendstr_chunk(req, buf);
        }
        closedir(dir);

        httpd_resp_sendstr_chunk(req, "</ul></body></html>");
        httpd_resp_sendstr_chunk(req, NULL);   /* finish chunked response */
        return ESP_OK;
    }

    /* ---- Serve a regular file ---- */
    FILE *f = fopen(path, "r");
    if (!f) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Cannot open file");
        return ESP_OK;
    }

    /* Set content type */
    const char *ext = strrchr(path, '.');
    if (ext && strcasecmp(ext, ".csv") == 0) {
        httpd_resp_set_type(req, "text/csv");
    } else if (ext && strcasecmp(ext, ".txt") == 0) {
        httpd_resp_set_type(req, "text/plain");
    } else {
        httpd_resp_set_type(req, "application/octet-stream");
    }

    /* Stream file in chunks */
    char *buf = malloc(FILE_BUF_SIZE);
    if (!buf) {
        fclose(f);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Out of memory");
        return ESP_OK;
    }

    size_t read_bytes;
    while ((read_bytes = fread(buf, 1, FILE_BUF_SIZE, f)) > 0) {
        if (httpd_resp_send_chunk(req, buf, read_bytes) != ESP_OK) {
            ESP_LOGE(TAG, "File send failed");
            break;
        }
    }
    fclose(f);
    free(buf);

    httpd_resp_send_chunk(req, NULL, 0);   /* finish */
    return ESP_OK;
}

/* ================================================================
 *  WiFi AP setup
 * ================================================================ */

static bool s_netif_inited = false;

static esp_err_t start_wifi_ap(void)
{
    /* NVS (required by WiFi) */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* These two calls are NOT idempotent — only call once. */
    if (!s_netif_inited) {
        ESP_ERROR_CHECK(esp_netif_init());
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        s_netif_inited = true;
    }

    s_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));

    wifi_config_t ap_cfg = {
        .ap = {
            .ssid           = AP_SSID,
            .password       = AP_PASS,
            .ssid_len       = strlen(AP_SSID),
            .channel        = AP_CHANNEL,
            .max_connection = AP_MAX_CONN,
            .authmode       = WIFI_AUTH_OPEN,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi AP started  SSID=\"%s\"  IP=192.168.4.1", AP_SSID);
    return ESP_OK;
}

static void stop_wifi_ap(void)
{
    esp_wifi_stop();
    esp_wifi_deinit();

    if (s_netif) {
        esp_netif_destroy_default_wifi(s_netif);
        s_netif = NULL;
    }

    /* Do NOT call esp_event_loop_delete_default() — other components
     * may still rely on the default event loop. */
    ESP_LOGI(TAG, "WiFi AP stopped");
}

/* ================================================================
 *  HTTP server start / stop
 * ================================================================ */

static esp_err_t start_http_server(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 4;
    config.stack_size = 8192;

    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Register a single wildcard URI that handles everything */
    httpd_uri_t file_uri = {
        .uri      = "/*",
        .method   = HTTP_GET,
        .handler  = file_get_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_server, &file_uri);

    ESP_LOGI(TAG, "HTTP file server started on port %d", config.server_port);
    return ESP_OK;
}

static void stop_http_server(void)
{
    if (s_server) {
        httpd_stop(s_server);
        s_server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}

/* ================================================================
 *  Public API
 * ================================================================ */

esp_err_t wifi_file_server_start(void)
{
    if (s_running) {
        ESP_LOGW(TAG, "Already running");
        return ESP_OK;
    }

    esp_err_t ret = start_wifi_ap();
    if (ret != ESP_OK) return ret;

    ret = start_http_server();
    if (ret != ESP_OK) {
        stop_wifi_ap();
        return ret;
    }

    s_running = true;
    return ESP_OK;
}

esp_err_t wifi_file_server_stop(void)
{
    if (!s_running) return ESP_OK;

    stop_http_server();
    stop_wifi_ap();

    s_running = false;
    return ESP_OK;
}

bool wifi_file_server_is_running(void)
{
    return s_running;
}
