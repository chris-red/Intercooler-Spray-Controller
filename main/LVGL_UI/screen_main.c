#include "screen_main.h"
#include "ui_common.h"
#include "screen_manager.h"
#include "PCF85063.h"

/***********************
 *  STATIC VARIABLES
 ***********************/
static lv_obj_t *container = NULL;
static lv_obj_t *lbl_temperature = NULL;
static lv_obj_t *lbl_time = NULL;
static lv_obj_t *led_relay_active = NULL;
static lv_obj_t *led_tank_empty = NULL;
static lv_timer_t *update_timer = NULL;

/***********************
 *  STATIC PROTOTYPES
 ***********************/
static void update_timer_cb(lv_timer_t *timer);

/***********************
 *  IMPLEMENTATIONS
 ***********************/

static void update_timer_cb(lv_timer_t *timer)
{
    screen_main_update_time();
}

lv_obj_t *screen_main_create(lv_obj_t *parent)
{
    const ui_fonts_t *fonts = ui_common_get_fonts();
    
    // ===== Create main container =====
    container = lv_obj_create(parent);
    lv_obj_set_size(container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 1, 0);
    lv_obj_set_flex_flow(container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(container, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling - no scrollable content
    lv_obj_add_flag(container, LV_OBJ_FLAG_GESTURE_BUBBLE);  // Allow gestures to bubble up

    // ===== TOP SECTION: Temperature Display =====
    lv_obj_t *temp_section = lv_obj_create(container);
    lv_obj_set_size(temp_section, LV_PCT(100), LV_PCT(75));
    lv_obj_set_style_bg_color(temp_section, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(temp_section, 0, 0);
    lv_obj_set_style_pad_all(temp_section, 0, 0);
    lv_obj_set_flex_flow(temp_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(temp_section, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(temp_section, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling
    lv_obj_add_flag(temp_section, LV_OBJ_FLAG_GESTURE_BUBBLE);  // Allow gestures to bubble up

    // Temperature value (large number)
    lbl_temperature = lv_label_create(temp_section);
    lv_label_set_text(lbl_temperature, "88°");
    lv_obj_set_style_text_color(lbl_temperature, COLOR_TEMP_NORMAL, 0);
    lv_obj_set_style_text_font(lbl_temperature, fonts->temp, 0);
    lv_obj_set_size(lbl_temperature, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_top(lbl_temperature, 50, 0);

    // ===== BOTTOM SECTION: Relay | Time | Tank (3 columns) =====
    lv_obj_t *bottom_section = lv_obj_create(container);
    lv_obj_set_size(bottom_section, LV_PCT(100), LV_PCT(25));
    lv_obj_set_style_bg_color(bottom_section, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(bottom_section, 0, 0);
    lv_obj_set_style_pad_all(bottom_section, 2, 0);
    lv_obj_set_flex_flow(bottom_section, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_section, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(bottom_section, LV_OBJ_FLAG_SCROLLABLE);  // Disable scrolling
    lv_obj_add_flag(bottom_section, LV_OBJ_FLAG_GESTURE_BUBBLE);  // Allow gestures to bubble up

    // --- Relay Indicator (left third) ---
    lv_obj_t *relay_container = lv_obj_create(bottom_section);
    lv_obj_set_size(relay_container, LV_PCT(33), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(relay_container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(relay_container, 0, 0);
    lv_obj_set_style_pad_all(relay_container, 2, 0);
    lv_obj_set_flex_flow(relay_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(relay_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(relay_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(relay_container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    led_relay_active = lv_led_create(relay_container);
    lv_obj_set_size(led_relay_active, 35, 35);
    lv_led_set_color(led_relay_active, COLOR_RELAY_ACTIVE);
    lv_led_off(led_relay_active);

    lv_obj_t *relay_label = lv_label_create(relay_container);
    lv_label_set_text(relay_label, "Relay");
    lv_obj_set_style_text_color(relay_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(relay_label, fonts->small, 0);

    // --- Time Display (middle third) ---
    lv_obj_t *time_section = lv_obj_create(bottom_section);
    lv_obj_set_size(time_section, LV_PCT(33), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(time_section, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(time_section, 0, 0);
    lv_obj_set_style_pad_all(time_section, 2, 0);
    lv_obj_set_flex_flow(time_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(time_section, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(time_section, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(time_section, LV_OBJ_FLAG_GESTURE_BUBBLE);

    lbl_time = lv_label_create(time_section);
    lv_label_set_text(lbl_time, "00:00");
    lv_obj_set_style_text_color(lbl_time, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl_time, fonts->large, 0);

    // --- Tank Empty Indicator (right third) ---
    lv_obj_t *tank_container = lv_obj_create(bottom_section);
    lv_obj_set_size(tank_container, LV_PCT(33), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(tank_container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(tank_container, 0, 0);
    lv_obj_set_style_pad_all(tank_container, 2, 0);
    lv_obj_set_flex_flow(tank_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tank_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(tank_container, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(tank_container, LV_OBJ_FLAG_GESTURE_BUBBLE);

    led_tank_empty = lv_led_create(tank_container);
    lv_obj_set_size(led_tank_empty, 35, 35);
    lv_led_set_color(led_tank_empty, COLOR_TANK_EMPTY);
    lv_led_off(led_tank_empty);

    lv_obj_t *tank_label = lv_label_create(tank_container);
    lv_label_set_text(tank_label, "Tank");
    lv_obj_set_style_text_color(tank_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(tank_label, fonts->small, 0);

    // Create timer to update time every 1 second
    update_timer = lv_timer_create(update_timer_cb, 1000, NULL);

    // Initial update
    screen_main_update_time();
    
    return container;
}

void screen_main_destroy(void)
{
    if (update_timer) {
        lv_timer_del(update_timer);
        update_timer = NULL;
    }
    
    if (container) {
        lv_obj_del(container);
        container = NULL;
    }
    
    lbl_temperature = NULL;
    lbl_time = NULL;
    led_relay_active = NULL;
    led_tank_empty = NULL;
}

void screen_main_show(void)
{
    // Resume timer
    if (update_timer) {
        lv_timer_resume(update_timer);
    }
}

void screen_main_hide(void)
{
    // Pause timer when not visible
    if (update_timer) {
        lv_timer_pause(update_timer);
    }
}

void screen_main_update_temperature(float temp_celsius)
{
    if (!lbl_temperature) return;

    char temp_str[20];
    snprintf(temp_str, sizeof(temp_str), "%.1f°", temp_celsius);

    lv_color_t color = COLOR_TEMP_NORMAL;
    if (temp_celsius >= 50.0f) {
        color = COLOR_TEMP_CRITICAL;
    } else if (temp_celsius >= 40.0f) {
        color = COLOR_TEMP_WARNING;
    }

    ui_common_set_label_color(lbl_temperature, temp_str, color);
}

void screen_main_update_time(void)
{
    if (!lbl_time) return;

    char time_str[20];
    snprintf(time_str, sizeof(time_str), "%02d:%02d",
             datetime.hour, datetime.minute);

    ui_common_set_label_color(lbl_time, time_str, COLOR_TEXT_PRIMARY);
}

void screen_main_set_tank_empty(bool is_empty)
{
    if (!led_tank_empty) return;

    if (is_empty) {
        lv_led_on(led_tank_empty);
    } else {
        lv_led_off(led_tank_empty);
    }
}

void screen_main_set_relay_active(bool is_active)
{
    if (!led_relay_active) return;

    if (is_active) {
        lv_led_on(led_relay_active);
    } else {
        lv_led_off(led_relay_active);
    }
}
