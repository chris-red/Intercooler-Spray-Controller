/**
 * @file screen_gmeter_cal.c
 * @brief G-meter calibration screen.
 *
 * Provides a CALIBRATE button that captures the current accelerometer
 * resting values as the zero offset.  Shows live Y/Z raw values so the
 * user can verify the device is stationary, and indicates whether a
 * calibration has already been applied.
 *
 * The calibration is automatically saved when the user reaches the
 * Save Settings screen.
 */

#include "screen_gmeter_cal.h"
#include "ui_common.h"
#include "screen_manager.h"
#include "G_Meter.h"
#include "Settings.h"
#include "esp_log.h"

#include <stdio.h>

static const char *TAG = "scr_cal";

/***********************
 *  GLOBAL VARIABLES
 ***********************/
bool g_trail_enabled = true;   /* default on */

/***********************
 *  STATIC VARIABLES
 ***********************/
static lv_obj_t   *container     = NULL;
static lv_obj_t   *lbl_status    = NULL;
static lv_obj_t   *lbl_live      = NULL;
static lv_obj_t   *cal_btn       = NULL;
static lv_obj_t   *sw_trail      = NULL;
static lv_timer_t *update_timer  = NULL;

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void update_timer_cb(lv_timer_t *timer);
static void cal_btn_event_cb(lv_event_t *e);
static void trail_toggle_cb(lv_event_t *e);

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void update_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    float lat = G_Meter_GetLateral();
    float lon = G_Meter_GetLongitudinal();

    if (lbl_live) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Live  L/R: %+.3f   F/B: %+.3f", lat, lon);
        lv_label_set_text(lbl_live, buf);
    }
}

static void cal_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
        screen_manager_reset_inactivity();
    }

    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Calibrate pressed");
        G_Meter_Calibrate();

        /* Save immediately so the calibration persists */
        settings_save();

        if (lbl_status) {
            float cy, cz;
            G_Meter_GetCalibration(&cy, &cz);
            char buf[64];
            snprintf(buf, sizeof(buf), LV_SYMBOL_OK " Calibrated\nY=%.4f  Z=%.4f", cy, cz);
            lv_label_set_text(lbl_status, buf);
            lv_obj_set_style_text_color(lbl_status, COLOR_TEMP_NORMAL, 0);
        }
    }
}

lv_obj_t *screen_gmeter_cal_create(lv_obj_t *parent)
{
    const ui_fonts_t *fonts = ui_common_get_fonts();

    /* ===== Container ===== */
    container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 5, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* ===== Title ===== */
    lv_obj_t *title = lv_label_create(container);
    lv_label_set_text(title, "G-Meter Cal");
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(title, fonts->large, 0);

    /* ===== Instructions ===== */
    lv_obj_t *lbl_instr = lv_label_create(container);
    lv_label_set_text(lbl_instr,
        "Mount the device in its\n"
        "final position and keep\n"
        "it completely still.\n"
        "Then press CALIBRATE.");
    lv_obj_set_style_text_color(lbl_instr, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(lbl_instr, fonts->small, 0);
    lv_obj_set_style_text_align(lbl_instr, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(lbl_instr, 10, 0);

    /* ===== Live values ===== */
    lbl_live = lv_label_create(container);
    lv_label_set_text(lbl_live, "Live  L/R: +0.000   F/B: +0.000");
    lv_obj_set_style_text_color(lbl_live, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl_live, fonts->small, 0);
    lv_obj_set_style_pad_top(lbl_live, 15, 0);

    /* ===== Status ===== */
    lbl_status = lv_label_create(container);
    if (G_Meter_IsCalibrated()) {
        float cy, cz;
        G_Meter_GetCalibration(&cy, &cz);
        char buf[64];
        snprintf(buf, sizeof(buf), LV_SYMBOL_OK " Calibrated\nY=%.4f  Z=%.4f", cy, cz);
        lv_label_set_text(lbl_status, buf);
        lv_obj_set_style_text_color(lbl_status, COLOR_TEMP_NORMAL, 0);
    } else {
        lv_label_set_text(lbl_status, LV_SYMBOL_WARNING " Not calibrated");
        lv_obj_set_style_text_color(lbl_status, COLOR_TEMP_WARNING, 0);
    }
    lv_obj_set_style_text_font(lbl_status, fonts->normal, 0);
    lv_obj_set_style_text_align(lbl_status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_pad_top(lbl_status, 10, 0);

    /* ===== Calibrate button ===== */
    cal_btn = lv_btn_create(container);
    lv_obj_set_size(cal_btn, 200, 50);
    lv_obj_set_style_bg_color(cal_btn, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_color(cal_btn, lv_color_hex(0x0099CC), LV_STATE_PRESSED);
    lv_obj_set_style_radius(cal_btn, 10, 0);
    lv_obj_set_style_pad_top(cal_btn, 0, 0);
    lv_obj_add_event_cb(cal_btn, cal_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *btn_label = lv_label_create(cal_btn);
    lv_label_set_text(btn_label, LV_SYMBOL_REFRESH " CALIBRATE");
    lv_obj_set_style_text_color(btn_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(btn_label, fonts->normal, 0);
    lv_obj_center(btn_label);

    /* ===== Trail toggle ===== */
    lv_obj_t *trail_row = lv_obj_create(container);
    lv_obj_remove_style_all(trail_row);
    lv_obj_set_size(trail_row, 250, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(trail_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(trail_row, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(trail_row, 10, 0);
    lv_obj_clear_flag(trail_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(trail_row, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lv_obj_t *trail_label = lv_label_create(trail_row);
    lv_label_set_text(trail_label, "Trail");
    lv_obj_set_style_text_color(trail_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(trail_label, fonts->normal, 0);

    sw_trail = lv_switch_create(trail_row);
    lv_obj_set_size(sw_trail, 50, 26);
    lv_obj_set_style_bg_color(sw_trail, COLOR_INDICATOR_OFF, 0);
    lv_obj_set_style_bg_color(sw_trail, COLOR_ACCENT, LV_STATE_CHECKED | LV_PART_INDICATOR);
    if (g_trail_enabled) {
        lv_obj_add_state(sw_trail, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(sw_trail, trail_toggle_cb, LV_EVENT_ALL, NULL);

    /* ===== Update timer (200 ms) ===== */
    update_timer = lv_timer_create(update_timer_cb, 200, NULL);

    return container;
}

void screen_gmeter_cal_destroy(void)
{
    if (update_timer) {
        lv_timer_del(update_timer);
        update_timer = NULL;
    }

    if (container) {
        lv_obj_del(container);
        container = NULL;
    }

    lbl_status  = NULL;
    lbl_live    = NULL;
    cal_btn     = NULL;
    sw_trail    = NULL;
}

void screen_gmeter_cal_show(void)
{
    if (update_timer) lv_timer_resume(update_timer);
}

void screen_gmeter_cal_hide(void)
{
    if (update_timer) lv_timer_pause(update_timer);
}

static void trail_toggle_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
        screen_manager_reset_inactivity();
    }

    if (code == LV_EVENT_VALUE_CHANGED) {
        g_trail_enabled = lv_obj_has_state(sw_trail, LV_STATE_CHECKED);
        settings_save();
        ESP_LOGI(TAG, "Trail %s", g_trail_enabled ? "ON" : "OFF");
    }
}
