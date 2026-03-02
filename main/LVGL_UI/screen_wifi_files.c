/**
 * @file screen_wifi_files.c
 * @brief WiFi file-server screen.
 *
 * A single toggle button starts / stops the WiFi AP + HTTP file
 * server.  When running, the screen shows connection instructions.
 */

#include "screen_wifi_files.h"
#include "ui_common.h"
#include "screen_manager.h"
#include "Wifi_File_Server.h"
#include "ST7701S.h"
#include "LVGL_Driver.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "scr_wifi";

/***********************
 *  STATIC VARIABLES
 ***********************/
static lv_obj_t   *container   = NULL;
static lv_obj_t   *lbl_title   = NULL;
static lv_obj_t   *lbl_status  = NULL;
static lv_obj_t   *lbl_info    = NULL;
static lv_obj_t   *toggle_btn  = NULL;
static lv_obj_t   *btn_label   = NULL;
static lv_timer_t *poll_timer  = NULL;

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void toggle_btn_event_cb(lv_event_t *e);
static void update_ui(void);
static void wifi_toggle_task(void *arg);
static void wifi_poll_timer_cb(lv_timer_t *t);

/* Flag to prevent double-tap while async operation is in progress */
static volatile bool s_wifi_busy = false;

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void update_ui(void)
{
    bool running = wifi_file_server_is_running();

    if (lbl_status) {
        if (running) {
            lv_label_set_text(lbl_status, LV_SYMBOL_WIFI " Server ON");
            lv_obj_set_style_text_color(lbl_status, COLOR_TEMP_NORMAL, 0);
        } else {
            lv_label_set_text(lbl_status, LV_SYMBOL_WIFI " Server OFF");
            lv_obj_set_style_text_color(lbl_status, COLOR_INDICATOR_OFF, 0);
        }
    }

    if (lbl_info) {
        if (running) {
            lv_label_set_text(lbl_info,
                "1. Connect phone WiFi to\n"
                "   \"ICSpray-Logs\"\n"
                "2. Open browser:\n"
                "   http://192.168.4.1");
        } else {
            lv_label_set_text(lbl_info, "");
        }
    }

    if (btn_label) {
        lv_label_set_text(btn_label, running ? LV_SYMBOL_CLOSE " STOP" : LV_SYMBOL_WIFI " START");
    }

    if (toggle_btn) {
        lv_obj_set_style_bg_color(toggle_btn,
            running ? COLOR_TEMP_CRITICAL : COLOR_ACCENT, 0);
    }
}

static void toggle_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
        screen_manager_reset_inactivity();
    }

    if (code == LV_EVENT_CLICKED) {
        if (s_wifi_busy) return;  /* operation already in progress */
        s_wifi_busy = true;

        /* Show a "please wait" state */
        if (btn_label) lv_label_set_text(btn_label, LV_SYMBOL_REFRESH " ...");
        lv_obj_set_style_bg_color(toggle_btn, lv_color_hex(0x555555), 0);

        /* Run WiFi start/stop in a separate FreeRTOS task so the LVGL
         * thread keeps running and processes display refreshes. */
        bool want_start = !wifi_file_server_is_running();
        xTaskCreate(wifi_toggle_task, "wifi_tog", 4096,
                    (void *)(uintptr_t)want_start, 5, NULL);

        /* Start polling timer to update UI when task finishes */
        if (poll_timer) lv_timer_del(poll_timer);
        poll_timer = lv_timer_create(wifi_poll_timer_cb, 200, NULL);
    }
}

/**
 * FreeRTOS task that starts or stops the WiFi file server and then
 * re-syncs the LCD panel to fix RGB DMA timing shift.
 */
