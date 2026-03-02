/**
 * @file screen_gmeter.c
 * @brief Directional G-meter with ring / target display and moving dot.
 *
 * Shows a 2D target with concentric rings and crosshair lines.
 * A dot moves in real-time to show lateral (left/right) and
 * longitudinal (forward/backward) G forces.  Per-direction peak
 * values are displayed.  Screen timeout is disabled while active.
 */

#include "screen_gmeter.h"
#include "screen_gmeter_cal.h"
#include "ui_common.h"
#include "screen_manager.h"
#include "G_Meter.h"
#include "esp_log.h"

#include <stdio.h>
#include <math.h>

static const char *TAG = "scr_gm";

/***********************
 *  CONFIGURATION
 ***********************/
#define TARGET_SIZE      320
#define TARGET_HALF      (TARGET_SIZE / 2)  /* 160 */

/* Maximum G range shown on each axis (±1.5 G) */
#define G_RANGE          1.5f

/* Concentric rings: 0.5, 1.0, 1.5 G */
#define NUM_RINGS        3

#define DOT_SIZE         16
#define CROSS_LINE_W     1

/* Trail: 5 seconds at 100 ms update = 50 trail samples */
#define TRAIL_LEN        50
#define TRAIL_DOT_SIZE   8

/***********************
 *  STATIC VARIABLES
 ***********************/
static lv_obj_t   *container     = NULL;
static lv_obj_t   *target_area   = NULL;
static lv_obj_t   *dot           = NULL;
static lv_obj_t   *trail_dots[TRAIL_LEN] = {NULL};
static int         trail_idx     = 0;
static lv_obj_t   *lbl_lat       = NULL;
static lv_obj_t   *lbl_lon       = NULL;
static lv_obj_t   *lbl_max_l     = NULL;
static lv_obj_t   *lbl_max_r     = NULL;
static lv_obj_t   *lbl_max_f     = NULL;
static lv_obj_t   *lbl_max_b     = NULL;
static lv_obj_t   *reset_btn     = NULL;
static lv_timer_t *update_timer  = NULL;

/* Edge labels */
static lv_obj_t   *lbl_left      = NULL;
static lv_obj_t   *lbl_right     = NULL;
static lv_obj_t   *lbl_accel     = NULL;
static lv_obj_t   *lbl_brake     = NULL;

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void update_timer_cb(lv_timer_t *timer);
static void reset_btn_event_cb(lv_event_t *e);
static void create_rings(lv_obj_t *parent);
static void create_crosshairs(lv_obj_t *parent);

/***********************
 *  HELPER: map G value to pixel offset from centre
 ***********************/
