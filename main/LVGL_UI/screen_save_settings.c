#include "screen_save_settings.h"
#include "ui_common.h"
#include "screen_manager.h"
#include "Settings.h"
#include "ST7701S.h"   /* LCD_Backlight */
#include "esp_log.h"

#include <stdio.h>

/* Access setting globals for display */
#include "screen_trigger_temp.h"
#include "screen_spray_duration.h"
#include "screen_spray_interval.h"
#include "screen_brightness.h"

static const char *TAG = "scr_save";

/***********************
 *  STATIC VARIABLES
 ***********************/
static lv_obj_t *container       = NULL;
static lv_obj_t *status_label    = NULL;
static lv_obj_t *summary_label   = NULL;
static lv_obj_t *save_btn        = NULL;

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void save_btn_event_cb(lv_event_t *e);
static void update_summary(void);

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void update_summary(void)
{
    if (!summary_label) return;

    static char buf[200];
    snprintf(buf, sizeof(buf),
             "Trigger Temp:    %ld C\n"
             "Spray Duration:  %.1f s\n"
             "Spray Interval:  %ld s\n"
             "Brightness:      %d%%",
             (long)g_trigger_temperature,
             g_sprayer_duration,
             (long)g_sprayer_interval,
             (int)g_brightness);
    lv_label_set_text(summary_label, buf);
}

static void save_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
        screen_manager_reset_inactivity();
    }

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Save button pressed");

        esp_err_t ret = settings_save();
        if (ret == ESP_OK) {
            lv_label_set_text(status_label, "Settings saved!");
            lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);
        } else {
            lv_label_set_text(status_label, "Save failed!");
            lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF0000), 0);
        }
    }
}

lv_obj_t *screen_save_settings_create(lv_obj_t *parent)
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
    lv_label_set_text(title, "SAVE SETTINGS");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, fonts->large, 0);

    /* ===== Spacer ===== */
    lv_obj_t *spacer1 = lv_obj_create(container);
    lv_obj_set_size(spacer1, 1, 15);
    lv_obj_set_style_bg_opa(spacer1, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer1, 0, 0);

    /* ===== Settings summary ===== */
    summary_label = lv_label_create(container);
    lv_obj_set_style_text_color(summary_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(summary_label, fonts->normal, 0);
    lv_obj_set_style_text_align(summary_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_width(summary_label, 300);
    update_summary();

    /* ===== Spacer ===== */
    lv_obj_t *spacer2 = lv_obj_create(container);
    lv_obj_set_size(spacer2, 1, 20);
    lv_obj_set_style_bg_opa(spacer2, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer2, 0, 0);

    /* ===== Save button ===== */
    save_btn = lv_btn_create(container);
    lv_obj_set_size(save_btn, 220, 60);
    lv_obj_set_style_bg_color(save_btn, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x0099CC), LV_STATE_PRESSED);
    lv_obj_set_style_radius(save_btn, 10, 0);
    lv_obj_add_event_cb(save_btn, save_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *btn_label = lv_label_create(save_btn);
    lv_label_set_text(btn_label, "SAVE");
    lv_obj_set_style_text_color(btn_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(btn_label, fonts->large, 0);
    lv_obj_center(btn_label);

    /* ===== Spacer ===== */
    lv_obj_t *spacer3 = lv_obj_create(container);
    lv_obj_set_size(spacer3, 1, 10);
    lv_obj_set_style_bg_opa(spacer3, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer3, 0, 0);

    /* ===== Status label ===== */
    status_label = lv_label_create(container);
    lv_label_set_text(status_label, "");
    lv_obj_set_style_text_color(status_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(status_label, fonts->normal, 0);

    return container;
}

void screen_save_settings_destroy(void)
{
    if (container) {
        lv_obj_del(container);
        container = NULL;
    }
    summary_label = NULL;
    status_label = NULL;
    save_btn = NULL;
}

void screen_save_settings_show(void)
{
    /* Refresh the summary with current values every time we show */
    update_summary();

    /* Clear any previous status message */
    if (status_label) {
        lv_label_set_text(status_label, "");
    }
}

void screen_save_settings_hide(void)
{
    /* Nothing special needed */
}