static void wifi_toggle_task(void *arg)
{
    bool want_start = (bool)(uintptr_t)arg;

    if (want_start) {
        ESP_LOGI(TAG, "Starting WiFi file server (async)");
        wifi_file_server_start();
    } else {
        ESP_LOGI(TAG, "Stopping WiFi file server (async)");
        wifi_file_server_stop();
    }

    /* Give the system a moment to settle after WiFi DMA changes */
    vTaskDelay(pdMS_TO_TICKS(200));

    /* Re-sync the LCD panel — WiFi DMA contention can shift the RGB
     * panel's horizontal position (known ESP32-S3 issue).
     * Must hold the LVGL port lock because LCD_Restart touches the
     * display that lv_timer_handler flushes to. */
    lvgl_port_lock(0);
    LCD_Restart();
    lvgl_port_unlock();

    s_wifi_busy = false;

    /* Schedule a UI update on the LVGL thread (timer runs there) */
    /* We can't call lv_* from this task, so just set the flag and the
     * screen_wifi_files_show timer or next touch will pick it up. */
    ESP_LOGI(TAG, "WiFi toggle complete, LCD re-synced");

    vTaskDelete(NULL);
}

/** Periodic LVGL timer: update UI once the async WiFi task is done. */
static void wifi_poll_timer_cb(lv_timer_t *t)
{
    if (!s_wifi_busy) {
        update_ui();
        lv_timer_del(t);
        poll_timer = NULL;
    }
}

lv_obj_t *screen_wifi_files_create(lv_obj_t *parent)
{
    const ui_fonts_t *fonts = ui_common_get_fonts();

    /* ===== Container ===== */
    container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scroll_dir(container, LV_DIR_NONE);
    /* Prevent scroll events from bubbling up and shifting the root screen */
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLL_CHAIN_HOR);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_add_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* ===== Title ===== */
    lbl_title = lv_label_create(container);
    lv_label_set_text(lbl_title, "WiFi Files");
    lv_obj_set_style_text_color(lbl_title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl_title, fonts->large, 0);
    lv_obj_set_width(lbl_title, LV_PCT(100));
    lv_obj_set_style_text_align(lbl_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_WRAP);

    /* ===== Status ===== */
    lbl_status = lv_label_create(container);
    lv_obj_set_style_text_font(lbl_status, fonts->normal, 0);
    lv_obj_set_style_pad_top(lbl_status, 10, 0);
    lv_obj_set_width(lbl_status, LV_PCT(100));
    lv_obj_set_style_text_align(lbl_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_long_mode(lbl_status, LV_LABEL_LONG_WRAP);

    /* ===== Toggle button ===== */
    toggle_btn = lv_btn_create(container);
    lv_obj_set_size(toggle_btn, 200, 50);
    lv_obj_set_style_bg_color(toggle_btn, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_color(toggle_btn, lv_color_hex(0x0099CC), LV_STATE_PRESSED);
    lv_obj_set_style_radius(toggle_btn, 10, 0);
    lv_obj_set_style_pad_top(toggle_btn, 0, 0);
    /* Prevent focus entirely so LVGL never triggers scroll-to-view */
    lv_obj_clear_flag(toggle_btn, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(toggle_btn, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_add_event_cb(toggle_btn, toggle_btn_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(toggle_btn, toggle_btn_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(toggle_btn, toggle_btn_event_cb, LV_EVENT_RELEASED, NULL);

    btn_label = lv_label_create(toggle_btn);
    lv_obj_set_style_text_color(btn_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(btn_label, fonts->normal, 0);
    lv_obj_center(btn_label);

    /* ===== Connection instructions ===== */
    lbl_info = lv_label_create(container);
    lv_obj_set_style_text_color(lbl_info, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(lbl_info, fonts->small, 0);
    lv_obj_set_style_text_align(lbl_info, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(lbl_info, 12, 0);
    lv_obj_set_width(lbl_info, LV_PCT(90));
    lv_label_set_long_mode(lbl_info, LV_LABEL_LONG_WRAP);

    update_ui();

    return container;
}

void screen_wifi_files_destroy(void)
{
    if (poll_timer) {
        lv_timer_del(poll_timer);
        poll_timer = NULL;
    }
    if (container) {
        lv_obj_del(container);
        container = NULL;
    }
    lbl_title  = NULL;
    lbl_status = NULL;
    lbl_info   = NULL;
    toggle_btn = NULL;
    btn_label  = NULL;
}

void screen_wifi_files_show(void)
{
    /* Keep screen alive while serving files */
    screen_manager_pause_inactivity();
    update_ui();
    /* Ensure no stale scroll offset */
    if (container) {
        lv_obj_scroll_to(container, 0, 0, LV_ANIM_OFF);
    }
}

void screen_wifi_files_hide(void)
{
    screen_manager_resume_inactivity();
}
