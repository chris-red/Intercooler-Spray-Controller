#include "intercooler_ui.h"

/***********************
 *  STATIC VARIABLES
 ***********************/

// UI objects
static lv_obj_t *lbl_temperature = NULL;
static lv_obj_t *lbl_time = NULL;
static lv_obj_t *led_relay_active = NULL;
static lv_obj_t *led_tank_empty = NULL;

// For updating the time periodically
static lv_timer_t *update_timer = NULL;

// Font references
static const lv_font_t *font_large = NULL;
static const lv_font_t *font_normal = NULL;
static const lv_font_t *font_small = NULL;

/***********************
 *  STATIC PROTOTYPES
 ***********************/

/**
 * Timer callback to update time display every second
 */
static void update_timer_cb(lv_timer_t *timer);

/**
 * Helper function to set label text and color
 */
static void label_set_text_color(lv_obj_t *label, const char *text, lv_color_t color);

/***********************
 *  FUNCTION IMPLEMENTATIONS
 ***********************/

void intercooler_ui_create(void)
{
    // Set up fonts - use available fonts from configuration
    font_large = &lv_font_montserrat_48;   // Large font for temperature (most screen space)
    font_normal = &lv_font_montserrat_12;  // Normal font for time
    font_small = &lv_font_montserrat_12;   // Small font for labels

    // Get screen and clear it
    lv_obj_t *screen = lv_scr_act();
    lv_obj_set_style_bg_color(screen, COLOR_BG_PRIMARY, 0);

    // ===== Create main container =====
    lv_obj_t *main_container = lv_obj_create(screen);
    lv_obj_set_size(main_container, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(main_container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(main_container, 0, 0);
    lv_obj_set_style_pad_all(main_container, 1, 0);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(main_container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // ===== TOP SECTION: Temperature Display =====
    lv_obj_t *temp_section = lv_obj_create(main_container);
    lv_obj_set_size(temp_section, LV_PCT(100), LV_PCT(75));
    lv_obj_set_style_bg_color(temp_section, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(temp_section, 0, 0);
    lv_obj_set_style_pad_all(temp_section, 0, 0);
    lv_obj_set_flex_flow(temp_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(temp_section, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Temperature value (large number) - no label
    lbl_temperature = lv_label_create(temp_section);
    lv_label_set_text(lbl_temperature, "12 °c");
    lv_obj_set_style_text_color(lbl_temperature, COLOR_TEMP_NORMAL, 0);
    lv_obj_set_style_text_font(lbl_temperature, font_large, 0);
    // Make the label itself larger to accommodate bigger text
    lv_obj_set_size(lbl_temperature, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

    // ===== BOTTOM SECTION: Relay | Time | Tank (3 columns) =====
    lv_obj_t *bottom_section = lv_obj_create(main_container);
    lv_obj_set_size(bottom_section, LV_PCT(100), LV_PCT(25));
    lv_obj_set_style_bg_color(bottom_section, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(bottom_section, 0, 0);
    lv_obj_set_style_pad_all(bottom_section, 2, 0);
    lv_obj_set_flex_flow(bottom_section, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottom_section, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // --- Relay Indicator (left third) ---
    lv_obj_t *relay_container = lv_obj_create(bottom_section);
    lv_obj_set_size(relay_container, LV_PCT(33), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(relay_container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(relay_container, 0, 0);
    lv_obj_set_style_pad_all(relay_container, 2, 0);
    lv_obj_set_flex_flow(relay_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(relay_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Relay LED indicator
    led_relay_active = lv_led_create(relay_container);
    lv_obj_set_size(led_relay_active, 35, 35);
    lv_led_set_color(led_relay_active, COLOR_RELAY_ACTIVE);
    lv_led_off(led_relay_active);

    // Relay label
    lv_obj_t *relay_label = lv_label_create(relay_container);
    lv_label_set_text(relay_label, "Relay");
    lv_obj_set_style_text_color(relay_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(relay_label, font_small, 0);

    // --- Time Display (middle third) ---
    lv_obj_t *time_section = lv_obj_create(bottom_section);
    lv_obj_set_size(time_section, LV_PCT(33), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(time_section, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(time_section, 0, 0);
    lv_obj_set_style_pad_all(time_section, 2, 0);
    lv_obj_set_flex_flow(time_section, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(time_section, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Time display
    lbl_time = lv_label_create(time_section);
    lv_label_set_text(lbl_time, "00:00");
    lv_obj_set_style_text_color(lbl_time, COLOR_TEXT_PRIMARY, 0);
    lv_obj_set_style_text_font(lbl_time, font_large, 0);

    // --- Tank Empty Indicator (right third) ---
    lv_obj_t *tank_container = lv_obj_create(bottom_section);
    lv_obj_set_size(tank_container, LV_PCT(33), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_color(tank_container, COLOR_BG_PRIMARY, 0);
    lv_obj_set_style_border_width(tank_container, 0, 0);
    lv_obj_set_style_pad_all(tank_container, 2, 0);
    lv_obj_set_flex_flow(tank_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(tank_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Tank LED indicator
    led_tank_empty = lv_led_create(tank_container);
    lv_obj_set_size(led_tank_empty, 35, 35);
    lv_led_set_color(led_tank_empty, COLOR_TANK_EMPTY);
    lv_led_off(led_tank_empty);

    // Tank label
    lv_obj_t *tank_label = lv_label_create(tank_container);
    lv_label_set_text(tank_label, "Tank");
    lv_obj_set_style_text_color(tank_label, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_font(tank_label, font_small, 0);

    // Create timer to update time every 1 second
    update_timer = lv_timer_create(update_timer_cb, 1000, NULL);

    // Initial update
    intercooler_ui_update_time();
}

/**
 * Timer callback - updates time display every second
 */
static void update_timer_cb(lv_timer_t *timer)
{
    intercooler_ui_update_time();
}

/**
 * Helper function to set label text with color
 */
static void label_set_text_color(lv_obj_t *label, const char *text, lv_color_t color)
{
    if (label != NULL) {
        lv_label_set_text(label, text);
        lv_obj_set_style_text_color(label, color, 0);
    }
}

void intercooler_ui_update_temperature(float temp_celsius)
{
    if (lbl_temperature == NULL) return;

    // Format temperature string
    char temp_str[20];
    snprintf(temp_str, sizeof(temp_str), "%.1f °C", temp_celsius);

    // Determine color based on temperature (you can adjust these thresholds)
    lv_color_t color = COLOR_TEMP_NORMAL;
    if (temp_celsius >= 50.0f) {
        color = COLOR_TEMP_CRITICAL;
    } else if (temp_celsius >= 40.0f) {
        color = COLOR_TEMP_WARNING;
    }

    label_set_text_color(lbl_temperature, temp_str, color);
}

void intercooler_ui_update_time(void)
{
    if (lbl_time == NULL) return;

    // Format time string from RTC (hours:minutes only)
    char time_str[20];
    snprintf(time_str, sizeof(time_str), "%02d:%02d",
             datetime.hour, datetime.minute);

    label_set_text_color(lbl_time, time_str, COLOR_TEXT_PRIMARY);
}

void intercooler_ui_set_tank_empty(bool is_empty)
{
    if (led_tank_empty == NULL) return;

    if (is_empty) {
        lv_led_on(led_tank_empty);
    } else {
        lv_led_off(led_tank_empty);
    }
}

void intercooler_ui_set_relay_active(bool is_active)
{
    if (led_relay_active == NULL) return;

    if (is_active) {
        lv_led_on(led_relay_active);
    } else {
        lv_led_off(led_relay_active);
    }
}

void intercooler_ui_cleanup(void)
{
    // Delete timer
    if (update_timer != NULL) {
        lv_timer_del(update_timer);
        update_timer = NULL;
    }

    // Clear all objects
    lv_obj_clean(lv_scr_act());

    // Reset object pointers
    lbl_temperature = NULL;
    lbl_time = NULL;
    led_relay_active = NULL;
    led_tank_empty = NULL;
}
