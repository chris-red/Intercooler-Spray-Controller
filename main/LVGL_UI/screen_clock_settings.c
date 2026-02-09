#include "screen_clock_settings.h"
#include "ui_common.h"
#include "screen_manager.h"
#include "PCF85063.h"
#include <stdio.h>

/***********************
 *  STATIC VARIABLES
 ***********************/
static lv_obj_t *container = NULL;
static lv_obj_t *hour_roller = NULL;
static lv_obj_t *minute_roller = NULL;
static lv_obj_t *day_roller = NULL;
static lv_obj_t *month_roller = NULL;
static lv_obj_t *year_roller = NULL;
static lv_obj_t *set_button = NULL;

// Current time values
static datetime_t current_time;

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void set_button_event_cb(lv_event_t *e);
static void roller_event_cb(lv_event_t *e);

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void set_button_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_CLICKED) {
        screen_manager_reset_inactivity();
        
        // Get values from rollers
        current_time.hour = lv_roller_get_selected(hour_roller);
        current_time.minute = lv_roller_get_selected(minute_roller);
        current_time.day = lv_roller_get_selected(day_roller) + 1;  // Days are 1-31
        current_time.month = lv_roller_get_selected(month_roller) + 1;  // Months are 1-12
        current_time.year = 2025 + lv_roller_get_selected(year_roller);  // 2025 + (0-25)
        current_time.second = 0;  // Reset seconds when setting time
        
        // Set the time in the RTC
        PCF85063_Set_All(current_time);
        
        // Visual feedback - briefly change button color
        lv_obj_set_style_bg_color(set_button, lv_color_hex(0x00AA00), 0);
        lv_obj_invalidate(set_button);
    }
    else if (code == LV_EVENT_PRESSED) {
        screen_manager_reset_inactivity();
    }
}

static void roller_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    if (code == LV_EVENT_VALUE_CHANGED || code == LV_EVENT_PRESSED) {
        screen_manager_reset_inactivity();
    }
}

