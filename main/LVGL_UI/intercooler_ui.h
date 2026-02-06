#pragma once

#include "lvgl.h"
#include "LVGL_Driver.h"
#include "PCF85063.h"

/***********************
 *  COLOR CONSTANTS
 ***********************/
// Main background
#define COLOR_BG_PRIMARY        lv_color_hex(0x1a1a2e)  // Dark blue background

// Text colors
#define COLOR_TEXT_PRIMARY      lv_color_hex(0xFFFFFF)  // White text
#define COLOR_TEXT_SECONDARY    lv_color_hex(0xB0B0B0)  // Light gray text
#define COLOR_TEXT_LABEL        lv_color_hex(0x7F7F7F)  // Medium gray text

// Temperature colors
#define COLOR_TEMP_NORMAL       lv_color_hex(0x00FF00)  // Green (safe)
#define COLOR_TEMP_WARNING      lv_color_hex(0xFFA500)  // Orange (warning)
#define COLOR_TEMP_CRITICAL     lv_color_hex(0xFF0000)  // Red (critical)

// Indicator colors
#define COLOR_INDICATOR_OFF     lv_color_hex(0x333333)  // Dark gray (off)
#define COLOR_RELAY_ACTIVE      lv_color_hex(0x00FF00)  // Green (relay active)
#define COLOR_TANK_EMPTY        lv_color_hex(0xFF0000)  // Red (tank empty)

// Accent
#define COLOR_ACCENT            lv_color_hex(0x00BFFF)  // Deep sky blue


/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Initialize and create the intercooler main screen
 * Call this function from app_main() to set up the UI
 */
void intercooler_ui_create(void);

/**
 * Update temperature display
 * @param temp_celsius Temperature in Celsius
 */
void intercooler_ui_update_temperature(float temp_celsius);

/**
 * Update time display
 * Called automatically every second by the UI update timer
 */
void intercooler_ui_update_time(void);

/**
 * Set tank empty indicator
 * @param is_empty true if tank is empty, false otherwise
 */
void intercooler_ui_set_tank_empty(bool is_empty);

/**
 * Set relay triggered indicator
 * @param is_active true if relay is active, false otherwise
 */
void intercooler_ui_set_relay_active(bool is_active);

/**
 * Cleanup UI resources when exiting
 */
void intercooler_ui_cleanup(void);