static lv_coord_t g_to_px(float g_val)
{
    if (g_val >  G_RANGE) g_val =  G_RANGE;
    if (g_val < -G_RANGE) g_val = -G_RANGE;
    return (lv_coord_t)(g_val / G_RANGE * TARGET_HALF);
}

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void update_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    float lat = G_Meter_GetLateral();
    float lon = G_Meter_GetLongitudinal();

    /* Move the dot */
    if (dot && target_area) {
        lv_coord_t px = g_to_px(lat);
        lv_coord_t py = g_to_px(-lon);  /* negate: forward = up */

        lv_coord_t cx = TARGET_HALF + px - DOT_SIZE / 2;
        lv_coord_t cy = TARGET_HALF + py - DOT_SIZE / 2;

        float res = sqrtf(lat * lat + lon * lon);
        lv_color_t col = COLOR_TEMP_NORMAL;
        if (res >= 1.2f)      col = COLOR_TEMP_CRITICAL;
        else if (res >= 0.8f) col = COLOR_TEMP_WARNING;

        /* --- Trail --- */
        if (g_trail_enabled) {
            lv_obj_t *td = trail_dots[trail_idx % TRAIL_LEN];
            if (td) {
                lv_coord_t tcx = TARGET_HALF + px - TRAIL_DOT_SIZE / 2;
                lv_coord_t tcy = TARGET_HALF + py - TRAIL_DOT_SIZE / 2;
                lv_obj_set_pos(td, tcx, tcy);
                lv_obj_set_style_bg_color(td, col, 0);
                lv_obj_set_style_bg_opa(td, LV_OPA_COVER, 0);
                lv_obj_clear_flag(td, LV_OBJ_FLAG_HIDDEN);
            }
            trail_idx = (trail_idx + 1) % TRAIL_LEN;

            /* Age all trail dots */
            for (int i = 0; i < TRAIL_LEN; i++) {
                if (!trail_dots[i]) continue;
                int age = (trail_idx - 1 - i) % TRAIL_LEN;
                if (age < 0) age += TRAIL_LEN;
                if (age >= TRAIL_LEN) {
                    lv_obj_add_flag(trail_dots[i], LV_OBJ_FLAG_HIDDEN);
                } else {
                    lv_opa_t opa = (lv_opa_t)(LV_OPA_COVER * (TRAIL_LEN - age) / TRAIL_LEN);
                    if (opa < LV_OPA_10) {
                        lv_obj_add_flag(trail_dots[i], LV_OBJ_FLAG_HIDDEN);
                    } else {
                        lv_obj_set_style_bg_opa(trail_dots[i], opa, 0);
                    }
                }
            }
        } else {
            /* Trail off — hide all trail dots */
            for (int i = 0; i < TRAIL_LEN; i++) {
                if (trail_dots[i]) {
                    lv_obj_add_flag(trail_dots[i], LV_OBJ_FLAG_HIDDEN);
                }
            }
        }

        /* Move main dot (always on top via creation order) */
        lv_obj_set_pos(dot, cx, cy);
        lv_obj_set_style_bg_color(dot, col, 0);
    }

    /* Live axis values */
    if (lbl_lat) {
        char buf[32];
        snprintf(buf, sizeof(buf), "L/R: %+.2fG", lat);
        lv_label_set_text(lbl_lat, buf);
    }
    if (lbl_lon) {
        char buf[32];
        snprintf(buf, sizeof(buf), "F/B: %+.2fG", lon);
        lv_label_set_text(lbl_lon, buf);
    }

    /* Per-direction max labels */
    if (lbl_max_l) {
        char buf[24];
        snprintf(buf, sizeof(buf), "L %.2f", G_Meter_GetMaxLeft());
        lv_label_set_text(lbl_max_l, buf);
    }
    if (lbl_max_r) {
        char buf[24];
        snprintf(buf, sizeof(buf), "R %.2f", G_Meter_GetMaxRight());
        lv_label_set_text(lbl_max_r, buf);
    }
    if (lbl_max_f) {
        char buf[24];
        snprintf(buf, sizeof(buf), "F %.2f", G_Meter_GetMaxForward());
        lv_label_set_text(lbl_max_f, buf);
    }
    if (lbl_max_b) {
        char buf[24];
        snprintf(buf, sizeof(buf), "B %.2f", G_Meter_GetMaxBrake());
        lv_label_set_text(lbl_max_b, buf);
    }
}

static void reset_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_PRESSED || code == LV_EVENT_RELEASED) {
        screen_manager_reset_inactivity();
    }
    if (code == LV_EVENT_CLICKED) {
        ESP_LOGI(TAG, "Reset all max G");
        G_Meter_ResetMax();
    }
}

