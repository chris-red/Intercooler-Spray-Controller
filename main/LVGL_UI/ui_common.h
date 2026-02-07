#pragma once

#include "lvgl.h"
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
 *  FONT REFERENCES
 ***********************/
extern const lv_font_t race_120;

/***********************
 *  TYPE DEFINITIONS
 ***********************/
typedef struct {
    const lv_font_t *temp;
    const lv_font_t *large;
    const lv_font_t *normal;
    const lv_font_t *small;
} ui_fonts_t;

/***********************
 *  FUNCTION DECLARATIONS
 ***********************/

/**
 * Initialize common UI fonts
 */
void ui_common_init_fonts(ui_fonts_t *fonts);

/**
 * Get the current UI fonts
 */
const ui_fonts_t *ui_common_get_fonts(void);

/**
 * Helper function to set label text with color
 */
void ui_common_set_label_color(lv_obj_t *label, const char *text, lv_color_t color);
