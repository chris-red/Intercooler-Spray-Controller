#include "screen_data_logging.h"
#include "screen_main.h"
#include "ui_common.h"
#include "screen_manager.h"
#include "Temp_Logger.h"
#include "esp_log.h"

static const char *TAG = "scr_log";

/***********************
 *  GLOBAL VARIABLES
 ***********************/
bool g_logging_enabled = false;   /* default off */

/***********************
 *  STATIC VARIABLES
 ***********************/
static lv_obj_t *container    = NULL;
static lv_obj_t *sw_toggle    = NULL;
static lv_obj_t *status_label = NULL;

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void toggle_event_cb(lv_event_t *e);
static void update_status(void);

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void update_status(void)
{
    if (!status_label) return;

    if (g_logging_enabled) {
        lv_label_set_text(status_label, "Logging active");
        lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(status_label, "Logging off");
        lv_obj_set_style_text_color(status_label, COLOR_TEXT_SECONDARY, 0);
    }
}

static void toggle_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
        screen_manager_reset_inactivity();
    }

    if (code == LV_EVENT_VALUE_CHANGED) {
        bool on = lv_obj_has_state(sw_toggle, LV_STATE_CHECKED);
        g_logging_enabled = on;
        Temp_Logger_SetEnabled(on);
        screen_main_set_logging_active(on);
        update_status();
        ESP_LOGI(TAG, "Data logging toggled %s", on ? "ON" : "OFF");
    }
}

lv_obj_t *screen_data_logging_create(lv_obj_t *parent)
{
    const ui_fonts_t *fonts = ui_common_get_fonts();

    /* ===== Container ===== */
    container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 20, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* ===== Title ===== */
    lv_obj_t *title = lv_label_create(container);
    lv_label_set_text(title, "DATA LOGGING");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, fonts->large, 0);

    /* ===== Spacer ===== */
    lv_obj_t *spacer1 = lv_obj_create(container);
    lv_obj_set_size(spacer1, 1, 30);
    lv_obj_set_style_bg_opa(spacer1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer1, 0, 0);

    /* ===== Toggle switch ===== */
    sw_toggle = lv_switch_create(container);
    lv_obj_set_size(sw_toggle, 120, 60);
    lv_obj_set_style_bg_color(sw_toggle, lv_color_hex(0x444444), 0);
    lv_obj_set_style_bg_color(sw_toggle, COLOR_ACCENT, LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_add_event_cb(sw_toggle, toggle_event_cb, LV_EVENT_ALL, NULL);

    /* Sync switch state with global */
    if (g_logging_enabled) {
        lv_obj_add_state(sw_toggle, LV_STATE_CHECKED);
    }

    /* ===== Spacer ===== */
    lv_obj_t *spacer2 = lv_obj_create(container);
    lv_obj_set_size(spacer2, 1, 20);
    lv_obj_set_style_bg_opa(spacer2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer2, 0, 0);

    /* ===== Status label ===== */
    status_label = lv_label_create(container);
    lv_obj_set_style_text_font(status_label, fonts->normal, 0);
    update_status();

    /* ===== Description ===== */
    lv_obj_t *spacer3 = lv_obj_create(container);
    lv_obj_set_size(spacer3, 1, 15);
    lv_obj_set_style_bg_opa(spacer3, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer3, 0, 0);

    lv_obj_t *desc = lv_label_create(container);
    lv_label_set_text(desc, "Logs temp to SD card\nevery second as CSV");
    lv_obj_set_style_text_color(desc, COLOR_TEXT_LABEL, 0);
    lv_obj_set_style_text_font(desc, fonts->small, 0);
    lv_obj_set_style_text_align(desc, LV_TEXT_ALIGN_CENTER, 0);

    return container;
}

void screen_data_logging_destroy(void)
{
    if (container) {
        lv_obj_del(container);
        container = NULL;
    }
    sw_toggle    = NULL;
    status_label = NULL;
}

void screen_data_logging_show(void)
{
    /* Sync switch to current state */
    if (sw_toggle) {
        if (g_logging_enabled) {
            lv_obj_add_state(sw_toggle, LV_STATE_CHECKED);
        } else {
            lv_obj_clear_state(sw_toggle, LV_STATE_CHECKED);
        }
    }
    update_status();
}

void screen_data_logging_hide(void)
{
    /* Nothing special */
}