lv_obj_t *screen_clock_settings_create(lv_obj_t *parent)
{
    const ui_fonts_t *fonts = ui_common_get_fonts();
    
    // Read current time from RTC
    PCF85063_Read_Time(&current_time);
    
    // ===== Create container =====
    container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 10, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_add_flag(container, LV_OBJ_FLAG_HIDDEN);  // Hidden by default
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    // ===== Title =====
    lv_obj_t *title = lv_label_create(container);
    lv_label_set_text(title, "SET CLOCK");
    lv_obj_set_style_text_font(title, fonts->large, 0);
    lv_obj_set_style_text_color(title, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_pad_bottom(title, 20, 0);

    // ===== All Rollers Section (Hour:Minute Day/Month/Year) =====
    lv_obj_t *roller_container = lv_obj_create(container);
    lv_obj_set_size(roller_container, LV_PCT(95), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(roller_container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(roller_container, 0, 0);
    lv_obj_set_style_pad_all(roller_container, 2, 0);
    lv_obj_set_flex_flow(roller_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(roller_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(roller_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(roller_container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    // Hour roller (0-23)
    hour_roller = lv_roller_create(roller_container);
    lv_roller_set_options(hour_roller, 
        "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n"
        "12\n13\n14\n15\n16\n17\n18\n19\n20\n21\n22\n23",
        LV_ROLLER_MODE_NORMAL);
    lv_obj_set_width(hour_roller, 65);
    lv_roller_set_visible_row_count(hour_roller, 2);
    lv_roller_set_selected(hour_roller, current_time.hour, LV_ANIM_OFF);
    lv_obj_set_style_text_font(hour_roller, fonts->large, 0);
    lv_obj_set_style_bg_color(hour_roller, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_text_color(hour_roller, COLOR_TEXT_PRIMARY, 0);
    lv_obj_add_event_cb(hour_roller, roller_event_cb, LV_EVENT_ALL, NULL);

    // Colon separator
    lv_obj_t *colon1 = lv_label_create(roller_container);
    lv_label_set_text(colon1, ":");
    lv_obj_set_style_text_font(colon1, fonts->normal, 0);
    lv_obj_set_style_text_color(colon1, COLOR_TEXT_PRIMARY, 0);

    // Minute roller (0-59)
    minute_roller = lv_roller_create(roller_container);
    char minute_options[300];
    char *ptr = minute_options;
    for (int i = 0; i < 60; i++) {
        ptr += sprintf(ptr, "%s%02d", (i > 0) ? "\n" : "", i);
    }
    lv_roller_set_options(minute_roller, minute_options, LV_ROLLER_MODE_NORMAL);
    lv_obj_set_width(minute_roller, 65);
    lv_roller_set_visible_row_count(minute_roller, 2);
    lv_roller_set_selected(minute_roller, current_time.minute, LV_ANIM_OFF);
    lv_obj_set_style_text_font(minute_roller, fonts->large, 0);
    lv_obj_set_style_bg_color(minute_roller, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_text_color(minute_roller, COLOR_TEXT_PRIMARY, 0);
    lv_obj_add_event_cb(minute_roller, roller_event_cb, LV_EVENT_ALL, NULL);

    // Space separator
    lv_obj_t *space1 = lv_label_create(roller_container);
    lv_label_set_text(space1, " ");
    lv_obj_set_style_text_font(space1, fonts->small, 0);

    // Day roller (1-31)
    day_roller = lv_roller_create(roller_container);
    char day_options[200];
    ptr = day_options;
    for (int i = 1; i <= 31; i++) {
        ptr += sprintf(ptr, "%s%02d", (i > 1) ? "\n" : "", i);
    }
    lv_roller_set_options(day_roller, day_options, LV_ROLLER_MODE_NORMAL);
    lv_obj_set_width(day_roller, 65);
    lv_roller_set_visible_row_count(day_roller, 2);
    lv_roller_set_selected(day_roller, current_time.day - 1, LV_ANIM_OFF);
    lv_obj_set_style_text_font(day_roller, fonts->large, 0);
    lv_obj_set_style_bg_color(day_roller, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_text_color(day_roller, COLOR_TEXT_PRIMARY, 0);
    lv_obj_add_event_cb(day_roller, roller_event_cb, LV_EVENT_ALL, NULL);

    // Slash separator
    lv_obj_t *slash1 = lv_label_create(roller_container);
    lv_label_set_text(slash1, "/");
    lv_obj_set_style_text_font(slash1, fonts->small, 0);
    lv_obj_set_style_text_color(slash1, COLOR_TEXT_PRIMARY, 0);

    // Month roller (1-12)
    month_roller = lv_roller_create(roller_container);
    lv_roller_set_options(month_roller, 
        "01\n02\n03\n04\n05\n06\n07\n08\n09\n10\n11\n12",
        LV_ROLLER_MODE_NORMAL);
    lv_obj_set_width(month_roller, 65);
    lv_roller_set_visible_row_count(month_roller, 2);
    lv_roller_set_selected(month_roller, current_time.month - 1, LV_ANIM_OFF);
    lv_obj_set_style_text_font(month_roller, fonts->large, 0);
    lv_obj_set_style_bg_color(month_roller, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_text_color(month_roller, COLOR_TEXT_PRIMARY, 0);
    lv_obj_add_event_cb(month_roller, roller_event_cb, LV_EVENT_ALL, NULL);

    // Slash separator
    lv_obj_t *slash2 = lv_label_create(roller_container);
    lv_label_set_text(slash2, "/");
    lv_obj_set_style_text_font(slash2, fonts->small, 0);
    lv_obj_set_style_text_color(slash2, COLOR_TEXT_PRIMARY, 0);

    // Year roller (25-50 for 2025-2050)
    year_roller = lv_roller_create(roller_container);
    char year_options[200];
    ptr = year_options;
    for (int i = 25; i <= 50; i++) {
        ptr += sprintf(ptr, "%s%02d", (i > 25) ? "\n" : "", i);
    }
    lv_roller_set_options(year_roller, year_options, LV_ROLLER_MODE_NORMAL);
    lv_obj_set_width(year_roller, 65);
    lv_roller_set_visible_row_count(year_roller, 2);
    int year_index = (current_time.year >= 2025 && current_time.year <= 2050) ? 
                     (current_time.year - 2025) : 0;
    lv_roller_set_selected(year_roller, year_index, LV_ANIM_OFF);
    lv_obj_set_style_text_font(year_roller, fonts->large, 0);
    lv_obj_set_style_bg_color(year_roller, lv_color_hex(0x0D0D1A), 0);
    lv_obj_set_style_text_color(year_roller, COLOR_TEXT_PRIMARY, 0);
    lv_obj_add_event_cb(year_roller, roller_event_cb, LV_EVENT_ALL, NULL);

    // Spacer between rollers and button
    lv_obj_t *spacer = lv_obj_create(container);
    lv_obj_set_size(spacer, 1, 20);
    lv_obj_set_style_bg_opa(spacer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spacer, 0, 0);

    // ===== Set Button =====
    set_button = lv_btn_create(container);
    lv_obj_set_size(set_button, 140, 50);
    lv_obj_set_style_bg_color(set_button, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(set_button, 6, 0);
    lv_obj_set_style_pad_all(set_button, 5, 0);
    lv_obj_add_event_cb(set_button, set_button_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t *btn_label = lv_label_create(set_button);
    lv_label_set_text(btn_label, "SET TIME");
    lv_obj_set_style_text_font(btn_label, fonts->normal, 0);
    lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
    lv_obj_center(btn_label);

    return container;
}

void screen_clock_settings_destroy(void)
{
    if (container) {
        lv_obj_del(container);
        container = NULL;
        hour_roller = NULL;
        minute_roller = NULL;
        day_roller = NULL;
        month_roller = NULL;
        year_roller = NULL;
        set_button = NULL;
    }
}

void screen_clock_settings_update_ui(void)
{
    // Read current time and update rollers
    PCF85063_Read_Time(&current_time);
    
    if (hour_roller) {
        lv_roller_set_selected(hour_roller, current_time.hour, LV_ANIM_OFF);
    }
    if (minute_roller) {
        lv_roller_set_selected(minute_roller, current_time.minute, LV_ANIM_OFF);
    }
    if (day_roller) {
        lv_roller_set_selected(day_roller, current_time.day - 1, LV_ANIM_OFF);
    }
    if (month_roller) {
        lv_roller_set_selected(month_roller, current_time.month - 1, LV_ANIM_OFF);
    }
    if (year_roller && current_time.year >= 2025 && current_time.year <= 2050) {
        lv_roller_set_selected(year_roller, current_time.year - 2025, LV_ANIM_OFF);
    }
}