/* ---- Concentric rings ---- */
static void create_rings(lv_obj_t *parent)
{
    for (int i = 1; i <= NUM_RINGS; i++) {
        lv_coord_t diam = (lv_coord_t)(TARGET_SIZE * i / NUM_RINGS);

        lv_obj_t *ring = lv_obj_create(parent);
        lv_obj_remove_style_all(ring);
        lv_obj_set_size(ring, diam, diam);
        lv_obj_set_style_radius(ring, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_opa(ring, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ring, 1, 0);
        lv_obj_set_style_border_opa(ring, LV_OPA_60, 0);

        lv_color_t col;
        if (i == 1)      col = COLOR_TEMP_NORMAL;    /* 0.5 G */
        else if (i == 2) col = COLOR_TEMP_WARNING;   /* 1.0 G */
        else             col = COLOR_TEMP_CRITICAL;  /* 1.5 G */
        lv_obj_set_style_border_color(ring, col, 0);

        lv_obj_center(ring);
        lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(ring, LV_OBJ_FLAG_GESTURE_BUBBLE);
    }
}

/* ---- Crosshair lines ---- */
static void create_crosshairs(lv_obj_t *parent)
{
    static lv_point_t h_pts[2];
    h_pts[0].x = 0;           h_pts[0].y = TARGET_HALF;
    h_pts[1].x = TARGET_SIZE; h_pts[1].y = TARGET_HALF;

    lv_obj_t *h_line = lv_line_create(parent);
    lv_line_set_points(h_line, h_pts, 2);
    lv_obj_set_style_line_width(h_line, CROSS_LINE_W, 0);
    lv_obj_set_style_line_color(h_line, COLOR_TEXT_LABEL, 0);
    lv_obj_set_style_line_opa(h_line, LV_OPA_80, 0);
    lv_obj_set_pos(h_line, 0, 0);
    lv_obj_clear_flag(h_line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(h_line, LV_OBJ_FLAG_GESTURE_BUBBLE);

    static lv_point_t v_pts[2];
    v_pts[0].x = TARGET_HALF; v_pts[0].y = 0;
    v_pts[1].x = TARGET_HALF; v_pts[1].y = TARGET_SIZE;

    lv_obj_t *v_line = lv_line_create(parent);
    lv_line_set_points(v_line, v_pts, 2);
    lv_obj_set_style_line_width(v_line, CROSS_LINE_W, 0);
    lv_obj_set_style_line_color(v_line, COLOR_TEXT_LABEL, 0);
    lv_obj_set_style_line_opa(v_line, LV_OPA_80, 0);
    lv_obj_set_pos(v_line, 0, 0);
    lv_obj_clear_flag(v_line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(v_line, LV_OBJ_FLAG_GESTURE_BUBBLE);
}

/* ============================================================
 *  PUBLIC API
 * ============================================================ */

lv_obj_t *screen_gmeter_create(lv_obj_t *parent)
{
    const ui_fonts_t *fonts = ui_common_get_fonts();

    /* ===== Container (full-screen, no title) ===== */
    container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    /* ===== Target area (ring/crosshair container) ===== */
    target_area = lv_obj_create(container);
    lv_obj_remove_style_all(target_area);
    lv_obj_set_size(target_area, TARGET_SIZE, TARGET_SIZE);
    lv_obj_set_style_bg_opa(target_area, LV_OPA_TRANSP, 0);
    lv_obj_align(target_area, LV_ALIGN_CENTER, 0, -30);
    lv_obj_clear_flag(target_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(target_area, LV_OBJ_FLAG_GESTURE_BUBBLE);

    create_rings(target_area);
    create_crosshairs(target_area);

    /* ===== Trail dots (created before main dot so they render behind) ===== */
    for (int i = 0; i < TRAIL_LEN; i++) {
        trail_dots[i] = lv_obj_create(target_area);
        lv_obj_remove_style_all(trail_dots[i]);
        lv_obj_set_size(trail_dots[i], TRAIL_DOT_SIZE, TRAIL_DOT_SIZE);
        lv_obj_set_style_radius(trail_dots[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(trail_dots[i], COLOR_TEMP_NORMAL, 0);
        lv_obj_set_style_bg_opa(trail_dots[i], LV_OPA_TRANSP, 0);
        lv_obj_clear_flag(trail_dots[i], LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(trail_dots[i], LV_OBJ_FLAG_HIDDEN | LV_OBJ_FLAG_GESTURE_BUBBLE);
        lv_obj_set_pos(trail_dots[i], TARGET_HALF - TRAIL_DOT_SIZE / 2, TARGET_HALF - TRAIL_DOT_SIZE / 2);
    }
    trail_idx = 0;

    /* Edge labels */
    lbl_left = lv_label_create(container);
    lv_label_set_text(lbl_left, "L");
    lv_obj_set_style_text_color(lbl_left, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(lbl_left, fonts->small, 0);
    lv_obj_align_to(lbl_left, target_area, LV_ALIGN_OUT_LEFT_MID, -6, 0);

    lbl_right = lv_label_create(container);
    lv_label_set_text(lbl_right, "R");
    lv_obj_set_style_text_color(lbl_right, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(lbl_right, fonts->small, 0);
    lv_obj_align_to(lbl_right, target_area, LV_ALIGN_OUT_RIGHT_MID, 6, 0);

    lbl_accel = lv_label_create(container);
    lv_label_set_text(lbl_accel, LV_SYMBOL_UP);
    lv_obj_set_style_text_color(lbl_accel, COLOR_TEMP_NORMAL, 0);
    lv_obj_set_style_text_font(lbl_accel, fonts->small, 0);
    lv_obj_align_to(lbl_accel, target_area, LV_ALIGN_OUT_TOP_MID, 0, -4);

    lbl_brake = lv_label_create(container);
    lv_label_set_text(lbl_brake, LV_SYMBOL_DOWN);
    lv_obj_set_style_text_color(lbl_brake, COLOR_TEMP_CRITICAL, 0);
    lv_obj_set_style_text_font(lbl_brake, fonts->small, 0);
    lv_obj_align_to(lbl_brake, target_area, LV_ALIGN_OUT_BOTTOM_MID, 0, 4);

    /* ===== Moving dot ===== */
    dot = lv_obj_create(target_area);
    lv_obj_remove_style_all(dot);
    lv_obj_set_size(dot, DOT_SIZE, DOT_SIZE);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, COLOR_TEMP_NORMAL, 0);
    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(dot, 2, 0);
    lv_obj_set_style_border_color(dot, COLOR_TEXT_PRIMARY, 0);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(dot, LV_OBJ_FLAG_GESTURE_BUBBLE);
    lv_obj_set_pos(dot, TARGET_HALF - DOT_SIZE / 2, TARGET_HALF - DOT_SIZE / 2);

    /* ===== Live values row ===== */
    lv_obj_t *live_row = lv_obj_create(container);
    lv_obj_remove_style_all(live_row);
    lv_obj_set_size(live_row, 300, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(live_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(live_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(live_row, LV_ALIGN_BOTTOM_MID, 0, -85);
    lv_obj_clear_flag(live_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(live_row, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lbl_lat = lv_label_create(live_row);
    lv_label_set_text(lbl_lat, "L/R: +0.00G");
    lv_obj_set_style_text_color(lbl_lat, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl_lat, fonts->small, 0);

    lbl_lon = lv_label_create(live_row);
    lv_label_set_text(lbl_lon, "F/B: +0.00G");
    lv_obj_set_style_text_color(lbl_lon, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl_lon, fonts->small, 0);

    /* ===== Max values row (L / R / F / B) ===== */
    lv_obj_t *max_row = lv_obj_create(container);
    lv_obj_remove_style_all(max_row);
    lv_obj_set_size(max_row, 360, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(max_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(max_row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_align(max_row, LV_ALIGN_BOTTOM_MID, 0, -60);
    lv_obj_clear_flag(max_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(max_row, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lbl_max_l = lv_label_create(max_row);
    lv_label_set_text(lbl_max_l, "L 0.00");
    lv_obj_set_style_text_color(lbl_max_l, COLOR_TEMP_WARNING, 0);
    lv_obj_set_style_text_font(lbl_max_l, fonts->small, 0);

    lbl_max_r = lv_label_create(max_row);
    lv_label_set_text(lbl_max_r, "R 0.00");
    lv_obj_set_style_text_color(lbl_max_r, COLOR_TEMP_WARNING, 0);
    lv_obj_set_style_text_font(lbl_max_r, fonts->small, 0);

    lbl_max_f = lv_label_create(max_row);
    lv_label_set_text(lbl_max_f, "F 0.00");
    lv_obj_set_style_text_color(lbl_max_f, COLOR_TEMP_WARNING, 0);
    lv_obj_set_style_text_font(lbl_max_f, fonts->small, 0);

    lbl_max_b = lv_label_create(max_row);
    lv_label_set_text(lbl_max_b, "B 0.00");
    lv_obj_set_style_text_color(lbl_max_b, COLOR_TEMP_WARNING, 0);
    lv_obj_set_style_text_font(lbl_max_b, fonts->small, 0);

    /* ===== Reset button ===== */
    reset_btn = lv_btn_create(container);
    lv_obj_set_size(reset_btn, 140, 36);
    lv_obj_set_style_bg_color(reset_btn, COLOR_ACCENT, 0);
    lv_obj_set_style_bg_color(reset_btn, lv_color_hex(0x0099CC), LV_STATE_PRESSED);
    lv_obj_set_style_radius(reset_btn, 10, 0);
    lv_obj_align(reset_btn, LV_ALIGN_BOTTOM_MID, 0, -22);
    lv_obj_add_event_cb(reset_btn, reset_btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *btn_label = lv_label_create(reset_btn);
    lv_label_set_text(btn_label, "RESET MAX");
    lv_obj_set_style_text_color(btn_label, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(btn_label, fonts->small, 0);
    lv_obj_center(btn_label);

    /* ===== Update timer (100 ms) ===== */
    update_timer = lv_timer_create(update_timer_cb, 100, NULL);

    return container;
}

void screen_gmeter_destroy(void)
{
    if (update_timer) {
        lv_timer_del(update_timer);
        update_timer = NULL;
    }

    if (container) {
        lv_obj_del(container);
        container = NULL;
    }

    target_area  = NULL;
    dot          = NULL;
    for (int i = 0; i < TRAIL_LEN; i++) trail_dots[i] = NULL;
    trail_idx    = 0;
    lbl_lat      = NULL;
    lbl_lon      = NULL;
    lbl_max_l    = NULL;
    lbl_max_r    = NULL;
    lbl_max_f    = NULL;
    lbl_max_b    = NULL;
    reset_btn    = NULL;
    lbl_left     = NULL;
    lbl_right    = NULL;
    lbl_accel    = NULL;
    lbl_brake    = NULL;
}

void screen_gmeter_show(void)
{
    /* Disable screen timeout while on G-meter */
    screen_manager_pause_inactivity();
    if (update_timer) lv_timer_resume(update_timer);
}

void screen_gmeter_hide(void)
{
    if (update_timer) lv_timer_pause(update_timer);
    /* Re-enable screen timeout when leaving */
    screen_manager_resume_inactivity();
}
